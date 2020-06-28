/*
   --------------------------------------------------------------------------------------------------------------------
   controle de acesso modificado por Gilmar Santos
   --------------------------------------------------------------------------------------------------------------------
   This is a MFRC522 library example; for further details and other examples see: https://github.com/miguelbalboa/rfid

   This example showing a complete Door Access Control System

  Simple Work Flow (not limited to) :
                                      +---------+
  +----------------------------------->LEITOR TAGS+>-------------------------------------------------------+
  |                              +--------------------+                                                    |
  |                              |                    |                                                    |
  |                              |                    |                                                    |
  |                         +----v-----+        +-----v-----+                                               |
  |                         |MASTER TAG|        |OUTRAS TAGS|                                              |
  |                         +--+-------+        ++-------------+                                           |
  |                            |                 |             |                                           |
  |                            |                 |             |                                           |
  |                      +-----v---+        +----v----+   +----v-------+                                   |
  |         +------------+LIDO TAGS+---+    |CONHECIDA|   |DESCONHECIDA|                                   |
  |         |            +-+-------+   |    +-----------+ +------------------+                             |
  |         |              |           |                |                    |                             |
  |    +----v-----+   +----v----+   +--v---------+     +-v----------------+  +------v------------+         |
  |    |MASTER TAG|   |CONHECIDA|   |DESCONHECIDA|     | ACCESSO GARANTIDO|  |  ACCESSO NEGADO   |         |
  |    +----------+   +---+-----+   +-----+------+     +-----+------------+  +-----+-------------+         |
  |                       |               |                  |                |                            |
  |       +-----+     +----v------+     +--v---+             |                +---------------------------->
  +-------+SAIDA|     |DELETADO DA|     |ADD A |             |                                             |
          +-----+     |  EEPROM   |     |EEPROM|             |                                             |
                      +-----------+      +------+            +---------------------------------------------+


   Use a Master Card which is act as Programmer then you can able to choose card holders who will granted access or not

 * **Easy User Interface**

   Just one RFID tag needed whether Delete or Add Tags. You can choose to use Leds for output or Serial LCD module to inform users.

 * **Stores Information on EEPROM**

   Information stored on non volatile Arduino's EEPROM memory to preserve Users' tag and Master Card. No Information lost
   if power lost. EEPROM has unlimited Read cycle but roughly 100,000 limited Write cycle.

 * **Security**
   To keep it simple we are going to use Tag's Unique IDs. It's simple and not hacker proof.

   @license Released into the public domain.

*/

#include <EEPROM.h>     // We are going to read and write PICC's UIDs from/to EEPROM
#include <SPI.h>        // RC522 Module uses SPI protocol
#include <MFRC522.h>  // Library for Mifare RC522 Devices
#include <arduino.h>
#include <Wire.h>
#include <MicroLCD.h>

//CONFIGURACAO DO DISPLAY MICRO LCD
//LCD_SH1106 lcd; /* para módulo controlado pelo CI SH1106 OLED */ 
LCD_SSD1306 lcd; /* para módulo controlado pelo CI SSD1306 OLED */


/*
  Instead of a Relay you may want to use a servo. Servos can lock and unlock door locks too
  Relay will be used by default
*/

// #include <Servo.h>

/*
  For visualizing whats going on hardware we need some leds and to control door lock a relay and a wipe button
  (or some other hardware) Used common anode led,digitalWriting HIGH turns OFF led Mind that if you are going
  to use common cathode led or just seperate leds, simply comment out #define COMMON_ANODE,
*/

#define COMMON_ANODE

#ifdef COMMON_ANODE
#define LED_ON LOW
#define LED_OFF HIGH
#else
#define LED_ON HIGH
#define LED_OFF LOW
#endif

#define redLed   7    // leds indicadores
#define greenLed 6
#define blueLed  5

#define relay 4     // rele abertura
#define wipeB 3     // Botao reset
#define tca   2    // acesso teclado
#define tcb   8    // acesso teclado

bool programMode = false;  // initialize programming mode to false

uint8_t successRead;    // Variable integer to keep if we have Successful Read from Reader

byte storedCard[4];   // Stores an ID read from EEPROM
byte readCard[4];   // Stores scanned ID read from RFID Module
byte masterCard[4];   // Stores master card's ID read from EEPROM

// Create MFRC522 instance.
#define SS_PIN 10
#define RST_PIN 9
MFRC522 MFRC522(SS_PIN, RST_PIN);

