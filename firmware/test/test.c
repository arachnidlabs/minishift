#define F_CPU 8000000UL  // 8 MHz

#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "../slave/hardware.h"

static void ioinit(void) {
	// Set rows and columns as output
	ROW_DDR = 0xff;
	DDRC |= PORTC_COLS;
	DDRB |= PORTB_COLS;

	sei();
}

struct {
	volatile uint8_t *port;
	uint8_t pin;
} out_pins[] = {
	{&PORTD, _BV(PD4)},
	{&PORTD, _BV(PD6)},
	{&PORTB, _BV(PB6)},
	{&PORTC, _BV(PC5)},
	{&PORTD, _BV(PD7)},
	{&PORTC, _BV(PC3)},
	{&PORTD, _BV(PD5)},
	{&PORTD, _BV(PD2)},

	{&PORTD, _BV(PD0)},
	{&PORTC, _BV(PC4)},
	{&PORTC, _BV(PC2)},
	{&PORTD, _BV(PD3)},
	{&PORTC, _BV(PC7)},
	{&PORTD, _BV(PD1)},
	{&PORTC, _BV(PC1)},
	{&PORTC, _BV(PC0)},
};

static uint8_t num_pins = 16;

void main(void) __attribute__((noreturn));

void main(void) {
	ioinit();

	int idx = 6;
	while(1) {
		// Wait for rising edge on SCK
		while(!(SPI_PIN & _BV(SPI_SCK)));

		*out_pins[idx].port &= ~out_pins[idx].pin;
		idx = (idx + 1) % num_pins;
		*out_pins[idx].port |= out_pins[idx].pin;

		// Wait for falling edge on SCK
		while(SPI_PIN & _BV(SPI_SCK));
	}
}
