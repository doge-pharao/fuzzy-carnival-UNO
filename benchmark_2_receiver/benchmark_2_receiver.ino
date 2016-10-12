// demo: CAN-BUS Shield, send data
#include <mcp_can.h>
#include <SPI.h>

#define TRAC_BATT_VOLTAGE 0           // Traction Battery Voltage
#define TRAC_BATT_CURR 1              // Traction Battery Current
#define AUX_BATT_TEMP 2               // Traction Battery Temp, Average
#define AUX_BATT_VOLTAGE 3            // Auxiliary Battery Voltage
#define TRAC_BATT_TEMP 4              // Traction Battery Temp, Max,
#define AUX_BATT_CURR 5               // Auxiliary Battery Current
#define ACC_POSITION 6                // Accelerator Position
#define BRAKE_PRESURE_CYL 7           // Brake Pressure, Master Cylinder
#define BRACKE_PRESURE_LINE 8         // Brake Pressure, Line
#define VEHICLE_SPEED 9              // Vehicle Speed

#define TRAC_BATT_VOLTAGE_SIGNAL 1           // Traction Battery Voltage
#define TRAC_BATT_CURR_SIGNAL 2              // Traction Battery Current
#define AUX_BATT_TEMP_SIGNAL 4              // Traction Battery Temp, Average
#define AUX_BATT_VOLTAGE_SIGNAL 8           // Auxiliary Battery Voltage
#define TRAC_BATT_TEMP_SIGNAL 16             // Traction Battery Temp, Max,
#define AUX_BATT_CURR_SIGNAL 32             // Auxiliary Battery Current
#define ACC_POSITION_SIGNAL 64               // Accelerator Position
#define BRAKE_PRESURE_CYL_SIGNAL 128          // Brake Pressure, Master Cylinder

#define BRACKE_PRESURE_LINE_SIGNAL 64        // Brake Pressure, Line
#define VEHICLE_SPEED_SIGNAL 128              // Vehicle Speed

#define MAX_MESSAGES 10
#define MAX_BOXES 10

volatile boolean tick = false; // La definimos como volatile
const byte signals_for_a[8] = { TRAC_BATT_VOLTAGE_SIGNAL, TRAC_BATT_CURR_SIGNAL, AUX_BATT_TEMP_SIGNAL, AUX_BATT_VOLTAGE_SIGNAL, TRAC_BATT_TEMP_SIGNAL, AUX_BATT_CURR_SIGNAL, ACC_POSITION_SIGNAL, BRAKE_PRESURE_CYL_SIGNAL };
const byte signals_for_c[2] = { BRACKE_PRESURE_LINE_SIGNAL, VEHICLE_SPEED_SIGNAL };

byte data[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
byte order[500];
const byte long_data[MAX_BOXES] = { 4, 8, 8, 8, 8, 8, 1, 8, 8, 8};
const byte id[MAX_BOXES] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA };
const int period[MAX_BOXES] = { 100, 10, 10, 10, 10, 10, 10, 10, 10, 20 };
const int deathline[MAX_BOXES] = { 5, 10, 10, 10, 10, 10, 10, 10, 10, 20 };
long times[MAX_BOXES][MAX_MESSAGES];
unsigned int ok_messages[MAX_BOXES], fail_messages[MAX_BOXES];
bool complete[MAX_BOXES];

boolean ready_to_send[MAX_BOXES];

unsigned int current_time[MAX_BOXES];
byte message;
unsigned long start_send;
int i, j;

byte signal_to_send;
byte signal_from_port_a, signal_from_port_c;

// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 10;
MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin


void setup()
{
  Serial.begin(115200);

  while (CAN_OK != CAN.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ))              // init can bus : baudrate = 500k
  {
    Serial.println("CAN BUS Shield init fail");
    Serial.println(" Init CAN BUS Shield again");
    delay(100);
  }
  CAN.setMode(MCP_NORMAL);
  Serial.println("CAN BUS Shield init ok!");

  for (i = 0; i < MAX_BOXES; i++) {
    ready_to_send[i] = false;
    current_time[i] = 0;
    ok_messages[i] = 0;
    fail_messages[i] = 0;
    complete[i] = false;

    for (j = 0; j < MAX_MESSAGES; j++)
      times[i][j] = 0;
  }

  signal_from_port_a = 0;
  signal_from_port_c = 0;

  //PORT A FOR SEGMENT 1
  DDRA = B00000000; //initialize port pins as output 22-29
  
  //PORT C FOR SEGMENT 2
  DDRC = B00000000; //initialize port pins as output 37-30

  PORTA = B0000000;
  PORTC = B0000000;

  pinMode(48, OUTPUT);
}