const PROGMEM uint8_t smile[48 * 48 / 8] = {
0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xC0,0xE0,0xF0,0xF8,0xF8,0xFC,0xFC,0xFE,0xFE,0x7E,0x7F,0x7F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x7F,0x7F,0x7E,0xFE,0xFE,0xFC,0xFC,0xF8,0xF8,0xF0,0xE0,0xC0,0x80,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0xC0,0xF0,0xFC,0xFE,0xFF,0xFF,0xFF,0x3F,0x1F,0x0F,0x07,0x03,0x01,0x00,0x80,0x80,0x80,0x80,0x80,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x80,0x80,0x80,0x80,0x80,0x00,0x01,0x03,0x07,0x0F,0x1F,0x3F,0xFF,0xFF,0xFF,0xFE,0xFC,0xF0,0xC0,0x00,
0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,0x07,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x1F,0x1F,0x1F,0x3F,0x1F,0x1F,0x02,0x00,0x00,0x00,0x00,0x06,0x1F,0x1F,0x1F,0x3F,0x1F,0x1F,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,
0x7F,0xFF,0xFF,0xFF,0xFF,0xFF,0xE0,0x00,0x00,0x30,0xF8,0xF8,0xF8,0xF8,0xE0,0xC0,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xC0,0xE0,0xF8,0xF8,0xFC,0xF8,0x30,0x00,0x00,0xE0,0xFF,0xFF,0xFF,0xFF,0xFF,0x7F,
0x00,0x03,0x0F,0x3F,0x7F,0xFF,0xFF,0xFF,0xFC,0xF8,0xF0,0xE1,0xC7,0x87,0x0F,0x1F,0x3F,0x3F,0x3E,0x7E,0x7C,0x7C,0x7C,0x78,0x78,0x7C,0x7C,0x7C,0x7E,0x3E,0x3F,0x3F,0x1F,0x0F,0x87,0xC7,0xE1,0xF0,0xF8,0xFC,0xFF,0xFF,0xFF,0x7F,0x3F,0x0F,0x03,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x03,0x07,0x0F,0x1F,0x1F,0x3F,0x3F,0x7F,0x7F,0x7E,0xFE,0xFE,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFE,0xFE,0x7E,0x7F,0x7F,0x3F,0x3F,0x1F,0x1F,0x0F,0x07,0x03,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
};

const PROGMEM unsigned char tick[16 * 16 / 8] =
{0x00,0x80,0xC0,0xE0,0xC0,0x80,0x00,0x80,0xC0,0xE0,0xF0,0xF8,0xFC,0x78,0x30,0x00,0x00,0x01,0x03,0x07,0x0F,0x1F,0x1F,0x1F,0x0F,0x07,0x03,0x01,0x00,0x00,0x00,0x00};

