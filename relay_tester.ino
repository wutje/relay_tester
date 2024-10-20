
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c LCD i/o class header

#include <stdarg.h>

hd44780_I2Cexp lcd;

const char driverPin = 13;

void p(const __FlashStringHelper *fmt, ... ){
  char buf[64]; // resulting string is limited
  va_list args;
  va_start (args, fmt);
#ifdef __AVR__
  vsnprintf_P(buf, sizeof(buf), (const char *)fmt, args); // progmem for AVR
#else
  vsnprintf(buf, sizeof(buf), (const char *)fmt, args); // for the rest of the world
#endif
  va_end(args);
  Serial.println(buf);
}

void p1(const __FlashStringHelper *fmt, ... ){
  char buf[64]; // resulting string is limited
  va_list args;
  va_start (args, fmt);
#ifdef __AVR__
  vsnprintf_P(buf, sizeof(buf), (const char *)fmt, args); // progmem for AVR
#else
  vsnprintf(buf, sizeof(buf), (const char *)fmt, args); // for the rest of the world
#endif
  va_end(args);
  Serial.print(buf);
}

void lp(const __FlashStringHelper *fmt, ...)
{
  char buf[21]; // resulting string is limited
  va_list args;
  va_start (args, fmt);
#ifdef __AVR__
  vsnprintf_P(buf, sizeof(buf), (const char *)fmt, args); // progmem for AVR
#else
  vsnprintf(buf, sizeof(buf), (const char *)fmt, args); // for the rest of the world
#endif
  va_end(args);
  lcd.write(buf);
}

//Clear display (and reset to 0,0)
//Print line
void lpc(const __FlashStringHelper *fmt, ...)
{
  char buf[21]; // resulting string is limited
  lcd.clear();
  va_list args;
  va_start (args, fmt);
#ifdef __AVR__
  vsnprintf_P(buf, sizeof(buf), (const char *)fmt, args); // progmem for AVR
#else
  vsnprintf(buf, sizeof(buf), (const char *)fmt, args); // for the rest of the world
#endif
  va_end(args);
  lcd.write(buf);
}


//Pin 0 and Pin 1 are use by the UART, lets no use them.
struct pinInfo {
  const byte arduino_pin;
  const byte relay_pin; //Pin as on defined on relay
};
const char RELAY_SWITCHES = 4;
const struct pinInfo inputpins_no[RELAY_SWITCHES] = {
{.arduino_pin = 6, .relay_pin = 5},
{.arduino_pin = 5, .relay_pin = 6},
{.arduino_pin = 4, .relay_pin = 7},
{.arduino_pin = 3, .relay_pin = 8},
};

const struct pinInfo inputpins_nc[RELAY_SWITCHES] = {
{.arduino_pin = 7,  .relay_pin = 1},
{.arduino_pin = 8,  .relay_pin = 2},
{.arduino_pin = 9,  .relay_pin = 3},
{.arduino_pin = 10, .relay_pin = 4},
};
const char pindriver[RELAY_SWITCHES] = {11,12,14,15}; //Pin 13 is led

void print_pins(const struct pinInfo *pin)
{
  for(int i = 0; i < RELAY_SWITCHES; i++) {
    int res = digitalRead(pin[i].arduino_pin);
    p1(F("%d:%d "),pin[i].relay_pin, res);
  }
  p(F(""));
}

void print_all_inputpins(void)
{
    print_pins(inputpins_nc);
    print_pins(inputpins_no);
}

void setup() {
  Serial.begin(9600);

  Serial.println("Startup. Setting up I/O");
  
  // put your setup code here, to run once:
  pinMode(driverPin, OUTPUT);
  for(int i = 0; i < RELAY_SWITCHES; i++)
  {
    pinMode(inputpins_no[i].arduino_pin, INPUT_PULLUP);
    pinMode(inputpins_nc[i].arduino_pin, INPUT_PULLUP);
    pinMode(pindriver[i], OUTPUT);
  }

  int status = lcd.begin(20, 2);
  p(F("LCD setup result %d"), status);

  lcd.print("Relay test v1.0");
}

