#include <LPC17xx.h>

// cant de interrupciones systick 10ms para tener tiempos de 1s, 0.5, 0.4, 0.3, 0.2
volatile int tiempos [5] = {100, 50, 40, 30, 20};
volatile int frec_actual = 0; // indice de frec actual

// led en P0.11
void led_init(void){
    LPC_PINCON -> PINSEL0 &= ~((0<<22)|(0<<23));
    LPC_GPIO0 -> FIODIR |= (1<<11);
    LPC_GPIO0 -> FIOCLR |= (1<<11);
}

// boton en P0.25 -> entrada, cuando presione pongo 0, sino pin esta en 1
void button_init(void) {
    LPC_PINCON -> PINSEL1 &= ~((0<<19)|(0<<18));
    LPC_PINCON -> PINMODE1 &= ~((0<<19)|(0<<18)); // pullup
    LPC_GPIO0 -> FIODIR &= ~(1<<25);

    // config gpio int por flanco desc (asumo sin rebote)
    LPC_GPIOINT -> IO0IntEnR &= ~(1<<25); // limpio
    LPC_GPIOINT -> IO0IntEnF &= ~(1<<25); // limpio
    LPC_GPIOINT -> IO0IntEnF |= (1<<25); // enable
    LPC_GPIOINT -> IO0IntClr |= (1<<25); // limpio flag

    // habilito nvic
    NVIC -> ISER[0] = (1<<EINT3_IRQn); // (1<<18)
}

void systick_init(uint32_t ticks){
	SysTick -> CTRL = 0; // apaga

	SysTick -> LOAD = ticks - 1; // ms = ticks / 100000

	SysTick -> VAL = 0; // valor del que empieza y baja hasta 0

	SysTick -> CTRL |= (1<<2) | (1<<1) | 1; // 0 enable, 1 habilito int, 2 fuente clk del core
}

void toggle_led(void) {
    LPC_GPIO0 -> FIOPIN ^= (1<<11);
}

int main(void) {
    led_init();
    button_init();
    systick_init(1000000); // 10ms

    while (1) { }
    return 0;
}

void SysTick_Handler(void){
    static int counter = 0;
    counter++;

    // cuando llego a cant de int igual a las previstas por el array para la frec actual hago toggle y reseteo contador
    if (counter >= tiempos[frec_actual]) {
        toggle_led();
        counter = 0;
    }
}

void EINT3_IRQHandler(void) {
    // asumo sin rebotes
    // aumento indice frec sin pasarme
    if (LPC_GPIOINT->IO0IntStatF & (1<<25)){
        LPC_GPIOINT->IO0IntClr |= (1<<25);
        frec_actual++;
        if (frec_actual >=5){
            frec_actual=0;
        }
    }
}
