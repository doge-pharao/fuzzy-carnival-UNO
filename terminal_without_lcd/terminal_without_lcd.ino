#include <mcp_can.h>
#include <SPI.h>

// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 10;


MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin


void setup()
{
  Serial.begin(115200);

  while (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) != CAN_OK)              // init can bus : baudrate = 500k
  {
    Serial.println("CAN BUS Shield init fail");
    delay(100);
  }

  Serial.println("CAN BUS Shield init ok!");

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
      sprintf(msgString, "%s%s\n", msgString, buffStr);
    }
    Serial.print(msgString);

    msgString[0] = '\0';
  }

}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

