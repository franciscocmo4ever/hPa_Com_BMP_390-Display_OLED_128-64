
/*
 * twi_master.h
 *
 * Created: 18/01/2026 07:48:37
 *  Author: franc
 */ 

#pragma once
#include <stdint.h>

void    twi_init(void);
uint8_t twi_start(uint8_t address_rw);
uint8_t twi_write(uint8_t data);
uint8_t twi_read_ack(void);
uint8_t twi_read_nack(void);
void    twi_stop(void);

