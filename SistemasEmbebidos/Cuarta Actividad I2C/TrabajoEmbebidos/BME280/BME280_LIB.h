/*
 * BME280_LIB.h
 * Sensor BME280 por I2C: temperatura, presion y humedad
 */

#ifndef BME280_LIB_H_
#define BME280_LIB_H_

#include <stdint.h>
#include "../TWI/TWI_LIB.h"

/* ============================================================
 *  CONFIGURACION
 * ============================================================ */
#define BME280_DIR        0x76     /* Direccion I2C de 7 bits (SDO a GND) */
#define BME280_CHIP_ID    0x60     /* Valor esperado en el registro 0xD0  */

/* ============================================================
 *  REGISTROS
 * ============================================================ */
#define BME280_REG_CALIB1     0x88   /* Calibracion parte 1 (26 bytes)  */
#define BME280_REG_ID         0xD0   /* Chip ID                         */
#define BME280_REG_RESET      0xE0   /* Reset (escribir 0xB6)           */
#define BME280_REG_CALIB2     0xE1   /* Calibracion parte 2 (7 bytes)   */
#define BME280_REG_CTRL_HUM   0xF2   /* Configuracion humedad           */
#define BME280_REG_STATUS     0xF3   /* Estado                          */
#define BME280_REG_CTRL_MEAS  0xF4   /* Configuracion temp/pres y modo  */
#define BME280_REG_CONFIG     0xF5   /* Tiempo de espera y filtro       */
#define BME280_REG_DATOS      0xF7   /* Datos crudos (8 bytes)          */

/* ============================================================
 *  DATOS DE SALIDA
 * ============================================================ */
typedef struct {
	int32_t  temperatura;   /* Centesimas de grado:  2534  = 25.34 C   */
	uint32_t presion;       /* Pascales:             89960 = 899.60 hPa */
	uint32_t humedad;       /* Centesimas de %:      6123  = 61.23 %    */
} BME280_Datos;

/* ============================================================
 *  FUNCIONES
 * ============================================================ */
uint8_t BME280_Init(void);                 /* 1 = listo, 0 = no responde */
void    BME280_Leer(BME280_Datos *datos);
uint8_t BME280_LeerRegistro(uint8_t reg);

#endif /* BME280_LIB_H_ */
