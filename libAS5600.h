#ifndef LIBAS5600_H_INCLUDED
#define LIBAS5600_H_INCLUDED

#define LIBTWI_IMPLEMENTATION
#include "libtwi.h"

// Configuration Registers
#define AS5600_ZMCO       0x00
#define AS5600_ZPOS       0x01
// #define AS5600_ZPOS0      0x02
#define AS5600_MPOS       0x03
// #define AS5600_MPOS0      0x04
#define AS5600_MANG       0x05
// #define AS5600_MANG0      0x06

#define AS5600_CONF       0x07
// #define AS5600_SF1        0b1     // Slow Filter:
// #define AS5600_SF0        0b0     // 00 = 16x (1); 01 = 8x; 10 = 4x; 11 = 2x
#define AS5600_SF         0b00000011     // 00 = 16x (1); 01 = 8x; 10 = 4x; 11 = 2x
// #define AS5600_FTH1       0b11    // Fast Filter Threshold:
// #define AS5600_FTH0       0b10    // 000 = slow filter only, 001 = 6 LSBs, 010 = 7 LSBs, 011 = 9 LSBs,100 = 18 LSBs, 101 = 21 LSBs, 110 = 24 LSBs, 111 = 10 LSBs
#define AS5600_FTH        0b00011100    // 000 = slow filter only, 001 = 6 LSBs, 010 = 7 LSBs, 011 = 9 LSBs,100 = 18 LSBs, 101 = 21 LSBs, 110 = 24 LSBs, 111 = 10 LSBs
// #define AS5600_WD         0b100   // Watchdog: 0 = OFF, 1 = ON
#define AS5600_WD         0b00100000   // Watchdog: 0 = OFF, 1 = ON

// #define AS5600_CONF0      0x08
#define AS5600_PM         0b00000011     // Power Mode:
// #define AS5600_PM1        0b1     // Power Mode:
// #define AS5600_PM0        0b0     // 00 = NOM, 01 = LPM1, 10 = LPM2, 11 = LPM3
#define AS5600_HYST       0b00001100    // Hysteresis:
// #define AS5600_HYST1      0b11    // Hysteresis:
// #define AS5600_HYST0      0b10    // 00 = OFF, 01 = 1 LSB, 10 = 2 LSBs, 11 = 3 LSBs
#define AS5600_OUTS       0b00110000   // Output Stage: 00 = analog (full range from 0% to 100% between GND and VDD, 01 = analog)
// #define AS5600_OUTS1      0b101   // Output Stage: 00 = analog (full range from 0% to 100% between GND and VDD, 01 = analog)
// #define AS5600_OUTS0      0b100   // (reduced range from 10% to 90% between GND and VDD, 10 = digital PWM)
#define AS5600_PWMF       0b11000000   // PWM Frequency:
// #define AS5600_PWMF1      0b111   // PWM Frequency:
// #define AS5600_PWMF0      0b110   // 00 = 115 Hz; 01 = 230 Hz; 10 = 460 Hz; 11 = 920 Hz

// Output Registers
#define AS5600_RAW_ANGLE  0x0C
// #define AS5600_RAW_ANGLE0 0x0D
#define AS5600_ANGLE      0x0E
// #define AS5600_ANGLE0     0x0F

// Status Registers
#define AS5600_STATUS     0x0B
#define AS5600_MH         0b1000    // AGC minimum gain overflow, magnet too strong
#define AS5600_ML         0b10000   // AGC maximum gain overflow, magnet too weak
#define AS5600_MD         0b100000  // Magnet was detected

#define AS5600_AGC        0x1A
#define AS5600_MAGNITUDE  0x1B
// #define AS5600_MAGNITUDE0 0x1C

// Burn Commands
#define AS5600_BURN         0xff // Register
#define AS5600_BURN_ANGLE   0x80 // Data to send after the register
#define AS5600_BURN_SETTING 0x40 // -- same --

#define AS5600_SLA_ADDRESS  0x36

typedef struct {
    uint8_t WD;
    uint8_t FTH;
    uint8_t SF;

    uint8_t PWMF;
    uint8_t OUTS;
    uint8_t HYST;
    uint8_t PM;
} AS5600_conf;

int AS5600_read_16b_register(uint8_t reg, uint16_t *data);
int AS5600_write_16b_register(uint8_t reg, uint16_t *data);
int AS5600_read_8b_register(uint8_t reg, uint8_t *data);
int AS5600_read_conf(AS5600_conf *ASConf);


# ifdef LIBAS5600_IMPLEMENTATION


// This function is real way to read a 2 bytes register from the AS5600
// int AS5600_read_16b_register_test(uint8_t reg, uint16_t *data)
// {
//     int res = 0;
//     uint8_t data8b[2] = {0};

// // START
//     res = TWI_start();
//     if (res != 0) return res;
// // SLA+W
//     res = TWI_sla_w(AS5600_SLA_ADDRESS);
//     if (res != 0) return res;
// // octet ANGLE_H
//     res = TWI_data_w(reg);
//     if (res != 0) return res;

// // RESTART
//     res = TWI_start();
//     if (res != 0) return res;
// // SLA+R
//     res = TWI_sla_r(AS5600_SLA_ADDRESS);
//     if (res != 0) return res;
// // octet ANGLE_H
//     res = TWI_data_r(NACK, &data8b[0]);
//     if (res != 0) return res;