const PROGMEM unsigned char novolog [160 * 64 / 8] = {
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0x7F, 0x7F, 0x7F, 0x1F, 0x0F, 0x5F, 0x07, 0x57, 0x03, 0x55, 0xD5, 0x80, 0xD5, 0xC0, 0xF5, 0xF5,
0xF8, 0xFD, 0xF8, 0xFD, 0xF8, 0xF5, 0xE0, 0xF5, 0xE0, 0xD5, 0x80, 0x55, 0x55, 0x07, 0x57, 0x07,
0x57, 0x5F, 0x1F, 0x7F, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0x0F, 0x57, 0x07, 0x57, 0x07, 0x57, 0x57, 0x07, 0x57, 0x07, 0x57, 0x07, 0x07, 0x57, 0x07, 0x57,
0x07, 0x07, 0x57, 0x07, 0x57, 0x07, 0x57, 0x07, 0x57, 0x07, 0x57, 0x07, 0x57, 0x07, 0x07, 0x57,
0x07, 0x57, 0x07, 0x57, 0x07, 0x57, 0x07, 0x57, 0x57, 0x07, 0x57, 0x03, 0x55, 0x01, 0xD5, 0x80,
0xD5, 0xC0, 0xF5, 0xF0, 0xF8, 0xFD, 0xFC, 0xFD, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xFD, 0xF8,
0xFD, 0xF5, 0xE0, 0xF5, 0xE0, 0xD5, 0x80, 0x03, 0x57, 0x07, 0x57, 0x07, 0x57, 0x07, 0x57, 0x07,
0x57, 0x07, 0x57, 0x57, 0x07, 0x57, 0x07, 0x07, 0x07, 0x57, 0x07, 0x57, 0x07, 0x07, 0x57, 0x07,
0x57, 0x07, 0x57, 0x07, 0x57, 0x07, 0x57, 0x07, 0x57, 0x07, 0x07, 0x57, 0x07, 0x57, 0x07, 0x07,
0x57, 0x07, 0x57, 0x07, 0x5F, 0xDF, 0x9F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFC, 0xFD, 0xF0, 0xF5, 0xF5, 0xC0, 0xD5, 0x80, 0x55, 0x00, 0x01, 0x57, 0x0F, 0x5F,
0x1F, 0x3F, 0x7F, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0x7F, 0x3F, 0x1F, 0x1F, 0x0F, 0x0F, 0x0F, 0x87, 0xE3, 0xE3, 0xF3, 0xFB, 0xFB, 0xFF, 0xFF,
0x7D, 0x7D, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0x7D, 0x03, 0x03, 0x03, 0x03, 0x03, 0x8F, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1F, 0x1F, 0x1F, 0x1F, 0x0F, 0x0F, 0x67, 0xF3, 0xFB, 0xFB, 0xFB,
0xFB, 0x7F, 0x7F, 0x3D, 0x3D, 0x1D, 0x1D, 0x0D, 0x0D, 0xCD, 0xCD, 0xED, 0xED, 0xFD, 0xFD, 0xFD,
0x79, 0x79, 0x03, 0x03, 0x03, 0x03, 0x07, 0x8F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F,
0x7F, 0x3F, 0x7F, 0x1F, 0x5F, 0x0F, 0x57, 0x03, 0x55, 0x01, 0x00, 0xD5, 0xC0, 0xD5, 0xE0, 0xF0,
0xF5, 0xFC, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x7D, 0x30, 0x75,
0x10, 0x00, 0x55, 0x00, 0x55, 0x00, 0x57, 0x03, 0xD7, 0x83, 0xDF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xFC,
0xFC, 0xFC, 0xF8, 0x79, 0x79, 0x79, 0x18, 0x18, 0x1C, 0x1C, 0x9C, 0x9C, 0xFE, 0xFE, 0x7F, 0x7F,
0x7F, 0x7F, 0x7F, 0x7F, 0x3F, 0x3E, 0x3E, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFE, 0xFF, 0xFF, 0x3F,
0x3F, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0xFC, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD, 0x79, 0x79,
0x00, 0x00, 0x00, 0x02, 0x06, 0x06, 0x8F, 0xFF, 0xFF, 0xDF, 0xCF, 0xD7, 0x83, 0x03, 0x55, 0x00,
0x55, 0x00, 0x55, 0x10, 0x55, 0x30, 0x7D, 0x7C, 0xFD, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0x7F, 0x7F, 0x7F, 0x7F, 0x5F, 0x0F, 0x57, 0x07, 0x57, 0x03, 0x00, 0xD5, 0xC0, 0xD5,
0xE0, 0xF0, 0xF5, 0xF8, 0xFD, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFE, 0xFC, 0xFC, 0xFC, 0xF8, 0xF8, 0xF8, 0x38, 0x99, 0x99, 0xC9, 0xED, 0xED, 0x6D, 0x6D,
0x04, 0x04, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0xE0, 0xFC, 0xFD, 0xFD, 0xFC, 0xFC, 0xFE, 0xFE,
0xFC, 0xFC, 0xFC, 0xFC, 0xF8, 0xF8, 0xF8, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFC, 0xFC, 0xFC,
0xFC, 0xFE, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0xF8, 0xFB, 0xFB, 0xFB, 0xFB, 0xF9, 0xF8, 0xF8,
0xFC, 0xFC, 0xFC, 0xFC, 0xFE, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD, 0xF8,
0xFD, 0xF0, 0xF5, 0xE0, 0xF5, 0xC0, 0xD5, 0x80, 0x55, 0x00, 0x03, 0x57, 0x0F, 0x5F, 0x1F, 0x7F,
0x7F, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xC1, 0xD5, 0xC0, 0xD5, 0xC0, 0xD5, 0xD5, 0xC0, 0xD5, 0xC0, 0xD7, 0xC2, 0xC2, 0xD7, 0xC3, 0xD7,
0xC3, 0xC3, 0xD7, 0xC3, 0xD7, 0xC3, 0xD7, 0xC3, 0xD7, 0xC3, 0xD7, 0xC3, 0xD7, 0xC3, 0xC3, 0xD7,
0xC3, 0xD7, 0xC3, 0xD7, 0xC3, 0xD7, 0xC3, 0xD7, 0xD6, 0x83, 0xD7, 0x03, 0x56, 0x02, 0x56, 0x06,
0x5E, 0x0E, 0x5E, 0x3F, 0x3F, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F,
0x7F, 0x5F, 0x1F, 0x5F, 0x0F, 0x57, 0x07, 0x03, 0x57, 0x83, 0xD7, 0xC3, 0xD7, 0xC3, 0xD7, 0xC3,
0xD7, 0xC3, 0xD7, 0xD7, 0xC3, 0xD7, 0xC3, 0xC3, 0xC3, 0xD7, 0xC3, 0xD7, 0xC3, 0xC3, 0xD7, 0xC3,
0xD7, 0xC3, 0xD7, 0xC3, 0xD7, 0xC3, 0xD7, 0xC3, 0xD7, 0xC2, 0xC2, 0xD5, 0xC0, 0xD5, 0xC0, 0xC0,
0xD5, 0xC0, 0xD5, 0xC1, 0xD5, 0xF7, 0xE7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0x9F, 0xDF, 0xDF, 0x3F, 0x1F, 0x9F, 0xFF, 0x3F, 0x1F, 0xFF, 0x1F, 0x9F, 0x07, 0xD7, 0xBF, 0x1F,
0x3F, 0x3F, 0x1F, 0x9F, 0x1F, 0xFF, 0x3F, 0x07, 0xD3, 0xE3, 0xFF, 0xFF, 0xFF, 0x7F, 0xDF, 0x1F,
0x1F, 0xFF, 0xDF, 0xDF, 0x3F, 0x9F, 0xDF, 0xDF, 0x1F, 0x1F, 0x9F, 0xFF, 0x9F, 0x1E, 0xFD, 0xFC,
0x9D, 0x18, 0xFD, 0xE0, 0xC0, 0xD5, 0xC0, 0xD5, 0x01, 0x55, 0x57, 0x07, 0x57, 0x07, 0x5F, 0x7F,
0x3F, 0x7F, 0x7F, 0x7F, 0x3F, 0x5F, 0x1F, 0x5F, 0x0F, 0x57, 0x07, 0x57, 0x55, 0x80, 0xD5, 0xC0,
0xD5, 0xF5, 0xF0, 0xFD, 0xF8, 0xFD, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFC, 0xFD, 0xFF, 0xFC, 0xFC, 0xFD, 0xFE, 0xFC, 0xFF, 0xFC, 0xFC, 0xFE, 0xFC, 0xFD, 0xFF, 0xFC,
0xFC, 0xFC, 0xFC, 0xFD, 0xFE, 0xFE, 0xFC, 0xFC, 0xFD, 0xFE, 0xFF, 0xFF, 0xFF, 0xFC, 0xFF, 0xFC,
0xFD, 0xFF, 0xFD, 0xFF, 0xFC, 0xFC, 0xFD, 0xFF, 0xFC, 0xFD, 0xFC, 0xFD, 0xFD, 0xFE, 0xFF, 0xFD,
0xFD, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFD, 0xF8, 0xFD, 0xF5,
0xE0, 0xF5, 0xE0, 0xF5, 0xE0, 0xF5, 0xF0, 0xFD, 0xF8, 0xFD, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};





