#include <TimerOne.h>

#define TRAC_BATT_VOLTAGE 4           // Traction Battery Voltage
#define TRAC_BATT_CURR 8              // Traction Battery Current
#define AUX_BATT_TEMP 16              // Traction Battery Temp, Average
#define AUX_BATT_VOLTAGE 32           // Auxiliary Battery Voltage
#define TRAC_BATT_TEMP 64             // Traction Battery Temp, Max,
#define AUX_BATT_CURR 128             // Auxiliary Battery Current

#define ACC_POSITION 1               // Accelerator Position
#define BRAKE_PRESURE_CYL 2          // Brake Pressure, Master Cylinder
#define BRACKE_PRESURE_LINE 4        // Brake Pressure, Line
#define VEHICLE_SPEED 8              // Vehicle Speed

#define MAX_MESSAGES 10
#define MAX_BOXES 10

volatile boolean tick = false; // La definimos como volatile

const byte signals_for_b[4] = { ACC_POSITION, BRAKE_PRESURE_CYL, BRACKE_PRESURE_LINE, VEHICLE_SPEED };
const byte signals_for_d[6] = { TRAC_BATT_VOLTAGE, TRAC_BATT_CURR, AUX_BATT_TEMP, AUX_BATT_VOLTAGE, TRAC_BATT_TEMP, AUX_BATT_CURR };
const int period[MAX_BOXES] = { 30, 100, 100, 100, 100, 100, 100, 100, 200, 300 };
unsigned int ok_messages[MAX_BOXES], fail_messages[MAX_BOXES];
bool complete[MAX_BOXES];
boolean ready_to_send[MAX_BOXES];
unsigned int current_time[MAX_BOXES];

byte signal_to_send;
byte signal_to_port_b, signal_to_port_d;

byte i, j;


void ISR_Tick()
{
  tick = true;     // Contador
}

void setup()
{
  Serial.begin(115200);

  Timer1.initialize(1000);         // Dispara cada 1 ms
  Timer1.attachInterrupt(ISR_Tick); // Activa la interrupcion y la asocia a ISR_Blink

  for (i = 0; i < MAX_BOXES; i++) {
    ready_to_send[i] = false;
    current_time[i] = 0;
    ok_messages[i] = 0;
    fail_messages[i] = 0;
    complete[i] = false;
  }

  signal_to_port_b = 0;
  signal_to_port_d = 0;

  //PORT A FOR SEGMENT 1
  DDRB = B11111111; //initialize port pins as output 22-29
  
  //PORT C FOR SEGMENT 2
  DDRD = B11111111; //initialize port pins as output 37-30

  PORTB = B0000000;
  PORTD = B0000000;
}

void loop()
{
  byte complete_amount = 0, sndStat;
  unsigned long mytick = 0;

  pinMode(12, INPUT_PULLUP);

  Serial.println("Esperando Placa CAN");
  while(!digitalRead(12));
  Serial.println("Enviando señales");
  
  while (mytick < 10000 && complete_amount < MAX_BOXES) {

    if (tick) {
      signal_to_port_b = 0;
      signal_to_port_d = 0;
      
      mytick ++;
      tick = false;

      for (i = 0; i < MAX_BOXES; i++) {
        if (ok_messages[i] + fail_messages[i] < MAX_MESSAGES && !complete[i]) {
          current_time[i]++;
          if (!ready_to_send[i]) {
            if (current_time[i] >= period[i]) {
              current_time[i] = 0;
              ready_to_send[i] = true;
            }
          } 
        } else {
          if (!complete[i]) {
            complete[i] = true;
            complete_amount++;
          }
        }
      }
    }

    for(i = 0; i < 6; i++){
      if (ready_to_send[i]){
        signal_to_port_d = signal_to_port_d | signals_for_d[i];
        ready_to_send[i] = false;
      }
    }
    for(i = 6; i < 10; i++){
      if (ready_to_send[i]){
        signal_to_port_b = signal_to_port_b | signals_for_b[i-6];
        ready_to_send[i] = false;
      }
    }
    
    PORTB = signal_to_port_b;
    PORTD = signal_to_port_d;
  }

  Serial.println("Done");

}

/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
