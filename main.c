#define F_CPU 16000000
#include <util/delay.h>
#include <avr/io.h>
// #include <avr/interrupt.h>

#define PRINTF_IMPLEMENTATION
#include "printf.h"

#define LIBTWI_IMPLEMENTATION
#include "libtwi.h"

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


// AS5600 I2C adress is 0x36

int Research_ADDR(uint8_t sla_addr)
{
    // START condition
    TWI_start();
    
    // SLA+W
    if (TWI_SLA_W(sla_addr) != 0) {
	TWI_stop();
	return -1;
    }

    TWI_stop();

    return 0;
}



void TWI_Init()
{
    // SCL_freq = CPU_clk_freq / (16 + 2*(TWBR) * PrescalerValue)
    // PrescalerValue = 1 => TWSR = 0
    // TWBR = 12; 400kHz
    TWBR = 72; // 100kHz
}

void PWM0_Init()
{
    DDRD = _BV(DDD6) | _BV(DDD5);
    TCCR0A = _BV(COM0A1) | _BV(COM0B1) | _BV(WGM01) | _BV(WGM00);
    TCCR0B = _BV(CS01) | _BV(CS00);
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

int main()
{
    USART_Init(3);
    TWI_Init();
    PORT_init();
    PWM0_Init();

    uint8_t addr = 0x00;
    int addr_found = -1;
    while (1) {
	// while (addr_found != 0) {
	//     if (Research_ADDR(addr) == 0) {
	// 	addr_found = 0;
	// 	printf("PASS avec: 0x%02x\n", addr);
	//     } else {
	// 	printf("NACK Address: 0x%02x\n", addr);
	// 	addr++;
	//     }
	//     PORTB ^= (1 << PORTB5);
	//     _delay_ms(50);
	// }
	Rotary_Encoder();
	PORTB ^= (1 << PORTB5);
	_delay_ms(100);
	
    }
}
