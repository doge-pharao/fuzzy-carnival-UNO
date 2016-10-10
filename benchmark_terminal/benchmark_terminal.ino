#include <mcp_can.h>
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

// Keep track of the drawing x coordinate
uint16_t xPos = 10;
uint16_t yPos = 20;
// For the byte we read from the serial port
byte data = 0;

// For the Adafruit shield
#define TFT_MOSI 11
#define TFT_CLK 13
#define TFT_RST 8
#define TFT_MISO 12
#define TFT_DC 7
#define TFT_CS 9

#define ILI9341_VSCRDEF 0x33
#define ILI9341_VSCRSADD 0x37

// If using the breakout, change pins as desired
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
//Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 10;


MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin


void setup()
{
  Serial.begin(115200);

  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  // read diagnostics (optional but can help debug problems)
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);

  while (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) != CAN_OK)              // init can bus : baudrate = 500k
  {
    Serial.println("CAN BUS Shield init fail");
    tft.println("CAN BUS Shield init fail");

    delay(100);
  }

  Serial.println("CAN BUS Shield init ok!");
  tft.println("CAN BUS Shield init ok!");

  CAN.setMode(MCP_NORMAL);                     // Set operation mode to normal so the MCP2515 sends acks to received data.
}


unsigned char len = 0;
unsigned char buf[8];
long unsigned int rxId;
char idString[40];
char buffStr[60];
char i;

void loop()
{

  while(true) {
    if (CAN.checkReceive() == CAN_MSGAVAIL)           // check if data coming
    {
      CAN.readMsgBuf(&rxId, &len, buf);      // Read data: len = data length, buf = data byte(s)

      //sprintf(idString, "ID:0x%.3lX", rxId);
      //Serial.println(idString);

      if (xPos>55) {
        xPos = 10;
        yPos = yPos + 5; // It takes about 13ms to scroll 16 pixel lines
      }
      tft.drawPixel(xPos,yPos, ILI9341_WHITE);
      xPos += 5;
  }

  }

}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

