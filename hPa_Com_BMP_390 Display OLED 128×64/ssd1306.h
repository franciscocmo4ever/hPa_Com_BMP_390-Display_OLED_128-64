
/*
 * ssd1306.h
 *
 * Created: 18/01/2026 07:47:58
 *  Author: franc
 */ 






#pragma once
#include <stdint.h>

typedef struct {
	uint8_t addr; // 0x3C ou 0x3D
} oled_t;

void oled_init(oled_t *o, uint8_t addr);
void oled_clear(oled_t *o);
void oled_goto(oled_t *o, uint8_t row, uint8_t col);
void oled_print(oled_t *o, const char *s);
