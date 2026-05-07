/*
 * Configurar la interrupción externa EINT1 para que interrumpa por flanco de bajada
 * y la interrupción EINT2 para que interrumpa por flanco de subida.
 *
 * Considerar que EINT1 tiene mayor prioridad que EINT2.
 *
 * En la interrupción por flanco de bajada configurar el systick para desbordar cada 25 mseg,
 * mientras que en la interrupción por flanco de subida configurarlo para que desborde cada 60 mseg.
 *
 *
 */

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>

#include <stdio.h>

volatile uint32_t i = 100;

void configurar_systick(uint32_t ticks){
    SysTick->CTRL = 0;
    SysTick->LOAD = ticks - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = (1<<2) | (1<<1) | 1;
}

int main(void) {

	LPC_GPIO2->FIODIR &= ~((1<<11) | (1<<12)); // entrada gpio de eint1 y eint2

	// EINT1 con pullup => esta a 1 y conecto boton a gnd para que cuando presione se genere flanco de bajada
	LPC_PINCON->PINMODE4 &= ~(3<<22);
	// EINT2 con pulldown => esta en 0 y conecto boton a vcc para que cuando presione se genere flanco de subida
	LPC_PINCON->PINMODE4 |= (3<<24);

	// LPC_PINCON->PINSEL4 &= ~(3<<22);  // limpio (ya viene en 00)
	LPC_PINCON->PINSEL4 |= (1<<22); // 01 EN PIN 22:23 -> P2.11 como EINT1
	// LPC_PINCON->PINSEL4 &= ~(3<<24);  // limpio (ya viene en 00)
	LPC_PINCON->PINSEL4 |= (1<<24); // 01 EN PIN 24:25 -> P2.12 como EINT2

	LPC_SC->EXTMODE |= (1<<1) | (1<<2); // POR FLANCO
	LPC_SC->EXTPOLAR &= ~(1<<1); // DESC ṔARA EINT1
	LPC_SC->EXTPOLAR |= (1<<2); // ASC ṔARA EINT2

	// PRIORIDAD EINT1 > EINT2 (0 Y 1) -> 5 bits altos cuentan [7:3]
	NVIC->IP[EINT1_IRQn] = (0<<3);
	NVIC->IP[EINT2_IRQn] = (1<<3);

	// mostrar cambio de frec de systick con un led P0.22
	LPC_GPIO0->FIODIR |= (1<<22);
	LPC_GPIO0->FIOSET = (1<<22);

	// activo int externa 1 y 2
	LPC_SC->EXTINT = (1<<1) | (1<<2); // limpio flags de int externa
	NVIC->ISER[0]|=(3<<19);

    while(1) {
    	// ...
    }

    return 0 ;
}

void SysTick_Handler(void){
	i--;
	if (i==0) {
		i=100;
		LPC_GPIO0->FIOPIN ^= (1<<22);
	}
}

void EINT1_IRQHandler(void){
    LPC_SC->EXTINT = (1<<1); // limpiar flag
    configurar_systick((SystemCoreClock / 1000) * 25);
}

void EINT2_IRQHandler(void){
    LPC_SC->EXTINT = (1<<2); // limpiar flag
    configurar_systick((SystemCoreClock / 1000) * 60);
}
