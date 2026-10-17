/*
 * SSD1306_LIB.h
 * OLED SSD1306 128x64 por I2C - solo texto (21 columnas x 8 renglones)
 */

#ifndef SSD1306_LIB_H_
#define SSD1306_LIB_H_

#include <stdint.h>
#include "../TWI/TWI_LIB.h"

/* ============================================================
 *  CONFIGURACION
 * ============================================================ */
#define SSD1306_DIR   0x3C     /* Direccion I2C de 7 bits   */
#define SSD1306_COLS  21       /* Caracteres por renglon    */
#define SSD1306_ROWS  8        /* Renglones                 */

/* ============================================================
 *  FUNCIONES
 * ============================================================ */
void SSD1306_Init(void);
void SSD1306_Clear(void);
void SSD1306_SetCursor(uint8_t col, uint8_t row);
void SSD1306_PutChar(char c);
void SSD1306_Print(const char *texto);
void SSD1306_PrintLine(uint8_t row, const char *texto);

#endif /* SSD1306_LIB_H_ */
