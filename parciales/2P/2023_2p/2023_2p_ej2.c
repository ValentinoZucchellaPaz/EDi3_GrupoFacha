/**
 * El ADC esta convirtiendo señal de entrada 16kHz y guarda en primer mitad de SRAM_BANK0 usando DMA
 * Cuando viene EXTI, el ADC debe completar el ciclo de conversión que estaba realizando y parar, luego saco muestras de ADC por el DAC con DMA (SRAM_BANK0_PRIMER_MITAD -> DAC)
 * Cuando viene EXTI de nuevo, saco por el DAC la forma de onda guardada en SRAM_BANK0_SEGUNDA_MITAD (crear forma de onda y guardarla) de forma tal q tenga periodo 614us, al terminar vuelvo a convertir con el ADC.
 * Se alterna así entre los dos estados del sistema con cada interrupción externa.
 *  CCLK = 80 Mhz
 *
 * FORMA_DE_ONDA:
 * - 8k muestras y debe salir con periodo 614us
 * - Separo la señal en 2 (4k subida pos, 4k subida neg)
 *      - debe subir de 512 a 1023 en 4k pasos → aumento en 1 cada 8 pasos
 *      - luego repito pero desde 0 hasta 512
 */

#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_exti.h"

#define SRAM_BANK0_COMIENZO 0X2007C000
#define SRAM_BANK0_MEDIO 0X2007E000

typedef enum
{
    ADC_2_MEM,       // muevo P2M de ADC a SRAM_BANK0_COMIENZO usando DMA
    ADC_2_DAC,       // muevo M2P de SRAM_BANK0_COMIENZO a DAC usando DMA
    WAVE_FORM_2_DAC, // muevo M2P de SRAM_BANK0_MEDIO a DAC (onda construida)
    // cuando saco la forma de onda por dac AL MISMO TIEMPO tengo que seguir convirtiendo por el ADC
} STATE_T;

STATE_T state = ADC_2_MEM;

volatile uint16_t *signal = (uint16_t)SRAM_BANK0_MEDIO; // array de datos que se guardan en mem
volatile uint32_t config_flag = 0;                      // flag para configurar perifericos cuando cambia estado

void createSignal(void);
void configDAC(void);
void configADC(void);
void configDMA(GPDMA_LLI_T *lli_adc_a_bank0, GPDMA_LLI_T *lli_bank0_a_dac, GPDMA_LLI_T *lli_bank0_mitad_a_dac);
void configEXTI(void);

