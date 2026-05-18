## 1. HECHO

Programar el microcontrolador LPC1769 en un código de lenguaje C para que, utilizando un timer y un pin de capture de esta placa sea posible demodular una señal PWM que ingresa por dicho pin (calcular el ciclo de trabajo y el periodo) y sacar una tensión continua proporcional al ciclo de trabajo a través del DAC de rango dinámico 0-2V con un rate de actualización de 0,5s del promedio de las últimos diez valores obtenidos en la captura.

## 2. HECHO

Programar el microcontrolador LPC1769 en un código de lenguaje C para que mediante su ADC digitalice una señal analógica cuyo ancho de banda es de 16 khz. La señal analógica tiene una amplitud de pico máxima positiva de 3.3 voltios. Los datos deben ser guardados utilizando el Hardware GDMA en la primera mitad de la memoria SRAM ubicada en el bloque **AHB SRAM - bank 0**, de manera tal que permita **almacenar todos los datos posibles** que esta memoria nos permita. Los datos deben ser almacenados como un buffer circular conservando siempre las últimas muestras.
Por otro lado se tiene una forma de onda como se muestra en la imagen a continuación. Esta señal debe ser generada por una función y debe ser reproducida por el DAC desde la segunda mitad de **AHB SRAM - bank 0 memoria** utilizando DMA de tal forma que se logre un periodo de 614us logrando la máxima resolución y máximo rango de tensión.
Durante operación normal se debe generar por el DAC la forma de onda mencionada como wave_form. Se debe indicar cuál es el mínimo incremento de tensión de salida de esa forma de onda.
Cuando interrumpe una extint conectada a un pin, el ADC configurado debe completar el ciclo de conversión que estaba realizando, y ser detenido, a continuación se comienzan a sacar las muestras del ADC por el DAC utilizando DMA y desde las posiciones de memoria originales.
Cuando interrumpe nuevamente en el mismo pin, se vuelve a repetir la señal del DAC generada por la forma de onda de wave_form previamente almacenada y se arranca de nuevo la conversión de datos del adc. Se alterna así entre los dos estados del sistema con cada interrupción externa.
Suponer una frecuencia de core cclk de 80 Mhz. El código debe estar debidamente comentado.
![Pasted image 20260517223931.png](/parciales/2P/docs/Pasted%20image%2020260517224208.png)

## 3. HECHO

Describa la función del bit BIAS de DAC del microcontrolador LPC1769.

**RTA**
El bit BIAS del DAC permite cambiar entre el modo de alto consumo y bajo consumo.

Con el bit en 0 se está en alto consumo, permitiendo un tiempo de establecimiento de la señal convertida de 1us, y hace que el DAC consuma hasta 700uA. En conjunto se puede tener una tasa de conversion de 1MHz máximo.

Con el bit en 1 se está en bajo consumo, permite tiempo de establecimiento de la señal convertida de 2.5us, y y hace que el DAC consuma hasta 350uA. En conjunto se puede tener una tasa de conversion de 400KHz máximo.

En resumen sacrificio velocidad de conversion por consumo de corriente.
