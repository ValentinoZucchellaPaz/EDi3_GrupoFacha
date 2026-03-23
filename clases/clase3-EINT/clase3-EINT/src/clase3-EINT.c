// Alumnos: Pedro Guzman, Tomas Garay, Valentino Zucchella Paz
// probar
#include "LPC17xx.h"

void EINT0_IRQHandler(void) {
	LPC_GPIO0->FIOPIN ^= (1<<22); // toggle de led
	LPC_SC->EXTINT = (1<<0); // bajo bandera
}

int main(void) {
	// codigo para poner un boton en P2.10 (4to pin der contando desde abajo) conectado a VCC (primer pin der contando desde arriba)
	// dispara interrupcion que prende y apaga led


	LPC_GPIO0->FIODIR |= (1<<22); // led (P0.22) como salida

	// config pin de entrada (P2.10) como EINT0
	LPC_PINCON->PINSEL4 &= ~(3<<20); // limpio bits
	LPC_PINCON->PINSEL4 |= (1<<20);

	// config pulldown
	LPC_PINCON->PINMODE4 &= ~(3 << 20); // limpio bits
	LPC_PINCON->PINMODE4 |=  (3 << 20);

	// config int
	LPC_SC->EXTMODE |= (1<<0); // por flanco
	LPC_SC->EXTPOLAR |= (1<<0); // asc

	// Limpiar cualquier interrupción pendiente
    LPC_SC->EXTINT = (1 << 0);

	// habilito int en NVIC - por funcion (no vimos todavia)
	//NVIC_EnableIRQ(EINT0_IRQn);
	NVIC->ISER[0] |= (1<<18); // a manopla


	while (1){
		// ...
	}

    return 0 ;
}