///////////////////////////////////////// Setup ///////////////////////////////////
void setup()      // inicia display oled

{
  lcd.begin();
  Wire.begin(); // join i2c bus (address optional for master)
  
  //Arduino Pin Configuration
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  pinMode(wipeB, INPUT_PULLUP);   // Enable pin's pull up resistor
  pinMode(relay, OUTPUT);
  pinMode(tca,INPUT);
  
  //Be careful how relay circuit behave on while resetting or power-cycling your Arduino
  digitalWrite(relay, LOW);    // Make sure door is locked
  digitalWrite(redLed, LED_OFF);  // Make sure led is off
  digitalWrite(greenLed, LED_OFF);  // Make sure led is off
  digitalWrite(blueLed, LED_OFF); // Make sure led is off

  //Protocol Configuration
  Serial.begin(9600);  // Initialize serial communications with PC
  SPI.begin();           // MFRC522 Hardware uses SPI protocol
  MFRC522.PCD_Init();    // Initialize MFRC522 Hardware

  //If you set Antenna Gain to Max it will increase reading distance
 // MFRC522.PCD_SetAntennaGain(MFRC522.RxGain_max);

  Serial.println(F("Controle de Acesso v1.1"));// For debugging purposes


  
  Serial.println(F("By Gilmar Santos"));// programador
  lcd.println("/////////////////////");
  lcd.println("/////////////////////");
  lcd.println(" Controle Acesso v1.1");
  lcd.println("");
  lcd.println("   By Gilmar Santos  ");
  lcd.println("/////////////////////");
  lcd.println("/////////////////////");
  delay(2000);
  lcd.clear();
  lcd.draw(novolog, 160, 64);
  delay(2000);
  lcd.clear();


 
  
  ShowReaderDetails();  // Show details of PCD - MFRC522 Card Reader details

  //Wipe Code - If the Button (wipeB) Pressed while setup run (powered on) it wipes EEPROM
  if (digitalRead(wipeB) == LOW) {  // when button pressed pin should get low, button connected to ground
    digitalWrite(redLed, LED_ON); // Red Led stays on to inform user we are going to wipe
    Serial.println(F("Botao de apagar pressionado"));
    Serial.println(F("Voce tem 10 segundos para cancelar"));
    Serial.println(F("isto vai apagar todos cartoes e nao pode ser desfeito"));
    lcd.clear();
  lcd.setFontSize(FONT_SIZE_SMALL);
  lcd.println(" apagar      tudo   pressionado!");
  lcd.println(" apagando em   15s");
    bool buttonState = monitorWipeButton(10000); // Give user enough time to cancel operation
    if (buttonState == true && digitalRead(wipeB) == LOW) {    // If button still be pressed, wipe EEPROM
      Serial.println(F("apagando EEPROM..."));
      lcd.println("apagando...");
      for (uint16_t x = 0; x < EEPROM.length(); x = x + 1) {    //Loop end of EEPROM address
        if (EEPROM.read(x) == 0) {              //If EEPROM address 0
          // do nothing, already clear, go to the next address in order to save time and reduce writes to EEPROM
        }
        else {
          EEPROM.write(x, 0);       // if not write 0 to clear, it takes 3.3mS
        }
      }
      Serial.println(F("EEPROM apagado com sucesso!"));
      lcd.println("EEPROM apagado!");
      delay(2000);
      lcd.clear();
      digitalWrite(redLed, LED_OFF);  // visualize a successful wipe
      delay(200);
      digitalWrite(redLed, LED_ON);
      delay(200);
      digitalWrite(redLed, LED_OFF);
      delay(200);
      digitalWrite(redLed, LED_ON);
      delay(200);
      digitalWrite(redLed, LED_OFF);
    }
    else {
      Serial.println(F("apagar cancelado")); // Show some feedback that the wipe button did not pressed for 15 seconds
      lcd.println("cancelado");
      delay(2000);
      digitalWrite(redLed, LED_OFF);
    }
  }
  // Check if master card defined, if not let user choose a master card
  // This also useful to just redefine the Master Card
  // You can keep other EEPROM records just write other than 143 to EEPROM address 1
  // EEPROM address 1 should hold magical number which is '143'
  if (EEPROM.read(1) != 143) {
    Serial.println(F("Nenhum Master Card Definido"));
    Serial.println(F("procurando PICC para definir como Master Card"));
    lcd.println("nenhum master card");
    do {
      successRead = getID();            // sets successRead to 1 when we get read from reader otherwise 0
      digitalWrite(blueLed, LED_ON);    // Visualize Master Card need to be defined
      delay(200);
      digitalWrite(blueLed, LED_OFF);
      delay(200);
    }
    while (!successRead);                  // Program will not go further while you not get a successful read
    for ( uint8_t j = 0; j < 4; j++ ) {        // Loop 4 times
      EEPROM.write( 2 + j, readCard[j] );  // Write scanned PICC's UID to EEPROM, start from address 3
    }
    EEPROM.write(1, 143);                  // Write to EEPROM we defined Master Card.
    Serial.println(F("Master Card Definido!"));
  }
  Serial.println(F("-------------------"));
  Serial.println(F("Master Card's UID"));
  for ( uint8_t i = 0; i < 4; i++ ) {          // Read Master Card's UID from EEPROM
    masterCard[i] = EEPROM.read(2 + i);    // Write it to masterCard
    Serial.print(masterCard[i], HEX);
  }
  Serial.println("");
  Serial.println(F("-------------------"));
  Serial.println(F("Tudo pronto para ser lido"));
  Serial.println(F("Esperando PICCs para ser lido"));
  cycleLeds();    // Everything ready lets give user some feedback by cycling leds
}



