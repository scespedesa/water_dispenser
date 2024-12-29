/* 
 * File:   LibDHT.h
 * Author: Robin & Juan
 *
 */

#ifndef LIBDHT11_H
#define	LIBDHT11_H

#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif
#include<xc.h>
#include<string.h>
#ifndef _XTAL_FREQ
#define _XTAL_FREQ 20000000
#endif
#ifndef DATA_DIR
#define DATA_DIR TRISC0	//El puerto de conexión de los datos el cual se puede cambiar
#endif
#ifndef DATA_IN
#define DATA_IN RC0	//Los pines de control al LCD los cuales se
#endif
#ifndef DATA_OUT
#define DATA_OUT LATC0	//pueden cambiar
#endif

unsigned char Temp,Hum,Che;

unsigned char* LeerHT11(void);
unsigned char LeerByte(void);
unsigned char LeerBit(void);
//unsigned char Check();

unsigned char* LeerHT11(void){
    unsigned char i,contr=0;
    unsigned char* Retornar;
    DATA_DIR=0;
    __delay_ms(18);
    DATA_DIR=1;
    while(DATA_IN==1);
    __delay_us(40);
    if(DATA_IN==0) contr++;
    __delay_us(80);
    if(DATA_IN==1) contr++;
    while(DATA_IN==1);
    Retornar[0]=LeerByte();
    LeerByte();
    Retornar[1]=LeerByte();
    LeerByte();
    Retornar[2]=LeerByte();
    Temp = Retornar[1];
    Hum = Retornar[0];
    Che = Retornar[2];
    return Retornar;
}

unsigned char LeerByte(void){
    unsigned char res=0,i;
    for(i=8;i>0;i--){
      res=(res<<1) | LeerBit();  
    }
    return res;
}

unsigned char LeerBit(void){
    unsigned char res=0;
    while(DATA_IN==0);
    __delay_us(13);
    if(DATA_IN==1) res=0;
    __delay_us(22);
    if(DATA_IN==1){
      res=1;
      while(DATA_IN==1);
    }  
    return res;  
}

unsigned char Check(void){
  unsigned char res=0,aux;
  aux=Temp+Hum;
  if(aux==Che) res=1;
  return res;  
}
#endif	/* LIBDHT11_H */
