#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>
#include <stdint.h>

/* =========================
   CONFIGURACIÓN GENERAL
   ========================= */

#define MAX_SECUENCIA 10
#define CANT_BOTONES  4
#define CANT_LEDS     4

#define TIEMPO_LED_ON   1000
#define TIEMPO_LED_OFF   300

/* =========================
   BOTONES - P0.0 a P0.3
   ========================= */

#define BOTON_0    (1 << 0)   // P0.0
#define BOTON_1    (1 << 1)   // P0.1
#define BOTON_2    (1 << 2)   // P0.2
#define BOTON_3    (1 << 3)   // P0.3

#define MASK_BOTONES   (BOTON_0 | BOTON_1 | BOTON_2 | BOTON_3)

/* =========================
   LEDS - P2.0, P2.1, P2.2, P2.3
   ========================= */

const uint32_t led_mask[CANT_LEDS] = {
    (1 << 0),   // LED 0 -> P2.0
    (1 << 1),   // LED 1 -> P2.1
    (1 << 2),   // LED 2 -> P2.2
    (1 << 3)    // LED 3 -> P2.3
};

/* =========================
   TIPOS DE DATOS
   ========================= */

typedef enum
{
    ESTADO_IDLE = 0,
    ESTADO_INICIO,
    ESTADO_GENERAR_PASO,
    ESTADO_MOSTRAR_SECUENCIA,
    ESTADO_ESPERAR_JUGADOR,
    ESTADO_VALIDAR_JUGADA,
    ESTADO_RONDA_SUPERADA,
    ESTADO_GAME_OVER,
    ESTADO_VICTORIA
} estado_juego_t;

typedef enum
{
    SUB_ENCENDER_LED = 0,
    SUB_ESPERAR_LED_ON,
    SUB_APAGAR_LED,
    SUB_ESPERAR_LED_OFF
} subestado_mostrar_t;

/* =========================
   PROTOTIPOS
   ========================= */

void config_botones(void);
void leds_init(void);
void leds_apagar_todos(void);
void led_encender(uint8_t indice);
void led_apagar(uint8_t indice);
void led_mostrar(uint8_t indice);
uint8_t tiempo_cumplido(uint32_t referencia, uint32_t demora_ms);
uint8_t boton_sigue_presionado(uint8_t boton);
void demora_150ms(void);
void systick_init(void);

/* =========================
   VARIABLES DEL JUEGO
   ========================= */

volatile estado_juego_t estado_actual = ESTADO_IDLE;

/* Secuencia que genera la placa */
uint8_t secuencia[MAX_SECUENCIA] = {0};

/* Cantidad actual de pasos válidos en la secuencia */
uint8_t longitud_secuencia = 0;

/* Índice usado para mostrar la secuencia */
uint8_t indice_mostrar = 0;

/* Índice usado para validar la entrada del jugador */
uint8_t indice_jugador = 0;

/* Último botón presionado por el jugador */
volatile int8_t boton_presionado = -1;

/* Bandera para indicar que hubo una nueva pulsación */
volatile uint8_t evento_boton = 0;

/* Bandera de inicio de juego */
volatile uint8_t evento_start = 0;

/* Puntaje o nivel actual */
uint8_t nivel = 0;

/* Variable auxiliar para saber si hubo error */
uint8_t error_jugada = 0;

/* =========================
   TEMPORIZACIÓN
   ========================= */

volatile uint32_t ticks_ms = 0;
uint32_t tiempo_referencia = 0;

/* Subestado para mostrar la secuencia */
subestado_mostrar_t subestado_mostrar = SUB_ENCENDER_LED;

/* Habilitación de lectura del usuario */
volatile uint8_t habilitar_lectura_usuario = 0;

/* =========================
   MAIN
   ========================= */