// // RESTART
//     res = TWI_start();
//     if (res != 0) return res;
// // SLA+W
//     res = TWI_sla_w(AS5600_SLA_ADDRESS);
//     if (res != 0) return res;
// // octet ANGLE_L
//     res = TWI_data_w(reg+1);
//     if (res != 0) return res;

// // RESTART
//     res = TWI_start();
//     if (res != 0) return res;
// // SLA+R
//     res = TWI_sla_r(AS5600_SLA_ADDRESS);
//     if (res != 0) return res;
// // octet ANGLE_L
//     res = TWI_data_r(NACK, &data8b[1]);
//     if (res != 0) return res;

// // STOP
//     TWI_stop();

//     *data = ((uint16_t)data8b[0] << 8) | data8b[1];
//     return 0;
// }

/*
 * Against all odds, the RAW ANGLE, ANGLE and MAGNITUDE register from the AS5600 get:
 *   Automatic Increment of the Address Pointer for ANGLE,
 *   RAW ANGLE and MAGNITUDE Registers
 */
int AS5600_read_16b_register(uint8_t reg, uint16_t *data)
{
    int res = 0;
    uint8_t data8b[2] = {0};

// START
    res = TWI_start();
    if (res != 0) return res;

// SLA+W
    res = TWI_sla_w(AS5600_SLA_ADDRESS);
    if (res != 0) return res;

// octet reg
    res = TWI_data_w(reg);
    if (res != 0) return res;

// RESTART
    res = TWI_start();
    if (res != 0) return res;

// SLA+R
    res = TWI_sla_r(AS5600_SLA_ADDRESS);
    if (res != 0) return res;

// octet data_H
    res = TWI_data_r(ACK, &data8b[0]);
    if (res != 0) return res;

// octet data_L
    res = TWI_data_r(NACK, &data8b[1]);
    if (res != 0) return res;

    TWI_stop();

    *data = ((uint16_t)data8b[0] << 8) | data8b[1];
    return 0;
}

int AS5600_write_16b_register(uint8_t reg, uint16_t *data)
{
    int res = 0;
    uint8_t data8b[2] = {(uint8_t)(*data >> 8), (uint8_t)(*data & 0x00ff)};

// START
    res = TWI_start();
    if (res != 0) return res;

// SLA+W
    res = TWI_sla_w(AS5600_SLA_ADDRESS);
    if (res != 0) return res;

// octet reg
    res = TWI_data_w(reg);
    if (res != 0) return res;

// octet data_H
    res = TWI_data_w(data8b[0]);
    if (res != 0) return res;

// RESTART
    res = TWI_start();
    if (res != 0) return res;

// SLA+W
    res = TWI_sla_w(AS5600_SLA_ADDRESS);
    if (res != 0) return res;

// octet reg
    res = TWI_data_w(reg+1);
    if (res != 0) return res;

// octet data_L
    res = TWI_data_w(data8b[1]);
    if (res != 0) return res;

    TWI_stop();

    return 0;
}

int AS5600_read_8b_register(uint8_t reg, uint8_t *data)
{
    int res = 0;

// START
    res = TWI_start();
    if (res != 0) return res;

// SLA+W
    res = TWI_sla_w(AS5600_SLA_ADDRESS);
    if (res != 0) return res;

// octet reg
    res = TWI_data_w(reg);
    if (res != 0) return res;

// RESTART
    res = TWI_start();
    if (res != 0) return res;

// SLA+R
    res = TWI_sla_r(AS5600_SLA_ADDRESS);
    if (res != 0) return res;

// octet data
    res = TWI_data_r(NACK, data);
    if (res != 0) return res;

    TWI_stop();

    return 0;
}

int AS5600_read_conf(AS5600_conf *ASConf)
{
    int res = 0;
    uint16_t conf;

    res = AS5600_read_16b_register(AS5600_CONF, &conf);
    if (res != 0) return res;

    ASConf->WD  = (uint8_t)(conf >> 8) & AS5600_WD;
    ASConf->FTH = (uint8_t)(conf >> 8) & AS5600_FTH;
    ASConf->SF  = (uint8_t)(conf >> 8) & AS5600_WD;

    ASConf->PWMF = (uint8_t)(conf & 0x0f) & AS5600_PWMF;
    ASConf->OUTS = (uint8_t)(conf & 0x0f) & AS5600_OUTS;
    ASConf->HYST = (uint8_t)(conf & 0x0f) & AS5600_HYST;
    ASConf->PM   = (uint8_t)(conf & 0x0f) & AS5600_PM;
    return 0;
}

static char *AS5600_DETECT[] = {
    [TW_BUS_ERROR]                       = "ERROR: 0b000",
    [AS5600_MH>>3]                       = "Magnet not detected too strong",
    [AS5600_ML>>3]                       = "Magnet not detected too weak",
    [(AS5600_MH|AS5600_ML)>>3]           = "ERROR: 0b011",
    [AS5600_MD>>3]                       = "Magnet detected",
    [(AS5600_MD|AS5600_MH)>>3]           = "Magnet too strong",
    [(AS5600_MD|AS5600_ML)>>3]           = "Magnet too weak",
    [(AS5600_MD|AS5600_MH|AS5600_ML)>>3] = "ERROR: 0b111",
};



// int i2c_AS5600_init(AS5600_conf *ASConf)
// {
//     int res = 0;
//     uint8_t status = 0;
//     uint16_t conf = 0;

//     res = AS5600_read_8b_register(AS5600_STATUS, &status);
//     if (res != 0) return res;

    
// }

# endif // LIBAS5600_IMPLEMENTATION
#endif // LIBAS5600_H_INCLUDED
