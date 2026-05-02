/*
CCLK = 100MHz
Sensor (P0.6) detecta cuando se abre una puerta (dispara EINT3,gpio)
Entonces, espero por 30 segundos (systick) que se ingrese la contraseña adecuada (1010 -> 0xAA)
usando los switches conectados a los pines P2.[0-3]
Si no se ingresa la correcta luego de los 30seg o se pifia dos veces se dispara una alarma (P1.11)
asumo sin rebotes
*/
#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>
#include <stdint.h>

#define INPUT_0         (1 << 0)   // P2.0
#define INPUT_1         (1 << 1)   // P2.1
#define INPUT_2         (1 << 2)   // P2.2
#define INPUT_3         (1 << 3)   // P2.3
#define MASK_INPUTS     (INPUT_0 | INPUT_1 | INPUT_2 | INPUT_3)

#define CONTRASEÑA      0XA
typedef enum
{
    ESTADO_ESPERA = 0,
    ESTADO_CONTRASEÑA,
    ESTADO_ALARMA
} estados_t;

// protipos funciones
void sensor_init(void);
void input_init(void);
void alarma_init(void);
void systick_init(uint32_t ticks);

// flags
volatile uint32_t contraseña_correcta = 0;
volatile uint32_t contraseña_intentos = 0;
volatile uint32_t fin_de_contra = 0; //  esta la setea systick dps de 30seg o EINT3_Handler cuando vino la buena

estados_t estado_actual = ESTADO_ESPERA;

int main (void) {
    input_init();
    sensor_init();
    alarma_init();

    NVIC->ISER[0] = (1<<EINT3_IRQn); // habilito EINT3 para gpio

    // interrumpo cada 10ms y CLK es 100MHz => ticks = 10ms * 100MHz - 1 = 999999
    systick_init(999999); // aprox 10ms -> 3000 veces 10ms es 30seg
    while(1){
        switch(estado_actual){
            case ESTADO_ESPERA:
            {
                // flags a 0
                contraseña_correcta = 0;contraseña_intentos = 0;fin_de_contra = 0;
                break;
            }
            case ESTADO_CONTRASEÑA:
            {
                if(contraseña_intentos>2){
                    contraseña_correcta=0;
                    fin_de_contra=1;
                }
                if (fin_de_contra==1){
                    SysTick->CTRL = 0;
                    if (contraseña_correcta==1){
                        estado_actual=ESTADO_ESPERA;
                    } else {
                        LPC_GPIO1->FIOSET = (1<<10);
                        estado_actual=ESTADO_ALARMA;
                    }
                }
                break;
            }
            case ESTADO_ALARMA:
            {
                // idle
                break;
            }
            default:{
                estado_actual = ESTADO_ESPERA;
                break;
            }
        }
    }
    return 0;
}

// P2.[0-3]
void input_init(void){
    // GPIO CONFIG
    LPC_PINCON -> PINSEL4 &= ~((3<<0) | (3<<2) | (3<<4) | (3<<6)); // GPIO
    LPC_PINCON -> PINMODE4 &= ~((3<<0) | (3<<2) | (3<<4) | (3<<6)); // limpio
    LPC_PINCON -> PINMODE4 |= (3<<0) | (3<<2) | (3<<4) | (3<<6); // pull down
    LPC_GPIO2 -> FIODIR &= ~(MASK_INPUTS); // entradas

	// config eint3
    LPC_GPIOINT->IO2IntEnR|= (MASK_INPUTS); // flanco asc
    LPC_GPIOINT->IO2IntEnF|= (MASK_INPUTS); // flanco desc
	LPC_GPIOINT->IO2IntClr = (MASK_INPUTS); // limpio int previas
}

// P0.6
void sensor_init(void){
    LPC_PINCON->PINSEL0 &= ~(3<<12);

    // config pulldown
	LPC_PINCON->PINMODE0 &= ~(3 << 12); // limpio bits
	LPC_PINCON->PINMODE0 |=  (3 << 12);

	LPC_GPIO0->FIODIR &= ~(1<<6); // entrada gpio

	LPC_GPIOINT->IO0IntEnR|= (1<<6); // P0.6 flanco asc
	LPC_GPIOINT->IO0IntClr = (1<<6); // limpio int previas
}

// P1.10
void alarma_init(void){
    LPC_PINCON->PINSEL2 &= ~(3<<20);
	LPC_GPIO1->FIODIR |= (1<<10); // salida gpio
    LPC_GPIO1->FIOCLR = (1<<10);
}

void systick_init(uint32_t ticks) {
    SysTick->CTRL = 0;
    SysTick->LOAD = ticks-1;
    SysTick->VAL=0;
    // SysTick->CTRL= (1<<0) | (1<<1) | (1<<2);
}

void SysTick_Handler(void) {
    if (estado_actual == ESTADO_CONTRASEÑA && fin_de_contra == 0){
        static uint32_t counter = 0;
        counter++;
        if(counter>=3000){
            counter=0;
            contraseña_correcta=0;
            fin_de_contra=1;
        }
    }
}

void EINT3_IRQHandler (void) {
    if (estado_actual==ESTADO_ESPERA
            && LPC_GPIOINT->IO0IntStatR & (1<<6))
    {
        estado_actual=ESTADO_CONTRASEÑA;
        // comienzo timer 30 segs
        SysTick->CTRL= (1<<0) | (1<<1) | (1<<2);
    }
    else if (estado_actual==ESTADO_CONTRASEÑA
            && ((LPC_GPIOINT->IO2IntStatR | LPC_GPIOINT->IO2IntStatF) & (MASK_INPUTS)) != 0)
    {
        if ((LPC_GPIO2->FIOPIN & MASK_INPUTS) == CONTRASEÑA){
            contraseña_correcta=1;
            fin_de_contra=1;
        } else {
            contraseña_intentos++;
        }
    }

    // limpio todas
    LPC_GPIOINT->IO0IntClr = (1<<6);
    LPC_GPIOINT->IO2IntClr = (MASK_INPUTS);
}