///////////////////////////////////////// Main Loop ///////////////////////////////////
void loop (){
  
  do {
    successRead = getID();  // sets successRead to 1 when we get read from reader otherwise 0
    // When device is in use if wipe button pressed for 10 seconds initialize Master Card wiping
    if (digitalRead(wipeB) == LOW) { // Check if button is pressed
      // Visualize normal operation is iterrupted by pressing wipe button Red is like more Warning to user
      digitalWrite(redLed, LED_ON);  // Make sure led is off
      digitalWrite(greenLed, LED_OFF);  // Make sure led is off
      digitalWrite(blueLed, LED_OFF); // Make sure led is off
      // Give some feedback
      Serial.println(F("Botao de apagar pressionado"));
      Serial.println(F("Cartao Master sera apagado! em 10s"));
      lcd.clear();
  lcd.setFontSize(FONT_SIZE_SMALL);
  lcd.println(" apagar");
  lcd.println("");
  lcd.println(" apagando em 10s");
      bool buttonState = monitorWipeButton(10000); // Give user enough time to cancel operation
      if (buttonState == true && digitalRead(wipeB) == LOW) {    // If button still be pressed, wipe EEPROM
        EEPROM.write(1, 0);                  // Reset Magic Number.
        Serial.println(F(" Master apagado!"));
        Serial.println(F("Entre com Master"));
        lcd.println(" Master apagado!");
        lcd.println("Entre novo Master");
        while (1);
      }
      Serial.println(F("apagar Cartao Master Cancelado!"));
      lcd.println(" Apagar Master ");
      lcd.println(" Cancelado!");
      delay(1000);
      lcd.clear();
    }
 if ( digitalRead(tca) == HIGH) {
    digitalWrite(relay,HIGH);
    lcd.println("*** BEM VINDO***");
    delay(200);
    lcd.clear();
    digitalWrite(relay,LOW);
    
 }

    
    if (programMode) {
      cycleLeds();              // Program Mode cycles through Red Green Blue waiting to read a new card
    }
    else {
      normalModeOn();     // Normal mode, blue Power LED is on, all others are off
    }
  }
 
  while (!successRead);   //the program will not go further while you are not getting a successful read
  if (programMode) {
    if ( isMaster(readCard) ) { //When in program mode check First If master card scanned again to exit program mode
      Serial.println(F(" Lendo Cartao Master"));
      Serial.println(F(" Saindo do modo programaçao"));
      Serial.println(F("-----------------------------"));
       lcd.clear();
       lcd.setFontSize(FONT_SIZE_SMALL);
       lcd.println(" Lendo Master card ");
       lcd.println("");
       lcd.println(" Saindo programacao ");
       delay(2000);
       lcd.clear();



      
      programMode = false;
      return;
    }
    else {
      if ( findID(readCard) ) { // If scanned card is known delete it
        Serial.println(F("nao conheco este PICC, REMOVENDO..."));
         lcd.println("REMOVENDO...");
         delay(2000);
         lcd.clear();
         
        deleteID(readCard);
        Serial.println("-----------------------------");
        Serial.println(F("procurando PICC REMOVE para EEPROM"));
        lcd.clear();
         lcd.println(F("---------------------"));
         lcd.println(F("procurar PICC REMOVE"));
         lcd.println(F(" da EEPROM"));
         lcd.println(F("---------------------"));
         delay(2000);
      }
      else {                    // If scanned card is not known add it
        Serial.println(F("nao conheco este PICC, ADICIONANDO..."));
         lcd.println("ADICIONANDO...");
         delay(2000);
         lcd.clear();
        writeID(readCard);
        Serial.println(F("---------------------"));
        Serial.println(F("procurando PICC para ADD para EEPROM"));
        lcd.clear();
         lcd.println(F("---------------------"));
         lcd.println(F("procurando PICC para"));
         lcd.println(F(" ADD para EEPROM"));
         lcd.println(F("---------------------"));
         delay(2000);
      }
    }
  }
  else {
    if ( isMaster(readCard)) {    // If scanned card's ID matches Master Card's ID - enter program mode
      programMode = true;
      Serial.println(F("Ola Master - Entrando na programaçao"));
      lcd.setFontSize(FONT_SIZE_SMALL);
      lcd.clear();
      lcd.println("  Ola Master");
      lcd.println("");
      lcd.println("Entrando programacao");
      delay(2000);
      uint8_t count = EEPROM.read(0);   // Read the first Byte of EEPROM that
      Serial.print(F("Eu tenho "));     // stores the number of ID's in EEPROM
      Serial.print(count);
      Serial.print(F(" gravado(s) na EEPROM"));
      lcd.clear();
      lcd.print(" Eu tenho(");
      lcd.print(count);
      lcd.println(")cartoes");
      lcd.println("");
      lcd.println("");
      lcd.print("gravado(s)");
      lcd.print(" na EEPROM");
      delay(2000);
      lcd.clear();
      
      Serial.println("");
      Serial.println(F("Aproxime um cartao para Adicionar ou Remover da EEPROM"));
      Serial.println(F("Aproxime o cartao Master novamente para sair da programacao"));
      Serial.println(F("-----------------------------"));
      lcd.setFontSize(FONT_SIZE_SMALL);
      lcd.clear();
      lcd.println("");
      lcd.println("Aproxime um cartao  ");
      lcd.println("para Adicionar ou ");
      lcd.println("Remover da EEPROM ");
      delay(2000);
      lcd.clear();
      lcd.println("  Aproxime o cartao");
      lcd.println("");
      lcd.println("---------------------");
      lcd.println("   Master sai da  ");
      lcd.println("    programacao ");
      lcd.println("---------------------");
      delay(2000);
    }
    else {
      if ( findID(readCard) ) { // If not, see if the card is in the EEPROM
        lcd.setFontSize(FONT_SIZE_SMALL);
        Serial.println(F("Bemvindo, voce pode passar."));
        lcd.clear();
        lcd.println("-----Bom Trabalho----");
        lcd.println("");
        lcd.println("---------------------");
        lcd.println("    Bemvindo,");
        lcd.println("      pode passar.");
        lcd.draw(tick, 16, 16);
        lcd.println(" -----------------.");
        delay(3000);
        lcd.clear();
        granted(300);         // Open the door lock for 300 ms
       } 
       else {      // If not, show that the ID was not valid
        Serial.println(F("Voce nao tem permissao para passar."));
        lcd.setFontSize(FONT_SIZE_SMALL);
        lcd.clear();
        lcd.println("  Ops.");
        lcd.println("");
        lcd.println("");
        lcd.println(" ...Acesso Negado!");
        delay(3000);
        lcd.clear();
        lcd.setCursor(40, 1);
        lcd.draw(smile, 48, 48);
        delay(1000);
        lcd.clear();
        denied();
       
      }
    }
  }
}

