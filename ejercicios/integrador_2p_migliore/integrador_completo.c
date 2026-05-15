#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_exti.h"

#define SIGNAL_INCREMENT 5
#define SRAM_BANK1 0X20080000
#define SRAM_BANK0_COMIENZO 0X2007C000
#define SRAM_BANK0_MEDIO 0X2007E000

typedef enum
{
    ADC_PUNTERO,
    ADC_DMA,
    PROMEDIO,     // muevo M2M (primer mitad a segunda mitad de sram0), cuando termina transf calc promedio y guardo en var
    ADC_A_DAC,    // muevo M2P de segunda mitad sram0 a dac
    FORMA_DE_ONDA // muevo M2P de onda construida a dac
} ESTADO_T;

ESTADO_T estado = ADC_PUNTERO;

void conf_ADC(void);
void conf_DMA(void);
void conf_DAC(void);
void conf_Timer(void);
void conf_Pines(void);
void conf_INTE(void);

int main(void)
{
    // array de datos que se guardan en mem
    uint16_t *signal = (uint16_t)SRAM_BANK1;
    signalGenerator(signal);
    conf_INTE();
    conf_ADC();
    conf_DAC();
    conf_DMA();
    conf_Pines();
    conf_Timer();

    while (1)
    {
    }
    return 0;
}

/**
 * @brief Toma un array y lo llena con una forma de onda triangular
 * de 382 muestras adecuada para ser mostrada por el DAC.
 * Se ponen los valores ya desplazados a los bits 15:6. (value de DAC)
 *
 * @param arr Puntero al array de datos
 */
void signalGenerator(uint16_t *arr)
{
    uint16_t counter = 0;
    while (counter < 382)
    {
        if (counter < 96)
        {
            // primera rampa asc
            arr[counter] = ((512 + SIGNAL_INCREMENT * counter) << 6);
        }
        else if (counter < 287)
        {
            // 2 rampas desc
            arr[counter] = ((1023 - SIGNAL_INCREMENT * (counter - 96)) << 6);
        }
        else
        {
            // ultima rampa asc
            arr[counter] = ((SIGNAL_INCREMENT * (counter - 287)) << 6);
        }
        counter++;
    }
}

// interrupme P2.10 por flanco desc, no tiene pull down/up
void conf_INTE(void)
{
    EXTI_CFG_T extiCfg;
    extiCfg.line = EXTI_EINT0;
    extiCfg.mode = EXTI_EDGE_SENSITIVE;
    extiCfg.polarity = EXTI_FALLING_EDGE;

    EXTI_Init();
    EXTI_ConfigEnable(&extiCfg);
    EXTI_PinConfig(EXTI_EINT0, EXTI_NOPULL);
    EXTI_EnableIRQ(EXTI_EINT0);
}

void EINT0_IRQHandler(void)
{
    if (EXTI_GetFlag(EXTI_EINT0) == SET)
    {
        EXTI_ClearFlag(EXTI_EINT0);
        switch (estado)
        {
        case ADC_PUNTERO:
            estado = ADC_DMA;
            break;
        case ADC_DMA:
            estado = PROMEDIO; // transf M2M y al final saco promedio
            break;
        case PROMEDIO:
            estado = ADC_A_DAC; // transf M2P de segunda mitad sram 0 a DAC
            break;
        case ADC_A_DAC:
            estado = FORMA_DE_ONDA;
            break;
        case FORMA_DE_ONDA:
            estado = ADC_PUNTERO;
            break;
        default:
            break;
        }
        conf_Timer();
        conf_ADC();
        conf_DAC();
        conf_DMA();
    }
}

void conf_Timer(void)
{
    switch (estado)
    {
    case ADC_PUNTERO:
    {
        // conf toggle de adc_puntero
        TIM_TIMERCFG_T tim;
        tim.prescaleOpt = TIM_US;
        tim.prescaleValue = 1;

        TIM_InitTimer(LPC_TIM0, &tim);
        TIM_MATCHCFG_T matchcfg;
        matchcfg.channel = TIM_MATCH_1;
        matchcfg.intEn = DISABLE;
        matchcfg.stopEn = DISABLE;
        matchcfg.resetEn = ENABLE;
        matchcfg.extOpt = TIM_TOGGLE;
        matchcfg.matchValue = 3;
        TIM_ConfigMatch(LPC_TIM0, &matchcfg);
        TIM_Enable(LPC_TIM0);
        break;
    }

    case ADC_DMA:
    case PROMEDIO:
    case ADC_A_DAC:
    case FORMA_DE_ONDA:
        TIM_DeInit(LPC_TIM0);
        break;
    default:
        break;
    }
}