void loop() {
  bool pin_has_error[2*RELAY_SWITCHES];
 
  memset(pin_has_error, 0, sizeof(pin_has_error));
  lpc(F("Relay OFF"));
  
  /* Set the relay off */
  digitalWrite(driverPin, 0);
  
  /* Set the outputs high (just the like the pull ups) */
  for(int i = 0; i < sizeof(pindriver); i++)
  {
    digitalWrite(pindriver[i], 1);
  }
  delay(10);
  
  /* Verify all normally open are reading high */
  for(int i = 0; i < RELAY_SWITCHES; i++)
  {
    int res = digitalRead(inputpins_no[i].arduino_pin);
    if (res != HIGH) 
    {
      int relay_pin = inputpins_no[i].relay_pin;
      lpc(F("%d (NO) not default not 1? Short?"), relay_pin);
      pin_has_error[relay_pin - 1] = true; 
    }  
  }
  
  /* Verify all nc are reading high */
  for(int i = 0; i < RELAY_SWITCHES; i++)
  {
    int res = digitalRead(inputpins_nc[i].arduino_pin);
    if (res != HIGH) 
    {
      int relay_pin = inputpins_no[i].relay_pin;
      lpc(F("%d (NC) not default not 1? Short?"), relay_pin);
      pin_has_error[relay_pin - 1] = true; 
    }
  }
  lpc(F("Default check done"));  

  /* Set a single driver pin and verify the NC moves along */
  for(int i = 0; i < RELAY_SWITCHES; i++)
  {
    digitalWrite(pindriver[i], 0);
    delay(1);

    int res = digitalRead(inputpins_nc[i].arduino_pin);
    if (res != LOW) {
      int relay_pin = inputpins_no[i].relay_pin;
      lpc(F("%d (NC) not low"), relay_pin);
      pin_has_error[relay_pin - 1] = true; 
    }
        
    res = digitalRead(inputpins_no[i].arduino_pin);
    if (res != HIGH) {      
      int relay_pin = inputpins_no[i].relay_pin;
      lpc(F("%d (NO) not high"), relay_pin);
      pin_has_error[relay_pin - 1] = true; 
    }
    //print_all_inputpins();
    digitalWrite(pindriver[i], 1);
    delay(1);
  }

  //lpc(F("Relay ON"));
  /* Set the relay */
  digitalWrite(driverPin, 1);
  delay(10);
  
  /* Set a single driver pin and verify the NO moves along */
  for(int i = 0; i < RELAY_SWITCHES; i++)
  {
    int res;
    digitalWrite(pindriver[i], 0);
    delay(1);
    res = digitalRead(inputpins_no[i].arduino_pin);

    if (res != LOW) {
      int relay_pin = inputpins_no[i].relay_pin;
      lpc(F("%d (NO) not 0. Relay broken?"), relay_pin);      
      pin_has_error[relay_pin - 1] = true; 
    }  
    
    res = digitalRead(inputpins_nc[i].arduino_pin);
    if (res != HIGH) {
      int relay_pin = inputpins_nc[i].relay_pin;
      lpc(F("%d (NC) not 1. Relay broken?"), relay_pin);      
      pin_has_error[relay_pin - 1] = true; 
    }  
    //print_all_inputpins();
    digitalWrite(pindriver[i], 1);
    delay(1);
  }
  lcd.clear();
          //"12345678901234567890"
  lcd.print("Results: ");
  lcd.setCursor(0, 1);
  lcd.print("");
  lp(F("%d %d %d %d %d %d %d %d"),
      pin_has_error[0],
      pin_has_error[1],
      pin_has_error[2],
      pin_has_error[3],
      pin_has_error[4],
      pin_has_error[5],
      pin_has_error[6],
      pin_has_error[7]
  );
  delay(5000);

}
