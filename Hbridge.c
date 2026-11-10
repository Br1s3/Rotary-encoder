#define F_CPU 16000000
#include <util/delay.h>
#include <avr/io.h>
// #include <avr/interrupt.h>

#define TWI_DEBUG
#define PRINTF_IMPLEMENTATION
#include "printf.h"

#define LIBTWI_IMPLEMENTATION
#include "libtwi.h"

#define LIBAS5600_IMPLEMENTATION
#include "libAS5600.h"



void TWI_Init()
{
    // SCL_freq = CPU_clk_freq / (16 + 2*(TWBR) * PrescalerValue)
    // PrescalerValue = 1 => TWSR = 0
    // TWBR = 12; 400kHz
    TWBR = 72; // 100kHz
}

/* 2 PWM with a duty cycle of 50% are generated
 * To move forward the goal is to increase the first PWM and
 * decrease the second.
 * To move backward just do the opposite.
 */
void PWM0_Init()
{
    DDRD   = _BV(DDD6)   | _BV(DDD5);
    TCCR0A = _BV(COM0A1) | _BV(COM0B1) | _BV(WGM01) | _BV(WGM00);
    TCCR0B = _BV(CS01)   | _BV(CS00);
    OCR0A = 0xff/2;
    OCR0B = 0xff/2;
}

void PORT_init()
{
    DDRB = (1 << DDB5);
    DDRC = ~_BV(DDC0) | ~_BV(DDC1) | ~_BV(DDC2) | ~_BV(DDC3);
}

/* L298N DUAL FULL-BRIDGE DRIVER */
/************************************************/
/* ENA | IN1 | IN2 || OUT1 | OUT2 | Motor State */
/*  0  |  X  |  X  ||  Xv  |  Xv  | freewheel   */
/*  1  |  0  |  0  ||  0v  |  0v  | STOP        */
/*  1  |  0  |  1  ||  0v  | 12v  | backward    */
/*  1  |  1  |  0  || 12v  |  0v  | forward     */
/*  1  |  1  |  1  || 12v  | 12v  | STOP        */
/************************************************/

/************************************************/
/* ENB | IN1 | IN2 || OUT1 | OUT2 | Motor State */
/*  0  |  X  |  X  ||  Xv  |  Xv  | freewheel   */
/*  1  |  0  |  0  ||  0v  |  0v  | STOP        */
/*  1  |  0  |  1  ||  0v  | 12v  | left        */
/*  1  |  1  |  0  || 12v  |  0v  | right       */
/*  1  |  1  |  1  || 12v  | 12v  | STOP        */
/************************************************/

/* Rotary switch:
 * Inner Rotary Switch -> IRS
 * Outer Rotary Switch -> ORS
 * Not Connected -> NC
 * _____________________________________________________
 * |  1.R  | 2.B |  3.B  | 4.B | 5.B | 6.B | 7.B | 8.B |
 * |-------|-----|-------|-----|-----|-----|-----|-----|
 * |  GND  | IRS |  Vcc  | IRS | ORS | ORS | ORS | ORS |
 * |       |  B1 |       |  B0 |  A3 |  A2 |  A1 |  A0 |
 * -----------------------------------------------------
 * 
 * B0: NC
 * B1: NC
 * A0: Connected to PC3
 * This pin determine the direction of the rotation,
 * clockwise = 0, counterclockwise = 1
 * A1: Connected to PC2
 * A2: Connected to PC0
 * A3: Connected to PC1
 * In the order A1 -> A3 -> A2 is the Gray code of the ORS
 */
void Rotary_Encoder()
{
    int8_t val = (bit_is_set(PINC, PINC0) != 0) << 0 | (bit_is_set(PINC, PINC1) != 0) << 1 | (bit_is_set(PINC, PINC2) != 0) << 2;

    val ^= val >> 2;
    val ^= val >> 1;
    val = bit_is_set(PINC, PINC3) != 0 ? -val : val;

    float ratio = (float)(val/7.f);
    uint8_t pwmP_val = (1.f + ratio)*127.5f;
    uint8_t pwmN_val = (1.f - ratio)*127.5f;

    printf("val: %d, ratio: %.2f, PWMP: %u, PWMN: %u\n", val, ratio, pwmP_val, pwmN_val);

    if (pwmP_val == 0) TCCR0A &= ~_BV(COM0A1);
    else               TCCR0A |= _BV(COM0A1);
    if (pwmN_val == 0) TCCR0A &= ~_BV(COM0B1);
    else               TCCR0A |= _BV(COM0B1);

    OCR0A = pwmP_val;
    OCR0B = pwmN_val;
}

int main()
{
    USART_Init(3);
    TWI_Init();
    PORT_init();
    PWM0_Init();

    AS5600_conf ASConf = {0};
    int res = i2c_AS5600_read_conf(&ASConf);
#ifdef TWI_DEBUG
	if (res != 0) printf("ERROR: 0x%02x: %s\n", res, twierror(res));
#endif // TWI_DEBUG

    printf("WD: 0x%02x\nFTH: 0x%02x\nSF: 0x%02x\n", ASConf.WD, ASConf.FTH, ASConf.SF);
    printf("PWMF: 0x%02x\nOUTS: 0x%02x\nHYST: 0x%02x\nPM: 0x%02x\n", ASConf.PWMF, ASConf.OUTS, ASConf.HYST, ASConf.PM);

    while (1) {
// 	uint16_t angle = 0;
// 	int res = AS5600_read_16B_register(AS5600_MAGNITUDE1, &angle);
// #ifdef TWI_DEBUG
// 	if (res != 0) printf("ERROR: 0x%02x: %s\n", res, twierror(res));
// #endif // TWI_DEBUG
// 	printf("angle: 0x%02x, 0x%02x: %u\n", (uint8_t)(angle >> 8), (uint8_t)(angle & 0x0f), angle);

	Rotary_Encoder();
	PORTB ^= (1 << PORTB5);
	_delay_ms(100);
    }
}
