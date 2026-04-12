/*
4 pines de entrada GPIO, leen y guardan un número de 4 bits.
Tengo pull up externas: pin -- pullup -- switch -- gnd
Almaceno en un array (stack) con los últimos 10 números elegidos,
el número ingresado más antiguo esta en el indice 9, el actual en el indice 0
(debo manejar corrimiento y overflow)

La interrupción por GPIO empieza con máxima prioridad
cada 200 números ingresados disminuye en 1 su prioridad
cuando alcanza la minima prioridad posible, deshabilitar interrupciones por GPIO

ASUMO SIN REBOTES
*/

#include <LPC17xx.h>

#define INPUT_0         (1 << 0)   // P0.0
#define INPUT_1         (1 << 1)   // P0.1
#define INPUT_2         (1 << 2)   // P0.2
#define INPUT_3         (1 << 3)   // P0.3
#define MASK_INPUTS     (INPUT_0 | INPUT_1 | INPUT_2 | INPUT_3)

#define ARR_LEN         10
#define MAX_PRIORITY    32

volatile uint32_t nums[ARR_LEN] = {0,0,0,0,0,0,0,0,0,0};
volatile uint32_t inserted_count = 0; // incremento en cada numero asignado, cada 200 resto prioridad y reset
volatile uint32_t priority = 0;


// pines P0.[3:0] son entradas GPIO con P0.0 LSB y P0.3 MSB y disparan interrupcion por flanco asc y desc (cambio de nivel)
void input_init(void){
    // GPIO CONFIG
    LPC_PINCON -> PINSEL0 &= ~((3<<0) | (3<<2) | (3<<4) | (3<<6)); // GPIO
    LPC_PINCON -> PINMODE0 &= ~((3<<0) | (3<<2) | (3<<4) | (3<<6)); // limpio
    LPC_PINCON -> PINMODE0 |= (2<<0) | (2<<2) | (2<<4) | (2<<6); // sin pullup/down interna
    LPC_GPIO0 -> FIODIR &= ~(MASK_INPUTS); // entradas

    // GPIO INT: como quiero cambios de nivel disparo en rising y falling
    // primero limpio ambos
    LPC_GPIOINT -> IO0IntEnR &= ~(MASK_INPUTS);
    LPC_GPIOINT -> IO0IntEnF &= ~(MASK_INPUTS);
    // ahora enable ambas
    LPC_GPIOINT -> IO0IntEnR |= MASK_INPUTS;
    LPC_GPIOINT -> IO0IntEnF |= MASK_INPUTS;
    // limpio flag
    LPC_GPIOINT -> IO0IntClr |= MASK_INPUTS;

    // seteo max prioridad y habilito NVIC
    NVIC->IP[EINT3_IRQn] &= ~((0x1F) << 3); // limpio prioridad
    NVIC->IP[EINT3_IRQn] |= ((priority & 0x1F) << 3); // escribo bits 7:3
    NVIC->ISER[0] |= (1<<EINT3_IRQn);
    // NVIC_SetPriority(EINT3_IRQn, priority);
    // NVIC_EnableIRQ(EINT3_IRQn);
}

void stack_new_input(uint32_t input) {
    for (int i = ARR_LEN - 1; i>0; i--){
        nums[i]=nums[i-1];
    }
    nums[0]=input;
}

int main(void){
    input_init();
    while(1){}
    return 0;
}

void EINT3_IRQHandler(void){
    uint32_t status = (LPC_GPIOINT->IO0IntStatF | LPC_GPIOINT->IO0IntStatR) & MASK_INPUTS; // solo quiero P0.[3:0]
    if (status!=0){
        // llego nuevo num, leo puerto (solo pines que quiero) y agrego a stack, aumento cuenta de agregados y me fijo si disminuyo prioridad
        stack_new_input((LPC_GPIO0->FIOPIN & MASK_INPUTS));
        
        inserted_count++;
        if (inserted_count >= 200) {
            // disminuyo prioridad y reset contador agregados
            // si llegué a minima posible dehabilito sino vuelvo a setear
            priority++;
            inserted_count=0;
            if (priority>=MAX_PRIORITY){
                NIVC->ICER[0] |= (1<<EINT3_IRQn);
            } else {
                NVIC->IP[EINT3_IRQn] |= ((priority & 0x1F) << 3);
            }
        }
        LPC_GPIOINT -> IO0IntClr |= MASK_INPUTS;
    }
}
