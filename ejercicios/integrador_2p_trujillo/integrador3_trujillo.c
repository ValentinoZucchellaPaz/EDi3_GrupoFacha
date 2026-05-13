#include "LPC17xx.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_gpio.h"

#define PATTERN1_MEMADDR 0x2007C000
#define PATTERN2_MEMADDR 0x2007C200 // 512 posiciones mas

volatile uint8_t *pattern1 = (uint8_t *)PATTERN1_MEMADDR;
volatile uint8_t *pattern2 = (uint8_t *)PATTERN2_MEMADDR;

volatile uint8_t pulsador = 0;

void configGPIOInt(void);
void configDMA(GPDMA_CH ch, uint8_t *srcAddr, uint32_t *dstAddr);
uint32_t DMA_GetLowestPriorityFreeChannel(void);

int main(void)
{
    configGPIOInt();
    while (1)
    {
        if (pulsador == 1)
        {
            configDMA(DMA_GetLowestPriorityFreeChannel(), (uint8_t *)pattern1, (uint32_t *)&LPC_GPIO0->FIOPIN);
            pulsador = 0;
        }
        if (pulsador == 2)
        {
            configDMA(DMA_GetLowestPriorityFreeChannel(), (uint8_t *)pattern2, (uint32_t *)&LPC_GPIO1->FIOPIN);
            pulsador = 0;
        }
    }
    return 1;
}

void configGPIOInt(void)
{
    GPIO_SetDir(PORT_2, 0x03, GPIO_INPUT);
    GPIO_SetDir(PORT_0, 0xFF, GPIO_OUTPUT);
    GPIO_SetDir(PORT_1, 0xFF, GPIO_OUTPUT);

    GPIO_IntConfigPin(PORT_2, PIN_0, GPIO_INT_FALLING, ENABLE);
    GPIO_IntConfigPin(PORT_2, PIN_1, GPIO_INT_FALLING, ENABLE);
    NVIC_EnableIRQ(EINT3_IRQn);
}

void configDMA(GPDMA_CH ch, uint8_t *srcAddr, uint32_t *dstAddr)
{
    if (ch < 0)
        return;

    GPDMA_Endpoint_T srcc = {width : GPDMA_BYTE, burst : GPDMA_BSIZE_1, increment : ENABLE};
    GPDMA_Endpoint_T dstc = {width : GPDMA_WORD, burst : GPDMA_BSIZE_1, increment : DISABLE};

    GPDMA_Channel_CFG_T dmaCfg;
    dmaCfg.channelNum = ch;
    dmaCfg.type = GPDMA_M2M;
    dmaCfg.transferSize = 512;
    dmaCfg.src = srcc;
    dmaCfg.srcConn = 0;
    dmaCfg.srcMemAddr = (uint32_t)srcAddr;
    dmaCfg.dst = dstc;
    dmaCfg.dstConn = 0;
    dmaCfg.dstMemAddr = (uint32_t)dstAddr;
    dmaCfg.intErr = DISABLE;
    dmaCfg.intTC = ENABLE;
    dmaCfg.linkedList = 0;

    GPDMA_Init();
    GPDMA_SetupChannel(&dmaCfg);
    GPDMA_ChannelStart(ch);
    NVIC_EnableIRQ(DMA_IRQn);
}

uint32_t DMA_GetLowestPriorityFreeChannel(void)
{
    uint32_t enabled;

    enabled = LPC_GPDMA->DMACEnbldChns;

    for (uint32_t ch = 7; ch >= 0; ch--)
    {
        if ((enabled & (1 << ch)) == 0)
        {
            return ch;
        }
    }

    return -1;
}

void EINT3_IRQHandler(void)
{
    if (GPIO_GetPinIntStatus(PORT_2, PIN_0, GPIO_INT_FALLING))
    {
        GPIO_ClearInt(PORT_2, 1);
        pulsador = 1;
    }

    if (GPIO_GetPinIntStatus(PORT_2, PIN_1, GPIO_INT_FALLING))
    {
        GPIO_ClearInt(PORT_2, 2);
        pulsador = 2;
    }
}

void DMA_IRQHandler(void)
{
    // cuando termine una transf debo limpiar canal que termino para que se vuelva a usar
    for (int ch = 0; ch < 8; ch++)
    {
        // limpio interrupcion
        if (GPDMA_IntGetStatus(GPDMA_INTTC, ch))
        {
            GPDMA_ClearIntPending(GPDMA_CLR_INTTC, ch);
            GPDMA_ChannelStop(ch);
        }
        // limpio errores
        if (GPDMA_IntGetStatus(GPDMA_INTERR, ch))
        {
            GPDMA_ClearIntPending(GPDMA_CLR_INTERR, ch);
            GPDMA_ChannelStop(ch);
        }
    }
}
