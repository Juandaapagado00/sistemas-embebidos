/*
 * BME280_LIB.c
 * Sensor BME280 por I2C: temperatura, presion y humedad
 * Formulas de compensacion: datasheet Bosch BME280, seccion 4.2.3
 */

#include "BME280_LIB.h"
#include <util/delay.h>

/* ============================================================
 *  BYTES DE DIRECCION
 * ============================================================ */
#define BME280_W   ((BME280_DIR << 1) | 0)    /* 0xEC escritura */
#define BME280_R   ((BME280_DIR << 1) | 1)    /* 0xED lectura   */


/* ============================================================
 *  CALIBRACION (se lee una vez del sensor en BME280_Init)
 * ============================================================ */
static uint16_t dig_T1;
static int16_t  dig_T2, dig_T3;
static uint16_t dig_P1;
static int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
static uint8_t  dig_H1, dig_H3;
static int16_t  dig_H2, dig_H4, dig_H5;
static int8_t   dig_H6;

static int32_t  t_fine;     /* Lo calcula la temperatura y lo usan presion y humedad */


/* ============================================================
 *  TRAMAS I2C
 * ============================================================ */

/* S, 0xEC, reg, valor, P */
static void Escribir_Registro(uint8_t reg, uint8_t valor){
	TWI_Start();
	TWI_Write(BME280_W);
	TWI_Write(reg);
	TWI_Write(valor);
	TWI_Stop();
}

/* S, 0xEC, reg, Sr, 0xED, dato(ACK) ... dato(NACK), P */
static void Leer_Rafaga(uint8_t reg, uint8_t *buffer, uint8_t cantidad){
	TWI_Start();
	TWI_Write(BME280_W);
	TWI_Write(reg);
	TWI_Repeat_Start();
	TWI_Write(BME280_R);
	for (uint8_t i = 0; i < cantidad; i++){
		if (i < cantidad - 1){
			buffer[i] = TWI_ReadAck();      /* "dame otro"  */
		} else {
			buffer[i] = TWI_ReadNack();     /* "ya no mas"  */
		}
	}
	TWI_Stop();
}

/* S, 0xEC, reg, Sr, 0xED, dato(NACK), P */
uint8_t BME280_LeerRegistro(uint8_t reg){
	uint8_t dato;
	Leer_Rafaga(reg, &dato, 1);
	return dato;
}


/* ============================================================
 *  LECTURA DE LA CALIBRACION
 * ============================================================ */

/* Une dos bytes (LSB primero, como vienen en el sensor) */
static uint16_t Unir16(uint8_t lsb, uint8_t msb){
	return ((uint16_t)msb << 8) | lsb;
}

static void Leer_Calibracion(void){
	uint8_t c[26];
	uint8_t h[7];

	Leer_Rafaga(BME280_REG_CALIB1, c, 26);     /* 0x88 a 0xA1 */
	Leer_Rafaga(BME280_REG_CALIB2, h, 7);      /* 0xE1 a 0xE7 */

	/* --- Temperatura --- */
	dig_T1 =          Unir16(c[0],  c[1]);
	dig_T2 = (int16_t)Unir16(c[2],  c[3]);
	dig_T3 = (int16_t)Unir16(c[4],  c[5]);

	/* --- Presion --- */
	dig_P1 =          Unir16(c[6],  c[7]);
	dig_P2 = (int16_t)Unir16(c[8],  c[9]);
	dig_P3 = (int16_t)Unir16(c[10], c[11]);
	dig_P4 = (int16_t)Unir16(c[12], c[13]);
	dig_P5 = (int16_t)Unir16(c[14], c[15]);
	dig_P6 = (int16_t)Unir16(c[16], c[17]);
	dig_P7 = (int16_t)Unir16(c[18], c[19]);
	dig_P8 = (int16_t)Unir16(c[20], c[21]);
	dig_P9 = (int16_t)Unir16(c[22], c[23]);

	/* --- Humedad (H4 y H5 comparten el registro 0xE5) --- */
	dig_H1 = c[25];
	dig_H2 = (int16_t)Unir16(h[0], h[1]);
	dig_H3 = h[2];
	dig_H4 = (int16_t)((int16_t)(int8_t)h[3] * 16) | (int16_t)(h[4] & 0x0F);
	dig_H5 = (int16_t)((int16_t)(int8_t)h[5] * 16) | (int16_t)(h[4] >> 4);
	dig_H6 = (int8_t)h[6];
}


