/*
 * Estudiante:
 * Carrera:
 * Descripción: Este programa simula un piedra papel o tijera entre el usuario y la maquina
 * el usuario puede tocar 3 botones piedra (P0.0), papel (P0,1) y tijera (P0.2)
 * para los botones ponerlos a vcc para tener un 1 (sin pulldown xq se activa por soft)
 *
 * el resultado se muestra con 3 leds conectados a los puertos P0.7 (empate), P0.8 (gana jugador), P0.9 (gana maquina)
 * Conectar leds a vcc y que hundan corriente al pin, se prenden con 0 en el pin
 */

#include "LPC17xx.h"
#include <stdio.h>
#include <stdlib.h>

int debounce(void);
void delay(int n);

int main(void){
    int entrada;
    int jugador;
    int cpu;
    int res;

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

    // activo pulldown para entradas
    LPC_PINCON->PINMODE0 |= 0x0000001F; // 0x0...011111 (pone 11, osea pulldown, los bit de los pines de P0.0, P0.1 y P0.2)

    // config entrada y salida (ya son gpio con el reset)
	LPC_GPIO0->FIODIR &= ~((1<<0) | (1<<1) | (1<<2));   // botones de entradas (bits 0-2)
    LPC_GPIO0->FIODIR |=  ((1<<7) | (1<<8) | (1<<9));   // leds de salidas (bits 7-9) (EMPATE, GANA JUGADOR, GANA CPU)

    // semilla para random - idea chat - preguntar
    srand(1234);

    // pongo salida en 1 para apagar leds
    LPC_GPIO0->FIOSET = ((1<<7) | (1<<8) | (1<<9));
	while(1){

        // leo entrada
        entrada = debounce();

        if (entrada == 0) {
            continue;
        }

        // hubo una entrada, me fijo si es valida
        jugador = mapa[entrada];

        if(jugador == -1){
            continue; // entrada invalida
        }

        // jugada valida de jugador y hago jugada de cpu
        cpu = rand() % 3;
        // jugCPU = (int)((SysTick->VAL ^ (uint32_t)entrada) % 3); // alternativa de otros chicos

        // me fijo res en matriz, muestro en leds y luego semihost
        res = resultado[jugador][cpu];

        if (res == 0) { // empate
            LPC_GPIO0->FIOCLR = (1<<7);
        } else if (res == 1) { // gana jugador
            LPC_GPIO0->FIOCLR = (1<<8);
        } else {
            LPC_GPIO0->FIOCLR = (1<<9); // gana cpu
        }

        // printf("Jugador: %d | CPU: %d | Resultado: %d\n", jugador, cpu, res);

        delay(200000);

        // vuelve a arrancar cuando se suelta boton, sino no
        while((LPC_GPIO0->FIOPIN & 0x07) != 0) {};
        LPC_GPIO0->FIOSET = ((1<<7) | (1<<8) | (1<<9)); // apago leds
	}

	return 0;
}

void delay(int n) {
    for (volatile int i = 0; i < n; i++); // 6000000 recomendado largo
}

int debounce(void) {
    // leo pines de entrada y tomo valor del puerto
	// espero a tener 2 lecturas consecutivas luego de un delay para evitar rebotes
    int e1, e2;

    do {
        e1=LPC_GPIO0->FIOPIN & 0x07;
        delay(50000);
        e2=LPC_GPIO0->FIOPIN & 0x07;
    } while(e1!=e2);

    return e1;
}
