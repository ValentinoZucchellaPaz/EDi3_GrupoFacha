#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <stdint.h>

// modo A (default): SysTick (50ms) -> ISR -> START ADC (software) -> ADC ISR -> update LEDs (8 bits)
// modo B: Timer0 MATCH (1s) -> ISR -> START ADC (software) -> ADC ISR -> update LEDs (3 bits)
// mode switch: Timer1 MATCH (1s) -> ISR -> count -> if Capture -> diff and load value to array

# define LED_MASK  0xFF

volatile uint32_t modo = 0;             // 0 modo A; 1 modo B
volatile uint32_t config = 0;           // flag para que main configure modo
volatile uint32_t conversion_val = 0;
volatile uint32_t converted = 0;

// captura de cuanto dura cada modo
volatile uint32_t duracion_modo[10] = {0,0,0,0,0,0,0,0,0,0};
volatile uint32_t indice_captura = 0;
volatile uint32_t captura_actual = 0;
volatile uint32_t captura_anterior = 0;
volatile uint32_t captura = 0; // flag


void init_leds(void);                   // 8 leds que muestran adc output P1.[7:0]
void init_adc(void);
void init_eint0(void);                  // P2.10
void init_systick_timer(void);          // modo A: disparo conversion adc cada 50ms
void init_timer0(void);                 // modo B: disparo conversion adc cada 1seg (match int)
void init_timer1(void);                 // captura de permanencia en cada modo

void init_priorities(void);
void update_leds(uint32_t load_val);

int main (void){
    init_priorities();
    init_leds();                        // 8 leds que muestran adc output P1.[7:0]
    init_eint0();                       // P2.10
    init_systick_timer();               // modo A: disparo conversion adc cada 50ms
    init_timer0();                      // modo B: disparo conversion adc cada 1seg (match int)
    init_timer1();                      // captura de permanencia en cada modo
    init_adc();

    while(1){
        if (modo==0)
        {
            if(config==1)
            {
                LPC_TIM0 -> TCR = 0; // apago timer, pongo a andar systick
                NVIC->ICER[0] |= (1<<TIMER0_IRQn); // apago timer irq
                SysTick->CTRL |= (1<<0) | (1<<1); // prendo systick
                config=0;
            }

            if (converted==1)
            {
                conversion_val=((conversion_val&0xFF0)>>4); // tomo 8 bits superiores y pongo que comiencen en el bit 0
                update_leds(conversion_val);
                converted=0;
            }
        }
        
        if(modo==1)
        {
            if(config==1)
            {
                SysTick->CTRL &= ~((1<<0) | (1<<1)); // apago systick, pongo a andar timer0
                LPC_TIM0 -> TCR = (1 << 1); // reset de timer
                LPC_TIM0 -> TCR = (1 << 0); // enable de timer
                NVIC->ISER[0] |= (1<<TIMER0_IRQn); // enable irq
                config=0;
            }

            if (converted==1)
            {
                if (conversion_val >=8)
                    update_leds(0x800); // prendo led n° 12
                else if (conversion_val >= 4)
                    update_leds(0x060); // prendo led n° 6 y 7
                else
                    update_leds(0x001); // prendo led n° 1
                
                converted=0;
            }
        }

        if (captura==1)
        {
            duracion_modo[indice_captura] = (captura_actual-captura_anterior);
            indice_captura = (indice_captura + 1) % 10;
            captura=0;
        }
    }
    return 0;
}

// ================================================
// CONFIGURACIONES INICIALES DE PERIFERICOS
// ================================================

void init_leds(void)
{
    // asumo pin es gpio de reset
    LPC_GPIO1->FIODIR |= LED_MASK;
    LPC_GPIO1->FIOCLR = LED_MASK;
}

void init_eint0(void)
{
    // asumo pin es gpio de reset
    LPC_PINCON -> PINSEL4 |= (1<<20);   // P2.10 eint0
    
    // sin pull up/down interna, boton se conecta a gnd con pullup externa
    LPC_PINCON -> PINMODE4 &= ~(1<<20);
    LPC_PINCON -> PINMODE4 |= (1<<21);
    
    LPC_SC -> EXTMODE |= (1<<0);        // por flanco
    LPC_SC -> EXTPOLAR &= ~(1<<0);      // de bajada
    LPC_SC -> EXTINT |= (1<<0);         // limpio flag
    NVIC->ISER[0] |= (1<<EINT0_IRQn);
}

void init_systick_timer(void)
{
    // asumo apagado de reset
    SysTick -> VAL = 0;
    SysTick -> LOAD = 4999999;          // 50ms = (LOAD + 1)/100MHz
    SysTick -> CTRL = (1<<0) 
                    | (1<<1) 
                    | (1<<2);           // enable, int, cclk
    NVIC_SetPriority(SysTick_IRQn, 2);
}

