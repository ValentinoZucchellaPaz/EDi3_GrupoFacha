/*
Implementar un sistema embebido en la LPC1769 que permita controlar la intensidad de un LED (P0.22) mediante un potenciómetro (AD0.2 en P0.25), utilizando el ADC y un PWM generado con Timer.
Debo:
    1- Leer periódicamente el valor del ADC (posición del potenciómetro)
    2- Convertir ese valor en un duty cycle
    3- Generar un PWM con Timer0 que controle el brillo del LED

PWM:
    - Periodo: 1 ms (1 kHz)
    - MR1 → periodo
    - MR0 → duty
    (MR1: reinicia el timer, MR0: apaga el LED)

Frecuencia de Placa: 100MHz
Frecuencia de periféricos: 25 MHz
Frecuencia de muestreo: 200k Hz -> Frec ADC = 200000 * 65=13MHz = PCLK/AD_CLKDIV -> CLKDIV ~= 2
Resolución ADC: 12 bits -> uso 10 superiores para tener 
Resolución PWM: mínimo 100 niveles -> PASO=MR1/100
*/

#include "LPC17xx.h"

#define MR1_PERIOD 100

// prototipos
void config_led(void);
void config_timer0(void);
void config_adc(void);

volatile uint32_t duty=0;
volatile uint32_t apagar_led_flag =0;
volatile uint32_t actualizar_duty_flag =0;
volatile uint32_t conversion_ready =0;
volatile uint32_t conversion_value =0;


int main(void){
    config_led();
    config_timer0();
    config_adc();
    while(1){
        if(conversion_ready==1 && actualizar_duty_flag==1)
        {
            duty= (conversion_value*MR1_PERIOD)/4095;
            LPC_TIM0->MR0=duty;

            actualizar_duty_flag=0;
            conversion_ready=0;
        }
        if (apagar_led_flag==1)
        {
            apagar_led();
            apagar_led_flag=0;
        }

    }
    return 1;
}

void config_timer0(void)
{
    // ya viene prendido y con PCLK 25MHz
    LPC_TIM0->TCR=0;
    LPC_TIM0->PR=249; // TC = (PR+1)/PCLK = 250 /(25 * 10^6) =  10 us
    LPC_TIM0->MR0=duty;
    LPC_TIM0->MR1=MR1_PERIOD;
    LPC_TIM0->MCR|= (1<<0) | (1<<3) | (1<<4); // MR0 dispara int (apagar led) MR1 int y reset (prende led y carga nuevo duty)
    
    LPC_TIM0->TCR=2;
    NVIC->ISER[0]=(1<<TIMER0_IRQn);
    LPC_TIM0->TCR=1;
}

void config_adc(void)
{
    LPC_SC->PCONP |= (1<<12); // prendo adc
    LPC_ADC->ADCR |= (1<<21); // on
    LPC_ADC->ADCR &=~(1<<16); // no burst
    LPC_ADC->ADCR |= (1<<2); // canal 2
    LPC_ADC->ADCR |= (2<<8); // clkdiv
    LPC_ADC->ADCR &= ~(7<<24); // no start
    LPC_ADC->ADINTEN |= (1<<2); // habilito int de canal 2
    NVIC->ISER[0]=(1<<ADC_IRQn);
}

void config_led(void)
{
    LPC_PINCON->PINSEL1 &= ~(3<<12);
    LPC_GPIO0->FIODIR |= (1<<22);
    LPC_GPIO0->FIOSET = (1<<22);
}

void apagar_led(void){
    LPC_GPIO0->FIOSET|=(1<<22);
}
void prender_led(void){
    LPC_GPIO0->FIOCLR|=(1<<22);
}

void TIMER0_IRQHandler(void){
    if((LPC_TIM0->IR & 1)==1)
    { 
        // mr0 match
        LPC_TIM0->IR|=(1<<0);
        apagar_led_flag = 1;
    }
    if((LPC_TIM0->IR & 2)==1)
    { 
        // mr1 match
        LPC_TIM0->IR|=(1<<1);
        LPC_ADC->ADCR |= (1<<24); // start conversion
        prender_led();
        actualizar_duty_flag = 1;
    }
}

void ADC_IRQHandler(void) {
    conversion_ready = 1;
    conversion_value = (LPC_ADC->ADDR2>>4) & 0xFFF;
}