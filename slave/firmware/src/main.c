#define F_CPU 8000000UL  // 8 MHz

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>

#define SPI_PORT PORTB
#define SPI_PIN PINB
#define SPI_DDR DDRB
#define SPI_SS PB2
#define SPI_MOSI PB3
#define SPI_MISO PB4
#define SPI_SCK PB5

#define ROW_PORT PORTD
#define ROW_DDR DDRD

#define PORTC_COLS (_BV(PC7) | _BV(PC5) | _BV(PC4) | _BV(PC3) | _BV(PC2) | _BV(PC1) | _BV(PC0))
#define PORTB_COLS (_BV(PB6))
#define ENABLE_COL(col) if(col != 1) PORTC |= pgm_read_byte(&col_pins[col]); else PORTB |= pgm_read_byte(&col_pins[col])

#define PWM_ON() (TCCR0A |= _BV(CS01) | _BV(CS00), DDRC |= PORTC_COLS, DDRB |= PORTB_COLS)

FUSES = {
    // Internal 8mhz oscillator; short startup; enable ISP programming.
    .low = FUSE_SUT0 & FUSE_CKSEL0,
    .high = FUSE_SPIEN,
};

const char col_pins[] PROGMEM = {
    _BV(PC7),
    _BV(PB6),
    _BV(PC5),
    _BV(PC4),
    _BV(PC3),
    _BV(PC2),
    _BV(PC1),
    _BV(PC0),
};

uint8_t display[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t sr[8] = {0, 0, 0, 0, 0, 0, 0, 0};
uint8_t sridx = 0;

ISR(TIMER0_COMPA_vect) {
    static uint8_t column = 0;

    // Turn off the old column
    PORTC &= ~PORTC_COLS;
    PORTB &= ~PORTB_COLS;

    // Set the row data
    column = (column + 1) & 0x7;
    ROW_PORT = display[column];
    
    // Turn on the new column
    ENABLE_COL(column);
}

ISR(PCINT0_vect) {
    if(SPI_PIN & _BV(SPI_SS)) {
        // SS went high; transfer data from shift register to display
        for(int i = 0; i < 8; i++)
            display[i] = ~sr[(i + sridx) & 0x7];
    }
}

ISR(SPI_STC_vect) {
    // Feed the SPI peripheral from the sr circular buffer
    uint8_t out = sr[sridx];
    sr[sridx] = SPDR;
    SPDR = out;
    sridx = (sridx + 1) & 0x7;
}

static void ioinit(void) {
    // Enable timer 0: display refresh
    OCR0A = 250; // 8 megahertz / 64 / 250 = 250Hz
    TCCR0A = _BV(CTC0); // CTC(OCR0A), /64 prescaler
    TIMSK0 |= _BV(OCIE0A); // Interrupt on counter reset

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
