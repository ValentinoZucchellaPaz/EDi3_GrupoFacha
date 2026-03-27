// Valentino Zucchella Paz, Pedro Guzman, Tomas Garay

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>

#include <stdio.h>

typedef enum {
	ROJO,
	VERDE,
	AZUL
} estado_t;
volatile uint32_t contador_ms = 0;

int main(void) {
    estado_t estado_actual = ROJO;
    estado_t estado_anterior = -1; // valor inválido para forzar primera actualización

	SysTick_Config(SystemCoreClock/1000);//config 1ms

	// config e inicializacion de leds
	LPC_PINCON->PINSEL1 &= ~((3 << 12) | (3 << 18) | (3 << 20)); // pongo como gpio
	LPC_GPIO0->FIODIR |= (1<<22) | (1<<25) | (1<<26); // led rojo, verde, azul
	LPC_GPIO0->FIOSET |= (1<<22) | (1<<25) | (1<<26); // inicializo con leds apagados (logica negativa)


    while(1) {
    	estado_actual = (contador_ms < 1000) ? ROJO :
    	         (contador_ms < 3000) ? VERDE :
    	         (contador_ms < 6000) ? AZUL : ROJO;

    	if(contador_ms >= 6000){
			contador_ms=0;
		}

    	if (estado_actual != estado_anterior){
    		estado_anterior = estado_actual;

    		switch(estado_actual){
    			case ROJO:
					// pasa a verde
					LPC_GPIO0->FIOSET |= (1<<25) | (1<<26);
					LPC_GPIO0->FIOCLR |= (1<<22);
					break;

				case VERDE:
					// pasa a azul
					LPC_GPIO0->FIOSET |= (1<<22) | (1<<26);
					LPC_GPIO0->FIOCLR |= (1<<25);
					break;

				case AZUL:
					// pasa a rojo
					LPC_GPIO0->FIOSET |= (1<<22) | (1<<25);
					LPC_GPIO0->FIOCLR |= (1<<26);
					break;
    		}
    	}

    }
    return 0 ;
}

void SysTick_Handler(void){
	 contador_ms++;
}



