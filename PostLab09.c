/* 
 * File:   PostLab09.c
 * Author: Marian López
 *
 * Created on 30 de abril de 2022, 09:56 AM
 */

// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include <stdint.h>
#define _XTAL_FREQ 250000

// VARIABLES
unsigned short CCPR = 0;
int x;
int x0;
int x1;
unsigned short y0;
unsigned short y1;
int pot;
int cont = 0;

// Prototipos de funciones
void setup(void);
unsigned short map(int val, int i_min, int i_max, short o_min, short o_max);

// INTERRUPCIONES
void __interrupt() isr (void){
    if (PIR1bits.ADIF){                      // Fue interrupción del ADC?
        if (ADCON0bits.CHS == 0){            // Verificar si fue el AN0
            CCPR = map(ADRESH, 0, 255, 15, 32);
            CCPR1L = (uint8_t)(CCPR>>2);
            CCP1CONbits.DC1B = CCPR & 0b11;
        }
        if (ADCON0bits.CHS == 1){            // Verificar si fue el AN0
            CCPR = map(ADRESH, 0, 255, 15, 32);
            CCPR2L = (uint8_t)(CCPR>>2);
            CCP2CONbits.DC2B1 = (CCPR & 0b10)>>1;
            CCP2CONbits.DC2B0 = CCPR & 0b01;
        }
        if(ADCON0bits.CHS == 2){
            pot = ADRESH;
        }
        PIR1bits.ADIF = 0;
    }
    if (INTCONbits.T0IF){
        cont++;
        PORTD = cont;
        if (cont == pot){
            PORTB = 0;
        }
        if (cont == 255){
            PORTB = 1;
            cont = 0;
        }
        INTCONbits.T0IF = 0;
        TMR0 = 253;
    }
}

// CICLO PRINCIPAL
void main(void){
    setup();
    while(1){
        if(ADCON0bits.GO == 0){          // No hay proceso de conversion
            if(ADCON0bits.CHS == 0)      // Cambia a canal 1
                ADCON0bits.CHS = 1;
            else if(ADCON0bits.CHS == 1) // Cambia a canal 0
                ADCON0bits.CHS = 2;
            else if(ADCON0bits.CHS == 2)
                ADCON0bits.CHS = 0;
        __delay_us(50);                 // Estabilización del capacitor           
        ADCON0bits.GO = 1;              // Inicia la conversión    
        }
    }
    return;
}

// CONFIGURACIONES
void setup(void){
    ANSEL = 0b111;                      // AN0 y AN1 como analogico
    ANSELH = 0;
    TRISA = 0b111;                      // AN0 y AN1 como entrada
    PORTA = 0;
    TRISBbits.TRISB0 = 0;
    PORTBbits.RB0 = 1;
    TRISD = 0;
    PORTD = 0;
    
    // Configuración reloj interno
    OSCCONbits.IRCF = 0b010;        // 250 kHz
    OSCCONbits.SCS = 1;             // OScilador interno
    
    // Configuración ADC
    ADCON0bits.ADCS = 0b01;         // Fosc/8
    ADCON1bits.VCFG0 = 0;           // VDD
    ADCON1bits.VCFG1 = 0;           // VSS
    ADCON0bits.CHS = 0b0000;        // Seleccionar AN0
    ADCON1bits.ADFM = 0;            // Justificado a la izquierda
    ADCON0bits.ADON = 1;            // Habilitar modulo ADC
    __delay_us(40);                 // Delay de 40 us
    
    // Configuración PWM (segun manual)
    TRISCbits.TRISC2 = 1;           // CCP1 como entrada
    TRISCbits.TRISC1 = 1;           // CCP2 como entrada
    PR2 = 38;                       // Período de 20 ms
    
    // Configuración CCP
    CCP1CON = 0;                    // CCP apagado
    CCP1CONbits.P1M = 0;            // Modo salida simple
    CCP1CONbits.CCP1M = 0b1100;     // PWM1
    CCP2CONbits.CCP2M = 0b1100;     // PWM1
    
    CCPR1L = 31>>2;                 // Valor inicial 2 ms ancho de pulso
    CCP1CONbits.DC1B = 31 & 0b11;
    
    CCPR2L = 31>>2;                 // Valor inicial 2 ms ancho de pulso
    CCP2CONbits.DC2B1 = 31 & 0b10;
    CCP2CONbits.DC2B0 = 31 & 0b01;
    
    // Configuración TMR2
    PIR1bits.TMR2IF = 0;            // Bandera int. TMR2 apagada
    T2CONbits.T2CKPS = 0b11;        // Prescaler 1:16
    T2CONbits.TMR2ON = 1;           // TMR2 ON
    while(!PIR1bits.TMR2IF);        // Esperar un cliclo del TMR2
    PIR1bits.TMR2IF = 0;            // Bandera int. TMR2 encendida
    
    TRISCbits.TRISC2 = 0;           // CCP1 como salida
    TRISCbits.TRISC1 = 0;           // CCP2 como salida
    
    // Configuración TMR0
    INTCONbits.T0IE = 1;
    INTCONbits.T0IF = 0;
    OPTION_REG = 0B01010000;
    TMR0 = 253;
    
    // Configuracion interrupciones
    PIR1bits.ADIF = 0;              // Bandera int. ADC apagada
    PIE1bits.ADIE = 1;              // Habilitar int. ADC
    INTCONbits.PEIE = 1;            // Habilitar int. de periféricos
    INTCONbits.GIE = 1;             // Habilitar int. globales
}

// Función que hace una interpolación de datos
unsigned short map(int x, int x0, int x1, short y0, short y1){
    return (unsigned short)(y0 + ((float)(y1-y0)/(x1-x0)) * (x-x0));
}