int main(void)
{
    leds_init();
    config_botones();
    systick_init();

    while (1) {
        switch (estado_actual) {
            case ESTADO_IDLE: {
                leds_apagar_todos();
                habilitar_lectura_usuario = 0;

                if (evento_start)
                {
                    evento_start = 0;
                    estado_actual = ESTADO_INICIO;
                }
                break;
            }

            case ESTADO_INICIO: {
                longitud_secuencia = 0;
                indice_mostrar = 0;
                indice_jugador = 0;
                boton_presionado = -1;
                evento_boton = 0;
                nivel = 0;
                error_jugada = 0;

                tiempo_referencia = 0;
                subestado_mostrar = SUB_ENCENDER_LED;
                habilitar_lectura_usuario = 0;

                leds_apagar_todos();

                // tmb apago leds de game over y victoria acá
                LPC_GPIO0->FIOSET |= (1<<22); // rojo
                LPC_GPIO3->FIOSET |= (1<<25); // verde

                estado_actual = ESTADO_GENERAR_PASO;
                break;
            }

            case ESTADO_GENERAR_PASO: {
                /* =========================================
                   COMPLETAR:
                   - Generar un nuevo paso de la secuencia
                   - Verificar si se alcanzó MAX_SECUENCIA
                   - Reiniciar variables necesarias para mostrar
                   - Definir próximo estado
                   =========================================
                */

                //DUDA: no verifico si se alcanzó MAX_SECUENCIA xq no deberia entrar 
                //aca si se adivino la ult ronda, esta bien? -> Teoricamente lo maneja 
                //el estado de ronda superada, pero se agrega una comprobacion por posible
                //error
                    
                    
                //Genero valor pseudoaleatorio entre 0 y 3 para el nuevo paso de la secuencia
                secuencia[longitud_secuencia] = ticks_ms % 4;

                //Actualizo la longitud de la secuencia
                longitud_secuencia++;                   

                // reinicio variables de mostrar leds y de input de jugador
                indice_mostrar = 0;
                indice_jugador=0;

                
                estado_actual = ESTADO_MOSTRAR_SECUENCIA;
                subestado_mostrar = SUB_ENCENDER_LED;
                    
                    
                break;
            }

            case ESTADO_MOSTRAR_SECUENCIA: {
                /* =========================================
                   COMPLETAR:
                   - Deshabilitar lectura del usuario
                   - Implementar submáquina de estados:
                        SUB_ENCENDER_LED
                        SUB_ESPERAR_LED_ON
                        SUB_APAGAR_LED
                        SUB_ESPERAR_LED_OFF
                   - Al finalizar la secuencia, pasar a
                     ESTADO_ESPERAR_JUGADOR
                   ========================================= */

                //Deshabilito lectura del usuario 
                habilitar_lectura_usuario = 0;
                
            	switch (subestado_mostrar){
					case SUB_ENCENDER_LED:
					{
						//seteo ref, prendo leds, me voy a esperar
						tiempo_referencia = ticks_ms;

                        //Si pongo solo indice_mostrar, me va a prender el primer led de la secuencia, 
                        //pero necesito prender el que corresponde al valor de la secuencia en esa posicion
						led_encender(secuencia[indice_mostrar]);

						subestado_mostrar = SUB_ESPERAR_LED_ON;
						break;
					}

                    //Me quedo en este subestado hasta que se cumpla el tiempo de encendido del led
					case SUB_ESPERAR_LED_ON:
					{
                        if (tiempo_cumplido(tiempo_referencia, TIEMPO_LED_ON)) {
                            subestado_mostrar = SUB_APAGAR_LED;
                        };
						break;
					}

					case SUB_APAGAR_LED:
					{
                        //1. Apago el led que acabo de mostrar
						led_apagar(secuencia[indice_mostrar]);

                        //2. Seteo referencia para el tiempo entre leds
                        tiempo_referencia=ticks_ms;

                        //3. Paso al subestado de espera entre leds
						subestado_mostrar=SUB_ESPERAR_LED_OFF;
						break;
					}

					case SUB_ESPERAR_LED_OFF:
					{
                        //1. Espero el tiempo entre leds
						if (tiempo_cumplido(tiempo_referencia, TIEMPO_LED_OFF)){
                            indice_mostrar++;
                        
                            //2. Si me quedan leds por mostrar, vuelvo a encender otro led
						    if (indice_mostrar < longitud_secuencia) { 
							    subestado_mostrar=SUB_ENCENDER_LED;
						    }else{
                                // ya mostre el ultimo indice, saltar prox estado
                                habilitar_lectura_usuario=1;
                                estado_actual=ESTADO_ESPERAR_JUGADOR;
                            }
                        };

						break;
					}

					default:
					{
						subestado_mostrar = SUB_ENCENDER_LED;
						break;
					}
                }
                break;
            }

            case ESTADO_ESPERAR_JUGADOR: {
                /* =========================================
                   COMPLETAR:
                   - Esperar evento_boton
                   - Mostrar el LED correspondiente al botón
                   - Esperar a que el botón sea soltado
                   - Pasar a ESTADO_VALIDAR_JUGADA
                   ========================================= */
            	if (evento_boton!=0 && boton_presionado!=-1){
            		//1. Esto me muestra el led presionado
                    led_encender(boton_presionado);

                    //2. Espera a que suelte el boton 
            		if (boton_sigue_presionado(boton_presionado)) continue;

                    //3. Se soltó el boton, tengo que apagar el led y bajar la bandera
            		led_apagar(boton_presionado);
                    evento_boton=0;    

            		estado_actual=ESTADO_VALIDAR_JUGADA;
            	}

                break;
            }

            case ESTADO_VALIDAR_JUGADA: {
                /* =========================================
                   COMPLETAR:
                   - Comparar boton_presionado con la secuencia
                   - Si acierta:
                        * avanzar indice_jugador
                        * decidir si sigue esperando botones
                          o si pasa a RONDA_SUPERADA
                   - Si falla:
                        * indicar error
                        * pasar a GAME_OVER
                   ========================================= */
            	if ((boton_presionado != secuencia[indice_jugador])) { // NO acerto
            		
                    error_jugada=1;
            		estado_actual=ESTADO_GAME_OVER;
                    continue;
                }    
                indice_jugador++;

                //puso toda la secuencia bien, paso de ronda
                //pongo == ya que indice jugador seria total de 
                //botones acertados (si ponemos > no llega)

                if (indice_jugador == longitud_secuencia){ 
                    estado_actual=ESTADO_RONDA_SUPERADA;
                    continue;
                } else { 
                    //aca es acierto pero no termino la ronda, sigo esperando que ponga el siguiente boton
                    estado_actual=ESTADO_ESPERAR_JUGADOR;
                }
                // reset de vars de esperar jugador
                evento_boton=0;
                boton_presionado=-1;
            
                break;
            }

            case ESTADO_RONDA_SUPERADA: {
                /* =========================================
                   COMPLETAR:
                   - Incrementar nivel
                   - Decidir si pasa a VICTORIA o a
                     GENERAR_PASO
                    ========================================= */
                
                //Se usa para mostrar el nivel alcanzado, pero para comparar
                //con el maximo de secuencia se usa longitud_secuencia, 
                //ya que  es la que se incrementa al generar un nuevo paso
                
                nivel++;

            	if (longitud_secuencia==MAX_SECUENCIA){
            		estado_actual=ESTADO_VICTORIA;
            	} else {
            		estado_actual=ESTADO_GENERAR_PASO;
            	}
                break;
            }

            case ESTADO_GAME_OVER: {
                /* =========================================
                   COMPLETAR:
                   - Definir comportamiento visual o lógico al perder -> ENCIENDO LED ROJO DE PLACA
                   - Definir transición de salida -> voy a iddle
                   ========================================= */

                //Apago todos los leds de juego por las dudas
                leds_apagar_todos();

                //Enciendo led rojo de game over
            	LPC_GPIO0->FIOCLR |= (1<<22); // luego en inicio debo apagarlo
            	estado_actual=ESTADO_IDLE;

                break;
            }

            case ESTADO_VICTORIA: {
                /* =========================================
                   COMPLETAR:
                   - Definir comportamiento visual o lógico al ganar -> ENCIENDO LED VERDE DE PLACA
                   - Esperar reinicio del juego si se desea -> voy a iddle
                   ========================================= */

                //Apago todos los leds de juego por las dudas
                leds_apagar_todos();
                
                //enciendo led verde de victoria
            	LPC_GPIO3->FIOCLR |= (1<<25); // luego en inicio debo apagarlo
            	estado_actual=ESTADO_IDLE;
                break;
            }

            default: {
                estado_actual = ESTADO_IDLE;
                break;
            }
        }
    }

    return 0;
}

