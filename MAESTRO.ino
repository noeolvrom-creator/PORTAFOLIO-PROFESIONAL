#include "robot_model.h" // en esta libreria estan los modelos matemáticos del robot (MCI Y MCD)
#include <Wire.h> // libreria para la comunicacion i2c
#include <math.h>

// Dirección del esclavo (PIC)
#define Q1 0x30
#define Q2 0x40
#define Q3 0x50
bool state = 1;

// DATOS PIC ->0ARDUINO (15 FLOTANTES)
float *u = (float*)malloc(3*sizeof(float)); // ESTE LO MANDA PIC
float *corriente = (float*)malloc(3*sizeof(float)); // RSTE LO MANDA PIC
float *pos = (float*)malloc(3*sizeof(float)); // RSTE LO MANDA PIC

// DATOS ARDUINO -> PIC (1 INT16 Y 3 FLOTANTES)
int16_t ejecucion = 98; // SE INGRESA EN LA INTERFAZ Y SE ENVIA
 float *articulacion = (float*)malloc(3*sizeof(float)); // ESTE DATO LO CALCULA ARDUIMO Y LO ENVIA

// DATOS QUE SE PUEDEN CALCULAR
float *coordenada = (float*)malloc(3*sizeof(float)); // SE INGRESA EN LA INTERFAZ
float *c_real = (float*)malloc(3*sizeof(float)); // SE INGRESA EN LA INTERFAZ
float *error = (float*)malloc(3*sizeof(float)); // ESTE LO CALCULA ARDUINO
uint16_t *porcentaje = (uint16_t*)malloc(3*sizeof(uint16_t)); // ESTE LO CALCULA ARDUINO

String refstring = "";
// banderas de recepcion de datos
uint16_t ind = 0;  
int flagx = 0; 
int flagy = 0; 
int flagz = 0; 
int flagt = 0; 

void setup() {
  // put your setup code here, to run once:
  Serial.begin(2000000); // Configura el puerto serial
  Wire.begin(); // Iniciar Arduino como maestro
  Wire.setClock(400000); // se configura la comunicacion i2c en modo rapido (400khz)
}

