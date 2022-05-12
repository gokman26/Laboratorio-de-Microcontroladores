#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define MAX_COUNT 100

// Boolean
enum bool {
    false,
    true
};

// para indicar si el boton ha sido presionado
enum bool solicitud_peatonal = false;

// para indicar si el contador ha llegado a cero 
enum bool cuenta_iniciada = false;

// patrones de luz para PORTB 
enum lights {
    luz_precaucion_peatonal = 0x4A,
    luz_paso_vehicular = 0x1A,
    luz_paso_peatonal = 0x45,
    luz_precaucion_vehicular = 0x2A   
};

// estados de la MSF
enum states{
    paso_vehicular,
    transicion_a_peatonal,
    precaucion_vehicular,
    paso_peatonal,
    transicion_a_vehicular,
    precaucion_peatonal
};

// valores a carga con interrupciones cada 10ms
enum times{
    tiempo_intermitencia = 50,
    tiempo_precaucion = 100,
    tiempo_minimo_paso = 1002,   
};

int counter = 0;

// interrupcion externa 0
ISR(INT0_vect){
    solicitud_peatonal = true;
}

// interrupcion externa 1 redirigida a interupcion externa 0 
ISR(INT1_vect,ISR_ALIASOF(INT0_vect));

// Output Compare 0A 
ISR(TIMER0_COMPA_vect){
    if (counter!=0){
        counter--;
    }
}

int main(void)
{
    // configuracion del puerto B 
    DDRB = 0x7F;
    // habilitar interrupcion externa 
    GIMSK = GIMSK ^ 0xC0;
    // escoger el flanco positivo en interrupciones externas 
    MCUCR = MCUCR | 0x03;
    // cuenta hasta 78 para interrupciones de 1kHz
    OCR0A = 0x4E;
    // Habilitar Interrupción de coincidencia de comparación de salida
    TIMSK = TIMSK | 0x01;
    // comparacion de salida en modo CTC
    TCCR0A = 0x02;
    // clk/1024
    TCCR0B = 0x05;
    // borrar todas las flags de interrupcion
    TIFR = 0xff;
    // habilitar interrupcion global 
    sei(); 

    enum states estado = paso_vehicular;
    enum states estado_sig = paso_vehicular;
    enum lights luces = luz_paso_vehicular;
    uint8_t pulso = 0;    

    // logica MSF 
    while (1){
        PORTB = luces;
        estado = estado_sig;
        switch(estado){
            case paso_vehicular:
                luces = luz_paso_vehicular;
                if(cuenta_iniciada == false){
                    counter = tiempo_minimo_paso;
                    cuenta_iniciada = true;
                }
                if(solicitud_peatonal == true && counter == 0){
                    solicitud_peatonal = false;
                    cuenta_iniciada = false;
                    estado_sig = transicion_a_peatonal;
                }
                break;
            case transicion_a_peatonal:
                if(pulso!=7){
                    if(cuenta_iniciada == false){
                        counter = tiempo_intermitencia;
                        cuenta_iniciada = true;
                        luces ^= 0x10;
                        pulso++;
                    }
                    if(counter == 0){
                        cuenta_iniciada = false;
                    }
                }
                else{
                    cuenta_iniciada = false;
                    estado_sig = precaucion_vehicular;
                    pulso = 0;
                }
                break;
            case precaucion_vehicular:
                luces = luz_precaucion_vehicular;
                if(cuenta_iniciada==false){
                    counter = tiempo_precaucion;
                    cuenta_iniciada = true;
                }
                if(counter == 0){
                    estado_sig = paso_peatonal;
                    cuenta_iniciada = false;
                }
                break;
            case paso_peatonal:
                luces = luz_paso_peatonal;
                if(cuenta_iniciada==false){
                    counter = tiempo_minimo_paso;
                    cuenta_iniciada = true;
                }
                if(counter == 0){
                    estado_sig = transicion_a_vehicular;
                    cuenta_iniciada = false;
                    solicitud_peatonal = false;
                }
                break;
            case transicion_a_vehicular:
                if(pulso!=7){
                    if(cuenta_iniciada == false){
                        counter = tiempo_intermitencia;
                        cuenta_iniciada = true;
                        luces ^= 0x05;
                        pulso++;
                    }
                    if(counter == 0){
                        cuenta_iniciada = false;
                    }
                }
                else{
                    cuenta_iniciada = false;
                    estado_sig = precaucion_peatonal;
                    pulso = 0;
                }
                break;
            case precaucion_peatonal:
                luces = luz_precaucion_peatonal;
                if(cuenta_iniciada==false){
                    counter = tiempo_precaucion;
                    cuenta_iniciada = true;
                }
                if(counter == 0){
                    estado_sig = paso_vehicular;
                    cuenta_iniciada = false;
                }
                break;
            default:
                estado_sig = paso_vehicular;
                cuenta_iniciada = false;
                counter = 0;
                pulso = 0;
                luces = 0x00;
        }
    }
}