int main(void)
{
    // DMA LLI's
    GPDMA_LLI_T lli_adc_a_bank0, lli_bank0_a_dac, lli_bank0_mitad_a_dac;
    lli_adc_a_bank0.srcAddr = (uint32_t)&LPC_ADC->ADDR0;
    lli_adc_a_bank0.dstAddr = (uint32_t)SRAM_BANK0_COMIENZO;
    lli_adc_a_bank0.nextLLI = &lli_adc_a_bank0;
    lli_adc_a_bank0.control = (4095) | (1 << 18) | (1 << 21) | (1 << 27);

    lli_bank0_a_dac.srcAddr = (uint32_t)SRAM_BANK0_COMIENZO;
    lli_bank0_a_dac.dstAddr = (uint32_t)&LPC_DAC->DACR;
    lli_bank0_a_dac.nextLLI = &lli_bank0_a_dac;
    lli_bank0_a_dac.control = (4095) | (1 << 18) | (1 << 21) | (1 << 26);

    lli_bank0_mitad_a_dac.srcAddr = (uint32_t)signal;
    lli_bank0_mitad_a_dac.dstAddr = (uint32_t)&LPC_DAC->DACR;
    lli_bank0_mitad_a_dac.nextLLI = &lli_bank0_mitad_a_dac;
    lli_bank0_mitad_a_dac.control = (614) | (1 << 18) | (1 << 21) | (1 << 26);

    createSignal();
    configEXTI();
    configDMA(&lli_adc_a_bank0, &lli_bank0_a_dac, &lli_bank0_mitad_a_dac);
    configADC();
    configDAC();

    while (1)
    {
        switch (state)
        {
        case ADC_2_MEM:
        {
            if (config_flag == 0)
                break;

            // encender adc
            ADC_PowerUp();
            // habilitar canal0 dma adc2mem
            GPDMA_ChannelGracefulStop(GPDMA_CH_1);
            GPDMA_ChannelGracefulStop(GPDMA_CH_2);
            GPDMA_ChannelStart(GPDMA_CH_0);

            config_flag = 0;
            break;
        }
        case ADC_2_DAC:
        {
            if (config_flag == 0)
                break;

            // apagar adc
            ADC_PowerDown();

            DAC_SetDMATimeOut(620); // saco datos a frecuencia de señal, 16kHz -> 62.5us de periodo
            // como muestree a 32kHz tengo 2 muestras por periodo, debo sacar a 31.25us cada muestra
            // trg = ticks / pclk -> ticks = 31us * 20MHz = 620

            // habilitar canal 1 dma mem2dac (primera mitad de sram bank0)
            GPDMA_ChannelGracefulStop(GPDMA_CH_0);
            GPDMA_ChannelGracefulStop(GPDMA_CH_2);
            GPDMA_ChannelStart(GPDMA_CH_1);

            config_flag = 0;
            break;
        }
        case WAVE_FORM_2_DAC:
        {
            if (config_flag == 0)
                break;

            DAC_SetDMATimeOut(20); // pide 1 dato cada 1us, como son 614 datos logro el periodo deseado

            // habilitar canal 2 dma mem2dac (segunda mitad de sram bank0)
            GPDMA_ChannelGracefulStop(GPDMA_CH_0);
            GPDMA_ChannelGracefulStop(GPDMA_CH_1);
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
 * @brief Crea señal de consigna:
 * Max frec conversion DAC es 1us -> hago 614 muestras para tener periodo 614us
 *
 * Para llegar de 0 a 512 en 307 muestras -> voy a tener saltos de 1.667
 *
 * RESOLUCION : 5.3mV
 * @note Como debo hacer saltos enteros, trunco a enteros
 */
void createSignal(void)
{
    uint32_t i = 0;
    while (i < 614)
    {
        if (i <= 307) // primer rampa
            signal[i] = ((uint32_t)(512 + i * 1.667)) << 6;
        else // segunda rampa
            signal[i] = ((uint32_t)(i * 1.667)) << 6;
    }
}

// configura EXTI0 con interrupcion por flanco asc y pin sin pull
void configEXTI(void)
{
    EXTI_CFG_T extiCfg;
    extiCfg.line = EXTI_EINT0;
    extiCfg.mode = EXTI_EDGE_SENSITIVE;
    extiCfg.polarity = EXTI_RISING_EDGE;

    EXTI_Init();
    EXTI_PinConfig(EXTI_EINT0, EXTI_NOPULL);
    EXTI_ConfigEnable(&extiCfg); // configura y habilita interrupcion
}

// config adc para que que convierta a maximo rate con burst mode de forma continua, comienza apagado
void configADC(void)
{
    ADC_Init(32000); // aunque pclk=20MHz los drivers manejan internamente
    ADC_ChannelEnable(ADC_CHANNEL_0);
    ADC_PinConfig(ADC_CHANNEL_0);
    ADC_BurstEnable();
    ADC_StartCmd(ADC_START_CONTINUOUS);
    ADC_PowerDown();
}

// Prende y configura DAC. Segun el estado en el que se está usa o no DMA. Si está en ADC_2_MEM no hace nada, en los otros hace requesta a DMA cada ~1us
void configDAC(void)
{
    DAC_Init();
    DAC_CONVERTER_CFG_T dacCfgON;
    dacCfgON.dmaCounter = ENABLE;
    dacCfgON.dmaRequest = ENABLE;
    dacCfgON.doubleBuffer = DISABLE;
    DAC_ConfigDAConverterControl(&dacCfgON); // luego habilito canal de DMA
    // DAC_SetDMATimeOut(20); // seteo en estado (main)
}

/**
 * @brief Configua 4 canales para distintos estados, no inicia ninguno:
 * \n
 * - CH0 (ADC_2_MEM): transf de 4095 datos de 16bits desde el ADC a SRAM_BANK0_COMIENZO
 * \n
 * - CH1 (ADC_2_DAC): transf de 4095 datos de 16bits desde SRAM_BANK0_COMIENZO a DAC
 * \n
 * - CH2 (WAVE_FORM_2_DAC): transf de 614 datos de 16bits desde el ADC a SRAM_BANK0_COMIENZO
 *
 * @param lli_adc_a_bank0 puntero a una GPDMA_LLI_T circular que configura CH0
 * @param lli_bank0_a_dac puntero a una GPDMA_LLI_T circular que configura CH1
 * @param lli_bank0_mitad_a_dac puntero a una GPDMA_LLI_T circular que configura CH2
 *  */
void configDMA(GPDMA_LLI_T *lli_adc_a_bank0, GPDMA_LLI_T *lli_bank0_a_dac, GPDMA_LLI_T *lli_bank0_mitad_a_dac)
{
    // channel 0: ADC_2_MEM
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
    ch0Cfg_adc2mem.linkedList = lli_adc_a_bank0;

    // channel 1: ADC_2_DAC
    GPDMA_Channel_CFG_T ch1Cfg_adc2dac;
    ch1Cfg_adc2dac.channelNum = GPDMA_CH_1;
    ch1Cfg_adc2dac.transferSize = 4095; // 4k datos de 16bits -> 8k bits (max tranf es de 4095, se pierde un dato)
    ch1Cfg_adc2dac.type = GPDMA_M2P;
    ch1Cfg_adc2dac.src.burst = GPDMA_BSIZE_1;
    ch1Cfg_adc2dac.src.width = GPDMA_HALFWORD;
    ch1Cfg_adc2dac.src.increment = ENABLE;
    ch1Cfg_adc2dac.dst.burst = GPDMA_BSIZE_1;
    ch1Cfg_adc2dac.dst.width = GPDMA_HALFWORD;
    ch1Cfg_adc2dac.dst.increment = DISABLE;
    ch1Cfg_adc2dac.srcMemAddr = (uint32_t)SRAM_BANK0_COMIENZO;
    ch1Cfg_adc2dac.dstMemAddr = (uint32_t)&LPC_DAC->DACR;
    ch1Cfg_adc2dac.dstConn = GPDMA_DAC;
    ch1Cfg_adc2dac.intTC = DISABLE;
    ch1Cfg_adc2dac.intErr = DISABLE;
    ch1Cfg_adc2dac.linkedList = lli_bank0_a_dac;

    // channel 2: WAVE_FORM_2_DAC
    GPDMA_Channel_CFG_T ch2Cfg_waveform2dac;
    ch2Cfg_waveform2dac.channelNum = GPDMA_CH_0;
    ch2Cfg_waveform2dac.transferSize = 614; // muestras guardadas
    ch2Cfg_waveform2dac.type = GPDMA_M2P;
    ch2Cfg_waveform2dac.src.burst = GPDMA_BSIZE_1;
    ch2Cfg_waveform2dac.src.width = GPDMA_HALFWORD;
    ch2Cfg_waveform2dac.src.increment = ENABLE;
    ch2Cfg_waveform2dac.dst.burst = GPDMA_BSIZE_1;
    ch2Cfg_waveform2dac.dst.width = GPDMA_HALFWORD;
    ch2Cfg_waveform2dac.dst.increment = DISABLE;
    ch2Cfg_waveform2dac.srcMemAddr = (uint32_t)signal;
    ch2Cfg_waveform2dac.dstMemAddr = (uint32_t)&LPC_DAC->DACR; // este valor se ignore y va a usar el de dstConn
    ch2Cfg_waveform2dac.dstConn = GPDMA_DAC;
    ch2Cfg_waveform2dac.intTC = DISABLE;
    ch2Cfg_waveform2dac.intErr = DISABLE;
    ch2Cfg_waveform2dac.linkedList = lli_bank0_mitad_a_dac;

    GPDMA_Init();
    GPDMA_SetupChannel(&ch0Cfg_adc2mem);
    GPDMA_SetupChannel(&ch1Cfg_adc2dac);
    GPDMA_SetupChannel(&ch2Cfg_waveform2dac);
}

// hace el cambio de estados y setea la flag de config para que se configure en el main
void EINT0_IRQHandler(void)
{
    if (EXTI_GetFlag(EXTI_EINT0))
    {
        EXTI_ClearFlag(EXTI_EINT0);
        switch (state)
        {
        case ADC_2_MEM:
        {
            state = ADC_2_DAC;
            break;
        }
        case ADC_2_DAC:
        {
            state = WAVE_FORM_2_DAC;
            break;
        }
        case WAVE_FORM_2_DAC:
        {
            state = ADC_2_MEM;
            break;
        }

        default:
            break;
        }
        config_flag = 1;
    }
}
