/*
 * hPa_Com_BMP_390 Display OLED 128×64.c
 *
 * Created: 18/01/2026 07:47:31
 * Author : franc
 */ 

#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "twi_master.h"
#include "ssd1306.h"
#include "bmp390.h"

static void oled_print_line(oled_t *o, uint8_t row, const char *txt)
{
	// imprime e “apaga resto” (linha 21 chars aprox)
	char buf[22];
	uint8_t i = 0;
	while (txt[i] && i < 21) { buf[i] = txt[i]; i++; }
	while (i < 21) { buf[i++] = ' '; }
	buf[i] = 0;

	oled_goto(o, row, 0);
	oled_print(o, buf);
}

int main(void)
{
	twi_init();

	oled_t oled;
	oled_init(&oled, 0x3C); // se não aparecer, troque para 0x3D

	bmp390_init(0x76); // troque para 0x77 se teu módulo for 0x77

	oled_print_line(&oled, 0, "hPa:");
	oled_print_line(&oled, 2, "Temp:");

	while (1)
	{
		float p_pa = bmp390_read_pressure_pa();
		float t_c  = bmp390_read_temperature_c();

		float p_hpa = p_pa / 100.0f;

		// pressão com 1 casa
		int32_t p10 = (int32_t)(p_hpa * 10.0f + (p_hpa >= 0 ? 0.5f : -0.5f));
		int32_t pi  = p10 / 10;
		int32_t pf  = p10 % 10; if (pf < 0) pf = -pf;

		// temp com 1 casa
		int32_t t10 = (int32_t)(t_c * 10.0f + (t_c >= 0 ? 0.5f : -0.5f));
		int32_t ti  = t10 / 10;
		int32_t tf  = t10 % 10; if (tf < 0) tf = -tf;

		char line1[22];
		char line3[22];

		// Só usei caracteres que existem na fonte mínima: números, '.', '-', ':', 'h','P','a','T','e','m','p','C',' '
		snprintf(line1, sizeof(line1), "hPa: %ld.%ld", (long)pi, (long)pf);
		snprintf(line3, sizeof(line3), "Temp: %ld.%ld C", (long)ti, (long)tf);

		oled_print_line(&oled, 1, line1);
		oled_print_line(&oled, 3, line3);

		_delay_ms(500);
	}
}
