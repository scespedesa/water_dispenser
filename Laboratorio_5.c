#include<xc.h>
#define _XTAL_FREQ 8000000
#include <stdio.h>          // Conversi�n de tipos de variables
#include <stdlib.h>         // Concatenaci�n de arreglos y apuntadores
#include <string.h>         // Manejo de cadenas de texto
//#include <math.h>           // Operaciones matem�ticas de cierta complejidad
#include "LibLCDXC8.h"      // Manejo de pantalla LCD
#include "LibDHT11.h"

#pragma config FOSC=INTOSC_EC            // Habilitar Pin RA6 como digital
//#pragma config WDT=OFF                 // Depronto puede existir un error
#pragma config LVP=OFF
#pragma config PBADEN=OFF

unsigned char* Valores;                       // Temp, Hum, Che
unsigned int Potenciometro;
unsigned char UnitTemp = 'C';
unsigned char DatoGuardado;

void Subrutina_Main(unsigned char);
unsigned char Existe_en_Arreglo(unsigned char, unsigned char*);
unsigned int Conversion(void);
void Transmitir(unsigned char);
void EnviarMedicion(float, unsigned char, unsigned char);
ConmutarLED(unsigned char);
Imprimir(float, unsigned char, unsigned char);
unsigned char LeerEEPROM(int);
void GuardarEEPROM(int, char);

// Interrupci�n

void interrupt Recepcion_TMR(void);

void main(void) {
    
    OSCCON = 0b01110000;                     // Frecuencia del oscilador interno de 8 MHz
    __delay_ms(1);
    TRISD = 0;                               // Puerto D salida PIC, entrada LCD
    LATD = 0;
    TRISE = 0;                               // Puerto E salida PIC, entrada LED
    LATE = 0;
    
    RC0 = 0;                                 // Data_Out = 0
    
    // Configuraci�n m�dulo EUSART
    
    TXSTA = 0b00100000;                     // Transmisi�n de 8 bits en configuraci�n de "baja velocidad"
    RCSTA = 0b10010000;                     // Habilitaci�n modulo Eusart, recepci�n de 8 bits en modo as�ncrono 
                                            // y habilitaci�n de la recpeci�n
    BAUDCON = 0b00000000;                   // Divisor de frecuencia para c�lculo de la velocidad de comunicaci�n
                                            // de 8 bits, en l�gica positiva
    SPBRG = 12;                             // Velocidad de transmisi�n de 9615.38 bps, aprox 9600 bps
    
    // Configuraci�n de la interrupci�n de recepci�n m�dulo EUSART
    
    
    RCIF = 0;
    RCIE = 1;
    
    IPEN = 0;                               // Esquema de prioridades deshabilitado
    PEIE = 1;
    
    // Cpnfiguraci�n del TMR0
    
    T0CON=0b00000110;
    // Prescaler = 1:128                    (bit 0-2)  (Mirar datasheet para valores de prescaler)
    // Permitir asignar prescaler           (bit 3)    (0: permitir, 1: no permitir)
    // Conteo incremental                   (bit 4)    (1: de alto a bajo, 0: de bajo a alto)
    // Uso de reloj interno                 (bit 5)    (0: uso de reloj interno, 1: uso de reloj por pin T0CLK)
    // Configuraci�n contador de 16 bits    (bit 6)    (1: 8 bits, 0: 16 bits)
    // Inicia parado el contador            (bit 7)    (1: inicio contador, 0: Inicio contador apagado)
    TMR0 = 49911;
    TMR0IF = 0;
    TMR0IE = 1;
    
    
    GIE = 1;
    
    // Configuracion modulo ADC
    
    ADCON0 = 0b00000001;                    // Habilitacion modulo ADC y seleccion del canal 0 (AN0)
    ADCON1 = 14;                            // Pin AN0 como analogo, el resto digitales
    ADCON2 = 0b10001101;                    // Justificado a la derecha, tiempo de adquisici�n del ADC 2 TAD
                                            // 4 us de tiempo de adquisicion mirando el datasheet del sensor
                                            // y reloj de conversion del A/D en FOSC/16  TADC = 2 us

    // Seleccion I/O puerto A
    
    TRISA = 0b00001111;                     // RA0 lectura potenciometro, RA1 y RA2 lectura del interruptor doble
                                            // RA3 lectura del alternador de modos, RA4 led potenciometro, 
                                            // RA5 led oscilante
    TRISC2 = 0;                             // RC2 verificar recepcion de caracter valido , salida
    LATC2 = 0;
    LATA = 0;
    
    ConfiguraLCD(4);                        // Modo de 4 bits
    InicializaLCD();                        // Inicializa el LCD
    
    DatoGuardado = LeerEEPROM(0X0);
    MensajeLCD_Var("Data: ");
    if (DatoGuardado < 10) EscribeLCD_n8(DatoGuardado, 1);
    else EscribeLCD_n8(DatoGuardado, 2);
    MensajeLCD_Var(" C");
    __delay_ms(1000);
    
    TMR0ON = 1;
    while(1){
        
        Valores = LeerHT11();
        GuardarEEPROM(0x0, Valores[1]);
        Potenciometro = Conversion();
        
        ConmutarLED(Valores[1]);
        if (Potenciometro <= 512) LATA4 = 0;
        else LATA4 = 1;
        
        if (RA3 == 0){                      // Selecci�n de la unidad de la temperatura con el interruptor doble
            
            if      ((PORTA & 0b00000110) == 0b00000000){
                UnitTemp = 'C';
            }else if ((PORTA & 0b00000110) == 0b00000010){
                UnitTemp = 'K';
            }else if ((PORTA & 0b00000110) == 0b00000100){
                UnitTemp = 'R';
            }else if ((PORTA & 0b00000110) == 0b00000110){
                UnitTemp = 'F';
            }
            
        }else{
            LATC2 = 1^Existe_en_Arreglo(UnitTemp, "CKRF");
        }
        Subrutina_Main(UnitTemp);
        
        __delay_ms(615);
  }  
}

