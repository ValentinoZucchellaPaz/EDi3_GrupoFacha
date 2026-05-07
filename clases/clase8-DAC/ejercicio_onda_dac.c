// hacer onda serrucho con periodo 1ms y 10 cambios por periodo

#include "LPC17xx.h"

#include "lpc17xx_dac.h"
#include "lpc17xx_timer.h"

uint16_t values[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

void configDAC(void);
void configTimer(void);

int main(void) {
    configDAC();
    configTimer();
    while(1){
    }
    return 1;
}

void configTimer(void) {
    // config prescaler struct
    TIM_TIMERCFG_T prescalerConfig;
    prescalerConfig.prescaleOpt = TIM_US;
    prescalerConfig.prescaleValue = 10;

    // config match0 struct
    TIM_MATCHCFG_T match0Config;
    match0Config.channel = TIM_MATCH_0;
    match0Config.extOpt = TIM_NOTHING;
    match0Config.intEn = ENABLE;
    match0Config.resetEn = ENABLE;
    match0Config.stopEn = DISABLE;
    match0Config.matchValue = 9;


    TIM_InitTimer(LPC_TIM0, &prescalerConfig);
    TIM_ConfigMatch(LPC_TIM0, &match0Config);
    NVIC_EnableIRQ(TIMER0_IRQn);
    TIM_Enable(LPC_TIM0);
}

void configDAC(){
	DAC_Init();
	DAC_SetBias(DAC_350uA);
	DAC_UpdateValue(0);
}

void TIMER0_IRQHandler(void){
    static uint8_t onda_count = 0;
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)){
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
        onda_count = (onda_count + 1) % 10;
        DAC_UpdateValue((1023/9)* values[onda_count]);
    }
}