/* =========================
   FUNCIONES AUXILIARES
   ========================= */

uint8_t tiempo_cumplido(uint32_t referencia, uint32_t demora_ms)
{
    return ((ticks_ms - referencia) >= demora_ms);
}

/* =========================
   SYSTICK
   ========================= */

void systick_init(void)
{
    /* Deshabilitar SysTick */
    SysTick->CTRL = 0;

    /* Cargar valor para 1 ms */
    SysTick->LOAD = (SystemCoreClock / 1000) - 1;

    /* Reiniciar contador actual */
    SysTick->VAL = 0;

    /* Habilitar SysTick:
       bit 0 = ENABLE
       bit 1 = TICKINT
       bit 2 = CLKSOURCE (clock del procesador) */
    SysTick->CTRL = (1 << 0) | (1 << 1) | (1 << 2);
}

void SysTick_Handler(void)
{
    ticks_ms++;
}

/* =========================
   INTERRUPCIÓN GPIO P0
   ========================= */

void EINT3_IRQHandler(void)
{
    uint32_t estado_p0;

    estado_p0 = LPC_GPIOINT->IO0IntStatF;

    /* Limpiar flags */
    LPC_GPIOINT->IO0IntClr = estado_p0;

    /* =========================================
       COMPLETAR:
       - Detectar evento de inicio del juego
       - Ignorar pulsaciones si no está habilitada
         la lectura del usuario
       - Detectar qué botón fue presionado
       - Actualizar boton_presionado y evento_boton
       ========================================= */
}