/////////////////////////////////////////  Access Granted    ///////////////////////////////////
void granted ( uint16_t setDelay){
  digitalWrite(blueLed, LED_OFF);   // Turn off blue LED
  digitalWrite(redLed, LED_OFF);  // Turn off red LED
  digitalWrite(greenLed, LED_ON);   // Turn on green LED
  digitalWrite(relay, HIGH);     // Unlock door!
  delay(setDelay);          // Hold door lock open for given seconds
  digitalWrite(relay, LOW);    // Relock door
  delay(1000);            // Hold green LED on for a second
}

///////////////////////////////////////// Access Denied  ///////////////////////////////////
void denied() {
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
  digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
  digitalWrite(redLed, LED_ON);   // Turn on red LED
  delay(1000);
}

////////////////////////////////////senha////////////////////////////////////////////////////
/*void tc(){
  
  digitalWrite(blueLed, LED_OFF);   // Turn off blue LED
  digitalWrite(redLed, LED_OFF);  // Turn off red LED
  digitalWrite(greenLed, LED_ON);   // Turn on green LED
  digitalWrite(relay, LOW);     // Unlock door!
  delay(300);          // Hold door lock open for given seconds
  digitalWrite(relay, HIGH);    // Relock door
  delay(1000);            // Hold green LED on for a second
  
  }
  */


