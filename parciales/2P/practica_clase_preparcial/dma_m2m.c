/*
 1)dma_m2m
Mediante la utilizacio del microcontrolador LPC1769, implemente el software necesario para cumplir con los siguientes requerimentos

1-Usar el hardware GPDMA para transferir los datos en palabras de 2bytes desde la primera mitad de la SRAM del banco 1 hacia la segunda mitad d ela SRAM del banco 0

direccion- 0x2007 C000 - 0x2007 FFFF  AHB SRAM -Bank 0
direccion- 0x2008 0000 - 0x2008 3FFF  AHB SRAM -Bank 1

2-Una vez finalizada la transferencia, mediante la interrupcion por cuenta terminal del GPDMA, se debe encender un led indicador conectado a P0.22

3-Determine usted Cuantos bytes fueron transferidos
 */

#include "LPC17xx.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_gpio.h"

#define bank1_start 0x20080000 // primer mitad banco 1
#define bank0_start 0x2007E000 // segunda mitad banco 0

void config_gpdma(void)
{
    GPDMA_Endpoint_T srcc = {width : GPDMA_HALFWORD, burst : GPDMA_BSIZE_1, increment : ENABLE};
    GPDMA_Endpoint_T dstc = {width : GPDMA_HALFWORD, burst : GPDMA_BSIZE_1, increment : ENABLE};

    GPDMA_Channel_CFG_T dmaCfg;
    dmaCfg.channelNum = 0;   // Canal menos prioritario
    dmaCfg.type = GPDMA_M2M; // tranferencia MEM->MEM
    dmaCfg.srcMemAddr = (uint32_t)bank1_start;
    dmaCfg.dstMemAddr = (uint32_t)bank0_start;
    dmaCfg.transferSize = 0x0FFF; // 4096
    dmaCfg.src = srcc;
    dmaCfg.dst = dstc;

    GPDMA_Init();
    GPDMA_SetupChannel(&dmaCfg);
    NVIC_EnableIRQ(DMA_IRQn);
    GPDMA_ChannelStart(0);
}
void config_LED(void)
{
    GPIO_SetDir(PORT_0, (1 << 22), GPIO_OUTPUT);
    GPIO_SetPinState(PORT_0, PIN_22, SET);
}

void DMA_IRQHandler(void)
{
    if (GPDMA_IntGetStatus(GPDMA_INTTC, 0))
    {
        GPDMA_ClearIntPending(GPDMA_INTTC, 0);   // limpiar bandera
        GPIO_SetPinState(PORT_0, PIN_22, RESET); // Enciende por bajo
    }
    if (GPDMA_IntGetStatus(GPDMA_INTERR, 0))
    {
        GPDMA_ClearIntPending(GPDMA_INTERR, 0); // limpiar bandera
    }
}

int main(void)
{

    config_LED();
    config_gpdma();
    while (1)
    {
        __asm volatile("nop");
    }
    return 0;
}
