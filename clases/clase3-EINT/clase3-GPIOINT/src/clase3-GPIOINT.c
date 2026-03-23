#include "LPC17xx.h"

// Handler de EINT3 (GPIO interrupt)
void EINT3_IRQHandler(void)
{
    // Verificar si fue en puerto 0
    if (LPC_GPIOINT->IO0IntStatR & (1 << 10)) {

        // toggle LED
        LPC_GPIO0->FIOPIN ^= (1 << 22);

        // limpiar interrupción
        LPC_GPIOINT->IO0IntClr = (1 << 10);
    }
}

int main(void)
{
    // Botón en P0.10 como entrada
    LPC_GPIO0->FIODIR &= ~(1 << 10);

    // Pull-down en P0.10
    LPC_PINCON->PINMODE0 &= ~(3 << (2 * 10));
    LPC_PINCON->PINMODE0 |=  (3 << (2 * 10));

    // Limpiar interrupciones previas
    LPC_GPIOINT->IO0IntEnR &= ~(1<<10); // limpia interrupciones por flanco de subida del pin
    LPC_GPIOINT->IO0IntEnF &= ~(1<<10); // limpia interrupciones por flanco de bajada del pin

    // Habilitar interrupción por flanco de subida
    LPC_GPIOINT->IO0IntEnR |= (1 << 10);

    // Limpiar flag
    LPC_GPIOINT->IO0IntClr = (1 << 10);

    // Habilitar en NVIC (EINT3)
    NVIC->ISER[0] |= (1 << 21);

    // LED P0.22 como salida y apagado inicialmente
    LPC_GPIO0->FIODIR |= (1 << 22);
    LPC_GPIO0->FIOCLR = (1 << 22);

    while (1)
    {
        // idle
    }
}