void init_timer0(void)
{
    // asumo PCONP de timer encendido de reset
    // asumo timer enable (TCR bit 0) apagado de reset
    // asumo CLK de perifericos por defecto (CCLK/4=25MHz) de reset

    // cuenta de 1seg
    LPC_TIM0 -> PR = 2499999;           // TC = (PR+1)/PCLK => 100ms = 2 500 000/25MHz
    LPC_TIM0 -> MR0 = 10;               // MR cambia a 10*100ms=1seg
    LPC_TIM0 -> MCR |= (3<<0);          // en match0 interrumpe y reset 
    // (cargo leds en adcISR y Tmr0ISR comienzo adc)

    // reset y comienza apagado
    LPC_TIM0 -> TCR = (1 << 1);
    LPC_TIM0 -> TCR = 0;
}

void init_timer1(void)
{
    // asumo PCONP de timer encendido de reset
    // asumo timer enable (TCR bit 0) apagado de reset
    // asumo CLK de perifericos por defecto (CCLK/4=25MHz) de reset
    LPC_PINCON->PINSEL3 |= (3 << 20);   // config pin P1.26 -> CAP1.0
    LPC_TIM1 -> PR = 24999999;          // TC = (PR+1)/PCLK => 1s = 25 000 000/25MHz
    LPC_TIM1->MCR = 0;                  // sin reset, sin match
                    
    LPC_TIM1 -> CCR = (1<<1) | (1<<2);  // captura en CAP1.0 por flanco de bajada y activa interrupcion
    // reset y enable
    LPC_TIM1 -> TCR = (1 << 1);
    LPC_TIM1 -> TCR = 1;
    NVIC -> ISER[0] |= (1<<TIMER1_IRQn);
}

void init_adc (void)
{
    // asumo CLK de perifericos por defecto (CCLK/4=25MHz) de reset
    LPC_PINCON->PINSEL1 |= (1<<18); // config pin P0.25 -> AD0.2
    
    // prendo periferico
    // selecciono canal 2 -> P0.25
    // seteo clk div para ADCLK=PCLK/2=12.5MHz (<13MHz)=> conversiones a ~192kHz
    // prendo adc
    // modo no burst
    // start: 000, depende del modo
    LPC_SC -> PCONP |= (1<<12);
    LPC_ADC -> ADCR |= (1<<2)
                    |  (1<<8)
                    |  (1<<21);

    LPC_ADC -> ADINTEN |= (1<<2);       // habilito int por canal 2 (cargo leds)
    
    NVIC -> ISER[0] |= (1<<ADC_IRQn);
}

void init_priorities(void)
{
    NVIC_SetPriority(EINT0_IRQn, 0);     // máxima prioridad a eint0
    NVIC_SetPriority(TIMER1_IRQn, 1);    // timer1 entra dps de eint0
    NVIC_SetPriority(TIMER0_IRQn, 2);
    NVIC_SetPriority(ADC_IRQn, 2);    
}

void update_leds(uint32_t load_val) 
{
    LPC_GPIO1 -> FIOCLR = LED_MASK;
    LPC_GPIO1 -> FIOSET = load_val&LED_MASK;
}

// isr de eint0 (cambio de modo), timer0 y systick(comenzar conversion adc), timer1 (contar y si viene capture sacar diff de tiempo y actualizar array), adc (pasar conversion a leds)

// ================================================
// CONFIGURACIONES DE HANDLERS DE INTERRUPCION
// ================================================


void EINT0_IRQHandler(void)
{ 
    if ((LPC_SC->EXTINT & 1) == 1){
        LPC_SC->EXTINT=1;
        // delay_10ms(); // antirebote bloqueante
        if (modo==0){
            // A->B
            modo=1;
            config=1;
        } else {
            // B->A
            modo=0;
            config=1;
        }
    }
}

void TIMER0_IRQHandler (void)
{
    // comenzar conversion adc
    if ((LPC_TIM0->IR & 1)==1){
        LPC_TIM0->IR |= (1<<0); // limpio int de match0
        LPC_ADC->ADCR |= (1<<24); // start adc
    }
}

void SysTick_IRQHandler (void)
{
    // comenzar conversion adc
    LPC_ADC->ADCR |= (1<<24); // start adc
}

void TIMER1_IRQHandler (void)
{
    if (LPC_TIM1->IR & (1<<4)) // CAP1.0
    {
        // delay_10ms(); // antirebote bloqueante
        LPC_TIM1->IR |= (1<<4); // limpiar
        captura_actual = LPC_TIM1->CR0;
        captura_anterior = captura_actual;
        captura=1;
    }
}

void ADC_IRQHandler(void)
{
    conversion_val = ((LPC_ADC->ADDR2 >> 4) & 0xFFF); // tomo 12 bits de conversion y pongo que comiencen en el bit 0
    converted=1;
}