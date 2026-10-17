/*
 * TrabajoEmbebidos.c
 *
 * Created: 16/10/2026 6:00:59 p. m.
 * Author : Juanda
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include "TWI/TWI_LIB.h"
#include "UART/UART_LIB.h"
#include "SSD1306/SSD1306_LIB.h"
#include "BME280/BME280_LIB.h"
#include "DS1307/DS1307_LIB.h"


#define AJUSTAR_HORA 0
#define BME280_DIR   0x76   
#define BME280_ID    0xD0   

/* ---------- Escáner ---------- 
void Escanear_I2C(void){
	uint8_t encontrados = 0;
	UART1_Transmit_Text("Escaneando bus I2C...\r\n");
	for (uint8_t dir = 1; dir < 128; dir++){
		TWI_Start();
		TWI_Write(dir << 1);            // dirección en modo escritura
		if (TWI_Status() == 0x18){      // 0x18 = alguien respondió ACK
			UART1_Transmit_Printf("  Encontrado: 0x%02X\r\n", dir);
			encontrados++;
		}
		TWI_Stop();
	}
	UART1_Transmit_Printf("Total: %u dispositivos\r\n", encontrados);
}
*/

static void Pantalla_Fija(void){
	SSD1306_PrintLine(0, "Estacion I2C");
	SSD1306_PrintLine(1, "---------------------");
}

/* Valores del BME280 (renglones 2, 3 y 4) */
static void Pantalla_BME280(BME280_Datos *d){
	char linea [22];
	int32_t t = d->temperatura;
	char signo = ' ';
	
	if (t < 0){
		signo = '-';
		t = -t;
	}
	
	snprintf(linea, sizeof(linea), "Temp:%c%ld.%02ld\x7F" "C", signo, t / 100, t % 100);
	SSD1306_PrintLine(2, linea);
	
	snprintf(linea, sizeof(linea), "Hum :  %lu.%02lu %%", d->humedad / 100, d->humedad % 100);
	SSD1306_PrintLine(3, linea);
	
	snprintf(linea, sizeof(linea), "Pres:  %lu.%02lu hPa", d->presion / 100, d->presion % 100);
	SSD1306_PrintLine(4, linea);
}

static void Pantalla_Reloj(DS1307_Tiempo *t){
	char linea[22];
	
	snprintf(linea, sizeof(linea), "Fecha: %02u/%02u/%02u", t->dia, t->mes, t->anio);
	SSD1306_PrintLine(6, linea);
	
	snprintf(linea, sizeof(linea), "Hora : %02u:%02u:%02u", t->hora, t->min, t->seg);
	SSD1306_PrintLine(7, linea);
}


int main(void){
	
	BME280_Datos datos;
	DS1307_Tiempo tiempo;
	uint8_t ultimo_seg = 99;
	
	UART1_Init(9600);
	TWI_Init();
	_delay_ms(100);
	
	SSD1306_Init();
	Pantalla_Fija();
	
	if (!BME280_Init()){
		SSD1306_PrintLine(3, "BME280: ERROR");
		UART1_Transmit_Text("BME280 no responde\r\n");
		while (1);
	}
	UART1_Transmit_Text("BME280 listo\r\n");
	
	#if AJUSTAR_HORA
	/* Valores en BCD: se escriben en hex tal como se leen (0x22 = 22 h) */
	DS1307_Set_Seg_Value(0x00);
	DS1307_Set_Min_Value(0x44);
	DS1307_Set_Hour24_Value(0x22);
	DS1307_Set_Week_Day_Value(0x05);
	DS1307_Set_Date_Day_Value(0x16);
	DS1307_Set_Month_Value(0x10);
	DS1307_Set_Year_Value(0x26);

	UART1_Transmit_Text("Hora ajustada\r\n");
	#endif
	
	UART1_Transmit_Text("Estacion lista\r\n");
	

	while (1){
		
		DS1307_Get_Tiempo(&tiempo);
		
		if (tiempo.seg != ultimo_seg){
			ultimo_seg = tiempo.seg;
			
			BME280_Leer(&datos);
			Pantalla_BME280(&datos);
			Pantalla_Reloj(&tiempo);
			
			UART1_Transmit_Printf("%02u:%02u:%02u  T=%ld  H=%lu  P=%lu\r\n",
			tiempo.hora, tiempo.min, tiempo.seg,
			datos.temperatura, datos.humedad, datos.presion);
		}
		
		_delay_ms(1000);
	}
}