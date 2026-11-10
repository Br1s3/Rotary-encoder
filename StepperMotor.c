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
void TIMER1_Init()
{
/* 20 kHz max for 25% Duty cycle or 13 for 50% duty cycle
 * 20 000 / 7 = 2 857 Hz
 * 
 * Fast CTC
 * f_OC1A    = f_clk / (2 * N * (1 + OCR1A))
 * OCR1A     = 1 + f_clk / (2 * N * f_OC1x)
 * f_clk     = 16 000 000 Hz
 * f_OC0x    =      2 857 Hz
 * N         =          8
 * __________________________________________________
 * | Num | PINC2 | PINC1 | PINC0 ||      OCR1A      |
 * |-----|-------|-------|-------||-----------------|
 * |   0 |     0 |     0 |     0 ||      0x400      |
 * |   1 |     0 |     0 |     1 ||        700      |
 * |   2 |     0 |     1 |     1 ||        350      |
 * |   3 |     0 |     1 |     0 ||       ~233      |
 * |   4 |     1 |     1 |     0 ||        175      |
 * |   5 |     1 |     1 |     1 ||        140      |
 * |   6 |     1 |     0 |     1 ||       ~116      |
 * |   7 |     1 |     0 |     0 ||        100      |
 * --------------------------------------------------
 */
    DDRB   = _BV(DDB1);
    TCCR1A = _BV(COM1A0);
    TCCR1B = _BV(WGM12) | _BV(CS11);
    OCR1A  = 349;
}



void PORT_init()
{
    DDRD =  _BV(DDD7) | _BV(DDD6); // DDD7 (DIR) DDD6 (ENA)
    DDRB =  _BV(DDB5); // DDB5 (LED)
    DDRC = ~_BV(DDC0) | ~_BV(DDC1) | ~_BV(DDC2) | ~_BV(DDC3);
}

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
    if (bit_is_set(PINC, PINC3) != 0) PORTD |=  _BV(PORTD7);
    else                              PORTD &= ~_BV(PORTD7);

    float ratio = (float)val/7.f;
    uint16_t pwm_val = (uint16_t)((1.f+1.f/7.f - ratio)*349);
    
    // printf("val: %d, ratio: %.2f, PWM: %u\n", val, ratio, pwm_val);

    OCR1A = pwm_val;

    if (val == 0) {TCCR1A |=  _BV(COM1A1); PORTD |=  _BV(PORTD6);}
    else          {TCCR1A &= ~_BV(COM1A1); PORTD &= ~_BV(PORTD6);}

    // printf("OCR1A: 0x%02x\n", OCR1A);
}

void do_rotation(uint16_t Prev, uint16_t angle, float speed)
{
    if (speed < 0.f) speed = 0.f;
    if (speed > 1.002f) speed = 1.002f;
    
    uint8_t buf_TCCR1A = TCCR1A;
    uint8_t buf_TCCR1B = TCCR1B;
    uint16_t buf_OCR1A = OCR1A;
    
    PORTD &= ~_BV(PORTD6);
    TCCR1A = _BV(COM1A0);
    TCCR1B = _BV(WGM12) | _BV(CS10);
    TCNT1 = 0;
    // OCR1A = 2500;
    // OCR1A = 401; // 20 KHz
    OCR1A = 401 + (1.f-speed)*0xffff;
    // OCR1A = 0xffff;
    // OCR1A = 200;

    for (int i = 0; i < Prev/(360/angle); i++) {
	loop_until_bit_is_set(PINB, PINB1);
	loop_until_bit_is_clear(PINB, PINB1);
    }

    PORTD |= _BV(PORTD6);
    TCCR1A = buf_TCCR1A;
    TCCR1B = buf_TCCR1B;
    OCR1A = buf_OCR1A;
}

void AS5600_calibrate()
{
    uint16_t Zpos = 0;
    AS5600_read_16b_register(AS5600_RAW_ANGLE, &Zpos);
    AS5600_write_16b_register(AS5600_ZPOS, &Zpos);
}

int main()
{
    USART_Init(3);
    TWI_Init();
    PORT_init();
    TIMER1_Init();

    AS5600_calibrate();
    // do_rotation(6400, 360, 1.f);
    do_rotation(200*32, 360, 0.8f);
    
    while (1) {
	uint16_t magnitude = 0;
	uint16_t angle = 0;
	uint8_t status = 0;
	uint8_t agc = 0;
	int res = AS5600_read_16b_register(AS5600_MAGNITUDE, &magnitude);
	res = AS5600_read_16b_register(AS5600_ANGLE, &angle);
	res = AS5600_read_8b_register(AS5600_STATUS, &status);
	res = AS5600_read_8b_register(AS5600_AGC, &agc);

	printf("status: %s, %03u, Magnitude: %u, angle: %u\n", AS5600_DETECT[(status & 0b00111000)>>3], agc, magnitude, angle);
// #ifdef TWI_DEBUG
// 	if (res != 0) printf("ERROR: 0x%02x: %s\n", res, twierror(res));
// #endif // TWI_DEBUG

	Rotary_Encoder();
	PORTB ^= (1 << PORTB5);
	_delay_ms(50);
    }
}
