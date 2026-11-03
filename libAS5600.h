#define LIBAS5600_H_INCLUDED
#define LIBAS5600_H_INCLUDED

#define LIBAS5600_IMPLEMENTATION
#include "libAS5600.h"

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
#define AS5600_RAW_ANGLE  0x0C
#define AS5600_RAW_ANGLE  0x0D
#define AS5600_ANGLE      0x0E
#define AS5600_ANGLE      0x0F

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

#define AS5600_ADDRESS 0x36

int i2c_AS5600_init();

# ifdef LIBAS5600_IMPLEMENTATION

int i2c_AS5600_init()
{
    TWI_start();

    if (TWI_SLA_W(AS5600_ADDRESS) != 0) return 1;

    for (int i = 0; i < )

    TWI_stop();
}


# endif // LIBAS5600_IMPLEMENTATION
#endif // LIBAS5600_H_INCLUDED