/* ============================================================
 *  FORMULAS DE COMPENSACION (datasheet, seccion 4.2.3)
 * ============================================================ */

/* Resultado en centesimas de grado */
static int32_t Compensar_T(int32_t adc_T){
	int32_t var1, var2;

	var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
	var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12)
	        * ((int32_t)dig_T3)) >> 14;
	t_fine = var1 + var2;
	return (t_fine * 5 + 128) >> 8;
}

/* Resultado en Pa x 256 */
static uint32_t Compensar_P(int32_t adc_P){
	int64_t var1, var2, p;

	var1 = ((int64_t)t_fine) - 128000;
	var2 = var1 * var1 * (int64_t)dig_P6;
	var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
	var2 = var2 + (((int64_t)dig_P4) << 35);
	var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) + ((var1 * (int64_t)dig_P2) << 12);
	var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dig_P1) >> 33;
	if (var1 == 0){
		return 0;                                   /* evita dividir entre cero */
	}
	p = 1048576 - adc_P;
	p = (((p << 31) - var2) * 3125) / var1;
	var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
	var2 = (((int64_t)dig_P8) * p) >> 19;
	p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4);
	return (uint32_t)p;
}

/* Resultado en % x 1024 */
static uint32_t Compensar_H(int32_t adc_H){
	int32_t v;

	v = (t_fine - ((int32_t)76800));
	v = (((((adc_H << 14) - (((int32_t)dig_H4) << 20) - (((int32_t)dig_H5) * v))
	      + ((int32_t)16384)) >> 15)
	    * (((((((v * ((int32_t)dig_H6)) >> 10)
	      * (((v * ((int32_t)dig_H3)) >> 11) + ((int32_t)32768))) >> 10)
	      + ((int32_t)2097152)) * ((int32_t)dig_H2) + 8192) >> 14));
	v = (v - (((((v >> 15) * (v >> 15)) >> 7) * ((int32_t)dig_H1)) >> 4));
	v = (v < 0) ? 0 : v;
	v = (v > 419430400) ? 419430400 : v;
	return (uint32_t)(v >> 12);
}


/* ============================================================
 *  INICIALIZACION
 * ============================================================ */

uint8_t BME280_Init(void){
	if (BME280_LeerRegistro(BME280_REG_ID) != BME280_CHIP_ID){
		return 0;
	}

	Escribir_Registro(BME280_REG_RESET, 0xB6);         /* reset            */
	_delay_ms(10);
	while (BME280_LeerRegistro(BME280_REG_STATUS) & 0x01);   /* copiando calibracion */

	Leer_Calibracion();

	Escribir_Registro(BME280_REG_CTRL_HUM,  0x01);     /* humedad x1        */
	Escribir_Registro(BME280_REG_CONFIG,    0xA0);     /* espera 1 s, sin filtro */
	Escribir_Registro(BME280_REG_CTRL_MEAS, 0x27);     /* temp x1, pres x1, modo normal */
	_delay_ms(50);                                     /* primera medicion  */

	return 1;
}


/* ============================================================
 *  LECTURA DE DATOS
 * ============================================================ */

void BME280_Leer(BME280_Datos *datos){
	uint8_t d[8];
	int32_t adc_T, adc_P, adc_H;

	Leer_Rafaga(BME280_REG_DATOS, d, 8);               /* 0xF7 a 0xFE */

	/* --- Unir bytes crudos (presion y temp de 20 bits, humedad de 16) --- */
	adc_P = (int32_t)(((uint32_t)d[0] << 12) | ((uint32_t)d[1] << 4) | (d[2] >> 4));
	adc_T = (int32_t)(((uint32_t)d[3] << 12) | ((uint32_t)d[4] << 4) | (d[5] >> 4));
	adc_H = (int32_t)(((uint32_t)d[6] << 8)  |  (uint32_t)d[7]);

	/* --- Compensar (la temperatura va primero: calcula t_fine) --- */
	datos->temperatura = Compensar_T(adc_T);
	datos->presion     = Compensar_P(adc_P) >> 8;                /* Pa          */
	datos->humedad     = (Compensar_H(adc_H) * 100) >> 10;       /* centesimas  */
}
