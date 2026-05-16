/**
 * Utilizando un TIMER y un pin de CAPTURE
 * 1- demodular una señal PWM que ingresa por dicho pin (calcular el ciclo de trabajo y el periodo)
 * 2- sacar una tensión continua proporcional y guardar en buffer circular de 10 elementos
 * 3- sacar promedio de 10 datos a través del DAC (rango dinámico 0-2V y actualizar cada 0,5s)
 */

#include "LPC17xx.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_systick.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_gpio.h"
// #include "lpc17xx_gpdma.h"

void configTimer(void);
void configDAC(void);
void configSystick(void);
void pushBuffer(int32_t dc_val);

volatile uint32_t pwm_dc_prop[10];
volatile uint32_t cur_start = 0;  // tiempo de rising edge de pwm
volatile uint32_t prev_start = 0; // cuando viene un nuevo rising edge actualizo
volatile uint32_t high_time = 0;  // tiempo de falling edge de pwm - cur_start
volatile uint32_t period = 0;     // cur_start - prev_start
volatile float duty = 0;          // (high_time/period) rango[0;1]

volatile uint32_t pin_status = 0;
// volatile uint32_t prev_pin_status = 0;

int main(void)
{
    while (1)
    {
    }
    return 0;
}

/**
 * @brief Configura el timer para que aumente la cuenta cada 1us y que el CAP0.0 capture e interrumpa en flanco asc y desc
 *
 * @note El máximo tiempo que funcion sin error es 71 minutos, ya que el contador del timer nunca se resetea va a hacer ovf
 */
void configTimer(void)
{
    TIM_TIMERCFG_T timer_cfg;
    timer_cfg.prescaleOpt = TIM_US;
    timer_cfg.prescaleValue = 1;

    // cap0
    TIM_CAPTURECFG_T cap_cfg;
    cap_cfg.channel = TIM_CAPTURE_0;
    cap_cfg.risingEn = ENABLE;
    cap_cfg.fallingEn = ENABLE;
    cap_cfg.intEn = ENABLE;

    TIM_InitTimer(LPC_TIM0, &timer_cfg);
    TIM_PinConfig(TIM_CAP0_0_P1_26);
    TIM_ConfigCapture(LPC_TIM0, &cap_cfg);
    NVIC_EnableIRQ(TIMER0_IRQn);
    TIM_Enable(LPC_TIM0);
}

/**
 * @brief Configura el systick para contar 100ms y lanzar int
 */
void configSystick(void)
{
    SYSTICK_InternalInit(100);
    SYSTICK_IntCmd(ENABLE);
}

/**
 * @brief Cuenta 5 interrupciones (500ms) y luego hace promedio y actualiza valor de DAC
 */
void SysTick_Handler(void)
{
    static uint8_t systick_counter = 0;
    systick_counter++;
    if (systick_counter == 5)
    {
        systick_counter = 0; // reset
        // hacer promedio y pasar a dac
        uint32_t sum = 0;
        for (int i = 0; i < 10; i++)
        {
            sum += pwm_dc_prop[i];
        }
        sum = (uint32_t)sum / 10;
        DAC_UpdateValue(sum);
    }
}

/**
 * @brief Cuando viene una interrupcion de timer (capture), veo si fue rising o falling (segun el estado del pin), luego actualizo el duty (high_time/period) y finalmente cargo el valor en el buffer circular
 */
void TIMER0_IRQHandler(void)
{
    if (TIM_GetIntStatus(LPC_TIM0, TIM_CR0_INT) == SET)
    {
        TIM_ClearIntPending(LPC_TIM0, TIM_CR0_INT);
        pin_status = (GPIO_ReadValue(PORT_1) & (1 << 26)) >> 26;

        if (pin_status == 1) // rising edge
        {
            prev_start = cur_start;
            cur_start = TIM_GetCaptureValue(LPC_TIM0, TIM_CAPTURE_0);

            if (prev_start == 0) // en primer iteracion no saco periodo, espero
                return;

            // manejo ovf
            if (cur_start < prev_start)
            {
                period = (0xFFFFFFFF - prev_start) + cur_start + 1;
            }

            period = cur_start - prev_start;
            duty = ((float)high_time / (float)period);

            // Veq = Vmax * duty = 2 * duty
            // 3.3 -> 1023
            // 2   -> 620
            // Veq -> 620 * (2 * duty) / 2 = 620*duty

            pushBuffer((uint32_t)(duty * 620));
        }
        else // falling edge
        {
            high_time = TIM_GetCaptureValue(LPC_TIM0, TIM_CAPTURE_0) - cur_start;
        }
    }
}

void pushBuffer(int32_t dc_val)
{
    static uint32_t counter = 0;
    pwm_dc_prop[counter] = dc_val;
    counter = (counter + 1) % 10;
}
