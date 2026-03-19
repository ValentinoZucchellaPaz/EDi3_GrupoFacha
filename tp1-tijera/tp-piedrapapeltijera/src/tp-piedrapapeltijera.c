/*
 * Estudiante:
 * Carrera:
 * Descripción: Este programa pende y apaga un led
 *              conectado al pin P0.22 de forma intermitente
 *
 */

#include "LPC17xx.h"
#include <stdio.h>
#include <stdlib.h>

// Matriz de resultados: 1 jugador, 0 empate, -1 cpu
// [jugador][cpu]
int resultado[3][3] = {
    { 0, -1, 1},
    { 1, 0, -1},
    {-1, 1, 0}
};

// Mapeo de entrada GPIO
// (solo se van a tener en cuenta esos indices y el valor de los leds corresponde a piedra papel o tijera como 0 1 2, el resto que no se toma en cuenta tienen -1)
int mapa[8] = { -1, 0, 1, -1, 2, -1, -1, -1 };


void delay(int n) {
    for (volatile int i = 0; i < n; i++); // 6000000 recomendado largo
}

int debounce(){
    int e1, e2;

    do {
        e1=LPC_GPIO0->FIOPIN & 0x07;
        delay(50000);
        e2=LPC_GPIO0->FIOPIN & 0x07;
    } while(e1!=e2);

    return e1;
}

int main(void){

	// int relojCpu = SystemCoreClock;

    // no pongo como GPIO xq ya vienen asi
	LPC_GPIO0->FIODIR &= ~(0x07);   // entradas (bits 0-2)
    LPC_GPIO0->FIODIR |=  (0x70);   // salidas (bits 4-6) (EMPATE, GANA JUGADOR, GANA CPU)

    int entrada;
    int jugador;
    int cpu;
    int res;

	while(1){

        // leo entrada
		entrada = LPC_GPIO0->FIOPIN & 0x07;

        if (entrada == 0) {
            continue;
        }

        entrada = debounce();

        // hubo una entrada, me fijo si es valida
        jugador = mapa[entrada];

        if(jugador == -1){
            continue; // entrada invalida
        }

        // jugada valida de jugador y hago jugada de cpu
        cpu = rand() % 3;

        // me fijo res en matriz, muestro en leds y luego semihost
        res = resultado[jugador][cpu];

        LPC_GPIO0->FIOCLR = 0x70;
        if (res == 0) { // empate
            LPC_GPIO0->FIOSET |= (1<<4);
        } else if (res == 1) { // gana jugador
            LPC_GPIO0->FIOSET |= (1<<5);
        } else {
            LPC_GPIO0->FIOSET |= (1<<6); // gana cpu
        }

        printf("Jugador: %d | CPU: %d | Resultado: %d\n", jugador, cpu, res);

        delay(200000);

        // vuelve a arrancar cuando se suelta boton, sino no
        while((LPC_GPIO0->FIOPIN & 0x07) != 0);

        delay(50000);
	}

	return 0;
}
