// Standalone AVR ISP programmer
// Modified July 2013 by Nick Johnson <nick@arachnidlabs.com>
// August 2011 by Limor Fried / Ladyada / Adafruit
// Jan 2011 by Bill Westfield ("WestfW")
//
// this sketch allows an Arduino to program a flash program
// into any AVR if you can fit the HEX file into program memory
// No computer is necessary. Two LEDs for status notification
// Press button to program a new chip. Piezo beeper for error/success 
// This is ideal for very fast mass-programming of chips!
//
// It is based on AVRISP
//
// using the following pins:
// 10: slave reset
// 11: MOSI
// 12: MISO
// 13: SCK
// ----------------------------------------------------------------------
#include <avr/pgmspace.h>
#include "optiLoader.h"
#include "SPI.h"

// Global Variables
int pmode=0;
byte pageBuffer[64];		       /* One page of flash */


/*
 * Pins to target
 */
#define SCK 13
#define MISO 12
#define MOSI 11
#define RESET 10

#define PIN_SRLATCH 6
#define PIN_SRCLK 7
#define PIN_SRDATA 8
#define PIN_LATCHA 2
#define PIN_LATCHB 3
#define PIN_SLAVEDET A1
#define PIN_DETECT A0

#define LED_WORKING A5
#define LED_OK A4
#define LED_ERR A3

void slow_down(boolean enable) {
  static uint16_t saved_ubrr;
  static boolean slow = false;

  // If we're already in the mode we're asking for, do nothing.  
  if(enable == slow) return;
  
  cli();
  if(enable) {
    CLKPR = _BV(CLKPCE);
    CLKPR = 0x1;
    saved_ubrr = (UBRR0H << 8) | UBRR0L;
    UBRR0H = saved_ubrr >> 9;
    UBRR0L = (saved_ubrr >> 1) & 0xFF;
    slow = true;
  } else {
    CLKPR = _BV(CLKPCE);
    CLKPR = 0x0;
    UBRR0H = saved_ubrr >> 8;
    UBRR0L = saved_ubrr & 0xFF;
    slow = false;
  }
  sei();
}

void setup () {
  Serial.begin(57600);			/* Initialize serial for status msgs */
  Serial.println("\nMatrixjig Bootstrap programmer (originally OptiLoader Bill Westfield (WestfW))");
  
  pinMode(PIN_DETECT, INPUT);
  digitalWrite(PIN_DETECT, HIGH);
  
  pinMode(PIN_SRLATCH, OUTPUT);
  pinMode(PIN_SRCLK, OUTPUT);
  pinMode(PIN_SRDATA, INPUT);
  digitalWrite(PIN_SRLATCH, HIGH);

  pulse(LED_WORKING, 2);
  pulse(LED_OK, 2);
  pulse(LED_ERR, 2);
}

boolean do_fuses(image_t *image) {
  delay(10);
  if (! programFuses(image->image_progfuses)) {
    error("Failed to program fuses");
    return false;
  }
  delay(10);
  if (! verifyFuses(image->image_progfuses, image->fusemask) ) {
    error("Failed to verify fuses");
    return false;
  }

  return true;
}

boolean do_program(image_t *image) {
  const unsigned char *imagedata = (const unsigned char*)pgm_read_word(&image->image_data);
  uint8_t pagesize = pgm_read_byte(&image->image_pagesize);
  uint16_t chipsize = pgm_read_word(&image->chipsize);
  
  programImage(imagedata, pagesize, chipsize);
  
  end_pmode();
  start_pmode();
  
  Serial.println("\nVerifing flash...");
  if (! verifyImage(imagedata, chipsize) ) {
    error("Failed to verify chip");
    return false;
  } else {
    Serial.println("\tFlash verified correctly!");
  }
  return true;
}

boolean load_test_program() {
  image_t *testimage;
  if (! (testimage = findImage("test.hex"))) {
    error("Failed to load test image");
    return false;
  }
  
  slow_down(true);
  target_poweron();

  uint16_t signature = pgm_read_word(&testimage->image_chipsig);
        
  if (signature != readSignature()) {
    error("Invalid signature");
    return false;
  }
  
  eraseChip();

  if(!do_fuses(testimage))
    return false;

  slow_down(false);

  end_pmode();
  start_pmode();
  
  if(!do_program(testimage))
    return false;

  end_pmode();
    
  return true;
}

