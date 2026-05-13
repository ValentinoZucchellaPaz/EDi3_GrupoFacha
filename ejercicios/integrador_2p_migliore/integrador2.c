/**
 *
 */

#include "LPC17xx.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_gpdma.h"

// periodo req dac es de 2604us -> con esto obtengo periodo de onda 1 seg para 382 muestras
// periodo req = ticks / PCLK
#define DAC_REQ_TICKS 65100
#define SIGNAL_INCREMENT 5
#define SINGAL_DATA_DIR 0X20080000

// firmas de funciones

/**
 * @brief Configura el DAC para que haga request al DMA
 * de manera que se vayan sacando datos del array con un periodo deseado (1s)
 */
void configDac(void);

/**
 * @brief Configura el DMA para
 * mandar datos desde la posicion de array recibida (de forma incremental)
 * hacia el DAC a pedido del DAC.
 *
 * @param lli Puntero a una GPDMA_LLI_T para conf proximos envios
 * @param arrSrcAddr Puntero al array de datos
 */
void configDma(GPDMA_LLI_T *lli, uint16_t *arrSrcAddr);

/**
 * @brief Toma un array y lo llena con una forma de onda triangular
 * de 382 muestras adecuada para ser mostrada por el DAC.
 * Se ponen los valores ya desplazados a los bits 15:6. (value de DAC)
 *
 * @param arr Puntero al array de datos
 */
void signalGenerator(uint16_t *arr);

int main(void)
{
    // array de datos que se guardan en mem
    uint16_t *signal = (uint16_t)SINGAL_DATA_DIR;

    GPDMA_LLI_T lli;
    lli.srcAddr = (uint32_t)&signal;
    lli.dstAddr = (uint32_t)&LPC_DAC->DACR; // datos ya estan shifteados a 15:6
    lli.nextLLI = (uint32_t)&lli;
    lli.control = 382 | 1 << 18 | 1 << 21 | 1 << 26;

    signalGenerator(signal);
    configDac();
    configDma(&lli, signal);

    while (1)
    {
    };
    return 1;
}

void signalGenerator(uint16_t *arr)
{
    uint16_t counter = 0;
    while (counter < 381)
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

void configDac(void)
{
    DAC_CONVERTER_CFG_T dacConfig;
    dacConfig.dmaCounter = ENABLE;
    dacConfig.dmaRequest = ENABLE;
    dacConfig.doubleBuffer = DISABLE;

    DAC_Init();
    DAC_SetBias(DAC_350uA);

    // pide un nuevo dato cada 2604 us (65100 ticks)
    // son 382 datos => muestro todo en 1s (periodo de onda 1 seg)
    DAC_ConfigDAConverterControl(&dacConfig);
    DAC_SetDMATimeOut(65100);
}

void configDma(GPDMA_LLI_T *lli, uint16_t *arrSrcAddr)
{
    GPDMA_Endpoint_T srcc;
    srcc.burst = GPDMA_BSIZE_1;
    srcc.width = GPDMA_HALFWORD;
    srcc.increment = ENABLE;

    GPDMA_Endpoint_T dstc;
    dstc.burst = GPDMA_BSIZE_1;
    dstc.width = GPDMA_HALFWORD;
    dstc.increment = DISABLE;

    GPDMA_Channel_CFG_T dmaConfig;
    dmaConfig.channelNum = GPDMA_CH_0;
    dmaConfig.transferSize = 382;
    dmaConfig.type = GPDMA_M2P;
    dmaConfig.src = srcc;
    dmaConfig.srcConn = 0;
    dmaConfig.srcMemAddr = (uint32_t)arrSrcAddr;
    dmaConfig.dst = dstc;
    dmaConfig.dstConn = GPDMA_DAC;
    dmaConfig.dstMemAddr = (uint32_t)&LPC_DAC->DACR;
    dmaConfig.linkedList = (uint32_t)lli;

    GPDMA_Init();
    GPDMA_SetupChannel(&dmaConfig);
    GPDMA_ChannelStart(GPDMA_CH_0);
}
