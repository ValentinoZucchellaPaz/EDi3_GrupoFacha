/*
Autores: Valentino Zucchella Paz, Pedro Guzmán, Tomás Garay
TP3 ED3 : hacer un PWM con el modulo Timer0
tal que se module la intensidad de un led,
de forma que crezca en 100 pasos del min a max
y luego decrementa (de nuevo en 100 pasos de max a min)
Esto resulta en fadein completo en 1seg
*/

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

// #include <cr_section_macros.h>

#include <stdio.h>

// Valores de match calculados con PCLK = 25MHz (tick = 40ns)
#define MR1_PERIOD    25000      // 1ms  - periodo PWM
#define DUTY_STEP     250        // paso de incremento (250 ticks = 10us ~ 1% de 1ms)
#define PERIODS_10MS  10         // 10 periodos de 1ms = 10ms

// Flags para duty cycle
volatile uint32_t flag_periodo = 0;
volatile uint32_t apagar_led = 0;

void led_init(void){
    // LED de P0.22 como salida GPIO, comienza off
    LPC_PINCON->PINSEL1 &= ~(0x3 << 12);
    LPC_GPIO0->FIODIR   |=  (1  << 22);
    LPC_GPIO0->FIOSET    =  (1  << 22);
}

void timer_init(void) {
    // habilito timer (ya viene asi por reset igual)
    LPC_SC->PCONP |= (1 << 1);

    // Config: reseteo, sin prescaler, cargo registros de match
    LPC_TIM0->TCR = 0x02;
    LPC_TIM0->PR  = 0;
    LPC_TIM0->MR0 = 0; // duty cycle, varia (comienza en 0)
    LPC_TIM0->MR1 = MR1_PERIOD;

    // Match Control Register
    // MR0: solo interrupcion (apaga led)
    // MR1: interrupcion (prende led, aumenta contador de periodos) + reset TC
    LPC_TIM0->MCR = (1 << 0) |             // MR0I: interrupcion en match MR0
                    (1 << 3) |             // MR1I: interrupcion en match MR1
                    (1 << 4);              // MR1R: reset TC en match MR1

    // Habilito int de timer en nvic
    NVIC->ISER[0]=(1<<TIMER0_IRQn);

    // arranca timer
    LPC_TIM0->TCR = 0x01;
}

int main(void) {
    led_init();
    timer_init();
    uint8_t contador = 0;
    uint32_t duty = 0;
    uint8_t fade_in = 1;    // 1 = fade in, 0 = fade out

    while(1) {
        if (apagar_led==1)
        {
            LPC_GPIO0->FIOSET = (1 << 22);
            apagar_led=0;
        }

        if(flag_periodo==1)
        {
            // cuento 10 MR1 (10ms) entonces:
            // 1. actualizo duty cycle segun fade in/out
            // 2. prendo led (inicio de nuevo periodo)
            // incrementos (fade in) y decrementos (fade out) se hacen con saltos (DUTY_STEP)

            flag_periodo=0;
            contador++;
            
            if (contador >= PERIODS_10MS) {
                contador = 0;

                if (duty > 0)
                    LPC_GPIO0->FIOCLR = (1 << 22); // prendo led

                if (fade_in) {
                    duty += DUTY_STEP;
                    // si llego a max (MR1) comienzo a hacer fade out
                    if (duty >= MR1_PERIOD) {
                        duty    = MR1_PERIOD;
                        fade_in = 0;
                    }
                } else {
                    // si llego a min (MR0 inicial) comienzo a hacer fade in
                    if (duty <= DUTY_STEP) {
                        duty    = 0;
                        fade_in = 1;
                    } else {
                        duty -= DUTY_STEP;
                    }
                }

                // cargo cambios a MR0
                LPC_TIM0->MR0 = duty;
            }
        }
    }

    return 0;
}

// --- ISR del Timer0 ---
void TIMER0_IRQHandler(void) {

    // MR0 match: apagar LED
    if (LPC_TIM0->IR & (1 << 0)) {
        LPC_TIM0->IR = (1 << 0);
        apagar_led=1;
    }

    // MR1 match: inicio de nuevo periodo, encender LED, actualizo periodo de MR0
    if (LPC_TIM0->IR & (1 << 1)) {
        // limpio flag
        LPC_TIM0->IR = (1 << 1);
        flag_periodo = 1;
    }
}