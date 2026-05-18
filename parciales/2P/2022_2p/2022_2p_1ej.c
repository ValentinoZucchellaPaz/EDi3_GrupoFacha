/**
Programar el microcontrolador LPC1769 para que mediante su ADC digitalice dos señales analógicas cuyos anchos de bandas son de 10 Khz cada una. Los canales utilizados deben ser el 2 y el 4 y los datos deben ser guardados en dos regiones de memorias distintas que permitan contar con los últimos 20 datos de cada canal. Suponer una frecuencia de core cclk de 100 Mhz. El código debe estar debidamente comentado.

frec de conversion de adc minima 40kHz (20k por Nyquist x2 por ser dos canales)
    40kHz -> 25us
    hago toggle de timer cada 12us -> 41,6kHz
 */

#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_timer.h"

#define N 20

volatile uint16_t señal1[N];
volatile uint16_t señal2[N];
volatile uint8_t idx1 = 0;
volatile uint8_t idx2 = 0;

void confTimer(void)
{
    TIM_TIMERCFG_T timCfg = {.prescaleOpt = TIM_US, .prescaleValue = 1};
    TIM_MATCHCFG_T matchCfg = {
        .channel = TIM_MATCH_0,
        .extOpt = TIM_TOGGLE,
        .resetEn = ENABLE,
        .matchValue = 12};

    TIM_InitTimer(LPC_TIM0, &timCfg);
    TIM_ConfigMatch(LPC_TIM0, &matchCfg);
    TIM_Enable(LPC_TIM0);
}

void confADC(void)
{
    ADC_Init(200000); // max frec posible, frec de muestreo se lo doy con timer (startCmd)
    ADC_BurstDisable();
    ADC_PinConfig(ADC_CHANNEL_2);
    ADC_PinConfig(ADC_CHANNEL_4);
    ADC_ChannelEnable(ADC_CHANNEL_2);
    ADC_ChannelEnable(ADC_CHANNEL_4);
    ADC_StartCmd(TIM_MAT0_0_P3_25);
    ADC_EdgeStartConfig(ADC_START_ON_FALLING);
    ADC_IntEnable(ADC_INT_CH2);
    ADC_IntEnable(ADC_INT_CH4);
    ADC_PowerUp();
}

void ADC_IRQHandler(void)
{
    if (ADC_ChannelGetStatus(ADC_CHANNEL_2, ADC_DATA_DONE) == SET)
    {
        señal1[idx1] = ADC_ChannelGetData(ADC_CHANNEL_2);
        idx1 = (idx1 + 1) % N;
    }
    if (ADC_ChannelGetStatus(ADC_CHANNEL_4, ADC_DATA_DONE) == SET)
    {
        señal2[idx2] = ADC_ChannelGetData(ADC_CHANNEL_4);
        idx2 = (idx2 + 1) % N;
    }
}

int main(void)
{
    confADC();
    confTimer();

    while (1)
        ;
    return 0;
}