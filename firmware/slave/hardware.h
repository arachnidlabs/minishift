#ifndef HARDWARE_H
#define HARDWARE_H

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

#define PWM_ON() (TCCR0A |= _BV(CS01) , DDRC |= PORTC_COLS, DDRB |= PORTB_COLS)

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

#endif
