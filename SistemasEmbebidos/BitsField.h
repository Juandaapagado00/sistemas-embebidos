/*
 * BitsField.h
 *
 * Created: 4/09/2026 6:39:26 p. m.
 *  Author: Juanda
 */ 


#ifndef BITSFIELD_H_
#define BITSFIELD_H_
#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>

typedef struct {
	unsigned char txb80 : 1;
	unsigned char rxb80 : 1;
	unsigned char ucsz02 : 1;
	unsigned char txen0 : 1;
	unsigned char rxen0 : 1;
	unsigned char udrien0 : 1;
	unsigned char txcie0 : 1;
	unsigned char rxcie0 : 1;
} volatile * UART0bits;

typedef struct {
	unsigned char txb81 : 1;
	unsigned char rxb81 : 1;
	unsigned char ucsz12 : 1;
	unsigned char txen1 : 1;
	unsigned char rxen1 : 1;
	unsigned char udrien1 : 1;
	unsigned char txcie1 : 1;
	unsigned char rxcie1 : 1;
} volatile * UART1bits;

typedef struct {
	unsigned char wgm00 : 1;
	unsigned char wgm01 : 1;
	unsigned char       : 1;
	unsigned char       : 1;
	unsigned char com0b0 : 1;
	unsigned char com0b1 : 1;
	unsigned char com0a0 : 1;
	unsigned char com0a1 : 1;
} volatile * TIMER0bits;

typedef struct {
	unsigned char wgm10 : 1;
	unsigned char wgm11 : 1;
	unsigned char       : 1;
	unsigned char       : 1;
	unsigned char com1b0 : 1;
	unsigned char com1b1 : 1;
	unsigned char com1a0 : 1;
	unsigned char com1a1 : 1;
} volatile * TIMER1bits;

typedef struct {
	unsigned char wgm20 : 1;
	unsigned char wgm21 : 1;
	unsigned char       : 1;
	unsigned char       : 1;
	unsigned char com2b0 : 1;
	unsigned char com2b1 : 1;
	unsigned char com2a0 : 1;
	unsigned char com2a1 : 1;
} volatile * TIMER2bits;

typedef struct {
	unsigned char porta0 : 1;
	unsigned char porta1 : 1;
	unsigned char porta2 : 1;
	unsigned char porta3 : 1;
	unsigned char porta4 : 1;
	unsigned char porta5 : 1;
	unsigned char porta6 : 1;
	unsigned char porta7 : 1;
} volatile * PORTAbits;

typedef struct {
	unsigned char portb0 : 1;
	unsigned char portb1 : 1;
	unsigned char portb2 : 1;
	unsigned char portb3 : 1;
	unsigned char portb4 : 1;
	unsigned char portb5 : 1;
	unsigned char portb6 : 1;
	unsigned char portb7 : 1;
} volatile * PORTBbits;

typedef struct {
	unsigned char portc0 : 1;
	unsigned char portc1 : 1;
	unsigned char portc2 : 1;
	unsigned char portc3 : 1;
	unsigned char portc4 : 1;
	unsigned char portc5 : 1;
	unsigned char portc6 : 1;
	unsigned char portc7 : 1;
} volatile * PORTCbits;

typedef struct {
	unsigned char portd0 : 1;
	unsigned char portd1 : 1;
	unsigned char portd2 : 1;
	unsigned char portd3 : 1;
	unsigned char portd4 : 1;
	unsigned char portd5 : 1;
	unsigned char portd6 : 1;
	unsigned char portd7 : 1;
} volatile * PORTDbits;

typedef struct {
	unsigned char spr0  : 1;
	unsigned char spr1  : 1;
	unsigned char cpha  : 1;
	unsigned char cpol  : 1;
	unsigned char mstr  : 1;
	unsigned char dord  : 1;
	unsigned char spe   : 1;
	unsigned char spie  : 1;
} volatile * SPIbits;

typedef struct {
	unsigned char twie  : 1;
	unsigned char       : 1;
	unsigned char twen  : 1;
	unsigned char twwc  : 1;
	unsigned char twsto : 1;
	unsigned char twsta : 1;
	unsigned char twea  : 1;
	unsigned char twint : 1;
} volatile * TWIbits;

// Puertos
#define Config_PA    (*(PORTAbits)_SFR_MEM_ADDR(DDRA))
#define Salida_PA (*(PORTAbits)_SFR_MEM_ADDR(PORTA))
#define Leer_PA   (*(PORTAbits)_SFR_MEM_ADDR(PINA))

#define Config_PB    (*(PORTBbits)_SFR_MEM_ADDR(DDRB))
#define Salida_PB (*(PORTBbits)_SFR_MEM_ADDR(PORTB))
#define Leer_PB   (*(PORTBbits)_SFR_MEM_ADDR(PINB))

#define Config_PC    (*(PORTCbits)_SFR_MEM_ADDR(DDRC))
#define Salida_PC (*(PORTCbits)_SFR_MEM_ADDR(PORTC))
#define Leer_PC   (*(PORTCbits)_SFR_MEM_ADDR(PINC))

#define Config_PD    (*(PORTDbits)_SFR_MEM_ADDR(DDRD))
#define Salida_PD (*(PORTDbits)_SFR_MEM_ADDR(PORTD))
#define Leer_PD   (*(PORTDbits)_SFR_MEM_ADDR(PIND))


// UARTS
#define Control_UART0  (*(UART0bits)_SFR_MEM_ADDR(UCSR0B))
#define Control_UART1  (*(UART1bits)_SFR_MEM_ADDR(UCSR1B))


// TIMERS
#define Control_TIMER0 (*(TIMER0bits)_SFR_MEM_ADDR(TCCR0A))
#define Control_TIMER1 (*(TIMER1bits)_SFR_MEM_ADDR(TCCR1A))
#define Control_TIMER2 (*(TIMER2bits)_SFR_MEM_ADDR(TCCR2A))

// SPI
#define Control_SPI   (*(SPIbits)_SFR_MEM_ADDR(SPCR))

// TWI
#define Control_TWI   (*(TWIbits)_SFR_MEM_ADDR(TWCR))


#endif /* BITSFIELD_H_ */
