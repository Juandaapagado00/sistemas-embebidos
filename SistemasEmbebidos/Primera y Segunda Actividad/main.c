/*
 * SistemasEmbebidos.c
 *
 * Created: 4/09/2026 6:38:05 p. m.
 * Author : Juanda
 */ 
#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include "BitsField.h"


void I2C_Inicio(void){
	Control_TWI.twsta = 1;
	Control_TWI.twint = 1;
	Control_TWI.twen = 1;
	
	while (!(Control_TWI.twint));
}

void I2C_Escribir(unsigned char data){
	TWDR = data;
	Control_TWI.twsta = 0;
	Control_TWI.twen = 1;
	Control_TWI.twint = 1;
	while (!(Control_TWI.twint));
}

void I2C_Stop(void) {
	Control_TWI.twsto = 1;
	Control_TWI.twen = 1;
	Control_TWI.twint = 1;
}

void LCD_Enviar(unsigned char val, unsigned char rs) {
    unsigned char dato = (val & 0xF0) | rs | 0x08;

    unsigned char pasos[] = { dato | 0x04, dato & ~0x04 };
    
    for(int i = 0; i < 2; i++) {
        I2C_Inicio();
        I2C_Escribir(0x40);
        I2C_Escribir(pasos[i]);
        I2C_Stop();
        _delay_us(50);
    }
}

void LCD_Comando(unsigned char cmd) {
    LCD_Enviar(cmd, 0x00);
    LCD_Enviar(cmd << 4, 0x00);
}

void LCD_Caracter(unsigned char car) {
    LCD_Enviar(car, 0x01);
    LCD_Enviar(car << 4, 0x01);
}

int main(void)
{
	Config_PA.porta0 = 1;
	Control_TWI.twen = 1;
	TWBR = 32;
	
	_delay_ms(50);
	
	// Inicialización rápida de la LCD en 4 bits
	LCD_Comando(0x33);
	LCD_Comando(0x32);
	LCD_Comando(0x28); // Modo 4 bits, 2 líneas
	LCD_Comando(0x0C); // Encender display sin cursor
	LCD_Comando(0x01); // Limpiar pantalla
	_delay_ms(2);
	
	
	
	LCD_Caracter('N');
	LCD_Caracter('a');
	LCD_Caracter('n');
	LCD_Caracter('a');
	LCD_Caracter('T');
	LCD_Caracter('e');
	LCD_Caracter('A');
	LCD_Caracter('m');
	LCD_Caracter('o');


	while (1)
	{
		Salida_PA.porta0 = 1;
		_delay_ms(500);
		
		Salida_PA.porta0 = 0;
		_delay_ms(500);
	}
}
