
/*
 * ssd1306.c
 *
 * Created: 18/01/2026 07:48:08
 *  Author: franc
 */ 
 /*
 * ssd1306.c - OLED 0.96" 128x64 I2C (SSD1306)
 * Driver simples: init, clear, goto (linha/col), print texto
 */

#define F_CPU 1000000UL


#include "ssd1306.h"
#include "twi_master.h"

#include <avr/pgmspace.h>
#include <stdint.h>
#include <string.h>

#define OLED_CTRL_CMD   0x00
#define OLED_CTRL_DATA  0x40

static uint8_t cur_page = 0;
static uint8_t cur_col  = 0;

// Fonte mínima (só o necessário para: "hPa:", "Temp:", números, ponto, sinal, espaço e C)
static const uint8_t font_min[][5] PROGMEM = {
	// 0: ' '
	{0x00,0x00,0x00,0x00,0x00},
	// 1: '.'
	{0x00,0x60,0x60,0x00,0x00},
	// 2: '-'
	{0x08,0x08,0x08,0x08,0x08},
	// 3..12: '0'..'9'
	{0x3E,0x51,0x49,0x45,0x3E}, // 0
	{0x00,0x42,0x7F,0x40,0x00}, // 1
	{0x62,0x51,0x49,0x49,0x46}, // 2
	{0x22,0x49,0x49,0x49,0x36}, // 3
	{0x18,0x14,0x12,0x7F,0x10}, // 4
	{0x2F,0x49,0x49,0x49,0x31}, // 5
	{0x3E,0x49,0x49,0x49,0x32}, // 6
	{0x01,0x71,0x09,0x05,0x03}, // 7
	{0x36,0x49,0x49,0x49,0x36}, // 8
	{0x26,0x49,0x49,0x49,0x3E}, // 9
	// 13: ':'
	{0x00,0x36,0x36,0x00,0x00},
	// 14: 'C'
	{0x3E,0x41,0x41,0x41,0x22},
	// 15: 'P'
	{0x7F,0x09,0x09,0x09,0x06},
	// 16: 'T'
	{0x01,0x01,0x7F,0x01,0x01},
	// 17: 'a'
	{0x20,0x54,0x54,0x54,0x78},
	// 18: 'e'
	{0x38,0x54,0x54,0x54,0x18},
	// 19: 'h'
	{0x7F,0x08,0x04,0x04,0x78},
	// 20: 'm'
	{0x7C,0x04,0x18,0x04,0x78},
	// 21: 'p'
	{0x7C,0x14,0x14,0x14,0x08},
};

static uint8_t map_char(char c)
{
	switch(c){
		case ' ': return 0;
		case '.': return 1;
		case '-': return 2;
		case '0': return 3;
		case '1': return 4;
		case '2': return 5;
		case '3': return 6;
		case '4': return 7;
		case '5': return 8;
		case '6': return 9;
		case '7': return 10;
		case '8': return 11;
		case '9': return 12;
		case ':': return 13;
		case 'C': return 14;
		case 'P': return 15;
		case 'T': return 16;
		case 'a': return 17;
		case 'e': return 18;
		case 'h': return 19;
		case 'm': return 20;
		case 'p': return 21;
		default:  return 0;
	}
}

static void oled_cmd(oled_t *o, uint8_t cmd)
{
	twi_start((o->addr << 1) | 0);
	twi_write(OLED_CTRL_CMD);
	twi_write(cmd);
	twi_stop();
}

static void oled_data_begin(oled_t *o)
{
	twi_start((o->addr << 1) | 0);
	twi_write(OLED_CTRL_DATA);
}

static void oled_data_end(void)
{
	twi_stop();
}

static void oled_set_pos(oled_t *o, uint8_t page, uint8_t col)
{
	cur_page = (page & 0x07);
	cur_col  = col;

	oled_cmd(o, 0xB0 | cur_page);
	oled_cmd(o, 0x00 | (cur_col & 0x0F));
	oled_cmd(o, 0x10 | ((cur_col >> 4) & 0x0F));
}

static void oled_putc(oled_t *o, char c)
{
	uint8_t idx = map_char(c);

	oled_data_begin(o);
	for(uint8_t i=0; i<5; i++){
		twi_write(pgm_read_byte(&font_min[idx][i]));
	}
	twi_write(0x00); // espaço
	oled_data_end();
}

void oled_init(oled_t *o, uint8_t addr)
{
	o->addr = addr;

	oled_cmd(o, 0xAE); // display off
	oled_cmd(o, 0xD5); oled_cmd(o, 0x80);
	oled_cmd(o, 0xA8); oled_cmd(o, 0x3F);
	oled_cmd(o, 0xD3); oled_cmd(o, 0x00);
	oled_cmd(o, 0x40);
	oled_cmd(o, 0x8D); oled_cmd(o, 0x14);
	oled_cmd(o, 0x20); oled_cmd(o, 0x00);
	oled_cmd(o, 0xA1);
	oled_cmd(o, 0xC8);
	oled_cmd(o, 0xDA); oled_cmd(o, 0x12);
	oled_cmd(o, 0x81); oled_cmd(o, 0xCF);
	oled_cmd(o, 0xD9); oled_cmd(o, 0xF1);
	oled_cmd(o, 0xDB); oled_cmd(o, 0x40);
	oled_cmd(o, 0xA4);
	oled_cmd(o, 0xA6);
	oled_cmd(o, 0xAF); // display on

	oled_clear(o);
}

void oled_clear(oled_t *o)
{
	for(uint8_t p=0; p<8; p++){
		oled_set_pos(o, p, 0);
		oled_data_begin(o);
		for(uint8_t i=0; i<128; i++){
			twi_write(0x00);
		}
		oled_data_end();
	}
	oled_set_pos(o, 0, 0);
}

void oled_goto(oled_t *o, uint8_t row, uint8_t col)
{
	oled_set_pos(o, row & 0x07, (uint8_t)(col * 6));
}

void oled_print(oled_t *o, const char *s)
{
	while(*s){
		if(*s=='\n'){
			cur_page = (cur_page + 1) & 0x07;
			oled_set_pos(o, cur_page, 0);
			s++;
			continue;
		}
		oled_putc(o, *s++);
	}
}
