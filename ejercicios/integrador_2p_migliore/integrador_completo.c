#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_exti.h"

// CONSTANTES Y DIRECCIONES DE MEMORIA

#define SIGNAL_INCREMENT 5
#define SRAM_BANK1 0X20080000
#define SRAM_BANK0_COMIENZO 0X2007C000
#define SRAM_BANK0_MEDIO 0X2007E000

// FIRMAS DE FUNCIONES

void conf_ADC(void);
void conf_DMA(GPDMA_LLI_T *lli_bank0_a_dac, GPDMA_LLI_T *lli_bank1_a_dac);
void conf_DAC(void);
void conf_Timer(void);
void conf_INTE(void);
void signalGenerator(void);

// VARIABLES Y ESTADOS GLOBALES

typedef enum
{
    ESPERA,       // idle inicial
    ADC_PUNTERO,  // muevo P2M de ADC a SRAM_BANK0_COMIENZO usando punteros en ADC_IRQHandler
    ADC_DMA,      // muevo P2M de ADC a SRAM_BANK0_COMIENZO
    PROMEDIO,     // muevo M2M de SRAM_BANK0_COMIENZO a SRAM_BANK0_MEDIO, cuando termina transf calc promedio y guardo en var (todo eso en isr)
    SRAM02DAC,    // muevo M2P de SRAM_BANK0_MEDIO a DAC
    FORMA_DE_ONDA // muevo M2P de SRAM_BANK1 a DAC (onda construida)
} ESTADO_T;

ESTADO_T estado = ADC_PUNTERO;

volatile uint16_t *signal = (uint16_t)SRAM_BANK1; // array de datos que se guardan en mem
volatile uint16_t *adc2mem_ptr = (uint16_t *)SRAM_BANK0_COMIENZO;
volatile uint16_t *mem2mem_ptr = (uint16_t *)SRAM_BANK0_MEDIO; // puntero para hacer promedio
volatile float promedio = 0;
volatile uint32_t promedio_flag = 0;
volatile uint32_t config_flag = 0; // flag para configurar perifericos cuando cambia estado

