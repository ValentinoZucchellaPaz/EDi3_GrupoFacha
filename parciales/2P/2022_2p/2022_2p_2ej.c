/**
Utilizando el timer0, un dac, interrupciones y el driver del LPC1769 , escribir un código que permita generar una señal triangular periódica simétrica, que tenga el mínimo periodo posible, la máxima excursión de voltaje pico a pico posible y el mínimo incremento de señal posible por el dac. Suponer una frecuencia de core cclk de 100 Mhz. El código debe estar debidamente comentado.

señal triangular periódica simétrica -> Δ
mínimo periodo posible -> DAC saca a 1MHz max -> 1us cada muestra
máxima excursión de voltaje pico a pico -> 0 a 3.3V
mínimo incremento de señal posible por el dac -> 0 a 1023 y 1023 a 0

aumenta de 1 en 1 hasta 1023 y comienza a bajar de 1023 a 0
cada muestra se saca a 1us -> 2048 muestras

el timer lanza una interrupcion cada 1us y cuando
 */

#include "LPC17xx.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_timer.h"

volatile uint32_t signal = 0;
volatile uint32_t bajada_flag = 0;

// configuro Timer0 para que Match0 cuente 1us e interrumpa
void configTimer(void)
{
    TIM_TIMERCFG_T timCfg = {.prescaleOpt = TIM_TICK, .prescaleValue = 0};
    TIM_MATCHCFG_T matchCfg = {
        .channel = TIM_MATCH_0,
        .intEn = ENABLE,
        .resetEn = ENABLE,
        .matchValue = 25};

    // cuenta 1us y tira una interrupcion
    TIM_InitTimer(LPC_TIM0, &timCfg);
    TIM_ConfigMatch(LPC_TIM0, &matchCfg);
    NVIC_EnableIRQ(TIMER0_IRQn);
    TIM_Enable(LPC_TIM0);
}

void configDAC(void)
{
    DAC_Init();
    DAC_SetBias(DAC_700uA);
    DAC_UpdateValue(0);
}

void TIMER0_IRQHandler(void)
{
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT) == SET)
    {
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);

        // aca va la salsa
        DAC_UpdateValue(signal);

        if (signal == 1023)
            bajada_flag = 1;
        else if (signal == 0)
            bajada_flag = 0;

        (bajada_flag == 0) ? (signal++) : (signal--);
    }
}

int main(void)
{
    configDAC();
    configTimer();
    while (1)
    {
    };
    return 0;
}