void loop()
{
  unsigned long start = millis();
  unsigned long current = millis();
  unsigned long start_tx, delay_tx, total_delay_tx = 0;
  byte complete_amount = 0, sndStat;
  unsigned int count_messages = 0;
  unsigned long mytick = 0;
  byte previous_a, previous_c;

  digitalWrite(48, HIGH);
  digitalWrite(48, LOW);
  
  while (mytick < 10000 && complete_amount < MAX_BOXES) {
    signal_from_port_a = PINA;
    signal_from_port_c = PINC;

    PINA = 0x00;
    PINB = 0x00;
    
    if (signal_from_port_a) {
        for (i = 0; i < 8; i++) {
          if (!ready_to_send[i] && (!complete[i]) && (signal_from_port_a & signals_for_a[i])){
            ready_to_send[i] = true;
            times[i][ok_messages[i] + fail_messages[i]] = micros();
          }
        }
        signal_from_port_a = 0x00;
     }
     
    if (signal_from_port_c) {
      for (i = 8; i < MAX_BOXES; i++) {
          if (!ready_to_send[i] && (!complete[i]) && (signal_from_port_c & signals_for_c[i])){
            ready_to_send[i] = true;
            times[i][ok_messages[i] + fail_messages[i]] = micros();
          }         
      }
    }  

    i = 0;
    while (i < MAX_BOXES && !ready_to_send[i]) {
      i++;
    }

    if (i < MAX_BOXES) {
      message = i;
      ready_to_send[message] = false;
      order[count_messages] = message;
      count_messages++; 
    }
    else
      message = 255;
    
    switch (message) {
      case (TRAC_BATT_VOLTAGE): {
          start_send = times[TRAC_BATT_VOLTAGE][ok_messages[TRAC_BATT_VOLTAGE] + fail_messages[TRAC_BATT_VOLTAGE]];

          sndStat = CAN.sendMsgBuf(id[TRAC_BATT_VOLTAGE], 0, long_data[TRAC_BATT_VOLTAGE], data);

          times[TRAC_BATT_VOLTAGE][ok_messages[TRAC_BATT_VOLTAGE] + fail_messages[TRAC_BATT_VOLTAGE]] = (micros() - start_send);
        }
        break;
      case (TRAC_BATT_CURR): {
          start_send = times[TRAC_BATT_CURR][ok_messages[TRAC_BATT_CURR] + fail_messages[TRAC_BATT_CURR]];

          sndStat = CAN.sendMsgBuf(id[TRAC_BATT_CURR], 0, long_data[TRAC_BATT_CURR], data);

          times[TRAC_BATT_CURR][ok_messages[TRAC_BATT_CURR] + fail_messages[TRAC_BATT_CURR]] =  (micros() - start_send) ;
        }
        break;
      case (AUX_BATT_TEMP): {
          start_send = times[AUX_BATT_TEMP][ok_messages[AUX_BATT_TEMP] + fail_messages[AUX_BATT_TEMP]];

          sndStat = CAN.sendMsgBuf(id[AUX_BATT_TEMP], 0, long_data[AUX_BATT_TEMP], data);

          times[AUX_BATT_TEMP][ok_messages[AUX_BATT_TEMP] + fail_messages[AUX_BATT_TEMP]] =  (micros() - start_send);
        }
        break;
      case (AUX_BATT_VOLTAGE): {
          start_send = times[AUX_BATT_VOLTAGE][ok_messages[AUX_BATT_VOLTAGE] + fail_messages[AUX_BATT_VOLTAGE]];

          sndStat = CAN.sendMsgBuf(id[AUX_BATT_VOLTAGE], 0, long_data[AUX_BATT_TEMP], data);

          times[AUX_BATT_VOLTAGE][ok_messages[AUX_BATT_VOLTAGE] + fail_messages[AUX_BATT_VOLTAGE]] =  (micros() - start_send);
        }
        break;
      case (TRAC_BATT_TEMP): {
          start_send = times[TRAC_BATT_TEMP][ok_messages[TRAC_BATT_TEMP] + fail_messages[TRAC_BATT_TEMP]];

          sndStat = CAN.sendMsgBuf(id[TRAC_BATT_TEMP], 0, long_data[TRAC_BATT_TEMP], data);

          times[TRAC_BATT_TEMP][ok_messages[TRAC_BATT_TEMP] + fail_messages[TRAC_BATT_TEMP]] =  (micros() - start_send);
        }
        break;
      case (AUX_BATT_CURR): {
          start_send = times[AUX_BATT_CURR][ok_messages[AUX_BATT_CURR] + fail_messages[AUX_BATT_CURR]];

          sndStat = CAN.sendMsgBuf(id[AUX_BATT_CURR], 0, long_data[AUX_BATT_CURR], data);

          times[AUX_BATT_CURR][ok_messages[AUX_BATT_CURR] + fail_messages[AUX_BATT_CURR]] =  (micros() - start_send);
        }
        break;
      case (ACC_POSITION): {
          start_send = times[ACC_POSITION][ok_messages[ACC_POSITION] + fail_messages[ACC_POSITION]];

          sndStat = CAN.sendMsgBuf(id[ACC_POSITION], 0, long_data[ACC_POSITION], data);

          times[ACC_POSITION][ok_messages[ACC_POSITION] + fail_messages[ACC_POSITION]] =  (micros() - start_send);
        }
        break;
      case (BRAKE_PRESURE_CYL): {
          start_send = times[BRAKE_PRESURE_CYL][ok_messages[BRAKE_PRESURE_CYL] + fail_messages[BRAKE_PRESURE_CYL]];

          sndStat = CAN.sendMsgBuf(id[BRAKE_PRESURE_CYL], 0, long_data[BRAKE_PRESURE_CYL], data);

          times[BRAKE_PRESURE_CYL][ok_messages[BRAKE_PRESURE_CYL] + fail_messages[BRAKE_PRESURE_CYL]] = (micros() - start_send);
        }
        break;
      case (BRACKE_PRESURE_LINE): {
          start_send = times[BRACKE_PRESURE_LINE][ok_messages[BRACKE_PRESURE_LINE] + fail_messages[BRACKE_PRESURE_LINE]];

          sndStat = CAN.sendMsgBuf(id[BRACKE_PRESURE_LINE], 0, long_data[BRACKE_PRESURE_LINE], data);

          times[BRACKE_PRESURE_LINE][ok_messages[BRACKE_PRESURE_LINE] + fail_messages[BRACKE_PRESURE_LINE]] = (micros() - start_send);
        }
        break;
      case (VEHICLE_SPEED): {
          start_send = times[VEHICLE_SPEED][ok_messages[VEHICLE_SPEED] + fail_messages[VEHICLE_SPEED]];

          sndStat = CAN.sendMsgBuf(id[VEHICLE_SPEED], 0, long_data[VEHICLE_SPEED], data);

          times[VEHICLE_SPEED][ok_messages[VEHICLE_SPEED] + fail_messages[VEHICLE_SPEED]] = (micros() - start_send);
        }
        break;
      default:
        sndStat = 255;
        delay_tx = 0;
        break;
    }

    if (sndStat != 255) {
      if (sndStat == CAN_OK) {
        ok_messages[message]++;
      } else {
        if (sndStat == CAN_SENDMSGTIMEOUT) {
          fail_messages[message]++;
        }
      }
      if (ok_messages[message] + fail_messages[message] == MAX_MESSAGES) {
        complete[message] = true;
        complete_amount++;
      }
    }
    current = millis();
    total_delay_tx = total_delay_tx + delay_tx;
  }

  Serial.println("Benchmark");
  char time_lecture[8];
  byte message_number[MAX_BOXES];
  
  float tmp;
  byte current_box;
  for (i = 0; i < MAX_BOXES; i++)
    message_number[i] = 0;
    
  for (i = 0; i < MAX_MESSAGES * MAX_BOXES; i++) {
    Serial.print(i);
    Serial.print(",");
    current_box = order[i];
    
    for (j = 0; j < MAX_BOXES; j++) {
      if (current_box != j) {
        Serial.print(0);  
      } 
      else {
        if (ready_to_send[current_box] && (times[current_box][message_number[current_box]]) == 0)
            Serial.print(-2);
          else {
            tmp = times[current_box][message_number[current_box]] / 1000.0;
            serialPrintFloat(tmp);
          } 
      }
      if ( j < MAX_BOXES - 1)
        Serial.print(",");
    }

    message_number[current_box]++;  
    Serial.println();
  }
  Serial.println("----");
  for (i = 0; i < 10; i++) {
    Serial.print("OK:"); Serial.print(ok_messages[i]);
    Serial.print(" Fail:"); Serial.print(fail_messages[i]);
    Serial.print(" Total:"); Serial.println(ok_messages[i] + fail_messages[i]);
  }
  Serial.print("Average delay on the transmition: ");
  serialPrintFloat((total_delay_tx / 500)/1000);
  Serial.println("Done");
  while (true);
}

void serialPrintFloat( float f){
  Serial.print((int)f);
  Serial.print(".");
  int temp = (f - (int)f) * 1000;
  Serial.print( abs(temp) ); 
}
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
