
/*
 * bmp390.c
 *
 * Created: 18/01/2026 08:27:01
 *  Author: franc
 */ 




/*
 * bmp390.c
 *
 * Driver simples para BMP390 (I2C) com compensação em float
 * - Endereço: 0x76 ou 0x77 (depende do pino SDO)
 * - Leituras: pressão (Pa) e temperatura (°C)
 *
 * Baseado no algoritmo de compensação do datasheet (Appendix).
 */

#define F_CPU 1000000UL

#include "bmp390.h"
#include "twi_master.h"

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

// =======================
// Registradores BMP390
// =======================
#define BMP390_REG_CHIP_ID     0x00
#define BMP390_CHIP_ID_VAL     0x60

#define BMP390_REG_STATUS      0x03
#define BMP390_REG_PRESS_XLSB  0x04  // 0x04..0x06 (24-bit)
#define BMP390_REG_TEMP_XLSB   0x07  // 0x07..0x09 (24-bit)

#define BMP390_REG_PWR_CTRL    0x1B
#define BMP390_REG_OSR         0x1C
#define BMP390_REG_ODR         0x1D
#define BMP390_REG_CONFIG      0x1F

#define BMP390_REG_CMD         0x7E
#define BMP390_CMD_SOFTRESET   0xB6

// Calibração: 0x31 .. 0x45 (21 bytes)
#define BMP390_REG_CALIB_START 0x31
#define BMP390_CALIB_LEN       21

// =======================
// Estado interno
// =======================
static uint8_t g_addr = 0x76;

// Estrutura de calibração em float (igual conceito do datasheet)
typedef struct {
    float par_t1;
    float par_t2;
    float par_t3;

    float par_p1;
    float par_p2;
    float par_p3;
    float par_p4;
    float par_p5;
    float par_p6;
    float par_p7;
    float par_p8;
    float par_p9;
    float par_p10;
    float par_p11;

    float t_lin; // carregado pela compensação de temperatura
} bmp390_calib_t;

static bmp390_calib_t cal;

// =======================
// Helpers I2C (TWI)
// =======================
static void bmp390_write_u8(uint8_t reg, uint8_t val)
{
    twi_start((g_addr << 1) | 0); // write
    twi_write(reg);
    twi_write(val);
    twi_stop();
}

static uint8_t bmp390_read_u8(uint8_t reg)
{
    uint8_t v;

    twi_start((g_addr << 1) | 0);
    twi_write(reg);

    twi_start((g_addr << 1) | 1);
    v = twi_read_nack();
    twi_stop();

    return v;
}

static void bmp390_read_buf(uint8_t reg, uint8_t *buf, uint8_t n)
{
    twi_start((g_addr << 1) | 0);
    twi_write(reg);

    twi_start((g_addr << 1) | 1);
    for (uint8_t i = 0; i < n; i++) {
        if (i == (n - 1)) buf[i] = twi_read_nack();
        else              buf[i] = twi_read_ack();
    }
    twi_stop();
}

