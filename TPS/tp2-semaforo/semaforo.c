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
	// LPC_PINCON->PINSEL1 &= ~(3 << 12); // rojo
	// LPC_PINCON->PINSEL7 &= ~(3<<18); // verde
	// LPC_PINCON->PINSEL7 &= ~(3<<20); // azul

	// pongo como salida
	LPC_GPIO0->FIODIR |= (1<<22); // rojo
	LPC_GPIO3->FIODIR |= (1<<25); // verde
	LPC_GPIO3->FIODIR |= (1<<26); // azul

	// leds apagados (logica neg)
	LPC_GPIO0->FIOSET |= (1<<22); // rojo
	LPC_GPIO3->FIOSET |= (1<<25); // verde
	LPC_GPIO3->FIOSET |= (1<<26); // azul


    while(1) {
    	estado_actual = (contador_ms < 4000) ? ROJO :
    	         (contador_ms < 8000) ? VERDE :
    	         (contador_ms < 10000) ? AZUL : ROJO;

    	if(contador_ms >= 10000){
			contador_ms=0;
		}

    	if (estado_actual != estado_anterior){
    		estado_anterior = estado_actual;


    		switch(estado_actual){
    			case ROJO:
					// pasa a rojo
					LPC_GPIO3->FIOSET |= (1<<26) | (1<<25);
					LPC_GPIO0->FIOCLR |= (1<<22);
					break;

				case VERDE:
					// pasa a verde
					LPC_GPIO0->FIOSET |= (1<<22);
					LPC_GPIO3->FIOSET |= (1<<26);
					LPC_GPIO3->FIOCLR |= (1<<25);
					break;

				case AZUL:
					// pasa a azul
					LPC_GPIO0->FIOSET |= (1<<22);
					LPC_GPIO3->FIOSET |= (1<<25);
					LPC_GPIO3->FIOCLR |= (1<<26);
					break;
    		}
    	}
    }
    return 0 ;
}

void SysTick_Handler(void){
	 contador_ms++;
}



