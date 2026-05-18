#include "LPC17xx.h"
#include "lpc17xx_dac.h"

#define CONFIG_ADDR 0x10004000

void demora(uint32_t tiempo);

int main(void)
{

    uint32_t config;

    uint8_t vmax;
    uint8_t delayTop;
    uint8_t delayUp;
    uint8_t delayDown;

    uint16_t valor;

    // ==========================
    // Inicialización DAC
    // ==========================

    DAC_Init();

    // ==========================
    // Lectura configuración
    // ==========================

    config = *((uint32_t *)CONFIG_ADDR);

    vmax = config & 0xFF;
    delayTop = (config >> 8) & 0xFF;
    delayUp = (config >> 16) & 0xFF;
    delayDown = (config >> 24) & 0xFF;

    while (1)
    {

        // ======================
        // SUBIDA
        // ======================

        for (valor = 0; valor <= vmax; valor++)
        {

            DAC_UpdateValue(valor);

            demora(delayUp);
        }

        // ======================
        // MESETA
        // ======================

        demora(delayTop);

        // ======================
        // BAJADA
        // ======================

        for (valor = vmax; valor > 0; valor--)
        {

            DAC_UpdateValue(valor);

            demora(delayDown);
        }

        // vuelve automáticamente
    }
}

void demora(uint32_t tiempo)
{

    volatile uint32_t i;

    for (i = 0; i < tiempo * 1000; i++)
        ;
}