///////////////////////////////////////// Get PICC's UID ///////////////////////////////////
uint8_t getID() {
  // Getting ready for Reading PICCs
  if ( ! MFRC522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! MFRC522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  Serial.println(F("Lendo PICC's UID:"));
  for ( uint8_t i = 0; i < 4; i++) {
    readCard[i] = MFRC522.uid.uidByte[i];
    Serial.print(readCard[i], HEX);
    /*lcd.print(readCard[i], HEX);
    delay(500);
    */
  }
  Serial.println("");
  MFRC522.PICC_HaltA(); // Stop reading
  return 1;
}

void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = MFRC522.PCD_ReadRegister(MFRC522.VersionReg);
  Serial.print(F("MFRC522 Software Version: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F(" (Desconhecido),provavelmente um clone chines?"));
   /* lcd.println("Desconhecido");
    delay(1000);
    lcd.clear();
    */
  Serial.println("");
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println(F("WARNING: comunicaçao falhou, e o MFRC522 esta conectado?"));
    Serial.println(F("SYSTEM HALTED: checando conecao."));
    lcd.println("warning");
    delay(1000);
    lcd.clear();
    
    // Visualize system is halted
    digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
    digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
    digitalWrite(redLed, LED_ON);   // Turn on red LED
    while (true); // do not go further
  }
}

///////////////////////////////////////// Cycle Leds (Program Mode) ///////////////////////////////////
void cycleLeds() {
  digitalWrite(redLed, LED_OFF);  // Make sure red LED is off
  digitalWrite(greenLed, LED_ON);   // Make sure green LED is on
  digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
  delay(200);
  digitalWrite(redLed, LED_OFF);  // Make sure red LED is off
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
  digitalWrite(blueLed, LED_ON);  // Make sure blue LED is on
  delay(200);
  digitalWrite(redLed, LED_ON);   // Make sure red LED is on
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
  digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
  delay(200);
}

//////////////////////////////////////// Normal Mode Led  ///////////////////////////////////
void normalModeOn () {
  digitalWrite(blueLed, LED_ON);  // Blue LED ON and ready to read card
  digitalWrite(redLed, LED_OFF);  // Make sure Red LED is off
  digitalWrite(greenLed, LED_OFF);  // Make sure Green LED is off
  digitalWrite(relay, LOW);    // Make sure Door is Locked
}

//////////////////////////////////////// Read an ID from EEPROM //////////////////////////////
void readID( uint8_t number ) {
  uint8_t start = (number * 4 ) + 2;    // Figure out starting position
  for ( uint8_t i = 0; i < 4; i++ ) {     // Loop 4 times to get the 4 Bytes
    storedCard[i] = EEPROM.read(start + i);   // Assign values read from EEPROM to array
  }
}

///////////////////////////////////////// Add ID to EEPROM   ///////////////////////////////////
void writeID( byte a[] ) {
  if ( !findID( a ) ) {     // Before we write to the EEPROM, check to see if we have seen this card before!
    uint8_t num = EEPROM.read(0);     // Get the numer of used spaces, position 0 stores the number of ID cards
    uint8_t start = ( num * 4 ) + 6;  // Figure out where the next slot starts
    num++;                // Increment the counter by one
    EEPROM.write( 0, num );     // Write the new count to the counter
    for ( uint8_t j = 0; j < 4; j++ ) {   // Loop 4 times
      EEPROM.write( start + j, a[j] );  // Write the array values to EEPROM in the right position
    }
    successWrite();
    Serial.println(F("sucesso em adicionar ID EEPROM"));
    lcd.println("sucesso ID adicionado");
    delay(1000);
    lcd.clear();
  }
  else {
    failedWrite();
    Serial.println(F("Falha! alguma coisa errada com ID ou erro EEPROM"));
    lcd.println("Falha! erro EEPROM");
    delay(1000);
    lcd.clear();
  }
}

///////////////////////////////////////// Remove ID from EEPROM   ///////////////////////////////////
void deleteID( byte a[] ) {
  if ( !findID( a ) ) {     // Before we delete from the EEPROM, check to see if we have this card!
    failedWrite();      // If not
    Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
    lcd.println("Falha! erro EEPROM");
    delay(1000);
    lcd.clear();
  }
  else {
    uint8_t num = EEPROM.read(0);   // Get the numer of used spaces, position 0 stores the number of ID cards
    uint8_t slot;       // Figure out the slot number of the card
    uint8_t start;      // = ( num * 4 ) + 6; // Figure out where the next slot starts
    uint8_t looping;    // The number of times the loop repeats
    uint8_t j;
    uint8_t count = EEPROM.read(0); // Read the first Byte of EEPROM that stores number of cards
    slot = findIDSLOT( a );   // Figure out the slot number of the card to delete
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;      // Decrement the counter by one
    EEPROM.write( 0, num );   // Write the new count to the counter
    for ( j = 0; j < looping; j++ ) {         // Loop the card shift times
      EEPROM.write( start + j, EEPROM.read(start + 4 + j));   // Shift the array values to 4 places earlier in the EEPROM
    }
    for ( uint8_t k = 0; k < 4; k++ ) {         // Shifting loop
      EEPROM.write( start + j + k, 0);
    }
    successDelete();
    Serial.println(F("Sucesso ao remover ID gravado da EEPROM"));
    lcd.println("Sucesso removido ID da EEPROM");
    delay(1000);
    lcd.clear();
  }
}

///////////////////////////////////////// Check Bytes   ///////////////////////////////////
bool checkTwo ( byte a[], byte b[] ) {   
  for ( uint8_t k = 0; k < 4; k++ ) {   // Loop 4 times
    if ( a[k] != b[k] ) {     // IF a != b then false, because: one fails, all fail
       return false;
    }
  }
  return true;  
}

///////////////////////////////////////// Find Slot   ///////////////////////////////////
uint8_t findIDSLOT( byte find[] ) {
  uint8_t count = EEPROM.read(0);       // Read the first Byte of EEPROM that
  for ( uint8_t i = 1; i <= count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);                // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      // is the same as the find[] ID card passed
      return i;         // The slot number of the card
    }
  }
}

