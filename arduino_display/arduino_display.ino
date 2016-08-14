#include <mcp_can.h>
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

#define CUSTOM_ILI9341_ORANGE      0xFC60
#define ORANGE_VALUE 0x1
#define YELLOW_VALUE 0x2
#define GREEN_VALUE 0x4
#define BLUE_VALUE 0x8

// For the Adafruit shield
#define TFT_MOSI 11
#define TFT_CLK 13
#define TFT_RST 8
#define TFT_MISO 12
#define TFT_DC 7
#define TFT_CS 9

// If using the breakout, change pins as desired
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
//Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 10;


MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin

int16_t oldValue;
bool firstTime;

void setup()
{
  Serial.begin(115200);

  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(1);

  while (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) != CAN_OK)              // init can bus : baudrate = 500k
  {
    Serial.println("CAN BUS Shield init fail");
    tft.println("CAN BUS Shield init fail");

    delay(100);
  }

  Serial.println("CAN BUS Shield init ok!");
  tft.println("CAN BUS Shield init ok!");

  CAN.setMode(MCP_NORMAL);                     // Set operation mode to normal so the MCP2515 sends acks to received data.

  tft.drawRect(189, 29, 42, 202, ILI9341_WHITE);
  tft.drawRect(188, 28, 44, 203, ILI9341_WHITE);

  tft.drawCircle(45, 280, 20, ILI9341_WHITE);
  tft.drawCircle(95, 280, 20, ILI9341_WHITE);
  tft.drawCircle(145, 280, 20, ILI9341_WHITE);
  tft.drawCircle(195, 280, 20, ILI9341_WHITE);

  char *message = "DAC Value";
  for (char i = 0; i < strlen(message); i++) {
    tft.drawChar(35 + (i * 16), 160, message[i], ILI9341_WHITE, ILI9341_BLACK, 2);
  }
  oldValue = 0x0001;

  updateBarStatus(0x0000);
}

uint16_t barColor;


void updateLedStatus(uint8_t status) {
  uint16_t blueStatus = ((status & BLUE_VALUE) ? ILI9341_BLUE : ILI9341_BLACK);
  uint16_t greenStatus = ((status & GREEN_VALUE) ? ILI9341_GREEN : ILI9341_BLACK);
  uint16_t yellowStatus = ((status & YELLOW_VALUE) ? ILI9341_YELLOW : ILI9341_BLACK);
  uint16_t orangeStatus = ((status & ORANGE_VALUE) ? ILI9341_RED : ILI9341_BLACK);

  tft.fillCircle(45, 280, 19, blueStatus);
  tft.fillCircle(95, 280, 19, greenStatus);
  tft.fillCircle(145, 280, 19, yellowStatus);
  tft.fillCircle(195, 280, 19, orangeStatus);
}

void updateBarStatus(uint16_t value) {
  char message[16];
  uint16_t graphValue = (value / 0x0147);

  tft.fillRect(190, 30, 40, 200 - graphValue, ILI9341_BLACK);
  tft.fillRect(190, 230 - graphValue, 40, graphValue , ILI9341_BLUE);

  tft.fillRect(50, 230, 130, 16, ILI9341_BLACK);
  sprintf(message, "0x%.4X", value);
  for (char i = 0; i < strlen(message); i++) {
    tft.drawChar(60 + (i * 16), 180, message[i], ILI9341_WHITE, ILI9341_BLACK, 2);
  }

}


unsigned char len = 0;
unsigned char buf[8];
long unsigned int rxId;
char msgString[80];
uint8_t errorCountRX, errorCountTX, errorMask;
uint8_t oldErrCountRX, oldErrCountTX, oldErrMask;

void loop()
{


  errorCountRX = CAN.errorCountRX();
  if (errorCountRX != 0 && oldErrCountRX != errorCountRX) {
    oldErrCountRX = errorCountRX;
    sprintf(msgString, "Error count RX: %d", errorCountRX);
    Serial.println(msgString);
  }

  errorCountTX = CAN.errorCountTX();
  if (errorCountTX != 0 && oldErrCountTX != errorCountTX) {
    oldErrCountTX = errorCountTX;
    sprintf(msgString, "Error count TX: %d", errorCountTX);
    Serial.println(msgString);
  }

  errorMask = CAN.getError();
  if (errorMask != 0 && oldErrMask != errorMask) {
    oldErrMask = errorMask;
    sprintf(msgString, "Error mask TX: %x", errorMask);
    Serial.println(msgString);
  }

  if (CAN.checkReceive() == CAN_MSGAVAIL)           // check if data coming
  {

    CAN.readMsgBuf(&rxId, &len, buf);      // Read data: len = data length, buf = data byte(s)

    if ((rxId & 0x80000000) == 0x80000000)    // Determine if ID is standard (11 bits) or extended (29 bits)
      sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d  Data:", (rxId & 0x1FFFFFFF), len);
    else
      sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data:", rxId, len);

    Serial.print(msgString);

    if ((rxId & 0x40000000) == 0x40000000) {  // Determine if message is a remote request frame.
      sprintf(msgString, " REMOTE REQUEST FRAME");
      Serial.println(msgString);
    } else {
      for (byte i = 0; i < len; i++) {
        sprintf(msgString, " 0x%.2X", buf[i]);
        Serial.print(msgString);
      }
      Serial.println();
    }

    switch (rxId) {
      case 0x070:
        updateLedStatus(buf[0]);
        break;
        
      case 0x071: { 
          uint16_t value = 0x0000;
          value = buf[1];
          value = value << 8;
          value = value | buf[0];
  
          if (value != oldValue) {
            oldValue = value;
            updateBarStatus(value);
          }
          sprintf(msgString, "DAC 0x%.4X", value);
          Serial.println(msgString);
        }
        break;
        
      default:
        break;
    }

  }
}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

