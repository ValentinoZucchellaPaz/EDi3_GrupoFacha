/*
 * Copyright 2022 NXP
 * NXP confidential.
 * This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms.  By expressly accepting
 * such terms or by downloading, installing, activating and/or otherwise using
 * the software, you are agreeing that you have read, and that you agree to
 * comply with and are bound by, such license terms.  If you do not agree to
 * be bound by the applicable license terms, then you may not retain, install,
 * activate or otherwise use the software.
 */

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

    uint_32_t ticks_1ms = SystemCoreClock/1000; // frec clock * tiempo deseado -> 1ms
	SysTick_Config(SystemCoreClock/1000);//config 1ms

	// config e inicializacion de leds
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
					LPC_GPIO3->FIOSET |= (1<<25) | (1<<26);
					LPC_GPIO0->FIOCLR |= (1<<22);
					break;

				case VERDE:
					// pasa a azul
					LPC_GPIO0->FIOSET |= (1<<22) | (1<<26);
					LPC_GPIO3->FIOCLR |= (1<<25);
					break;

				case AZUL:
					// pasa a rojo
					LPC_GPIO0->FIOSET |= (1<<22) | (1<<25);
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