boolean run_tests() {
  pinMode(SCK, OUTPUT);
  boolean ret = true;
  delay(200);
  for(int i = 0; i < 16; i++) {
    digitalWrite(SCK, HIGH);
    digitalWrite(PIN_SRLATCH, LOW);
    delay(1);
    digitalWrite(PIN_SRLATCH, HIGH);
    uint16_t incoming = digitalRead(PIN_SRDATA);
    incoming |= shiftIn(PIN_SRDATA, PIN_SRCLK, MSBFIRST) << 8;
    incoming |= shiftIn(PIN_SRDATA, PIN_SRCLK, MSBFIRST);
    if(incoming != (1 << i)) {
      Serial.print("Expected 0x");
      Serial.print(1 << i, HEX);
      Serial.print(" but got 0x");
      Serial.println(incoming);
      ret = false;
    }

    digitalWrite(SCK, LOW);
    delay(1);
  }
  return ret;
}

boolean load_main_program() {
  image_t *mainimage;
  if(!(mainimage = findImage("minishift.hex"))) {
    error("Failed to load main image");
    return false;
  }
  
  start_pmode();
  eraseChip();
  end_pmode();
  
  start_pmode();
  if(!do_program(mainimage))
    return false;
    
  const unsigned char *eepromdata = (const unsigned char*)pgm_read_word(&mainimage->eeprom_data);
  uint8_t pagesize = pgm_read_byte(&mainimage->eeprom_pagesize);
  uint16_t eepromsize = pgm_read_word(&mainimage->eepromsize);
  Serial.println("Programming EEPROM...");
  if(!programEEPROM(eepromdata, pagesize, eepromsize))
    return false;
    
  Serial.println("Verifying EEPROM...");
  if(!verifyEEPROM(eepromdata, eepromsize))
    return false;
    
  end_pmode();
  
  return true;
}

boolean program_and_test() {
  led_on(LED_WORKING);
  
  if(!load_test_program()) {
    led_off(LED_WORKING);
    led_on(LED_ERR);
    return false;
  }

  if(!run_tests()) {
    led_off(LED_WORKING);
    led_on(LED_ERR);
    return false;
  }

  if(!load_main_program()) {
    led_off(LED_WORKING);
    led_on(LED_ERR);
    return false;
  }

  led_off(LED_WORKING);
  led_on(LED_OK);
  return true;
}

void loop (void) {

  Serial.println("\nType 'G' or hit BUTTON for next chip");
  while (1) {
    if  (Serial.read() == 'G')
      break;  
    if(!digitalRead(PIN_DETECT))
      break;
  }
  delay(1000);

  if(program_and_test()) {
    Serial.println("PASS");
  } else {
    Serial.println("FAIL");
  }
  delay(1000);
  while(!digitalRead(PIN_DETECT));
 
  led_off(LED_OK);
  led_off(LED_ERR);
}

void error(char *string) { 
  slow_down(false);
  Serial.println(string); 
}

void start_pmode () {
  pinMode(13, INPUT); // restore to default

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV128); 
  
  debug("...spi_init done");
  // following delays may not work on all targets...
  pinMode(RESET, OUTPUT);
  digitalWrite(RESET, HIGH);
  pinMode(SCK, OUTPUT);
  digitalWrite(SCK, LOW);
  delay(50);
  digitalWrite(RESET, LOW);
  delay(50);
  pinMode(MISO, INPUT);
  pinMode(MOSI, OUTPUT);
  debug("...spi_transaction");
  spi_transaction(0xAC, 0x53, 0x00, 0x00);
  debug("...Done");
  pmode = 1;
}

void end_pmode () {
  SPCR = 0;				/* reset SPI */
  digitalWrite(MISO, 0);		/* Make sure pullups are off too */
  pinMode(MISO, INPUT);
  digitalWrite(MOSI, 0);
  pinMode(MOSI, INPUT);
  digitalWrite(SCK, 0);
  pinMode(SCK, INPUT);
  digitalWrite(RESET, HIGH);
  //pinMode(RESET, INPUT);
  pmode = 0;
}


/*
 * target_poweron
 * begin programming
 */
boolean target_poweron ()
{
  digitalWrite(RESET, LOW);  // reset it right away.
  pinMode(RESET, OUTPUT);
  Serial.print("Starting Program Mode");
  start_pmode();
  Serial.println(" [OK]");
  return true;
}

boolean target_poweroff ()
{
  end_pmode();
  return true;
}

