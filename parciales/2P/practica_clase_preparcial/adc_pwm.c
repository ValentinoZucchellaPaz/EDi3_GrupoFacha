#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"

volatile uint32_t calcPromedio = 0;
volatile uint32_t arr[4];

void config_adc(void);
void config_timer0_adc(void);
void config_timer1_pwm(uint32_t duty);
void startPWM(void);
void stopPWM(void);
float avg(void);

int main(void)
{
    config_adc();
    config_timer0_adc();

    uint32_t duty = 50;
    float valorProm;

    while (1)
    {
        if (calcPromedio == 1)
        {
            valorProm = avg();
            if (valorProm <= 1241) // 1V
            {
                stopPWM();
                GPIO_ClearPins(0, 1);
            }
            else if (valorProm <= 2482) // 2V
            {
                float valorTension = (valorProm * 3.3 / 4096.0);
                duty = (uint32_t)(50.0 + (valorTension - 1) * 40); // 1V -> 50%; 2V -> 90%;
                config_timer1_pwm(duty);
                startPWM();
            }
            else
            {
                stopPWM();
                GPIO_SetPins(0, 1);
            }

            calcPromedio = 0;
        }
    }
    return 0;
}

/**
 * @brief Configura adc para convertir a maxima velocidad y que convierta con el cambio asc de MAT01 (cada 30s), luego interrumpe
 */
void config_adc(void)
{
    ADC_Init(200000); // max frec conv
    ADC_ChannelEnable(ADC_CHANNEL_0);
    ADC_BurstDisable();
    ADC_PinConfig(ADC_CHANNEL_0);
    ADC_IntEnable(ADC_CHANNEL_0);
    ADC_StartCmd(ADC_START_ON_MAT01);          // conf para que timer haga trigger cada 30s
    ADC_EdgeStartConfig(ADC_START_ON_FALLING); // MAT0.1 _ _ _|---|_ _ _|---|
    ADC_PowerUp();                             // encendemos el ADC
}

/**
 * @brief Configura timer 0 para que match 1 haga reset y toggle de MAT01 cada 15s, consiguiendo flanco asc de MAT01 cada 30s
 */
void config_timer0_adc(void)
{

    TIM_TIMERCFG_T tim;
    tim.prescaleOpt = TIM_US;
    tim.prescaleValue = 1000000; // TC aumenta cada 1s

    TIM_InitTimer(LPC_TIM0, &tim);
    TIM_MATCHCFG_T matchcfg;
    matchcfg.channel = TIM_MATCH_1;
    matchcfg.intEn = DISABLE;
    matchcfg.stopEn = DISABLE;
    matchcfg.resetEn = ENABLE;
    matchcfg.extOpt = TIM_TOGGLE;
    matchcfg.matchValue = 15;
    TIM_ConfigMatch(LPC_TIM0, &matchcfg);
    TIM_Enable(LPC_TIM0);
}

/**
 * @brief Configura timer 1 para que match 0 haga reset e int cada 50us, match 1 hace una int segun el valor de `duty` (cambiar antes de llamar con el duty cycle deseado)
 */
void config_timer1_pwm(uint32_t duty)
{
    TIM_TIMERCFG_T tim;
    tim.prescaleOpt = TIM_US;
    tim.prescaleValue = 1;

    TIM_MATCHCFG_T match0cfg, match1cfg;
    // MATCH 0
    match0cfg.channel = TIM_MATCH_0;
    match0cfg.intEn = ENABLE;
    match0cfg.stopEn = DISABLE;
    match0cfg.resetEn = ENABLE;
    match0cfg.extOpt = 0;
    match0cfg.matchValue = 50;

    // MATCH 1
    match1cfg.channel = TIM_MATCH_1;
    match1cfg.intEn = ENABLE;
    match1cfg.stopEn = DISABLE;
    match1cfg.resetEn = DISABLE;
    match1cfg.extOpt = 0;
    match1cfg.matchValue = duty / 2; // duty es un valor de 0 a 100, pero como el periodo es 50 lo parto a la mitad

    TIM_InitTimer(LPC_TIM1, &tim);
    TIM_ConfigMatch(LPC_TIM1, &match0cfg);
    TIM_ConfigMatch(LPC_TIM1, &match1cfg);
    NVIC_EnableIRQ(TIMER1_IRQn);
}

// enciende timers de pwm
void startPWM(void)
{
    TIM_Enable(LPC_TIM1);
}

// apaga timers de pwm
void stopPWM(void)
{
    TIM_Disable(LPC_TIM1);
}

// Calcula el promedio de los elementos del array
float avg(void)
{
    float p = 0;
    for (uint32_t i = 0; i < 3; i++)
    {
        p += arr[i];
    }
    return p / 4.0;
}

/**
 * @brief Agrega el elemento convertido al array, luego de 4 conversiones va a setear la flag para que se calcule el promedio en el main.
 */
void ADC_IRQHandler(void)
{
    static int counter = 0;
    if (ADC_GlobalGetStatus(ADC_DATA_DONE))
    {
        arr[counter] = ADC_GlobalGetData(); // array guardando las conversiones
        counter++;
        if (counter == 4)
        {
            calcPromedio = 1;
            counter = 0;
        }
    }
}

/**
 * @brief hace el toggle manual de P0.0 para tener el pwm con `duty` (match1) y periodo 50us -> frec 20kHz (match0)
 * _____|--|_____|--|_____
 * _____|__|-----|__|-----
 * ___MR0_MR1
 */
void TIM1_IRQHandler(void)
{
    if (TIM_GetIntStatus(LPC_TIM1, TIM_MR0_INT) == SET)
    {
        TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT);
        GPIO_SetPinState(PORT_0, PIN_0, 1);
    }

    if (TIM_GetIntStatus(LPC_TIM1, TIM_MR1_INT) == SET)
    {
        TIM_ClearIntPending(LPC_TIM1, TIM_MR1_INT);
        GPIO_SetPinState(PORT_0, PIN_0, 0);
    }
}