void conf_ADC(void)
{
    ADC_Init(200000); // max frec conv
    ADC_ChannelEnable(ADC_CHANNEL_0);
    ADC_PinConfig(ADC_CHANNEL_0);
    ADC_PowerUp(); // encendemos el ADC

    switch (estado)
    {
    case ADC_PUNTERO:
    {
        ADC_BurstDisable();
        ADC_StartCmd(ADC_START_ON_MAT01); // conf para que timer haga trigger cada 30s
        ADC_EdgeStartConfig(ADC_START_ON_RISING);
        ADC_IntEnable(ADC_CHANNEL_0);
        NVIC_EnableIRQ(ADC_IRQn);
        break;
    }
    case ADC_DMA:
    {
        ADC_BurstEnable();
        NVIC_DisableIRQ(ADC_IRQn);
        break;
    }

    // resto de casos
    case PROMEDIO:
    case ADC_A_DAC:
    case FORMA_DE_ONDA:
    {
        ADC_DeInit();
        break;
    }

    default:
        break;
    }
}

void conf_DAC(void)
{
    DAC_Init();

    DAC_CONVERTER_CFG_T dacConfig;
    dacConfig.dmaCounter = ENABLE;
    dacConfig.dmaRequest = ENABLE;
    dacConfig.doubleBuffer = DISABLE;
    DAC_ConfigDAConverterControl(&dacConfig);

    switch (estado)
    {
    case ADC_A_DAC: // m2p: mueve de sram0 a dac con periodo 1s
    {
        // pide un nuevo dato cada 2604 us (6103 ticks)
        // son 4k datos (de 16bytes) => muestro todo en 1s (periodo de onda 1 seg)
        DAC_SetDMATimeOut(6103); // HACER CALCULOS
        break;
    }

    case FORMA_DE_ONDA: // m2p: mueve de sram1 a dac con periodo 1s
    {
        // pide un nuevo dato cada 2604 us (65100 ticks)
        // son 382 datos => muestro todo en 1s (periodo de onda 1 seg)
        DAC_SetDMATimeOut(65100);
        break;
    }

    case ADC_PUNTERO:
    case PROMEDIO:
    case ADC_DMA:
    {
        break;
    }

    default:
        break;
    }
}

void config_DMA(void)
{
    GPDMA_Init();
    GPDMA_Endpoint_T srcc;
    srcc.burst = GPDMA_BSIZE_1;
    srcc.width = GPDMA_HALFWORD;
    srcc.increment = ENABLE;

    GPDMA_Endpoint_T dstc;
    dstc.burst = GPDMA_BSIZE_1;
    dstc.width = GPDMA_HALFWORD;
    dstc.increment = DISABLE;

    GPDMA_Channel_CFG_T dmaConfig = {0};
    dmaConfig.transferSize = 0;
    dmaConfig.src = srcc;
    dmaConfig.dst = dstc;
    dmaConfig.channelNum = GPDMA_CH_0;
    dmaConfig.transferSize = 4096;
    dmaConfig.type = GPDMA_M2P;
    dmaConfig.srcMemAddr = (uint32_t)SRAM_BANK0_MEDIO;
    dmaConfig.dstMemAddr = (uint32_t)&LPC_DAC->DACR;
    dmaConfig.dstConn = GPDMA_DAC;
    GPDMA_SetupChannel(&dmaConfig);

    switch (estado)
    {
    case ADC_A_DAC:
        GPDMA_ChannelStart(GPDMA_CH_0); /// cambiar
        break;

    case FORMA_DE_ONDA:
        dmaConfig.channelNum = GPDMA_CH_1;
        dmaConfig.transferSize = 382;
        dmaConfig.type = GPDMA_M2P;
        dmaConfig.srcMemAddr = (uint32_t)SRAM_BANK1;
        dmaConfig.dstMemAddr = (uint32_t)&LPC_DAC->DACR;
        dmaConfig.dstConn = GPDMA_DAC;
        break;

    case PROMEDIO:
        dmaConfig.channelNum = GPDMA_CH_2;
        dmaConfig.transferSize = 4096;
        dmaConfig.type = GPDMA_M2M;
        dmaConfig.srcMemAddr = (uint32_t)SRAM_BANK0_COMIENZO;
        dmaConfig.dstMemAddr = (uint32_t)SRAM_BANK0_MEDIO;
        break;
    default:
        break;
    }

    GPDMA_SetupChannel(&dmaConfig);
    GPDMA_ChannelStart(GPDMA_CH_0); /// cambiar
}