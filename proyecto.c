#include <PROYECTO.h>

float corriente = 0; // esta variable requiero enviarla a arduino
int flag_pid = 0;


// Interrupciones

// interrupcion para el timer del pid
#INT_TIMER1
void TIMER1_ISR() {
    set_timer1(58036); // Reiniciar el Timer1
    flag_pid = 1;
    time_ms+=5;
    time_lcd+=5;
}

// interrupcion para el contador del encoder
#INT_EXT2
void EXT2_ISR() {
    if (input(PIN_B2) == input(PIN_B3)) {
        contador=contador+1.0;
    } else {
        contador=contador-1.0;
    }
}

// interrupcion para la comunicacion con arduino (es importante colocar el nombre con el canal que se quiere comunicar para evitar que el pic se bloquee, en este caso quermos comunicarnos con arduino y especificamos el nobre de flujo 'sc_0')
#INT_SSP
void SSP_isr(void) {
    state = i2c_isr_state(sc_0); // Leer el estado del protocolo I2C con sc_0

    if (state == 0) {
        i2c_read(sc_0); // Limpiar el buffer al recibir la direccion (sc_0)
        buffer_index = 0; // Reiniciar el índice del buffer
    } 
    else if (state > 0 && state <= BUFFER_SIZE ) {
        rcv_buffer[buffer_index++] = i2c_read(sc_0); // Almacenar byte recibido
        if (buffer_index >= BUFFER_SIZE) {
            data_ready = 1; // Senalar que los datos están completos
        }
    } 
    else if (state >= 0x80) {
        i2c_write(send_buffer[state - 0x80]); // Enviar datos al maestro
    }
}

// Configuración inicial
void main() {
   setup_timer_1(T1_INTERNAL | T1_DIV_BY_8); 
   set_timer1(58036); // Configurar el Timer1 para el muestreo del pid a 5ms
   setup_timer_2(T2_DIV_BY_16,255,1);      //819 us overflow, 819 us interrupt
   setup_ccp1(CCP_PWM|CCP_SHUTDOWN_AC_L|CCP_SHUTDOWN_BD_L);
   set_pwm1_duty((int16)0);
   setup_ccp2(CCP_PWM|CCP_SHUTDOWN_AC_L|CCP_SHUTDOWN_BD_L);
   set_pwm2_duty((int16)0); // configurar canales de pwm

    actualizarConstantesPID(); // se configuran ganancias con la funcion
    // se habilitan interrupciones
    enable_interrupts(INT_TIMER1);
    enable_interrupts(INT_EXT2);
    enable_interrupts(INT_SSP);
    enable_interrupts(GLOBAL);
    
    // habilitamos la comunicacion con el sensor de corriente (es necesario recalcar que internamente la libreria utiliza la comunicacion especicficada con el nombre sc_1, para que no interfiera con la comunicacion con arduino))
    INA219_setAddr(&s1, INA219_ADDR);
    INA219_calibrate(&s1, 3.2, 0.1);
    INA219_setBADCResolition(&s1, INA219_SAMPLES_32);
    INA219_setSADCResolition(&s1, INA219_SAMPLES_32);
    
    
    while (TRUE) {
         // cuando la bandera se active empaqueta los datos en bytes y los envia arduino
         if (data_ready) {
            ejecucion = make16(rcv_buffer[0],rcv_buffer[1]);
            ref = bytes2Float(rcv_buffer,2);
            //ref*=57.295779513082320876798154814105;
            ref*=143.23944878270580219199538703526;
            data_ready = 0;        // Reiniciar bandera para próxima recepción 
            time_ms = 0;
         }
         //  envio de datos cada 100 ms
         if(time_lcd >= 100){
            corriente = INA219_getCurrentmA(&s1);
            //addData(0,retro*0.01745329251994329576923690768489,corriente);
            addData(0,retro*0.00698131700797731830769476307396,corriente);
            time_lcd = 0;
         }
         // si se recibio un tiempo de ejecucion enviado por arduino, se comienza a ejecutar el pid 
        while(time_ms <= (ejecucion*1000)){
         if(flag_pid == 1){
            flag_pid = 0;
            set_pid();
            corriente = INA219_getCurrentmA(&s1);
            addData(u[0],retro*0.00698131700797731830769476307396,corriente);
            //addData(u[0],retro*0.01745329251994329576923690768489,corriente);
            output_toggle(PIN_D3);
            }
        }
        actualizarPWM(0.0);
        output_low(PIN_D3);     
    }
}

