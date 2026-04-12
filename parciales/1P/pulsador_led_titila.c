/*
Escribir un programa que haga que un led titile cuando se presiona un boton
*/

#include <LPC17xx.h>

#define TICKS_1SEG 10000000
volatile uint32_t pulsado = 0; // flag para saber cuando este presionado el boton

void systick_init(uint32_t ticks) {
    SysTick->CTRL = 0;
    SysTick->LOAD = ticks -1;
    SysTick->VAL = 0;
    SysTick->CTRL |= (1<<2) | (1<<1) | (1<<0);
}

void led_init(void){
    LPC_PINCON->PINSEL0 &= ~(3<<12); // led de placa P0.22
    LPC_PINCON->PINSEL0 &= ~(2<<12); // sin pullup
    LPC_GPIO0->FIODIR |= (1<<22);    // salida
    LPC_GPIO0->FIOSET |= (1<<22); // apagado
}

void button_init(void) {
    LPC_PINCON->PINSEL0 &= ~(3<<0); // boton en P0.0
    LPC_PINCON->PINMODE0 &= ~(3<<0);// pullup activa
    LPC_GPIO0->FIODIR &= ~(1<<0);    // entrada
    // vcc --- pullup --- pin --- boton --- gnd
    
    // interrupcion: va a tener por alto y por bajo para determinar cuando se suelta el boton
    LPC_GPIOINT -> IO0IntEnF &= ~1; // limpio
    LPC_GPIOINT -> IO0IntEnR &= ~1; // limpio
    LPC_GPIOINT -> IO0IntEnF |= 1;  // enable
    LPC_GPIOINT -> IO0IntEnR |= 1;  // enable
    LPC_GPIOINT -> IO0IntClr |= 1;  // limpio flag

    // habilito nvic
    NVIC->ISER[0] |= (1<<EINT3_IRQn); // o el 18
}

void toggle_led(void) {
    LPC_GPIO0->FIOPIN^=1;
}

int main(void){
    led_init();
    button_init();
    systick_init(TICKS_1SEG);
    while(1){}
    return 0;
}


// asumo sin rebote
void EINT3_IRQHandler(void){
    // se presiona
    if (LPC_GPIOINT->IO0IntStatF & 1){
        presionado=1;
        LPC_GPIOINT -> IO0IntClr |= 1;
    }
    // se suelta
    if (LPC_GPIOINT->IO0IntStatR & 1){
        presionado=0;
        LPC_GPIOINT -> IO0IntClr |= 1;
    }
}

void SysTick_Handler(void) {
    if (presionado == 1){
        toggle_led();
    }
}
