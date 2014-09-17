#define F_CPU 8000000UL  // 8 MHz

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>

#include "hardware.h"

FUSES = {
    // Internal 8mhz oscillator; short startup; enable ISP programming.
    .low = FUSE_SUT0 & FUSE_CKSEL0,
    .high = FUSE_SPIEN,
};

const uint8_t col_brightness_adj[] PROGMEM = { 255, 48, 77, 107, 136, 166, 195, 225, 255 };

uint8_t display[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t sr[8] = {0, 0, 0, 0, 0, 0, 0, 0};
uint8_t sridx = 0;
uint8_t latch = 0;

uint8_t count_set_bits(uint8_t v) {
    uint8_t c;

    for(c = 0; v; c++)
        v &= v - 1;

    return c;
}

ISR(TIMER0_COMPA_vect) {
    static uint8_t column = 0;

    if(column == 0 && latch) {
        for(int i = 0; i < 8; i++)
            display[i] = ~sr[(i + sridx) & 0x7];
        latch = 0;
    }

    // Turn off the old column
    PORTC &= ~PORTC_COLS;
    PORTB &= ~PORTB_COLS;

    // Set the row data
    column = (column + 1) & 0x7;
    ROW_PORT = display[column];

    // Add dead time as required
    OCR0B = pgm_read_byte(&col_brightness_adj[count_set_bits(~ROW_PORT)]);
    
    // Turn on the new column
    ENABLE_COL(column);
}

ISR(TIMER0_COMPB_vect) {
    PORTC &= ~PORTC_COLS;
    PORTB &= ~PORTB_COLS;
}

ISR(PCINT0_vect) {
    if(SPI_PIN & _BV(SPI_SS)) {
        // SS went high; transfer data from shift register to display
        latch = 1;
    }
}

ISR(SPI_STC_vect) {
    // Feed the SPI peripheral from the sr circular buffer
    sr[sridx] = SPDR;
    sridx = (sridx + 1) & 0x7;
    SPDR = sr[sridx];
}

static void ioinit(void) {
    // Enable timer 0: display refresh
    OCR0A = 250; // 8 megahertz / 64 / 250 = 250Hz
    OCR0B = 255;
    TCCR0A = _BV(CTC0); // CTC(OCR0A), /8 prescaler
    TIMSK0 |= _BV(OCIE0A) | _BV(OCIE0B); // Interrupt on counter reset and on OCR0B match

    // PORTD is all output for rows
    ROW_DDR = 0xff;

    // MISO is an output
    SPI_DDR |= _BV(SPI_MISO);

    // Enable SPI
    SPCR = _BV(SPE) | _BV(SPIE);

    // Enable pin change interrupt for SS
    PCICR |= _BV(PCIE0);
    PCMSK0 |= _BV(PCINT2);

    // Disable analog comparator
    ACSR = _BV(ACD);

    // Disable unused modules - ADC, TWI, Timer 1
    PRR = _BV(PRTWI) | _BV(PRTIM1) | _BV(PRADC);

    sei();
}

void main(void) __attribute__((noreturn));

void main(void) {
    ioinit();

    PWM_ON();

    while(1)
        sleep_mode();
}