int main(void)
{
    GPDMA_LLI_T lli_bank0_a_dac = {
        .srcAddr = (uint32_t)SRAM_BANK0_MEDIO,
        .dstAddr = (uint32_t)&LPC_DAC->DACR,
        .nextLLI = (uint32_t)&lli_bank0_a_dac,
        .control = 4095 | (1 << 18) | (1 << 21) | (1 << 26)};
    GPDMA_LLI_T lli_bank1_a_dac = {
        .srcAddr = (uint32_t)&signal,
        .dstAddr = (uint32_t)&LPC_DAC->DACR,
        .nextLLI = (uint32_t)&lli_bank1_a_dac,
        .control = 382 | 1 << 18 | 1 << 21 | 1 << 26};

    signalGenerator();
    conf_INTE();
    conf_ADC();
    conf_DMA(&lli_bank0_a_dac, &lli_bank1_a_dac);
    conf_Timer();

    while (1)
    {
        switch (estado)
        {
        case ESPERA:
            break;
        case ADC_PUNTERO:
        {
            if (config_flag == 0)
                break;

            // encender timer (MAT01 toggle cada 15s)
            TIM_Enable(LPC_TIM0);

            // conf adc para comenzar conversion con MAT01 rising edge (30s)
            // tire interrupcion cuando termine conversion y en isr muevo dato con punteros
            ADC_PowerUp(); // encendemos el ADC
            ADC_BurstDisable();
            ADC_StartCmd(ADC_START_ON_MAT01); // conf para que timer haga trigger cada 30s
            ADC_IntEnable(ADC_CHANNEL_0);
            config_flag = 0;
            break;
        }
        case ADC_DMA:
        {
            if (config_flag == 0)
                break;

            // apago y reset timer (MAT01 toggle cada 15s)
            TIM_Disable(LPC_TIM0);
            TIM_ResetCounter(LPC_TIM0);

            // conf adc modo burst + dma, sin interrupcion
            ADC_PowerUp(); // encendemos el ADC
            ADC_StartCmd(ADC_START_CONTINUOUS);
            ADC_BurstEnable();
            ADC_IntDisable(ADC_CHANNEL_0);

            // conf dma: usa canal 0 para transferir del adc a SRAM_BANK0_COMIENZO
            GPDMA_ChannelGracefulStop(GPDMA_CH_1);
            GPDMA_ChannelGracefulStop(GPDMA_CH_2);
            GPDMA_ChannelGracefulStop(GPDMA_CH_7);
            GPDMA_ChannelStart(GPDMA_CH_0);
            config_flag = 0;
            break;
        }

        case PROMEDIO:
        {
            if (promedio_flag == 1)
            {
                int sum = 0;
                for (int i = 0; i < 4095; i++)
                {
                    sum += mem2mem_ptr[i];
                }

                promedio = sum / 4095;
                promedio_flag = 0;
            }
            if (config_flag == 0)
                break;

            // apagar adc
            ADC_PowerDown();

            // conf dma + isr: usa canal 7 para transferir de SRAM_BANK0_COMIENZO a SRAM_BANK0_MEDIO
            GPDMA_ChannelGracefulStop(GPDMA_CH_0);
            GPDMA_ChannelGracefulStop(GPDMA_CH_1);
            GPDMA_ChannelGracefulStop(GPDMA_CH_2);
            GPDMA_ChannelStart(GPDMA_CH_7);
            config_flag = 0;
            break;
        }

        case SRAM02DAC:
        {
            if (config_flag == 0)
                break;

            // conf dac (estado actual setea timeout para dma request)
            // en este caso pide
            conf_DAC();

            // conf dma: usa canal 1 para transferir de SRAM_BANK0_MEDIO a DAC
            GPDMA_ChannelGracefulStop(GPDMA_CH_0);
            GPDMA_ChannelGracefulStop(GPDMA_CH_2);
            GPDMA_ChannelGracefulStop(GPDMA_CH_7);
            GPDMA_ChannelStart(GPDMA_CH_1);
            config_flag = 0;
            break;
        }
        case FORMA_DE_ONDA:
        {
            if (config_flag == 0)
                break;

            // conf dac (estado actual setea timeout para dma request)
            conf_DAC();

            // conf dma: usa canal 2 para transferir de SRAM_BANK1 a DAC
            GPDMA_ChannelGracefulStop(GPDMA_CH_0);
            GPDMA_ChannelGracefulStop(GPDMA_CH_1);
            GPDMA_ChannelGracefulStop(GPDMA_CH_7);
            GPDMA_ChannelStart(GPDMA_CH_2);
            config_flag = 0;
            break;
        }

        default:
            break;
        }
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
void signalGenerator(void)
{
    uint16_t counter = 0;
    while (counter < 382)
    {
        if (counter < 96)
        {
            // primera rampa asc
            signal[counter] = ((512 + SIGNAL_INCREMENT * counter) << 6);
        }
        else if (counter < 287)
        {
            // 2 rampas desc
            signal[counter] = ((1023 - SIGNAL_INCREMENT * (counter - 96)) << 6);
        }
        else
        {
            // ultima rampa asc
            signal[counter] = ((SIGNAL_INCREMENT * (counter - 287)) << 6);
        }
        counter++;
    }
}

// Interrumpe cuando P2.10 tiene flanco desc, no tiene pull down/up
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

// Hace cambios de estados cuando viene la interrupcion, solo cambia flags
void EINT0_IRQHandler(void)
{
    if (EXTI_GetFlag(EXTI_EINT0) == SET)
    {
        EXTI_ClearFlag(EXTI_EINT0);
        switch (estado)
        {
        case ESPERA:
            estado = ADC_PUNTERO;
            break;
        case ADC_PUNTERO:
            estado = ADC_DMA;
            break;
        case ADC_DMA:
            estado = PROMEDIO; // transf M2M y al final saco promedio
            break;
        case PROMEDIO:
            estado = SRAM02DAC; // transf M2P de segunda mitad sram 0 a DAC
            break;
        case SRAM02DAC:
            estado = FORMA_DE_ONDA;
            break;
        case FORMA_DE_ONDA:
            estado = ADC_PUNTERO;
            break;
        default:
            break;
        }
        config_flag = 1;
    }
}

// Configura el timer0 para que haga toggle de MAT01 cada 15s
void conf_Timer(void)
{
    // conf toggle de adc_puntero
    TIM_TIMERCFG_T tim;
    tim.prescaleOpt = TIM_US;
    tim.prescaleValue = 1;

    TIM_MATCHCFG_T matchcfg;
    matchcfg.channel = TIM_MATCH_1;
    matchcfg.intEn = DISABLE;
    matchcfg.stopEn = DISABLE;
    matchcfg.resetEn = ENABLE;
    matchcfg.extOpt = TIM_TOGGLE;
    matchcfg.matchValue = 3;

    TIM_InitTimer(LPC_TIM0, &tim);
    TIM_ConfigMatch(LPC_TIM0, &matchcfg);
}

// Configura adc con maxima frec de conversion, channel 0, start con rising edge y habilita las interrupciones de adc
void conf_ADC(void)
{
    ADC_Init(200000); // max frec conv
    ADC_ChannelEnable(ADC_CHANNEL_0);
    ADC_PinConfig(ADC_CHANNEL_0);
    ADC_EdgeStartConfig(ADC_START_ON_RISING);
    NVIC_EnableIRQ(ADC_IRQn);
}

// Setea el DAC para que convierta y haga request a DMA con distintos tiempos de request dependiendo el estado actual cuando se llame a la funcion
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
    case SRAM02DAC: // m2p: mueve de sram0 a dac con periodo 1s
    {
        // pide un nuevo dato cada 2604 us (6103 ticks)
        // son 4k datos (de 16bytes) => muestro todo en 1s (periodo de onda 1 seg)
        DAC_SetDMATimeOut(6103);
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
    default:
        break;
    }
}

/**
 * @brief Configua 4 canales para distintos estados, no hicia ninguno:
 * \n
 * - CH0 (ADC_DMA): transf de 4095 datos de 16bits desde el ADC a SRAM_BANK0_COMIENZO
 * \n
 * - CH1 (SRAM2DAC): transf de 4095 datos de 16bits desde SRAM_BANK0_MEDIO a DAC
 * \n
 * - CH2 (ADC2MEM): transf de 4095 datos de 16bits desde el ADC a SRAM_BANK0_COMIENZO
 * \n
 * - CH7 (ADC2MEM): transf de 4095 datos de 16bits desde el ADC a SRAM_BANK0_COMIENZO
 * @param lli_bank0_a_dac puntero a una GPDMA_LLI_T circular que configura CH1
 * @param lli_bank1_a_dac puntero a una GPDMA_LLI_T circular que configura CH2
 *  */
void conf_DMA(GPDMA_LLI_T *lli_bank0_a_dac, GPDMA_LLI_T *lli_bank1_a_dac)
{
    // channel 0: ADC2MEM
    GPDMA_Channel_CFG_T ch0Cfg_adc2mem;
    ch0Cfg_adc2mem.channelNum = GPDMA_CH_0;
    ch0Cfg_adc2mem.transferSize = 4095; // 4k datos de 16bits -> 8k bits (max tranf es de 4095, se pierde un dato)
    ch0Cfg_adc2mem.type = GPDMA_P2M;
    ch0Cfg_adc2mem.src.burst = GPDMA_BSIZE_1;
    ch0Cfg_adc2mem.src.width = GPDMA_HALFWORD;
    ch0Cfg_adc2mem.src.increment = DISABLE;
    ch0Cfg_adc2mem.dst.burst = GPDMA_BSIZE_1;
    ch0Cfg_adc2mem.dst.width = GPDMA_HALFWORD;
    ch0Cfg_adc2mem.dst.increment = ENABLE;
    ch0Cfg_adc2mem.srcMemAddr = (uint32_t)&LPC_ADC->ADGDR;
    ch0Cfg_adc2mem.dstMemAddr = (uint32_t)SRAM_BANK0_COMIENZO;
    ch0Cfg_adc2mem.srcConn = GPDMA_ADC;
    ch0Cfg_adc2mem.intTC = DISABLE;
    ch0Cfg_adc2mem.intErr = DISABLE;
    ch0Cfg_adc2mem.linkedList = 0;

    // channel 1: MEM2DAC (desde 2da mitad sram bank 0)
    GPDMA_Channel_CFG_T ch1Cfg_m2p_bank0;
    ch1Cfg_m2p_bank0.channelNum = GPDMA_CH_1;
    ch1Cfg_m2p_bank0.transferSize = 4095; // 4k datos de 16bits -> 8k bits (max tranf es de 4095, se pierde un dato)
    ch1Cfg_m2p_bank0.type = GPDMA_M2P;
    ch1Cfg_m2p_bank0.src.burst = GPDMA_BSIZE_1;
    ch1Cfg_m2p_bank0.src.width = GPDMA_HALFWORD;
    ch1Cfg_m2p_bank0.src.increment = ENABLE;
    ch1Cfg_m2p_bank0.dst.burst = GPDMA_BSIZE_1;
    ch1Cfg_m2p_bank0.dst.width = GPDMA_HALFWORD;
    ch1Cfg_m2p_bank0.dst.increment = DISABLE;
    ch1Cfg_m2p_bank0.srcMemAddr = (uint32_t)SRAM_BANK0_MEDIO;
    ch1Cfg_m2p_bank0.dstMemAddr = (uint32_t)&LPC_DAC->DACR;
    ch1Cfg_m2p_bank0.dstConn = GPDMA_DAC;
    ch1Cfg_m2p_bank0.intTC = DISABLE;
    ch1Cfg_m2p_bank0.intErr = DISABLE;
    ch1Cfg_m2p_bank0.linkedList = (uint32_t)lli_bank0_a_dac; // circular

    // channel 2: MEM2DAC (desde sram bank 1)
    GPDMA_Channel_CFG_T ch2Cfg_m2p_bank1;
    ch2Cfg_m2p_bank1.channelNum = GPDMA_CH_2;
    ch2Cfg_m2p_bank1.transferSize = 382; // 382 datos de 16bits
    ch2Cfg_m2p_bank1.type = GPDMA_M2P;
    ch2Cfg_m2p_bank1.src.burst = GPDMA_BSIZE_1;
    ch2Cfg_m2p_bank1.src.width = GPDMA_HALFWORD;
    ch2Cfg_m2p_bank1.src.increment = ENABLE;
    ch2Cfg_m2p_bank1.dst.burst = GPDMA_BSIZE_1;
    ch2Cfg_m2p_bank1.dst.width = GPDMA_HALFWORD;
    ch2Cfg_m2p_bank1.dst.increment = DISABLE;
    ch2Cfg_m2p_bank1.srcMemAddr = (uint32_t)SRAM_BANK1;
    ch2Cfg_m2p_bank1.dstMemAddr = (uint32_t)&LPC_DAC->DACR;
    ch2Cfg_m2p_bank1.dstConn = GPDMA_DAC;
    ch2Cfg_m2p_bank1.intTC = DISABLE;
    ch2Cfg_m2p_bank1.intErr = DISABLE;
    ch2Cfg_m2p_bank1.linkedList = (uint32_t)lli_bank1_a_dac; // circular

    // channel 7: MEM2MEM + promedio cuando termina (isr handler)
    GPDMA_Channel_CFG_T ch7Cfg_m2m_promedio;
    ch7Cfg_m2m_promedio.channelNum = GPDMA_CH_7;
    ch7Cfg_m2m_promedio.transferSize = 4095; // 4k datos de 16bits -> 8k bits (max tranf es de 4095, se pierde un dato)
    ch7Cfg_m2m_promedio.type = GPDMA_M2M;
    ch7Cfg_m2m_promedio.src.burst = GPDMA_BSIZE_1;
    ch7Cfg_m2m_promedio.src.width = GPDMA_HALFWORD;
    ch7Cfg_m2m_promedio.src.increment = ENABLE;
    ch7Cfg_m2m_promedio.dst.burst = GPDMA_BSIZE_1;
    ch7Cfg_m2m_promedio.dst.width = GPDMA_HALFWORD;
    ch7Cfg_m2m_promedio.dst.increment = ENABLE;
    ch7Cfg_m2m_promedio.srcMemAddr = (uint32_t)SRAM_BANK0_COMIENZO;
    ch7Cfg_m2m_promedio.dstMemAddr = (uint32_t)SRAM_BANK0_MEDIO;
    ch7Cfg_m2m_promedio.intTC = ENABLE;
    ch7Cfg_m2m_promedio.intErr = DISABLE;
    ch7Cfg_m2m_promedio.linkedList = 0;

    GPDMA_Init();
    GPDMA_SetupChannel(&ch0Cfg_adc2mem);
    GPDMA_SetupChannel(&ch1Cfg_m2p_bank0);
    GPDMA_SetupChannel(&ch2Cfg_m2p_bank1);
    GPDMA_SetupChannel(&ch7Cfg_m2m_promedio);
    NVIC_EnableIRQ(DMA_IRQn);
}

// Handler que cuando se hace una conversion, guarda en memoria con un puntero (adc2mem_ptr)
void ADC_IQRHandler(void)
{
    static uint32_t counter = 0;
    if (ADC_ChannelGetStatus(ADC_CHANNEL_0, ADC_DATA_DONE) == SET)
    {
        adc2mem_ptr[counter] = LPC_ADC->ADDR0 & (0xFFF << 4); // guardo con bits shifteados asi luego dma no tiene problema cuando mueva a dac
        counter = (counter + 1) % 4096;                       // buffer circular en memoria
    }
}

// Handler que cuando se termina transferencia M2M entre mitades de SRAM_BANK0, hace promedio y guarda (solo setea flags)
void DMA_IRQHandler(void)
{
    if (GPDMA_IntGetStatus(GPDMA_INTTC, GPDMA_CH_0) == SET)
    {
        GPDMA_ClearIntPending(GPDMA_INTTC, GPDMA_CH_0);
        promedio_flag = 1;
    }
}
