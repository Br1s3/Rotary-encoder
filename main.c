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

/***************************************************/
/* ENA | IN1 | IN2 || OUT1 | OUT2 | Etat du moteur */
/*  0  |  X  |  X  ||  XV  |  XV  | roue libre     */
/*  1  |  0  |  0  ||  0V  |  0V  | ARRET          */
/*  1  |  0  |  1  ||  0V  | 12V  | arriere        */
/*  1  |  1  |  0  || 12V  |  0V  | avant          */
/*  1  |  1  |  1  || 12V  | 12V  | ARRET          */
/***************************************************/

/***************************************************/
/* ENB | IN1 | IN2 || OUT1 | OUT2 | Etat du moteur */
/*  0  |  X  |  X  ||  X   |  X   | roue libre     */
/*  1  |  0  |  0  ||  0V  |  0V  | ARRET          */
/*  1  |  0  |  1  ||  0V  | 12V  | gauche         */
/*  1  |  1  |  0  || 12V  |  0V  | droit          */
/*  1  |  1  |  1  || 12V  | 12V  | ARRET          */
/***************************************************/


void TWI_Init()
{
    // SCL_freq = CPU_clk_freq / (16 + 2*(TWBR) * PrescalerValue)
    // PrescalerValue = 1 => TWSR = 0
    // TWBR = 12; 400kHz
    TWBR = 72; // 100kHz
}

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

int read_the_3first_data()
{
    int res = 0;
    res = TWI_start();
    if (res != 0) return res;

    res = TWI_sla_r(AS5600_SLA_ADDRESS);
    if (res != 0) return res;

    uint8_t dt[3];
    res = TWI_data_r(ACK, &dt[0]);
    if (res != 0) return res;
    printf("res[0]: 0x%02x\n", dt[0]);

    res = TWI_data_r(ACK, &dt[1]);
    if (res != 0) return res;
    printf("res[1]: 0x%02x\n", dt[1]);

    res = TWI_data_r(NACK, &dt[2]);
    if (res != 0) return res;
    printf("res[2]: 0x%02x\n", dt[2]);

    TWI_stop();
    return 0;
}


int main()
{
    USART_Init(3);
    TWI_Init();
    PORT_init();
    PWM0_Init();


    // printf("result: %d\n", read_the_3first_data());

    while (1) {
	uint16_t angle = 0;
	int res = AS5600_read_16B_register(AS5600_MAGNITUDE1, &angle);
	if (res != 0) printf("ERROR: 0x%02x: %s\n", res, twierror(res));
	printf("angle: 0x%02x, 0x%02x: %u\n", (uint8_t)(angle >> 8), (uint8_t)(angle & 0x0f), angle);

	// Rotary_Encoder();
	PORTB ^= (1 << PORTB5);
	_delay_ms(100);
    }
}
