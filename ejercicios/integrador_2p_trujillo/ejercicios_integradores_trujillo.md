## Generador de ondas cuadradas

Se desea 4 ondas cuadradas de distinta frecuencia por los pines P0.0-P0.3, por lo que se pide que desarrolle un programa que utilice el módulo GPDMA para generar el patrón de ondas mostradas de manera simultanea.

![ondas](/ejercicios/integrador_2p_trujillo/ondas_ej1_integrador_trujillo.png)

- La secuencia debe estar almacenada en memoria, en la dirección 0x2007C000
- El periodo de cada una de las 4 ondas es 1, 2, 4 y 8 [ms] respectivamente.
- La transferencia debe ser continua mediante DMA, sin el uso de
  interrupciones.

## Promedio del ADC

Se tiene una señal analógica conectada al P1.30 de la LPC1769. Se pide implementar un programa que permita muestrear la señal, cada cien muestras tomadas se deberá de calcular el promedio de las mismas y el resultado, apropiadamente tratado, deberá ser mostrado mediante el pin P0.26, se debe mantener el mismo nivel de tensión a la salida hasta que se tome un nuevo conjunto de muestras

- La señal es de frecuencia variable y tiene como máximo 5 [kHz].
- Elija una frecuencia de muestreo adecuada y justifique.
- No se debe cargar al core con el trabajo de almacenar las muestras.
- Tenga cuidado con las condiciones de carrera en el buffer de almacenamiento.
- Las muestras se deben guardar en el banco 1 de la AHB SRAM

## Inyección Dinámica de Patrones en Bus Paralelo:

Se dispone de 2 pulsadores conectados a los pines P2.0 y P2.1.
Dependiendo del pulsador presionado, el sistema debe volcar un patrón de prueba específico de 512 bytes hacia los bits más bajos de los puertos P0 y P1 respectivamente

- Ambos array se ubican de manera contigua desde el inicio del banco 0 de la AHB SRAM.
- La transferencia se debe realizar a la máxima velocidad posible que permita el DMA.
- Cada transferencia debe usar el canal con la prioridad más baja disponible en ese momento.
- La transferencia del array correspondiente se realiza una sola vez, cada que se presiona el pulsador.
