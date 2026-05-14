## 1) dma_m2m

Mediante la utilizacio del microcontrolador LPC1769, implemente el software necesario para cumplir con los siguientes requerimentos

1-Usar el hardware GPDMA para transferir los datos en palabras de 2bytes desde la primera mitad de la SRAM del banco 1 hacia la segunda mitad d ela SRAM del banco 0

direccion- 0x2007 C000 - 0x2007 FFFF AHB SRAM -Bank 0
direccion- 0x2008 0000 - 0x2008 3FFF AHB SRAM -Bank 1

2-Una vez finalizada la transferencia, mediante la interrupcion por cuenta terminal del GPDMA, se debe encender un led indicador conectado a P0.22

3-Determine usted Cuantos bytes fueron transferidos
fueron transferidos 8K bytes

---

## 2) adc_pwm

Por un pin del ADC del microcontrolador LPC1769 ingresa una tension de rango dinamico 0 a 3,3V proveniente de un sensor de temperatura. Debido a la baja tasas de variacion de la señal, se pide tomar una muestra cada 30s. Pasados los 2min se debe promediar las ultimas 4 muestras y en funcion de este valor, tomar una decision sobre una salda digital de la placa:

- Si el valor es <1 V colocar la salida en 0V
- Si el valor es >= 1V y <=2V modular una señal PWM con un ciclo de trabajo que va dsde el 50% hasta el 90% proporcional al valor de tension, con un periodo de 20KHz
- Si el valor es >2 V colocar la salida 1

CCLOCK= 100MHz
