/*
 * Realizar un programa que configure el puerto P0.0 y P2.0 para que provoquen una interrupción
 * por flanco de subida.
 * Si la interrupción es por P0.0 guardar el valor binario 100111 en la variable "auxiliar",
 * si es por P2.0 guardar el valor binario 111001011010110.
 */

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>

#include <stdio.h>

volatile uint32_t AUX = 0;

int main(void) {

	// LPC_PINCONC->PINSEL0 &= ~(3<<0);
	// LPC_PINCON->PINSEL4 &= ~(3<<0);

	LPC_GPIO2->FIODIR &= ~(1<<0); // P2.0 entradas gpio
	LPC_GPIO0->FIODIR &= ~(1<<0); // P0.0 entradas gpio

	LPC_GPIOINT->IO2IntEnR|= (1<<0); // P2.0 flanco asc
	LPC_GPIOINT->IO0IntEnR |= (1<<0); // P0.0 flanco asc

	LPC_GPIOINT->IO2IntClr = (1<<0); // limpio int previas
	LPC_GPIOINT->IO0IntClr = (1<<0); // limpio int previas

	NVIC->ISER[0] = (1<<21); // habilito EINT3

    while(1) {
    	// ...
    }
}

void EINT3_IRQHandler(void){
	// P2.0
	if (LPC_GPIOINT->IO2IntStatR & (1<<0)){
		LPC_GPIOINT->IO2IntClr = (1<<0);
		AUX = 0x000072D6; // 111001011010110
	}

	// P0.0
	if (LPC_GPIOINT->IO0IntStatR & (1<<0)){
		LPC_GPIOINT->IO0IntClr = (1<<0);
		AUX = 0x00000027; // 100111
	}
}
