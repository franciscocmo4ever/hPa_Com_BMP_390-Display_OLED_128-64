
/*
 * bmp390.h
 *
 * Created: 18/01/2026 08:15:19
 *  Author: franc
 */ 




#pragma once
#include <stdint.h>

void  bmp390_init(uint8_t addr);          // ex: 0x76 ou 0x77
float bmp390_read_pressure_pa(void);      // retorna em Pa
float bmp390_read_temperature_c(void);    // retorna em °C
