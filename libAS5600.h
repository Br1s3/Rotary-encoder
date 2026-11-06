#ifndef LIBAS5600_H_INCLUDED
#define LIBAS5600_H_INCLUDED

#define LIBTWI_IMPLEMENTATION
#include "libtwi.h"

// Configuration Registers
#define AS5600_ZMCO       0x00
#define AS5600_ZPOS1      0x01
#define AS5600_ZPOS0      0x02
#define AS5600_MPOS1      0x03
#define AS5600_MPOS0      0x04
#define AS5600_MANG1      0x05
#define AS5600_MANG0      0x06

#define AS5600_CONF1      0x07
#define AS5600_SF1        0b1     // Slow Filter:
#define AS5600_SF0        0b0     // 00 = 16x (1); 01 = 8x; 10 = 4x; 11 = 2x
#define AS5600_FTH1       0b11    // Fast Filter Threshold:
#define AS5600_FTH0       0b10    // 000 = slow filter only, 001 = 6 LSBs, 010 = 7 LSBs, 011 = 9 LSBs,100 = 18 LSBs, 101 = 21 LSBs, 110 = 24 LSBs, 111 = 10 LSBs
#define AS5600_WD         0b100   // Watchdog: 0 = OFF, 1 = ON

#define AS5600_CONF0      0x08
#define AS5600_PM1        0b1     // Power Mode:
#define AS5600_PM0        0b0     // 00 = NOM, 01 = LPM1, 10 = LPM2, 11 = LPM3
#define AS5600_HYST1      0b11    // Hysteresis:
#define AS5600_HYST0      0b10    // 00 = OFF, 01 = 1 LSB, 10 = 2 LSBs, 11 = 3 LSBs
#define AS5600_OUTS1      0b101   // Output Stage: 00 = analog (full range from 0% to 100% between GND and VDD, 01 = analog)
#define AS5600_OUTS0      0b100   // (reduced range from 10% to 90% between GND and VDD, 10 = digital PWM)
#define AS5600_PWMF1      0b111   // PWM Frequency:
#define AS5600_PWMF0      0b110   // 00 = 115 Hz; 01 = 230 Hz; 10 = 460 Hz; 11 = 920 Hz

// Output Registers
#define AS5600_RAW_ANGLE1 0x0C
#define AS5600_RAW_ANGLE0 0x0D
#define AS5600_ANGLE1     0x0E
#define AS5600_ANGLE0     0x0F

// Status Registers
#define AS5600_STATUS     0x0B
#define AS5600_MH         0b1000    // AGC minimum gain overflow, magnet too strong
#define AS5600_ML         0b10000   // AGC maximum gain overflow, magnet too weak
#define AS5600_MD         0b100000  // Magnet was detected

#define AS5600_AGC        0x1A
#define AS5600_MAGNITUDE1 0x1B
#define AS5600_MAGNITUDE0 0x1C

// Burn Commands
#define AS5600_BURN         0xff // Register
#define AS5600_BURN_ANGLE   0x80 // Data to send after the register
#define AS5600_BURN_SETTING 0x40 // -- same --

#define AS5600_SLA_ADDRESS  0x36

int AS5600_read_angle(uint16_t *angle);

# ifdef LIBAS5600_IMPLEMENTATION


// This function is real way to read a 2 bytes register from the AS5600
int AS5600_read_16B_register_test(uint8_t reg, uint16_t *data)
{
    int res = 0;
    uint8_t data8b[2] = {0};

// START
    res = TWI_start();
    if (res != 0) return res;
// SLA+W
    res = TWI_sla_w(AS5600_SLA_ADDRESS);
    if (res != 0) return res;
// octet ANGLE_H
    res = TWI_data_w(reg);
    if (res != 0) return res;

// RESTART
    res = TWI_start();
    if (res != 0) return res;
// SLA+R
    res = TWI_sla_r(AS5600_SLA_ADDRESS);
    if (res != 0) return res;
// octet ANGLE_H
    res = TWI_data_r(NACK, &data8b[0]);
    if (res != 0) return res;

// RESTART
    res = TWI_start();
    if (res != 0) return res;
// SLA+W
    res = TWI_sla_w(AS5600_SLA_ADDRESS);
    if (res != 0) return res;
// octet ANGLE_L
    res = TWI_data_w(reg+1);
    if (res != 0) return res;

// RESTART
    res = TWI_start();
    if (res != 0) return res;
// SLA+R
    res = TWI_sla_r(AS5600_SLA_ADDRESS);
    if (res != 0) return res;
// octet ANGLE_L
    res = TWI_data_r(NACK, &data8b[1]);
    if (res != 0) return res;

// STOP
    TWI_stop();

    *data = ((uint16_t)data8b[0] << 8) | data8b[1];
    return 0;
}

/*
 * Against all odds, the RAW ANGLE, ANGLE and MAGNITUDE register from the AS5600 get:
 *   Automatic Increment of the Address Pointer for ANGLE,
 *   RAW ANGLE and MAGNITUDE Registers
 */
int AS5600_read_16B_register(uint8_t reg, uint16_t *data)
{
    int res = 0;
    uint8_t data8b[2] = {0};

// START
    res = TWI_start();
    if (res != 0) return res;
    // printf("START: 0x%02x\n", res);

// SLA+W
    res = TWI_sla_w(AS5600_SLA_ADDRESS);
    if (res != 0) return res;
    // printf("SLA+W: 0x%02x\n", res);

// octet ANGLE_H
    res = TWI_data_w(reg);
    if (res != 0) return res;
    // printf("DATA WRITE: 0x%02x\n", res);

// RESTART
    res = TWI_start();
    if (res != 0) return res;
    // printf("RESTART: 0x%02x\n", res);

// SLA+R
    res = TWI_sla_r(AS5600_SLA_ADDRESS);
    if (res != 0) return res;
    // printf("SLA+R: 0x%02x\n", res);

// octet ANGLE_H
    res = TWI_data_r(ACK, &data8b[0]);
    if (res != 0) return res;
    // printf("DATA READ: 0x%02x\n", res);

// octet ANGLE_L
    res = TWI_data_r(NACK, &data8b[1]);
    if (res != 0) return res;
    // printf("DATA READ: 0x%02x\n", res);

    TWI_stop();

    *data = ((uint16_t)data8b[0] << 8) | data8b[1];
    return 0;
}


// int i2c_AS5600_init()
// {
//     TWI_start();

//     if (TWI_sla_w(AS5600_ADDRESS) != 0) return 1;

//     for (int i = 0; i < )

//     TWI_stop();
// }

# endif // LIBAS5600_IMPLEMENTATION
#endif // LIBAS5600_H_INCLUDED