// =======================
// Conversões / parsing
// =======================
static int16_t s16_le(const uint8_t *p)
{
    return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint16_t u16_le(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t u24_le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
}

// =======================
// Compensação (datasheet)
// =======================
static float bmp390_compensate_temperature(uint32_t uncomp_temp)
{
    float partial_data1 = (float)uncomp_temp - cal.par_t1;
    float partial_data2 = partial_data1 * cal.par_t2;

    cal.t_lin = partial_data2 + (partial_data1 * partial_data1) * cal.par_t3;
    return cal.t_lin;
}

static float bmp390_compensate_pressure(uint32_t uncomp_press)
{
    float partial_data1;
    float partial_data2;
    float partial_data3;
    float partial_data4;
    float partial_out1;
    float partial_out2;

    partial_data1 = cal.par_p6 * cal.t_lin;
    partial_data2 = cal.par_p7 * (cal.t_lin * cal.t_lin);
    partial_data3 = cal.par_p8 * (cal.t_lin * cal.t_lin * cal.t_lin);
    partial_out1   = cal.par_p5 + partial_data1 + partial_data2 + partial_data3;

    partial_data1 = cal.par_p2 * cal.t_lin;
    partial_data2 = cal.par_p3 * (cal.t_lin * cal.t_lin);
    partial_data3 = cal.par_p4 * (cal.t_lin * cal.t_lin * cal.t_lin);
    partial_out2  = (float)uncomp_press * (cal.par_p1 + partial_data1 + partial_data2 + partial_data3);

    partial_data1 = (float)uncomp_press * (float)uncomp_press;
    partial_data2 = cal.par_p9 + cal.par_p10 * cal.t_lin;
    partial_data3 = partial_data1 * partial_data2;

    partial_data4 = partial_data3
                  + ((float)uncomp_press * (float)uncomp_press * (float)uncomp_press) * cal.par_p11;

    return partial_out1 + partial_out2 + partial_data4;
}

// =======================
// Lê e converte calibração
// =======================
static void bmp390_read_calibration(void)
{
    uint8_t b[BMP390_CALIB_LEN];
    bmp390_read_buf(BMP390_REG_CALIB_START, b, BMP390_CALIB_LEN);

    // Offsets no bloco 0x31..0x45 (21 bytes)
    // 0x31 T1_LSB, 0x32 T1_MSB
    // 0x33 T2_LSB, 0x34 T2_MSB
    // 0x35 T3
    // 0x36 P1_LSB, 0x37 P1_MSB
    // 0x38 P2_LSB, 0x39 P2_MSB
    // 0x3A P3
    // 0x3B P4
    // 0x3C P5_LSB, 0x3D P5_MSB
    // 0x3E P6_LSB, 0x3F P6_MSB
    // 0x40 P7
    // 0x41 P8
    // 0x42 P9_LSB, 0x43 P9_MSB
    // 0x44 P10
    // 0x45 P11

    uint16_t nvm_t1 = u16_le(&b[0]);
    uint16_t nvm_t2 = u16_le(&b[2]);
    int8_t   nvm_t3 = (int8_t)b[4];

    int16_t  nvm_p1 = s16_le(&b[5]);
    int16_t  nvm_p2 = s16_le(&b[7]);
    int8_t   nvm_p3 = (int8_t)b[9];
    int8_t   nvm_p4 = (int8_t)b[10];
    uint16_t nvm_p5 = u16_le(&b[11]);
    uint16_t nvm_p6 = u16_le(&b[13]);
    int8_t   nvm_p7 = (int8_t)b[15];
    int8_t   nvm_p8 = (int8_t)b[16];
    int16_t  nvm_p9 = s16_le(&b[17]);
    int8_t   nvm_p10= (int8_t)b[19];
    int8_t   nvm_p11= (int8_t)b[20];

    // Conversões para float (escala do datasheet)
    cal.par_t1  = (float)nvm_t1 * (1.0f / 256.0f);                 // /2^8
    cal.par_t2  = (float)nvm_t2 * (1.0f / 1073741824.0f);          // /2^30
    cal.par_t3  = (float)nvm_t3 * (1.0f / 281474976710656.0f);     // /2^48

    cal.par_p1  = ((float)nvm_p1 - 16384.0f) * (1.0f / 1048576.0f);          // (x-2^14)/2^20
    cal.par_p2  = ((float)nvm_p2 - 16384.0f) * (1.0f / 536870912.0f);        // (x-2^14)/2^29
    cal.par_p3  = (float)nvm_p3 * (1.0f / 4294967296.0f);                    // /2^32
    cal.par_p4  = (float)nvm_p4 * (1.0f / 137438953472.0f);                  // /2^37

    // Atenção: no datasheet aparece "2^-3" (equivale a multiplicar por 8)
    cal.par_p5  = (float)nvm_p5 * 8.0f;                                      // /2^-3  == *2^3
    cal.par_p6  = (float)nvm_p6 * (1.0f / 64.0f);                            // /2^6
    cal.par_p7  = (float)nvm_p7 * (1.0f / 256.0f);                           // /2^8
    cal.par_p8  = (float)nvm_p8 * (1.0f / 32768.0f);                         // /2^15
    cal.par_p9  = (float)nvm_p9 * (1.0f / 281474976710656.0f);               // /2^48
    cal.par_p10 = (float)nvm_p10 * (1.0f / 281474976710656.0f);              // /2^48

    // 2^65 é gigante; esse coef fica bem pequeno mesmo
    cal.par_p11 = (float)nvm_p11 * (1.0f / 36893488147419103232.0f);         // /2^65

    cal.t_lin = 0.0f;
}

// =======================
// API pública (bmp390.h)
// =======================
void bmp390_init(uint8_t addr)
{
    g_addr = addr;

    // Soft reset
    bmp390_write_u8(BMP390_REG_CMD, BMP390_CMD_SOFTRESET);
    _delay_ms(10);

    // (Opcional) checar CHIP_ID
    // Se quiser: você pode colocar um if aqui e tratar erro
    // uint8_t id = bmp390_read_u8(BMP390_REG_CHIP_ID);

    // Configura oversampling (exemplo bom pra barômetro)
    // osr_p = x8 (011), osr_t = x2 (001)
    // OSR: bits 2..0 (press) e 5..3 (temp)
    uint8_t osr = (1u << 3) | (3u); // temp x2, press x8
    bmp390_write_u8(BMP390_REG_OSR, osr);

    // Filtro IIR: bypass (0) ou leve (ex: coef_3 = 010)
    // CONFIG bits 3..1 = iir_filter
    uint8_t cfg = (0u << 1); // coef_0 (bypass)
    // cfg = (2u << 1);      // coef_3 (se quiser suavizar)
    bmp390_write_u8(BMP390_REG_CONFIG, cfg);

    // ODR não importa em forced, mas deixa um valor válido
    bmp390_write_u8(BMP390_REG_ODR, 0x00); // 200Hz (tanto faz aqui)

    // Lê calibração
    bmp390_read_calibration();

    // Deixa em sleep com sensores habilitados (mode=00)
    // press_en=1, temp_en=1, mode=00
    bmp390_write_u8(BMP390_REG_PWR_CTRL, (1u << 0) | (1u << 1));
}

static void bmp390_force_measurement(void)
{
    // mode[1:0] fica nos bits 5..4
    // forced mode = 01 ou 10 -> vamos usar 01 (bit4 = 1)
    uint8_t pwr = (1u << 0) | (1u << 1) | (1u << 4);
    bmp390_write_u8(BMP390_REG_PWR_CTRL, pwr);

    // Tempo de conversão depende dos OSR (mas esse delay resolve na prática)
    _delay_ms(40);
}

float bmp390_read_temperature_c(void)
{
    uint8_t d[6];

    bmp390_force_measurement();

    // lê press(3) + temp(3) de uma vez (0x04..0x09)
    bmp390_read_buf(BMP390_REG_PRESS_XLSB, d, 6);

    uint32_t uncomp_temp  = u24_le(&d[3]);  // bytes 0x07..0x09

    return bmp390_compensate_temperature(uncomp_temp);
}

float bmp390_read_pressure_pa(void)
{
    uint8_t d[6];

    bmp390_force_measurement();

    bmp390_read_buf(BMP390_REG_PRESS_XLSB, d, 6);

    uint32_t uncomp_press = u24_le(&d[0]);  // bytes 0x04..0x06
    uint32_t uncomp_temp  = u24_le(&d[3]);  // bytes 0x07..0x09

    // sempre atualiza t_lin antes de compensar pressão
    (void)bmp390_compensate_temperature(uncomp_temp);

    return bmp390_compensate_pressure(uncomp_press);
}