void Subrutina_Main(unsigned char Unidad){
    
    if (Unidad == 'K'){
        EnviarMedicion(Valores[1] + 273.16, Valores[0], 'K');
        Imprimir(Valores[1] + 273.16, Valores[0], 'K');
    }else if (Unidad == 'R'){
        EnviarMedicion((Valores[1]*1.8) + 491.67, Valores[0], 'R');
        Imprimir((Valores[1]*1.8) + 491.67, Valores[0], 'R');
    }else if (Unidad == 'F'){
        EnviarMedicion((Valores[1]*1.8) + 32, Valores[0], 'F');
        Imprimir((Valores[1]*1.8) + 32, Valores[0], 'F');
    }else{
        EnviarMedicion(Valores[1], Valores[0], 'C');
        Imprimir(Valores[1], Valores[0], 'C');
    }
}

unsigned char Existe_en_Arreglo(unsigned char Elemento, unsigned char* Arreglo){
    
    unsigned char Contenido = 0;
    for (int i = 0; i < strlen(Arreglo); i++){
        if (Arreglo[i] == Elemento){
            Contenido = 1;
            break;
        }
    }return Contenido;
    
}

void interrupt Recepcion_TMR(void){
    
    if (RCIF == 1){
        UnitTemp = RCREG;
        RCIF = 0;

    }if (TMR0IF == 1){
        TMR0 = 49911;
        TMR0IF = 0;
        LATA5 = 1^LATA5;
    } 
}

void Transmitir(unsigned char BufferT){
    while(TRMT==0);
    TXREG=BufferT;  
}

unsigned int Conversion(void){
    GO=1;   //bsf ADCON0,1                      // Inicio de la conversi�n
    while(GO==1);                               // Bloquear el micro mientra se hace la conversi�n
    return ADRES;                               // Retornar solo cuando se termine la conversi�n
}

