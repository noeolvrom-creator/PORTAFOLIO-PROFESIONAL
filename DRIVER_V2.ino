#include <OneWire.h>
#include <DallasTemperature.h>
#include <ModbusEthernet.h> // comunicacion como servidor via tcp/ip con shield w5100

#define ONE_WIRE_BUS 17 // Pin de datos
//Modbus Registers Offsets (0-9999)
#define SENSOR_IREG 0  //para parte entera
#define SENSOR_IREG1 1  //para parte decimal
#define CMD 10
#define V1_IREG     2   // Parte entera de litros
#define V1_IREG1    3   // Parte decimal de litros

#define V2_IREG     4   // Parte entera de litros
#define V2_IREG1    5   // Parte decimal de litros

#define V3_IREG     6   // Parte entera de litros
#define V3_IREG1    7   // Parte decimal de litros

#define V4_IREG     8   // Parte entera de litros
#define V4_IREG1    9   // Parte decimal de litros



#define F1 3 // L1
#define F2 18
#define F3 19
#define F4 20

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

//ModbusEthernet object
ModbusEthernet mb;

float temp = 0.0;
float temp2 = 0.0;
float V_L1 = 0.00;
float V_L1_P2 = 0.0;
float V_L2 = 0.0;
float V_L2_P2 = 0.0;
float V_L3 = 0.0;
float V_L3_P2 = 0.0;
float V_L4 = 0.0;
float V_L4_P2 = 0.0;
float pulsosacc1 = 0.0;
float pulsosacc2 = 0.0;
float pulsosacc3 = 0.0;
float pulsosacc4 = 0.0;
unsigned long currentMillis = 0;
float cConstant = 300.0;
int instruction = 0; // necesito recibir este dato como cliente


unsigned long lastRequestTime = 0;
const unsigned long conversionDelay = 750; // máximo para 12 bits
bool conversionInProgress = false;


void  flujo() {
  pulsosacc1++;
}

void  flujo1() {
  pulsosacc2++;
}

void  flujo2() {
  pulsosacc3++;
}

void  flujo3() {
  pulsosacc4++;
}

void setup() {
  delay(1000);
  //Serial.begin(2000000);
  pinMode(F1, INPUT_PULLUP);
  pinMode(F2, INPUT_PULLUP);
  pinMode(F3, INPUT_PULLUP);
  pinMode(F4, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(F1), flujo, RISING);
  attachInterrupt(digitalPinToInterrupt(F2), flujo1, RISING);
  attachInterrupt(digitalPinToInterrupt(F3), flujo2, RISING);
  attachInterrupt(digitalPinToInterrupt(F4), flujo3, RISING);
  sensors.begin();
  sensors.setWaitForConversion(false);

  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  
  // The IP address for the shield
  byte ip[] = { 192, 168, 1, 55};   
  // Config Modbus TCP 
  mb.config(mac, ip);

  // Add SENSOR_IREG register - Use addIreg() for analog Inputs
  mb.addIreg(SENSOR_IREG);
  mb.addIreg(SENSOR_IREG1);

  mb.addIreg(V1_IREG);
  mb.addIreg(V1_IREG1);

  mb.addIreg(V2_IREG);
  mb.addIreg(V2_IREG1);

  mb.addIreg(V3_IREG);
  mb.addIreg(V3_IREG1);

  mb.addIreg(V4_IREG);
  mb.addIreg(V4_IREG1);

  mb.addHreg(CMD);
}

void loop() {
  mb.task();
  instruction = mb.Hreg(CMD);
  readCelsius();
  
  // Aquí puedes realizar otras tareas sin bloqueo
  readF1();
  readF2();
  readF3();
  readF4();
}

void readCelsius(){
  currentMillis = millis();

  if (!conversionInProgress) {
    sensors.requestTemperatures(); // Inicia conversión
    lastRequestTime = currentMillis;
    conversionInProgress = true;
  }

  // Espera el tiempo necesario antes de leer
  if (conversionInProgress && currentMillis - lastRequestTime >= conversionDelay) {
    temp = sensors.getTempCByIndex(0); // Leer temperatura
    //Serial.print("T: ");
    //Serial.println(temp);
    temp2 = (temp - int(temp))*100.0;
    //Setting raw value (0-1024)
    mb.Ireg(SENSOR_IREG, int(temp));
    mb.Ireg(SENSOR_IREG1, int(temp2));
    //Serial.println(temp);
    conversionInProgress = false;
  }
}

void readF1() {
  // Simulación de otras tareas del programa
  V_L1  = pulsosacc1 / cConstant;
  V_L1_P2 = (V_L1 - int(V_L1))*100.0;
  mb.Ireg(V1_IREG, int(V_L1));
  mb.Ireg(V1_IREG1, int(V_L1_P2));
  //Serial.print("1:");
  //Serial.println(V_L1);
}

void readF2() {
  // Simulación de otras tareas del programa
  V_L2  = pulsosacc2 / cConstant;
  V_L2_P2 = (V_L2 - int(V_L2))*100.0;
  mb.Ireg(V2_IREG, int(V_L2));
  mb.Ireg(V2_IREG1, int(V_L2_P2));
  //Serial.print("2:");
  //Serial.println(V_L2);
}

void readF3() {
  // Simulación de otras tareas del programa
  V_L3  = pulsosacc3 / cConstant;
  V_L3_P2 = (V_L3 - int(V_L3))*100.0;
  mb.Ireg(V3_IREG, int(V_L3));
  mb.Ireg(V3_IREG1, int(V_L3_P2));
  //Serial.print("3:");
  //Serial.println(V_L3);
}

void readF4() {
  // Simulación de otras tareas del programa
  V_L4  = pulsosacc4 / cConstant;
  V_L4_P2 = (V_L4 - int(V_L4))*100.0;
  mb.Ireg(V4_IREG, int(V_L4));
  mb.Ireg(V4_IREG1, int(V_L4_P2));
  //Serial.print("4:");
  //Serial.println(V_L4);
}



