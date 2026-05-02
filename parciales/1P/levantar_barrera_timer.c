/*
CCLK = 70MHz
Viene un auto (se prende sensor y tengo 1 en P2.4) y levanto una barrera (1 en P0.15) durante X segundos

Los segundos que tarda levantada se configuran antes de que se levante, eso se hace con un boton en P3.4, cuando se presione 1 vez comienza un contador de 3 segundos para contar cuantas veces se presiona
1 pulsada -> 5 seg
2 pulsada -> 10 seg
3 pulsada -> 20 seg
4 pulsada -> 40 seg
luego de la 4ta pulsada vuelve a 1
asumo sin rebotes
*/
#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>
#include <stdint.h>

typedef enum
{
    ESTADO_ESPERA = 0,
    ESTADO_CONFIG,
    ESTADO_ABRIR
} estados_t; // DUDA CHAT XQ COLOREA CON _t

// protipos funciones
void button_init(void);
void sensor_init(void);
void barrera_init(void);
void systick_init(uint32_t ticks);
void timer0_init(void);

// flags
volatile uint32_t fin_de_config = 0; //  esta la setea systick dps de 3seg
volatile uint32_t fin_de_abrir = 0; //  esta la setea timer0 dps de tiempo_abrir[config_counter] seg

estados_t estado_actual = ESTADO_ESPERA;

volatile uint32_t config_counter = 0; // contador que incrementa en EINT0 y elije tiempo de apertura
// CCLK=70 MHz -> PCLK=CCLK/4=17.5 MHz -> T = 57.14 ns => quiero TC aumente cada 1ms -> PR=17499
// con ese periodo la cant de ticks de timer0 que necesito para contar son las siguientes
// 5seg-> 4999
// 10seg->9999
// 20seg->19999
// 40seg->39999
volatile uint32_t tiempo_abrir[4] = {4999, 9999, 19999, 39999};

int main (void) {
    button_init();
    sensor_init();
    barrera_init();
    
    // interrumpo cada 10ms y CLK es 70MHz => ticks = 10ms * 70MHz - 1
    systick_init(699999); // aprox 10ms -> 300 veces 10ms es 3seg
    while(1){
        switch(estado_actual){
            case ESTADO_ESPERA:
            {
                // flags a 0
                fin_de_config=0;
                fin_de_abrir=0;
                break;
            }
            case ESTADO_CONFIG:
            {
                if (fin_de_config==1){
                    estado_actual = ESTADO_ESPERA;
                }
                break;
            }
            case ESTADO_ABRIR:
            {
                if (fin_de_abrir==1){
                    LPC_GPIO0 -> FIOCLR |= (1<<15); // pongo 0 bit de abrir barrera
                    estado_actual = ESTADO_ESPERA;
                }
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

// P2.10
void button_init(void){
    LPC_PINCON->PINSEL4 &= ~(3<<20); // limpio bits
	LPC_PINCON->PINSEL4 |= (1<<20);

	// config pulldown
	LPC_PINCON->PINMODE4 &= ~(3 << 20); // limpio bits
	LPC_PINCON->PINMODE4 |=  (3 << 20);

	// config int
	LPC_SC->EXTMODE |= (1<<0); // por flanco
	LPC_SC->EXTPOLAR |= (1<<0); // asc
    LPC_SC->EXTINT |= (1 << 0); // IMPORTANTE limpio pendientes

	// habilito int en NVIC
	NVIC->ISER[0] |= (1<<EINT0_IRQn);
}

// P2.4
void sensor_init(void){
    LPC_PINCON->PINSEL4 &= ~(3<<8);

    // config pulldown
	LPC_PINCON->PINMODE4 &= ~(3 << 8); // limpio bits
	LPC_PINCON->PINMODE4 |=  (3 << 8);

	LPC_GPIO2->FIODIR &= ~(1<<10); // entrada gpio

	LPC_GPIOINT->IO2IntEnR|= (1<<4); // P2.0 flanco asc
	LPC_GPIOINT->IO2IntClr = (1<<4); // limpio int previas

	NVIC->ISER[0] = (1<<EINT3_IRQn); // habilito EINT3
}

// P0.15
void barrera_init(void){
    LPC_PINCON->PINSEL0 &= ~(3<<30);
	LPC_GPIO0->FIODIR |= (1<<15); // salida gpio
    LPC_GPIO0->FIOCLR |= (1<<15);
}

void systick_init(uint32_t ticks) {
    SysTick->CTRL = 0;
    SysTick->LOAD = ticks-1;
    SysTick->VAL=0;
    // SysTick->CTRL= (1<<0) | (1<<1) | (1<<2);
}

void timer0_init(void) {
    // habilito timer (ya viene asi por reset igual)
    LPC_SC->PCONP |= (1 << 1);

    // Config: reseteo, sin prescaler (PCLK=17.5 MHz -> T = 17.5 us), cargo registros de match
    // CCLK=70 MHz -> PCLK=CCLK/4=17.5 MHz -> T = 57.14 ns => quiero TC aumente cada 1ms -> PR=17499
    LPC_TIM0->TCR = 0x02;
    LPC_TIM0->PR  = 17499;
    LPC_TIM0->MR0 = tiempo_abrir[0];

    // Match Control Register: cuenta det cant de segundos dada por MR0 y luego reset
    LPC_TIM0->MCR = (1 << 0) |             // MR0I: interrupcion en match MR0
                    (1 << 2);             // MR0R: reset TC en match MR0

    // Habilito int de timer en nvic
    NVIC->ISER[0]=(1<<TIMER0_IRQn);

    // arranca timer NO AUN, esto lo arranco cuando paso a estado abrir
    // LPC_TIM0->TCR = 0x01;
}

void SysTick_Handler(void) {
    if (estado_actual == ESTADO_CONFIG && fin_de_config == 0){
        static uint32_t counter = 0;
        counter++;
        if(counter>=300){
            counter=0;
            fin_de_config=1;
            // deshabilitar systick
            SysTick->CTRL = 0;
        }
    }
}

void TIMER0_IRQHandler(void){
    LPC_TIM0->IR = (1 << 0); // solo tengo MR0
    if (estado_actual==ESTADO_ABRIR){
        fin_de_abrir = 1;
        // deshabilitar timer0
        LPC_TIM0->TCR = 0x02;
    }
}

void EINT3_IRQHandler (void) {
    if (estado_actual==ESTADO_ESPERA){
        if (LPC_GPIOINT->IO2IntStatR & (1<<4)){
            estado_actual=ESTADO_ABRIR;
            LPC_GPIOINT->IO2IntClr = (1<<4);
            LPC_GPIO0 -> FIOSET |= (1<<15); // pongo 1 bit de abrir barrera

            // pongo valor de espera abrir y comienzo timer
            LPC_TIM0->MR0 = tiempo_abrir[config_counter]; 
            LPC_TIM0->TCR = 0x01;
        }
    }
}

void EINT0_IRQHandler(void){
    LPC_SC->EXTINT = (1<<0); // limpiar flag
    if(estado_actual==ESTADO_ESPERA){
        estado_actual=ESTADO_CONFIG;
        // prender systick
        SysTick->CTRL= (1<<0) | (1<<1) | (1<<2);
    } else if(estado_actual==ESTADO_CONFIG){
        config_counter++;
        if (config_counter >= 4){
            config_counter=0;
        }
    }
}