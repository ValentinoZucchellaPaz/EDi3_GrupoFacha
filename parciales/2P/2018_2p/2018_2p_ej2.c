#include "LPC17xx.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpdma.h"

void configDMA(void)
{
    GPDMA_Channel_CFG_T chCfg = {
        .channelNum = 0,
        .transferSize = 256,
        .type = GPDMA_M2P,
        .src = {.burst = GPDMA_BSIZE_1, .increment = ENABLE, .width = GPDMA_HALFWORD},
        .dst = {.burst = GPDMA_BSIZE_1, .increment = ENABLE, .width = GPDMA_HALFWORD},
        .srcMemAddr = (uint32_t)0x10000800,
        .dstConn = TIM_MAT0_0_P3_25};
    GPDMA_Init();
    GPDMA_SetupChannel(&chCfg);
    GPDMA_ChannelStart(GPDMA_CH_0);
    LPC_GPDMACH0->DMACCDestAddr = 0x10002800;
}

void configTimer(void)
{
    TIM_TIMERCFG_T timCfg = {
        .prescaleOpt = TIM_US,
        .prescaleValue = 1};
    TIM_MATCHCFG_T matchCfg = {
        .channel = TIM_MATCH_0,
        .extOpt = TIM_TOGGLE,
        .matchValue = 10,
        .resetEn = ENABLE};

    TIM_InitTimer(LPC_TIM0, &timCfg);
    TIM_ConfigMatch(LPC_TIM0, &matchCfg);
    TIM_Enable(LPC_TIM0);
}

int main(void)
{
    configDMA();
    configTimer();

    while (1)
    {
    }
    return 0;
}