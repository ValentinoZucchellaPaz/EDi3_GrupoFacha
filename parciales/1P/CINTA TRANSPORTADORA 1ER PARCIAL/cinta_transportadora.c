#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

void iniciar_sistema(void);
void encender_timer0();
void analizar_resultado();

/*
 * P2[10]=> Entrada Pulso sensor
 * P0[20]=> CONTROLA EL AVANCE DE LA CINTA
 * P0[0, 1 ,2, 3] => Son las entradas digitales del sensor
 *
 */

#define PULSO_SENSOR      ( (uint32_t) (1<<10) )
#define AVANCE_CINTA      ( (uint32_t) (1<<20) )
#define SENSOR_0             ( (uint32_t) (1<<0) )
#define SENSOR_1             ( (uint32_t) (1<<1) )
#define SENSOR_2            ( (uint32_t) (1<<2) )
#define SENSOR_3            ( (uint32_t) (1<<3) )
#define LED_ROJO               ( (uint32_t) (1<<22) )
#define RISING_EDGE           ( (uint8_t) 0)
#define PORT_ZERO            ( (uint8_t) 0)
#define PORT_TWO            ( (uint8_t) 2)
#define OUTPUT           ( (uint8_t) 1)
#define INTPUT         ( (uint8_t) 0)
#define WAIT_TIME         (uint32_t) 400
#define MAX_CANTIDAD       ((uint32_t) 10 )

// el problema me indica las direcciones de mem donde estan los datos
volatile  int32_t* limite_inf = (int*) 0x10003001;
volatile  int32_t* limite_sup = (int*) 0x10003005;

/*
* Configurar salidas digitales (Cinta, Led, 4 pines de sensor)
* Configurar timer0
* Configurar Interrupciones externas
*/

volatile int32_t  mediciones[MAX_CANTIDAD ]= {0,0,0,0,0,0,0,0,0,0};
volatile uint8_t flag_medir = 0;

void EINT0_IRQHandler(void) {
    LPC_SC->EXTINT |= (1<<0); // limpio flag de int
    LPC_GPIO0->FIOCLR |=(1<<20); // detengo cinta
    LPC_TIM0->TCR |= (3<<0); // inicio timer
}

void TIMER0_IRQHandler(void) {
    // aca entra dps de 400us
	LPC_TIM0->IR = 1; // se limpia con escritura
    LPC_TIM0->TCR = 0; // detengo timer
	flag_medir=1; // indico a main que tome mediciones
}


void System_Init(void) {
    // ======================================
    // Configurar pines
    // ======================================
    LPC_PINCON->PINSEL4 |= (1 << 20);    // Configurar P2[10] como EINT0
    LPC_GPIO0->FIODIR |= (1 << 20);      // P0[20] como salida (Control Cinta)
    LPC_GPIO0->FIODIR &= ~(0x0F);        // P0[3:0] como entradas (Sensor altura)
    LPC_GPIO0->FIODIR |= (1 << 22);      // Asumimos P0[22] como salida para LED rojo (falla)
    
    LPC_GPIO0->FIOSET = (1 << 20);       // Iniciar con la cinta avanzando
    LPC_GPIO0->FIOCLR = (1 << 22);       // LED apagado

    // ======================================
    // Configurar Interrupción Externa (EINT0)
    // ======================================
    LPC_SC->EXTMODE |= (1 << 0);         // Sensible a flanco
    LPC_SC->EXTPOLAR |= (1 << 0);          // Flanco de subida (pulso)
    NVIC_EnableIRQ(EINT0_IRQn);

    // ======================================
    // Configurar TIMER0
    // ======================================
    LPC_SC->PCONP |= (1 << 1); // Encender Timer0

    //CONFIG TIMER: PCLK=25MHz
	LPC_TIM0->TCR |= (3<<0);
	LPC_TIM0->TC = 0;
	LPC_TIM0->PR |=249; // periodo de TC = (PR+1)/PCLK = 250/25 MHz = 10 * 10^-6 segundos = 10 us
	//CONFIGURACION DEL MATCH
	LPC_TIM0->MR0 = 40; // cuento 400us
	LPC_TIM0->MCR|= (3<<0); // MR0 genera int y resetea TC
	NVIC_EnableIRQ(TIMER0_IRQn);
}

int main(void) {
    System_Init();

    while(1) {
        if (flag_medir) {
            flag_medir = 0;
            uint32_t suma = 0;

            // Realizar las 10 mediciones de forma secuencial rápida
            for(int i = 0; i < 10; i++) {
                mediciones[i] = LPC_GPIO0->FIOPIN & 0x0F; // leo pines de entrada P0.[3:0]
                suma += mediciones[i];
            }

            uint32_t promedio = suma / 10;

            // Comparar con los límites establecidos en memoria
            if (promedio >= *limite_inf && promedio <= *limite_sup) {
                // Pieza correcta: Apagar led rojo y avanzar cinta
                LPC_GPIO0->FIOSET = (1 << 22);
                LPC_GPIO0->FIOSET = (1 << 20); 
            } else {
                // Falla: Cinta queda detenida (ya lo hizo EINT0), encender led rojo
                LPC_GPIO0->FIOCLR = (1 << 22);
            }
        }
    }
    return 0;
}


