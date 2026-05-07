/*
 * Realizar un programa que configure el puerto P2.0 y P2.1 para que provoquen una interrupción
 * por flanco de subida para el primer pin y por flanco de bajada para el segundo.
 * Cuando la interrupción sea por P2.0 se enviará por el pin P0.0 la secuencia de bits 010011010.
 * Si la interrupción es por P2.1 se enviará por el pin P0.1 la secuencia 011100110.
 * Las secuencias se envían únicamente cuando se produce una interrupción,
 * en caso contrario la salida de los pines tienen valores 1 lógicos.
 * ¿que prioridad tienen configuradas por defecto estas interrupciones? -> 0 (la mas alta posible)
 *
 *
 *
 * problemas de implementacion que faltan: bloquear interrupciones asi no se pisan
 */

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>

#include <stdio.h>

#define SEQ1 0b010011010
#define SEQ2 0b011100110
#define LEN  9

void delay(void){
    for(volatile int i = 0; i < 50000; i++);
}

void enviar_secuencia(uint8_t pin, uint16_t seq, uint8_t len){

    for(int i = len-1; i >= 0; i--){

        if(seq & (1 << i)){
            LPC_GPIO0->FIOSET = (1 << pin);  // 1 lógico
        } else {
        	LPC_GPIO0->FIOCLR = (1 << pin);  // 0 lógico
        }

        delay();
    }

    LPC_GPIO0->FIOSET = (1 << pin); // dejar en 1 lógico al final
}

int main(void) {

	// LPC_PINCONC->PINSEL0 &= ~(0x0000000F);
	// LPC_PINCON->PINSEL4 &= ~((3<<0) | (3<<2));

	LPC_GPIO2->FIODIR &= ~((1<<0) | (1<<1)); // P2.0 y P2.1 entradas gpio

	LPC_GPIOINT->IO2IntEnR|= (1<<0); // P2.0 flanco asc
	LPC_GPIOINT->IO2IntEnF |= (1<<1); // P2.1 flanco desc
	LPC_GPIOINT->IO2IntClr = (1<<0) | (1<<1); // limpio int previas

	NVIC->ISER[0] = (1<<21); // habilito EINT3

	LPC_GPIO0->FIODIR |= (1<<0) | (1<<1); // P0.0 y P0.1 salidas gpio
	LPC_GPIO0->FIOSET = (1<<0) | (1<<1);

    while(1) {
    	// ...
    }
}

void EINT3_IRQHandler(void){
	if (LPC_GPIOINT->IO2IntStatR & (1<<0)){
		LPC_GPIOINT->IO2IntClr = (1<<0);
		enviar_secuencia(0, SEQ1, LEN);
	}
	if (LPC_GPIOINT->IO2IntStatF & (1<<1)){
		LPC_GPIOINT->IO2IntClr = (1<<1);
		enviar_secuencia(1, SEQ2, LEN);
	}
}


