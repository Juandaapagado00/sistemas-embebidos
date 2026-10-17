# Estación I2C con ATmega328P

Arduino Nano programado en C a nivel de registros (sin el IDE de Arduino) que lee temperatura, humedad y presión de un **BME280**, la fecha y hora de un **DS3231**, y lo muestra todo en una **OLED SSD1306** de 128×64. Los tres dispositivos comparten un solo bus I²C (TWI).

Proyecto de la asignatura Sistemas Embebidos. Basado en las librerías de clase del repositorio [iSebass/micros_UCEVA](https://github.com/iSebass/micros_UCEVA).

## Hardware

| Componente | Dirección I²C | Notas |
| --- | --- | --- |
| Arduino Nano (ATmega328P, 16 MHz) | — | Maestro del bus |
| OLED SSD1306 0.96" 128×64 | 0x3C | Solo recibe datos |
| BME280 (módulo de 3.3 V) | 0x76 | Chip ID 0x60 |
| DS3231 (módulo ZS-042) | 0x68 | Trae una EEPROM AT24C32 en 0x57 |

### Conexiones (todo a 3.3 V)

| Nano | BME280 | OLED | DS3231 |
| --- | --- | --- | --- |
| 3V3 | VCC | VDD | VCC |
| GND | GND | GND | GND |
| A4 (SDA) | SDA | SDA | SDA |
| A5 (SCL) | SCL | SCK | SCL |

El BME280 es de 3.3 V y los pull-ups de los módulos van a su propio VCC, por eso todo se alimenta desde el pin 3V3.

## Estructura

```
TrabajoEmbebidos.atsln              Solución de Microchip Studio
TrabajoEmbebidos/
├── main.c                          Programa principal
├── TWI/       TWI_LIB              Bus I²C (librería de clase + TWI_Status)
├── UART/      UART_LIB             Depuración por el puerto serie (librería de clase)
├── DS1307/    DS1307_LIB           Reloj: funciones Set de clase + DS1307_Get_Tiempo
├── SSD1306/   SSD1306_LIB          Pantalla OLED: inicio, cursor y texto (fuente 5x7)
├── BME280/    BME280_LIB           Sensor: calibración, configuración y compensación
└── Debug/TrabajoEmbebidos.hex      Programa compilado
```

## Compilar y subir

1. Abrir `TrabajoEmbebidos.atsln` en Microchip Studio 7.
2. Verificar en *Properties → Toolchain → Symbols* que esté `F_CPU=16000000UL`.
3. Compilar (F7).
4. Subir por USB con avrdude, usando el bootloader del Nano:

```
avrdude -C avrdude.conf -p atmega328p -c arduino -P COMx -b 115200 -D -U flash:w:TrabajoEmbebidos.hex:i
```

Para ajustar la hora del reloj, poner `AJUSTAR_HORA` en 1 en `main.c` con la hora actual en BCD, subir una vez, y luego volver a 0.

## Pantalla

```
Estacion I2C
---------------------
Temp: 25.34°C
Hum :  61.23 %
Pres:  899.60 hPa

Fecha: 16/10/26
Hora : 22:44:00
```

La pantalla se actualiza cada vez que cambia el segundo del reloj. Por el puerto serie (9600 baudios) sale una línea con los mismos datos.
