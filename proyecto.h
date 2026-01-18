#include <18F4550.h>
#device ADC=10
#FUSES NOWDT // No Watch Dog Timer
#use delay(clock=48MHz, crystal=20MHz, USB_FULL) // Iniciar reloj interno a 48Mhz
#use i2c(Slave, Fast,sda=PIN_B0, scl=PIN_B1, force_hw, address=0x60, stream = sc_0) // comunicacion como esclavo con arduino, especificamos el nombre de flujo sc_0
#use i2c(Master,fast,sda=PIN_B4,scl=PIN_B5, force_sw, stream = sc_1) // comunicacion como maestro por software con arduino, especificamos el nombre de flujo sc_1

#include <lcd.c>
#include <ieeefloat.c> // libreria para convertir los bytes del flotante recibidos de ardiuinp
#include <INA219.c>         //read .h file to settings
   
#define BUFFER_SIZE 6 // Tamaño suficiente para cmd + entero de 16 bits
struct INA219 s1;



// Variables globales
float contador = 0;
float retro = 0;
float ref = 0;
float u[2] = {0, 0};
float e[3] = {0, 0, 0};


float kp = 6.582; //ganancias de modelo para Q0
float ki = 20.05;
float kd = .2458;

/*float kp = 11.5; //ganancias de modelo para Q1 y Q2 
float ki = 25.3;
float kd = 0.25;*/


float k1, k2, k3; // ganancias de la ecuacion en diferencias
float pwmValue = 0;
float time_ms = 0;
int16 time_lcd = 0;
int  rcv_buffer[BUFFER_SIZE]; // Buffer para almacenar datos recibidos
int  buffer_index = 0;       // Índice actual del buffer
int  data_ready = 0;         // Bandera: datos completos y listos para procesar
int send_buffer[12]; // buffer para almacenar los bytes a enviar al arduino

int16 ejecucion = 0; // tiempo de ejecucion
int state = 0; // estado de comunicacion i2c como esclavo

// Función para convertir un arreglo de bytes recibido de arduino a flotante
float bytes2float(int8 *data, int start) {
   union u_f {
      int8 b[4];   // 4 bytes para almacenar el flotante
      int32 f;     // Variable para pasar a f_IEEEtoPIC
   } u;
   // Copiar los 4 bytes del arreglo al campo b
   for (int i = 0; i < 4; i++) {
      u.b[i] = data[start + i];
   }
   // Convertir el flotante en formato IEEE 754 a formato CCS
   float value = f_IEEEtoPIC(u.f);
   return value;
}
// esta funcion prepara y empaqueta los datos para poder enviarlos a arduino
void float2bytes(float value, int8 *data) {
   union u_f {
      int32 f;    // Flotante
      int8 b[4];  // Arreglo de bytes
   } u;

   // Convertir el flotante de formato PIC a IEEE 754
   u.f = f_PICtoIEEE(value);

   // Copiar los bytes al arreglo proporcionado
   for (int i = 0; i < 4; i++) {
      data[i] = u.b[i];
   }
}
void addData(float Q, float U, float A) {
    int index = 0;
    // Empaquetar dato1
    byte qdata[4];
    float2bytes(Q,qdata);
    for (int i = 0; i < 4; i++) {
        send_buffer[index++] = qdata[i];
    }
    // Empaquetar dato2
    byte udata[4];
    float2bytes(U,udata);
    for (int i = 0; i < 4; i++) {
        send_buffer[index++] = udata[i];
    }

    // Empaquetar dato3
    byte adata[4]; 
    float2bytes(A,adata);
    for (int i = 0; i < 4; i++) {
        send_buffer[index++] = adata[i];
    }
}

// funcion para configurar las ganancias de la ecuacion en diferencias
void actualizarConstantesPID() { 
    k1 = kp + (ki * 0.005) + (kd / 0.005);
    k2 = -kp - (2.0 * kd) / 0.005;
    k3 = kd / 0.005;
}

// funcion de ejecucion del pwm para los motores
void actualizarPWM(float pwmValue) {
    if (pwmValue >= 0) {
        set_pwm1_duty(0);
        set_pwm2_duty((int16)pwmValue);
    } else {
        set_pwm1_duty((int16)(-pwmValue));
        set_pwm2_duty(0);
    }
}

// funcion para el PID de los motores
void set_pid() {
    //retro = contador * 0.419580419580419580;
    retro = contador*1.558441558441558;
    e[0] = ref - retro;
    u[0] = k1 * e[0] + k2 * e[1] + k3 * e[2] + u[1];
    if (u[0] > 1023) u[0] = 1023;
    if (u[0] < -1023) u[0] = -1023;
    e[2] = e[1];
    e[1] = e[0];
    u[1] = u[0];
    
    actualizarPWM(u[0]);
    
}