void loop() {
  //envio de datos, solo si se reciben los datos completos de la interfaz
  if(flagx && flagy && flagz && flagt){
    flagx = 0;
    flagy = 0;
    flagz = 0;
    flagt = 0;
    // calcular el ángulo de las articulaciones mediante las corrdenadas
    articulacion = MCI(coordenada);
    // si las coordenadas no son alcanzables por el robot, se envian las articulaciones en posición de reposo
    if (isnan(articulacion[0]) || isnan(articulacion[1]) || isnan(articulacion[2])) {
      coordenada[0] = 0.21; // Restablece a reposo si es NaN
      coordenada[1] = 0; // Restablece a reposo si es NaN
      coordenada[2] = 0.088; // Restablece a reposo si es NaN
      articulacion = MCI(coordenada);
      }
    // se envian los angulos de las articulaciones, a cada uno de los esclavos
    sendData(Q1,ejecucion,articulacion[0]);
    sendData(Q2,ejecucion,articulacion[1]);
    sendData(Q3,ejecucion,articulacion[2]);
  }
  // se reciben los datos que retornan los esclavos
  receiveData(Q1,0);
  receiveData(Q2,1);
  receiveData(Q3,2);
  //calculo de variables
  // error entre posicion real y retroalimentada
  error[0] = articulacion[0] - pos[0];
  error[1] = articulacion[1] - pos[1];
  error[2] = articulacion[2] - pos[2];
  // porcentaje de PWM en los motores 
  porcentaje[0] = (int)((abs(u[0]) / 1023) * 100);
  porcentaje[1] = (int)((abs(u[1]) / 1023) * 100);
  porcentaje[2] = (int)((abs(u[2]) / 1023) * 100);
  // calculo de coordenadas reales
  c_real = MCD(pos);
  // actualizacion de valores en la interfaz de usuario
  updtadeData();
}
void serialEvent(){
  while (Serial.available()) {
    String command = Serial.readStringUntil('\n'); // Leer datos enviados desde PyQt
    command.trim();
    // Separar comando y valor
      int delimiterIndex = command.indexOf(':');
      if (delimiterIndex > 0) {
        String cmd = command.substring(0, delimiterIndex);
        String value = command.substring(delimiterIndex + 1);
        // se activan las banderas si los datos llegaron correctamente
        if (cmd == "X") {
          flagx = 1;
          coordenada[0] = value.toFloat();
        } else if (cmd == "Y") {
          flagy = 1;
          coordenada[1] = value.toFloat();
        } else if (cmd == "Z") {
          flagz = 1;
          coordenada[2] = value.toFloat();
        } else if (cmd == "T") {
          flagt = 1;
          ejecucion = value.toInt();
        } 
      }
  }
}
// función para enviar los datos al PIC
void sendData(int s,uint16_t time,float angle){
  Wire.setWireTimeout(30000, true); // Tiempo de espera en microsegundos
  Wire.beginTransmission(s);
  Wire.write(highByte(time)); // Enviar byte alto
  Wire.write(lowByte(time));  // Enviar byte bajo*/
  byte* data = (byte*)&angle; // Convertir flotante a bytes
  for (int i = 0; i < 4; i++) {
    Wire.write(data[i]); // Enviar cada byte del flotante
  }
  Wire.endTransmission(s);
}
// Función para recibir datos desde el esclavo I2C
void receiveData(int s,int index_s) {
  byte data[3 * sizeof(float)];  // Buffer para recibir los bytes
  Wire.setWireTimeout(30000, true); // Tiempo de espera en microsegundos
  Wire.requestFrom(s, 3 * sizeof(float)); // Solicitar 3 flotantes (12 bytes)
  int i = 0;
  while (Wire.available()) {
    data[i++] = Wire.read(); // Leer cada byte enviado por el esclavo
  }
  // Convertir los bytes recibidos a flotantes
  u[index_s] = bytes2float(data, 0);
  pos[index_s] = bytes2float(data, 4);
  corriente[index_s] = bytes2float(data, 8);
}
// Función para convertir bytes en un número flotante
float bytes2float(byte *data, int start) {
   union u_f {
      byte b[4];   // 4 bytes para almacenar el flotante
      float f;     // Variable flotante
   } u;
   // Copiar los bytes al campo `b` de la unión
   for (int i = 0; i < 4; i++) {
      u.b[i] = data[start + i];
   }
   return u.f; // Retornar el flotante reconstruido
}
// esta funcion envia los datos a la interfaz de usuario
void updtadeData(){
  // Enviar los datos predefinidos
        Serial.print(articulacion[0]); Serial.print(","); // posición articular calculada
        Serial.print(articulacion[1]); Serial.print(","); // posición articular calculada
        Serial.print(articulacion[2]); Serial.print(","); // posición articular calculada
        Serial.print(pos[0]); Serial.print(","); // posición articular real
        Serial.print(pos[1]); Serial.print(","); // posición articular real
        Serial.print(pos[2]); Serial.print(","); // posición articular real
        Serial.print(error[0]); Serial.print(","); // error
        Serial.print(error[1]); Serial.print(","); // error
        Serial.print(error[2]); Serial.print(",");// Final de la línea // error
        Serial.print(c_real[0]); Serial.print(","); // posición cartesiana real
        Serial.print(c_real[1]); Serial.print(","); // posición cartesiana real
        Serial.print(c_real[2]); Serial.print(","); // posición cartesiana real
        Serial.print(u[0]); Serial.print(","); // señal de control U
        Serial.print(u[1]); Serial.print(","); // señal de control U
        Serial.print(u[2]); Serial.print(","); // señal de control U
        Serial.print(porcentaje[0]); Serial.print(","); // % PWM
        Serial.print(porcentaje[1]); Serial.print(","); // % PWM
        Serial.print(porcentaje[2]); Serial.print(","); // % PWM
        Serial.print(corriente[0]); Serial.print(","); // corriente
        Serial.print(corriente[1]); Serial.print(","); // corriente
        Serial.println(corriente[2]); // corriente
}