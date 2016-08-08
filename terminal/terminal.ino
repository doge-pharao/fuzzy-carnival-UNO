#include <mcp_can.h>
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

// The scrolling area must be a integral multiple of TEXT_HEIGHT
#define TEXT_HEIGHT 16 // Height of text to be printed and scrolled
#define BOT_FIXED_AREA 0 // Number of lines in bottom fixed area (lines counted from bottom of screen)
#define TOP_FIXED_AREA 16 // Number of lines in top fixed area (lines counted from top of screen)

// The initial y coordinate of the top of the scrolling area
uint16_t yStart = TOP_FIXED_AREA;
// yArea must be a integral multiple of TEXT_HEIGHT
uint16_t yArea = 320-TOP_FIXED_AREA-BOT_FIXED_AREA;
// The initial y coordinate of the top of the bottom text line
uint16_t yDraw = 320 - BOT_FIXED_AREA - TEXT_HEIGHT;

// Keep track of the drawing x coordinate
uint16_t xPos = 0;
// For the byte we read from the serial port
byte data = 0;

// We have to blank the top line each time the display is scrolled, but this takes up to 13 milliseconds
// for a full width line, meanwhile the serial buffer may be filling... and overflowing
// We can speed up scrolling of short text lines by just blanking the character we drew
int blank[19]; // We keep all the strings pixel lengths to optimise the speed of the top line blanking

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
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(1);
yDraw = scroll_line();
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
char msgString[80];
char buffStr[20];
char i;

void loop()
{


  if (CAN.checkReceive() == CAN_MSGAVAIL)           // check if data coming
  {

    CAN.readMsgBuf(&rxId, &len, buf);      // Read data: len = data length, buf = data byte(s)

    if ((rxId & 0x80000000) == 0x80000000)    // Determine if ID is standard (11 bits) or extended (29 bits)
      sprintf(msgString, "Ext ID:0x%.8lX  Len:%1d  Data:", (rxId & 0x1FFFFFFF), len);
    else
      sprintf(msgString, "Std ID:0x%.3lX  Len:%1d  Data:", rxId, len);

    if ((rxId & 0x40000000) == 0x40000000) {  // Determine if message is a remote request frame.
      sprintf(msgString, " REMOTE REQUEST FRAME");
      Serial.print(msgString);
    } else {
      buffStr[0]='\0';
      for (byte i = 0; i < len; i++) {
        sprintf(buffStr, "%s 0x%.2X", buffStr, buf[i]);
      }
      sprintf(msgString, "%s%s\r", msgString, buffStr);
    }
    Serial.print(msgString);

    

  for(char i=0; i<strlen(msgString); i++){
    if (msgString[i] == '\r' || xPos>231) {
      xPos = 0;
      yDraw = scroll_line(); // It takes about 13ms to scroll 16 pixel lines
    }
    if (msgString[i] > 31 && msgString[i] < 128) {
      tft.drawChar(xPos,yDraw,msgString[i], ILI9341_WHITE, ILI9341_BLACK, 1);
      xPos += 6;
      blank[(18+(yStart-TOP_FIXED_AREA)/TEXT_HEIGHT)%19]=xPos; // Keep a record of line lengths
    }
    //change_colour = 1; // Line to indicate buffer is being emptied
  }


    msgString[0] = '\0';
  }

}


// ##############################################################################################
// Call this function to scroll the display one text line
// ##############################################################################################
int scroll_line() {
  int yTemp = yStart; // Store the old yStart, this is where we draw the next line
  // Use the record of line lengths to optimise the rectangle size we need to erase the top line
  tft.fillRect(0,yStart,blank[(yStart-TOP_FIXED_AREA)/TEXT_HEIGHT],TEXT_HEIGHT, ILI9341_BLACK);

  // Change the top of the scroll area
  yStart+=TEXT_HEIGHT;
  // The value must wrap around as the screen memory is a circular buffer
  if (yStart >= 320 - BOT_FIXED_AREA) yStart = TOP_FIXED_AREA + (yStart - 320 + BOT_FIXED_AREA);
  // Now we can scroll the display
  scrollAddress(yStart);
  return  yTemp;
}

// ##############################################################################################
// Setup a portion of the screen for vertical scrolling
// ##############################################################################################
// We are using a hardware feature of the display, so we can only scroll in portrait orientation
void setupScrollArea(uint16_t TFA, uint16_t BFA) {
  tft.writecommand(ILI9341_VSCRDEF); // Vertical scroll definition
  tft.writedata(TFA >> 8);
  tft.writedata(TFA);
  tft.writedata((320-TFA-BFA)>>8);
  tft.writedata(320-TFA-BFA);
  tft.writedata(BFA >> 8);
  tft.writedata(BFA);
}

// ##############################################################################################
// Setup the vertical scrolling start address
// ##############################################################################################
void scrollAddress(uint16_t VSP) {
  tft.writecommand(ILI9341_VSCRSADD); // Vertical scrolling start address
  tft.writedata(VSP>>8);
  tft.writedata(VSP);
}
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

