
/*
 * twi_master.c
 *
 * Created: 18/01/2026 07:48:44
 *  Author: franc
 */ 





/*
 * twi_master.c - I2C (TWI) Master AVR (ATmega328P)
 */




/*
 * twi_master.c - I2C (TWI) Master AVR (ATmega328P)
 */

#ifndef F_CPU
#define F_CPU 1000000UL
#endif

#include "twi_master.h"
#include <avr/io.h>
#include <util/twi.h>

void twi_init(void)
{
    // Prescaler = 1
    TWSR = 0x00;

    // TWBR = ((F_CPU / SCL) - 16) / 2
    // Com F_CPU=1MHz, SCL real vai ficar abaixo de 100kHz (ok)
    TWBR = 2;

    // Enable TWI
    TWCR = (1 << TWEN);
}

static uint8_t twi_status(void)
{
    return (TWSR & 0xF8);
}

uint8_t twi_start(uint8_t address_rw)
{
    // START
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));

    uint8_t st = twi_status();
    if (st != TW_START && st != TW_REP_START) return 0;

    // SLA+W / SLA+R
    TWDR = address_rw;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));

    st = twi_status();
    if ((address_rw & 0x01) == 0) {
        if (st != TW_MT_SLA_ACK) return 0;
    } else {
        if (st != TW_MR_SLA_ACK) return 0;
    }
    return 1;
}

uint8_t twi_write(uint8_t data)
{
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));

    uint8_t st = twi_status();
    return (st == TW_MT_DATA_ACK);
}

uint8_t twi_read_ack(void)
{
    // Recebe e envia ACK (quer mais bytes)
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}

uint8_t twi_read_nack(void)
{
    // Recebe último byte e envia NACK
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}

void twi_stop(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
    // não precisa esperar
}