/* =========================
   CONFIGURACIÓN BOTONES
   ========================= */

void config_botones(void)
{
    /* P0.0 a P0.3 como GPIO */
    LPC_PINCON->PINSEL0 &= ~0x000000FF;

    /* Pull-up internos */
    LPC_PINCON->PINMODE0 &= ~0x000000FF;

    /* Entradas */
    LPC_GPIO0->FIODIR &= ~MASK_BOTONES;

    /* Interrupción por flanco descendente */
    LPC_GPIOINT->IO0IntEnF |= MASK_BOTONES;
    LPC_GPIOINT->IO0IntEnR &= ~MASK_BOTONES;

    /* Limpiar pendientes */
    LPC_GPIOINT->IO0IntClr = MASK_BOTONES;

    /* Habilitar EINT3 */
    NVIC_EnableIRQ(EINT3_IRQn);
}

/* =========================
   MANEJO DE LEDS
   ========================= */

void leds_apagar_todos(void)
{
    LPC_GPIO2->FIOCLR = (
        (1 << 0) |
        (1 << 1) |
        (1 << 2) |
        (1 << 3)
    );
}

void led_mostrar(uint8_t indice)
{
    leds_apagar_todos();

    if (indice < CANT_LEDS)
    {
        LPC_GPIO2->FIOSET = led_mask[indice];
    }
}

void leds_init(void)
{
    /* P2.0, P2.1, P2.2, P2.3 como GPIO */
    LPC_PINCON->PINSEL4 &= ~(
        (3 << 0) |
        (3 << 2) |
        (3 << 4) |
        (3 << 6)
    );

    /* Salidas */
    LPC_GPIO2->FIODIR |= (
        (1 << 0) |
        (1 << 1) |
        (1 << 2) |
        (1 << 3)
    );

    // leds de game over (rojo P0.22) y victoria (verde P3.25)
    // config como salidas e inician apagados
    LPC_PINCON->PINSEL4 &= ~(3<<12); // rojo
    LPC_PINCON->PINSEL7 &= ~(3<<18); // verde
	LPC_GPIO0->FIODIR |= (1<<22); // rojo
	LPC_GPIO3->FIODIR |= (1<<25); // verde
	LPC_GPIO0->FIOSET |= (1<<22); // rojo
	LPC_GPIO3->FIOSET |= (1<<25); // verde

    leds_apagar_todos();
}

void led_encender(uint8_t indice)
{
    if (indice >= CANT_LEDS)
        return;

    LPC_GPIO2->FIOSET = led_mask[indice];
}

void led_apagar(uint8_t indice)
{
    if (indice >= CANT_LEDS)
        return;

    LPC_GPIO2->FIOCLR = led_mask[indice];
}

uint8_t boton_sigue_presionado(uint8_t boton)
{
    switch (boton)
    {
        case 0:
            return ((LPC_GPIO0->FIOPIN & BOTON_0) == 0);

        case 1:
            return ((LPC_GPIO0->FIOPIN & BOTON_1) == 0);

        case 2:
            return ((LPC_GPIO0->FIOPIN & BOTON_2) == 0);

        case 3:
            return ((LPC_GPIO0->FIOPIN & BOTON_3) == 0);

        default:
            return 0;
    }
}
