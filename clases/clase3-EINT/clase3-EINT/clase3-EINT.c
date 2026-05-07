// Alumnos: Pedro Guzman, Tomas Garay, Valentino Zucchella Paz
// probar
#include "LPC17xx.h"

void EINT0_IRQHandler(void) {
	LPC_GPIO0->FIOPIN ^= (1<<22); // toggle de led
	LPC_SC->EXTINT |= (1<<0); // bajo bandera
}

int main(void) {
	// codigo para poner un boton en P2.10 (4to pin der contando desde abajo) conectado a VCC (primer pin der contando desde arriba)
	// dispara interrupcion que prende y apaga led	

	// config P2.10 como EINT0
	LPC_PINCON->PINSEL4 &= ~(3<<20); // limpio bits
	LPC_PINCON->PINSEL4 |= (1<<20);

	LPC_GPIO2->FIODIR &= ~(1<<10); // para evitar conflictos hago entrada GPIO (PREGUNTAR A PROFE)

	// config pulldown
	LPC_PINCON->PINMODE4 &= ~(3 << 20); // limpio bits
	LPC_PINCON->PINMODE4 |=  (3 << 20);

	// config int
	LPC_SC->EXTMODE |= (1<<0); // por flanco
	LPC_SC->EXTPOLAR |= (1<<0); // asc
    LPC_SC->EXTINT |= (1 << 0); // IMPORTANTE limpio pendientes

	
	// habilito int en NVIC - por funcion (no vimos todavia)
	//NVIC_EnableIRQ(EINT0_IRQn);
	NVIC->ISER[0] |= (1<<18); // a manopla

	// config led P0.22
	LPC_PINCON->PINSEL1 &= ~(3 << 12);
	LPC_GPIO0->FIODIR |= (1<<22); // led (P0.22) como salida
	LPC_GPIO0->FIOSET |= (1<<22) // comienza apagado (logica neg)

	while (1){
		// ...
	}

    return 0 ;
}
