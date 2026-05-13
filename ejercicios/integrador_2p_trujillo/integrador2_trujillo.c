// ejercicio trujillo: promedio adc -> convertir señal con adc, llenar un array de 100 elementos (con dma y burst mode), hacer promedio y mostrar señal promediada por dac

#include "LPC17xx.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_dac.h"

void config_DMA(GPDMA_LLI_T lli);
void config_ADC(void);
void config_DAC(void);
void processData(uint16_t *buff);

// arrays continuos de 100 posicion que estan en al inicio de AHB_SRAM
uint16_t *buffer1 = (uint16_t *)0x2007C000;
uint16_t *buffer2 = (uint16_t *)0x2007C0C8;

volatile uint8_t currentBuffer = 0;
volatile uint8_t processBuffer = 0;

int main(void)
{
    GPDMA_LLI_T lli1;
    GPDMA_LLI_T lli2;

    lli1.srcAddr = (uint32_t)&LPC_ADC->ADGDR;
    lli1.dstAddr = (uint32_t)&buffer1;
    lli1.nextLLI = (uint32_t)&lli2;
    lli1.control = 100 | 1 << 18 | 1 << 21 | 1 << 27 | 1 << 31;

    lli2.srcAddr = (uint32_t)&LPC_ADC->ADGDR;
    lli2.dstAddr = (uint32_t)&buffer2;
    lli2.nextLLI = (uint32_t)&lli1;
    lli2.control = 100 | 1 << 18 | 1 << 21 | 1 << 27 | 1 << 31;

    config_DMA(lli2);
    config_DAC();
    config_ADC();

    while (1)
    {
        if (processBuffer == 0)
            continue;
        (processBuffer == 1) ? processData(buffer1) : processData(buffer2);
        processBuffer = 0;
    }
    return 0;
}

void config_DMA(GPDMA_LLI_T lli)
{
    GPDMA_Endpoint_T dsrc = {burst : GPDMA_HALFWORD, width : GPDMA_BSIZE_1, increment : DISABLE};
    GPDMA_Endpoint_T ddst = {burst : GPDMA_HALFWORD, width : GPDMA_BSIZE_1, increment : ENABLE};

    GPDMA_Channel_CFG_T dmaCfg;
    dmaCfg.transferSize = 100;
    dmaCfg.channelNum = GPDMA_CH_0;
    dmaCfg.type = GPDMA_P2M;
    dmaCfg.srcMemAddr = (uint32_t)&LPC_ADC->ADGDR;
    dmaCfg.dstMemAddr = (uint32_t)&buffer1;
    dmaCfg.srcConn = (GPDMA_ADC);
    dmaCfg.src = dsrc;
    dmaCfg.dst = ddst;
    dmaCfg.intTC = ENABLE;
    dmaCfg.intErr = DISABLE;
    dmaCfg.linkedList = (uint32_t)&lli;

    GPDMA_Init();
    GPDMA_SetupChannel(&dmaCfg);
    GPDMA_ChannelStart(GPDMA_CH_0);
    NVIC_EnableIRQ(DMA_IRQn);
}

void config_ADC()
{
    ADC_Init(25000);
    ADC_PinConfig(ADC_CHANNEL_4);
    ADC_ChannelEnable(ADC_CHANNEL_4);
    ADC_BurstEnable();
}

void config_DAC()
{
    DAC_Init();
    DAC_SetBias(DAC_350uA);
    DAC_UpdateValue(0);
}

void processData(uint16_t *buf)
{
    uint32_t sum = 0;
    for (int i = 0; i < 100; i++)
    {
        sum += (uint32_t)((buf[i] >> 4) & 0xFFF);
    }
    DAC_UpdateValue((sum / 100) >> 2);
}

void DMA_IRQHandler(void)
{
    if (GPDMA_IntGetStatus(GPDMA_INTTC, GPDMA_CH_0) == SET)
    {
        if (currentBuffer == 0)
        {
            processBuffer = 1; // buffer1 listo
            currentBuffer = 1;
        }
        else
        {
            processBuffer = 2; // buffer2 listo
            currentBuffer = 0;
        }

        GPDMA_ClearIntPending(GPDMA_CLR_INTTC, GPDMA_CH_0);
    }
}
