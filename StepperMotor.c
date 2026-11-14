#define F_CPU 16000000
#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define TWI_DEBUG
#define PRINTF_IMPLEMENTATION
#include "printf.h"

#define LIBTWI_IMPLEMENTATION
#include "libtwi.h"

#define LIBAS5600_IMPLEMENTATION
#include "libAS5600.h"

#define ABS(x) ((x) < 0 ? (-x) : (x))
#define MIN_MICRO_STEP 200
volatile uint8_t MICRO_STEP = 32;
#define STEP_MOTOR (MICRO_STEP*MIN_MICRO_STEP)

void TWI_Init()
{
    // SCL_freq = CPU_clk_freq / (16 + 2*(TWBR) * PrescalerValue)
    // PrescalerValue = 1 => TWSR = 0
    // TWBR = 12; // 400kHz
    TWBR = 72; // 100kHz
}

/* CTC timer mode is used. To move forward the goal is to
 * decrease the OCR1A register. To move backward just do the
 * opposite.
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
 */
    DDRB   = _BV(DDB1);
    TCCR1A = _BV(COM1A0);
    TCCR1B = _BV(WGM12) | _BV(CS11);
    OCR1A  = 349;
}



void PORT_init()
{
    DDRD =  _BV(DDD7) | _BV(DDD6); // DDD7 (DIR) DDD6 (ENA)
    DDRB =  _BV(DDB5);             // DDB5 (LED)
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
 * A0: Connected to PC3
 * A1: Connected to PC1
 * A2: Connected to PC0
 * A3: Connected to PC2
 *
 * _________________________________________________________
 * | Num | PINC2 (A3) | PINC1 (A1) | PINC0 (A2) ||   OCR1A |
 * |-----|------------|------------|------------||---------|
 * |   0 |          0 |          0 |          0 ||   0x400 |
 * |   1 |          0 |          0 |          1 ||     700 |
 * |   2 |          0 |          1 |          1 ||     350 |
 * |   3 |          0 |          1 |          0 ||    ~233 |
 * |   4 |          1 |          1 |          0 ||     175 |
 * |   5 |          1 |          1 |          1 ||     140 |
 * |   6 |          1 |          0 |          1 ||    ~116 |
 * |   7 |          1 |          0 |          0 ||     100 |
 * ---------------------------------------------------------
 */
void knob() // other name for Rotary-Encoder
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

void do_blind_rotation(uint16_t Prev, float angle, float speed)
{
    if (speed < 0.f) speed = 0.f;
    if (speed > 1.002f) speed = 1.002f;

    uint8_t buf_TCCR1A = TCCR1A;
    uint8_t buf_TCCR1B = TCCR1B;
    uint16_t buf_OCR1A = OCR1A;

    PORTD &= ~_BV(PORTD6);
    if (angle < 0) {PORTD |=  _BV(PORTD7); angle = -angle;}
    else            PORTD &= ~_BV(PORTD7);
    TCCR1A = _BV(COM1A0);
    TCCR1B = _BV(WGM12) | _BV(CS10);
    TCNT1 = 0;
    OCR1A = 401 + (1.f-speed)*0xffff;

    for (int i = 0; i < Prev/(360.f/angle); i++) {
	loop_until_bit_is_set(PINB, PINB1);
	loop_until_bit_is_clear(PINB, PINB1);
    }

    PORTD |= _BV(PORTD6);
    TCCR1A = buf_TCCR1A;
    TCCR1B = buf_TCCR1B;
    OCR1A  = buf_OCR1A;
}

void goto_pos(uint16_t Prev, float desire_360_angle, float speed)
{
    if (speed < 0.f) speed = 0.f;
    if (speed > 1.002f) speed = 1.002f;
    /***** future to get the the choice by a parameter *****/
    // if (_360_angle < 0) {PORTD |=  _BV(PORTD7); _360_angle = -_360_angle;}
    // else                PORTD &= ~_BV(PORTD7);
    if (desire_360_angle < 0) {desire_360_angle = -desire_360_angle;}

    uint16_t mes_12b_angle = 0;
    AS5600_read_16b_register(AS5600_ANGLE, &mes_12b_angle);
    uint16_t desire_12b_angle = (desire_360_angle/360.f)*4096.f;

    uint16_t mes_12b_angle_wider = mes_12b_angle;
    uint16_t desire_12b_angle_wider = desire_12b_angle;

    if (mes_12b_angle < 0x1000/2) mes_12b_angle_wider = mes_12b_angle + 0x1000;
    else                          desire_12b_angle_wider = desire_12b_angle + 0x1000;

    int target = mes_12b_angle - desire_12b_angle;
    int tg_buf = mes_12b_angle_wider - desire_12b_angle_wider;

    if (target == 0) return ;
    else if (ABS(target) > ABS(tg_buf)) target = mes_12b_angle_wider - desire_12b_angle_wider;

    if (target < 0) {PORTD &= ~_BV(PORTD7); target = ABS(target);}
    else             PORTD |=  _BV(PORTD7);

    uint8_t buf_TCCR1A = TCCR1A;
    uint8_t buf_TCCR1B = TCCR1B;
    uint16_t buf_OCR1A = OCR1A;

    PORTD &= ~_BV(PORTD6);
    TCCR1A = _BV(COM1A0);
    TCCR1B = _BV(WGM12) | _BV(CS10);
    TCNT1 = 0;
    OCR1A = 401 + (1.f-speed)*0xffff;

    for (int i = 0; i < ((float)target/(float)(1<<12))*Prev; i++) {
	loop_until_bit_is_set(PINB, PINB1);
	loop_until_bit_is_clear(PINB, PINB1);
    }

    PORTD |= _BV(PORTD6);
    TCCR1A = buf_TCCR1A;
    TCCR1B = buf_TCCR1B;
    OCR1A  = buf_OCR1A;
}



void goto_zero_pos()
{
    uint8_t buf_TCCR1A = TCCR1A;
    uint8_t buf_TCCR1B = TCCR1B;
    uint16_t buf_OCR1A = OCR1A;

    PORTD &= ~_BV(PORTD6); // ENA the motor
    PORTD |=  _BV(PORTD7); // Bug in the opposite direction
    TCCR1A = _BV(COM1A0);
    TCCR1B = _BV(WGM12) | _BV(CS10);
    TCNT1 = 0;
    OCR1A = 1000;

    uint16_t zero = 0;
    do {
	AS5600_read_16b_register(AS5600_ANGLE, &zero);
	// if (zero < 11 || zero > 4096) printf("Pos: %u\n", zero);
    } while (zero > 0);

    PORTD |= _BV(PORTD6);
    PORTD &=  ~_BV(PORTD7);
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

    sei();

    while (1) {
	knob();
	PORTB ^= (1 << PORTB5);
	_delay_ms(50);
    }
}

/* interuption de reception d'un caractere*/
ISR (USART_RX_vect)
{
    /* Returne le recept */
    char recept = UDR0;
    // static char buf[4] = {0};
    // static uint8_t index = 0;
    // if (recept != 13' && index < 3) {
    // 	buf[index++] = recept;
    // } else if (recept != 13 && index >= 3) {
    // 	printf("ERROR: Invalid command\n");
    // }

    printf("Command: \'%c\'\n", recept);
    uint16_t angle = 0;

    switch (recept)
    {
	case '0':
	    goto_zero_pos();
	    printf("Position set to 0\n");
	    break;
	case 'c':
	    AS5600_calibrate();
	    printf("Calibration done\n");
	    break;
	case '1':
	    goto_pos(STEP_MOTOR, 90, 1.f);
	    // do_blind_rotation(STEP_MOTOR, 90, 1);
	    break;
	case '2':
	    goto_pos(STEP_MOTOR, 180, 1.f);
	    // do_blind_rotation(STEP_MOTOR, 180, 1);
	    break;
	case '3':
	    goto_pos(STEP_MOTOR, 90*3, 1.f);
	    // do_blind_rotation(STEP_MOTOR, -90, 1);
	    break;
	case '4':
	    goto_pos(STEP_MOTOR, 0, 1.f);
	    // do_blind_rotation(STEP_MOTOR, -6*360, 1);
	    break;
	case '5':
	    // for (int i = 0; i < 100; i++) {
		do_blind_rotation(STEP_MOTOR, 360, 0.99f);
		// printf("Tour: %d\n", i);
	    // }
	    break;
	case '6':
	    // for (int i = 0; i < 100; i++) {
		do_blind_rotation(STEP_MOTOR, -360, 0.99f);
		// printf("Tour: %d\n", i);
	    // }
	    break;
	case '9':
	    for (int i = 0; i < 5; i++)
	        goto_pos(STEP_MOTOR, 72*i, 0.95f);
	    break;
	case 'm':
	    AS5600_read_16b_register(AS5600_ANGLE, &angle);
	    printf("Position: %u\n", angle);
	    break;
	default:
	    break;
    }
}