///////////////////////////////////////// Find ID From EEPROM   ///////////////////////////////////
bool findID( byte find[] ) {
  uint8_t count = EEPROM.read(0);     // Read the first Byte of EEPROM that
  for ( uint8_t i = 1; i < count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);          // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      return true;
    }
    else {    // If not, return false
    }
  }
  return false;
}

///////////////////////////////////////// Write Success to EEPROM   ///////////////////////////////////
// Flashes the green LED 3 times to indicate a successful write to EEPROM
void successWrite() {
  digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
  digitalWrite(redLed, LED_OFF);  // Make sure red LED is off
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is on
  delay(200);
  digitalWrite(greenLed, LED_ON);   // Make sure green LED is on
  delay(200);
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
  delay(200);
  digitalWrite(greenLed, LED_ON);   // Make sure green LED is on
  delay(200);
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
  delay(200);
  digitalWrite(greenLed, LED_ON);   // Make sure green LED is on
  delay(200);
}

///////////////////////////////////////// Write Failed to EEPROM   ///////////////////////////////////
// Flashes the red LED 3 times to indicate a failed write to EEPROM
void failedWrite() {
  digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
  digitalWrite(redLed, LED_OFF);  // Make sure red LED is off
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
  delay(200);
  digitalWrite(redLed, LED_ON);   // Make sure red LED is on
  delay(200);
  digitalWrite(redLed, LED_OFF);  // Make sure red LED is off
  delay(200);
  digitalWrite(redLed, LED_ON);   // Make sure red LED is on
  delay(200);
  digitalWrite(redLed, LED_OFF);  // Make sure red LED is off
  delay(200);
  digitalWrite(redLed, LED_ON);   // Make sure red LED is on
  delay(200);
}

///////////////////////////////////////// Success Remove UID From EEPROM  ///////////////////////////////////
// Flashes the blue LED 3 times to indicate a success delete to EEPROM
void successDelete() {
  digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
  digitalWrite(redLed, LED_OFF);  // Make sure red LED is off
  digitalWrite(greenLed, LED_OFF);  // Make sure green LED is off
  delay(200);
  digitalWrite(blueLed, LED_ON);  // Make sure blue LED is on
  delay(200);
  digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
  delay(200);
  digitalWrite(blueLed, LED_ON);  // Make sure blue LED is on
  delay(200);
  digitalWrite(blueLed, LED_OFF);   // Make sure blue LED is off
  delay(200);
  digitalWrite(blueLed, LED_ON);  // Make sure blue LED is on
  delay(200);
}

////////////////////// Check readCard IF is masterCard   ///////////////////////////////////
// Check to see if the ID passed is the master programing card
bool isMaster( byte test[] ) {
	return checkTwo(test, masterCard);
}

bool monitorWipeButton(uint32_t interval) {
  uint32_t now = (uint32_t)millis();
  while ((uint32_t)millis() - now < interval)  {
    // check on every half a second
    if (((uint32_t)millis() % 500) == 0) {
      if (digitalRead(wipeB) != LOW)
        return false;
    }
  }
  return true;
}
