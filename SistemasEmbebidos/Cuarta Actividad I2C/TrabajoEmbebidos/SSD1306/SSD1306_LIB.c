/*
 * SSD1306_LIB.c
 * OLED SSD1306 128x64 por I2C - solo texto
 */

#include "SSD1306_LIB.h"
#include "SSD1306_DATA.h"

/* ============================================================
 *  BYTES DE LA TRAMA
 * ============================================================ */
#define SSD1306_W   ((SSD1306_DIR << 1) | 0)   /* 0x78 escritura    */
#define CTRL_CMD    0x00                       /* Siguen comandos   */
#define CTRL_DATA   0x40                       /* Siguen pixeles    */


/* ============================================================
 *  FUNCIONES INTERNAS
 * ============================================================ */

/* S, 0x78, 0x00 */
static void Abrir_Comandos(void){
	TWI_Start();
	TWI_Write(SSD1306_W);
	TWI_Write(CTRL_CMD);
}

/* S, 0x78, 0x40 */
static void Abrir_Datos(void){
	TWI_Start();
	TWI_Write(SSD1306_W);
	TWI_Write(CTRL_DATA);
}

/* Zona donde caen los siguientes pixeles */
static void Ventana(uint8_t col_ini, uint8_t col_fin, uint8_t pag_ini, uint8_t pag_fin){
	Abrir_Comandos();
	TWI_Write(0x21);
	TWI_Write(col_ini);
	TWI_Write(col_fin);
	TWI_Write(0x22);
	TWI_Write(pag_ini);
	TWI_Write(pag_fin);
	TWI_Stop();
}

/* 5 columnas de la fuente + 1 de separacion (trama de datos ya abierta) */
static void Enviar_Caracter(char c){
	uint8_t indice;

	if ((uint8_t)c < FONT5X7_FIRST || (uint8_t)c > FONT5X7_LAST){
		c = '?';
	}
	indice = (uint8_t)c - FONT5X7_FIRST;

	for (uint8_t i = 0; i < FONT5X7_WIDTH; i++){
		TWI_Write(pgm_read_byte(&FONT5X7[indice][i]));
	}
	TWI_Write(0x00);
}


/* ============================================================
 *  INICIALIZACION Y LIMPIEZA
 * ============================================================ */

void SSD1306_Init(void){
	Abrir_Comandos();
	for (uint8_t i = 0; i < SSD1306_INIT_LEN; i++){
		TWI_Write(pgm_read_byte(&SSD1306_INIT_CMDS[i]));
	}
	TWI_Stop();
	SSD1306_Clear();
}

void SSD1306_Clear(void){
	Ventana(0, 127, 0, 7);
	Abrir_Datos();
	for (uint16_t i = 0; i < 1024; i++){
		TWI_Write(0x00);
	}
	TWI_Stop();
	SSD1306_SetCursor(0, 0);
}


/* ============================================================
 *  CURSOR
 * ============================================================ */

void SSD1306_SetCursor(uint8_t col, uint8_t row){
	if (col >= SSD1306_COLS) col = SSD1306_COLS - 1;
	if (row >= SSD1306_ROWS) row = SSD1306_ROWS - 1;
	Ventana(col * 6, 127, row, 7);
}


/* ============================================================
 *  ESCRITURA DE TEXTO
 * ============================================================ */

void SSD1306_PutChar(char c){
	Abrir_Datos();
	Enviar_Caracter(c);
	TWI_Stop();
}

void SSD1306_Print(const char *texto){
	Abrir_Datos();
	while (*texto){
		Enviar_Caracter(*texto);
		texto++;
	}
	TWI_Stop();
}

/* Escribe el renglon completo y rellena con espacios */
void SSD1306_PrintLine(uint8_t row, const char *texto){
	uint8_t n = 0;

	SSD1306_SetCursor(0, row);
	Abrir_Datos();
	while (*texto && n < SSD1306_COLS){
		Enviar_Caracter(*texto);
		texto++;
		n++;
	}
	while (n < SSD1306_COLS){
		Enviar_Caracter(' ');
		n++;
	}
	TWI_Stop();
}
