/**
 * sacar ondas cuadradas por P0.[3-0]
 * almacenar secuencia en ahb sram bank0
 * => periodos de 1, 2, 4, y 8 ms
 *        => toogle cada 0.5ms la secuencia (van a aumentar con el menor paso)
 *              => 16 elementos en arr
 * transf continua con dma sin uso de int
 */

#include "LPC17xx.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_timer.h"

#define sram_bank0 ((uint32_t)0x2007C000)
volatile uint8_t *arr = (uint8_t *)sram_bank0;

void configDMA(GPDMA_LLI_T *lliptr)
{
    GPDMA_Channel_CFG_T dmaCfg;
    dmaCfg.channelNum = GPDMA_CH_0;
    dmaCfg.transferSize = 16;
    dmaCfg.type = GPDMA_M2P;
    dmaCfg.intErr = DISABLE;
    dmaCfg.intTC = DISABLE;
    dmaCfg.linkedList = (uint32_t)lliptr;

    // src y dst config
    dmaCfg.src.width = GPDMA_BYTE;
    dmaCfg.src.burst = GPDMA_BSIZE_1;
    dmaCfg.src.increment = ENABLE;
    dmaCfg.dst.width = GPDMA_WORD;
    dmaCfg.dst.burst = GPDMA_BSIZE_1;
    dmaCfg.dst.increment = DISABLE;
    dmaCfg.srcMemAddr = (uint32_t)arr;
    dmaCfg.dstMemAddr = (uint32_t)&LPC_GPIO0->FIOPIN;
    dmaCfg.srcConn = GPDMA_MAT0_0;
    dmaCfg.dstConn = 0;

    GPDMA_Init();
    GPDMA_SetupChannel(&dmaCfg);
    GPDMA_ChannelStart(GPDMA_CH_0);
}

void configTimer0(void)
{
    TIM_TIMERCFG_T timCfg;
    timCfg.prescaleOpt = TIM_US;
    timCfg.prescaleValue = 2499; // -> 1match = 0.1ms

    TIM_MATCHCFG_T matchCfg;
    matchCfg.channel = 0;
    matchCfg.extOpt = TIM_TOGGLE;
    matchCfg.intEn = DISABLE;
    matchCfg.resetEn = ENABLE;
    matchCfg.stopEn = DISABLE;
    matchCfg.matchValue = 5;

    TIM_InitTimer(LPC_TIM0, &timCfg);
    TIM_ConfigMatch(LPC_TIM0, &matchCfg);
    TIM_Enable(LPC_TIM0);
}

void configPines(void)
{
    GPIO_SetDir(PORT_0, 0xF, GPIO_OUTPUT);
}

int main(void)
{
    // inicializo arr (representa las 4 ondas en cada instante)
    for (int i = 0; i < 16; i++)
        arr[i] = i;

    return 1;

    GPDMA_LLI_T lli;
    lli.srcAddr = (uint32_t)arr;
    lli.dstAddr = (uint32_t)&LPC_GPIO0->FIOPIN;
    lli.nextLLI = &lli;
    lli.control = 16 | (1 << 22) | (1 << 26);

    configPines();
    configDMA(&lli);

    while (1)
    {
    }
    return 1;
}