void EnviarMedicion(float Temperatura, unsigned char Humedad, unsigned char TempUnity){
    
    int ValorF = (int)(Temperatura);
    float decimal = Temperatura - ValorF;
    int Decimales = (int)(decimal*100);
    
    Transmitir('T');
    Transmitir('e');
    Transmitir('m');
    Transmitir('p');
    Transmitir(':');
    Transmitir(' ');
    
    if (ValorF < 10) Transmitir(ValorF + 48);
    else if (ValorF < 100){
        Transmitir(ValorF/10 + 48);
        Transmitir(ValorF%10 + 48);
    }else{
        Transmitir(ValorF/100 + 48);
        Transmitir((ValorF%100)/10 + 48);
        Transmitir(ValorF%10 + 48);
    }
    if (Decimales != 0){
        Transmitir('.');
        if (Decimales % 10 == 0) Transmitir((Decimales/10) + 48);
        else{
           Transmitir((Decimales/10) + 48);
           Transmitir((Decimales%10) + 48);
        }
    }
    Transmitir(' ');
    Transmitir(TempUnity);
    Transmitir(' ');
    Transmitir('\n');
    
    /*Transmitir('H');
    Transmitir('u');
    Transmitir('m');
    Transmitir(':');
    Transmitir(' ');
    Transmitir(Humedad/10 + 48);
    Transmitir(Humedad%10 + 48);
    Transmitir(' ');
    Transmitir('%');
    Transmitir(' ');
    Transmitir('\n');*/
}

ConmutarLED(unsigned char Valor){
                                 // BGR
    if     (Valor < 22) LATE=0b00000000;
    else if(Valor < 25) LATE=0b00000101;
    else if(Valor < 28) LATE=0b00000100;
    else if(Valor < 31) LATE=0b00000110; 
    else if(Valor < 34) LATE=0b00000010;
    else if(Valor < 37) LATE=0b00000011;
    else if(Valor < 40) LATE=0b00000001;
    else                LATE=0b00000111;   
}

Imprimir(float Temperatura, unsigned char Humedad, unsigned char TempUnity){
    
    int ValorF = (int)(Temperatura);
    float decimal = Temperatura - ValorF;
    int Decimales = (int)(decimal*100);
    
    BorraLCD();
    
    MensajeLCD_Var("Temp: ");
    
    if (Temperatura < 10) EscribeLCD_n8(ValorF, 1);
    else if (Temperatura < 100) EscribeLCD_n8(ValorF, 2);
    else EscribeLCD_n16(ValorF, 3);
    
    if (Decimales != 0){
        EscribeLCD_c('.');
        if (Decimales % 10 == 0) EscribeLCD_n8(Decimales/10, 1);
        else EscribeLCD_n8(Decimales, 2);
    }
    
    EscribeLCD_c(' ');
    EscribeLCD_c(TempUnity);
    
    ComandoLCD(0xC0);
    MensajeLCD_Var("Hum: ");
    if (Humedad < 10) EscribeLCD_n8(Humedad, 1);
    else EscribeLCD_n8(Humedad, 2);
    MensajeLCD_Var(" %");
}

unsigned char LeerEEPROM(int dir){
    EEADR = dir;                        // Adress read EEPROM
    EEPGD = 0;                          // 0 = Access data EEPROM memory
    CFGS = 0;                           // 0 = Access Flash program or data EEPROM memory
    RD =  1;                            // 1 = Iniciar la lectura de la EEPROM
    return EEDATA;
}

void GuardarEEPROM(int dir, char data){
    EEADR = dir;                        // Adress write EEPROM
    EEDATA = data;                      // Data for writes into EEPROM
    EEPGD = 0;                          // 0 = Access data EEPROM memory
    CFGS = 0;                           // 0 = Access Flash program or data EEPROM memory
    WREN = 1;                           // 1 = Allows write cycles to Flash program/data EEPROM
                                        // permitir la escritura
    GIE = 0;                            // Desahabilitar las interrupciones
    
    // Secuencia requerida siempre
    EECON2 = 0x55;
    EECON2 = 0X0AA;
    WR = 1;                             // Inicializar la escritura en la memoria EEPROM
    
    GIE = 1;                            // Habilitar las interrupciones
    while (EEIF == 0);                  // 1 = The write operation is complete (must be cleared in software)
    EEIF = 0;                           // Clear EEIF
    WREN = 0;                           // Deshabilitar la escritura
    
}
