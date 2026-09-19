/*
 * SistemasEmbebidos.c
 *
 * Created: 4/09/2026 6:38:05 p. m.
 * Author : Juanda
 */ 

#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "BitsField.h"

// TWI Y LCD
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


// UART0 Y ADC

void UART0_Init(void) {
	uint16_t ubrr = 51;
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	
	
	UCSR0B = (1<<TXEN0);
	
	
	UCSR0C = (1<<UCSZ01) | (1<<UCSZ00); // 8 bits, 1 stop, sin paridad
}

void UART0_Transmit(unsigned char data) {
	
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = data;
}

void UART0_Transmit_String(char* str) {
	while (*str) {
		UART0_Transmit(*str++);
	}
}

void ADC_Init(void) {
	
	ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1); // Habilitar ADC y prescaler de 64
	ADMUX = (1<<REFS0); // Referencia AVCC
}

uint16_t ADC_Read(uint8_t channel) {
	ADMUX = (ADMUX & 0xF8) | (channel & 0x07);
	ADCSRA |= (1<<ADSC);
	while (ADCSRA & (1<<ADSC)); 
	return ADC;
}


int main(void)
{
	// Configuraciones iniciales
	Config_PA.porta0 = 1; 
	
	// Configuracion PB0 y PB1 
	Config_PB.portb0 = 0;
	Config_PB.portb1 = 0;

	Control_TWI.twen = 1;
	TWBR = 32;
	
	// Inicio de Modulos
	UART0_Init();
	ADC_Init();
	
	_delay_ms(50);
	
	// Inicialización de la LCD
	//LCD_Comando(0x33);
	//LCD_Comando(0x32);
	//LCD_Comando(0x28);
	//LCD_Comando(0x0C);
	//LCD_Comando(0x01);
	//_delay_ms(2);
	
	//LCD_Caracter('H');
	//LCD_Caracter('M');
	//LCD_Caracter('I');

	// Buffer para la trama serial
	char tx_buffer[50];
	
	 UART0_Transmit_String("Hola\r\n");

	while (1)
	{
		
		Salida_PA.porta0 = 1;

		// Sensores PA1 y PA2
		uint16_t analog1 = ADC_Read(1);
		uint16_t analog2 = ADC_Read(2);
		
		// Sensores PB0 y PB1
		uint8_t digital1 = Leer_PB.portb0;
		uint8_t digital2 = Leer_PB.portb1;

		// Empaquetado de Datos
		sprintf(tx_buffer, "A1:%u,A2:%u,D1:%u,D2:%u\r\n", analog1, analog2, digital1, digital2);

		// Envio de Datos
		UART0_Transmit_String(tx_buffer);

		_delay_ms(250);
		
		Salida_PA.porta0 = 0;
		_delay_ms(250);
	}
}
