/**
 Mediante la utilización del microcontrolador LPC1769, implemente el software necesario para cumplir con los siguientes requerimientos:
1. Usar el hardware GPDMA para transferir datos en palabras de 2 bytes desde la primera mitad de la SRAM del banco 1 hacia la segunda mitad de la SRAM del banco 0
2. Una vez finalizada la transferencia, mediante la interrupción por cuenta terminal del GPDMA, se debe encender un LED indicador conectado a P0.22.
3. Determine usted Cuántos bytes fueron transferidos => Se transfirieron 8k bytes (8192)
 */

#include "LPC17xx.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"

#define SRAM1_COMIENZO 0x20080000
#define SRAM0_MEDIO 0x2007E000

void confPin(void)
{
    // PINSEL_CFG_T pinCfg = {.port=PORT_0,.pin=PIN_22, .mode=PINSEL_TRISTATE, .openDrain=DISABLE, .func=PINSEL_FUNC_00};
    // PINSEL_ConfigPin(&pinCfg);
    GPIO_SetDir(PORT_0, PIN_22, GPIO_OUTPUT);
}
void confDMA(void)
{
    GPDMA_Channel_CFG_T chCfg = {
        .channelNum = GPDMA_CH_0,
        .type = GPDMA_M2M,
        .transferSize = 4095, // 8k de memoria en datos de 16bits son 4k, pero max es 4095, se pierde un dato
        .src = {.burst = GPDMA_BSIZE_1, .increment = ENABLE, .width = GPDMA_HALFWORD},
        .dst = {.burst = GPDMA_BSIZE_1, .increment = ENABLE, .width = GPDMA_HALFWORD},
        .srcMemAddr = (uint32_t)SRAM1_COMIENZO,
        .dstMemAddr = (uint32_t)SRAM0_MEDIO,
        .intTC = ENABLE,
        .intErr = ENABLE};

    GPDMA_Init();
    GPDMA_SetupChannel(&chCfg);
    GPDMA_ChannelStart(GPDMA_CH_0);
}

void DMA_IRQHandler(void)
{
    if (GPDMA_IntGetStatus(GPDMA_INTTC, GPDMA_CH_0) == SET)
    {
        GPDMA_ClearIntPending(GPDMA_INTTC, GPDMA_CH_0);
        GPIO_SetPins(PORT_0, (1 << 22));
    }
    if (GPDMA_IntGetStatus(GPDMA_INTERR, GPDMA_CH_0) == SET)
    {
        GPDMA_ClearIntPending(GPDMA_INTERR, GPDMA_CH_0);
    }
}

int main(void)
{
    confPin();
    confDMA();

    while (1)
    {
    }
    return 0;
}
