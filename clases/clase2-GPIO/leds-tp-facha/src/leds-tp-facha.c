/*
 * Estudiante:
 * Carrera:
 * Descripción: Este programa pende y apaga un led
 *              conectado al pin P0.22 de forma intermitente
 *
 */

#include"LPC17xx.h"

void retardo(void);

int main(void){

	uint32_t relojCpu = SystemCoreClock;

	//LPC_PINCON->PINSEL1 &= ~(3<<12);   // 0B00111111111111 & 0B10010101010101 = 0B00_010101010101
	LPC_GPIO0->FIODIR     |= (1<<22);     // 0B1000...000 | 0B010101..101 = 1_10101..101
	//LPC_GPIO0->FIOMASK =0xFFFFFFFF;  //~FIOMASK & REGISTRO =0000

	while(1){
		LPC_GPIO0->FIOCLR |= (1<<22);  // apaga el led
		retardo();
		LPC_GPIO0->FIOSET |= (1<<22);  // apaga el led
		retardo();
	}

	return 0;
}

void retardo(void){

	uint32_t contador;
	for(contador =0 ; contador<6000000; contador++){};

}
