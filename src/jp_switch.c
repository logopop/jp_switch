//*********************************************************************************************************************
//  Jeep Switch control circuit
//   
//  v1.0.0
//    
//*********************************************************************************************************************
//

// CONFIG
#pragma config FOSC = INTOSCIO  // Oscillator Selection bits (INTOSC oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = ON        // Watchdog Timer Enable bit (WDT enabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT enabled)
#pragma config MCLRE = OFF      // RA5/MCLR/VPP Pin Function Select bit (RA5/MCLR/VPP pin function is digital input, MCLR internally tied to VDD)
#pragma config BOREN = ON       // Brown-out Detect Enable bit (BOD enabled)
#pragma config LVP = OFF        // Low-Voltage Programming Enable bit (RB4/PGM pin has digital I/O function, HV on MCLR must be used for programming)
#pragma config CPD = OFF        // Data EE Memory Code Protection bit (Data memory code protection off)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>
#include <stdint.h>
#include <time.h>
#include <jp_switch.h>
 
 
//*********************************************************************************************************************
//
 
#define _XTAL_FREQ      1000000           // 4 MHz

// Port defines
// Port A
#define LED_3           0b10000000        // (7)Out: LED 3 Blue
#define LED_2           0b01000000        // (6)Out: LED 2 Green
#define ACC_SW          0b00100000        // (5)In:  Accessory switch in
#define LED_1           0b00010000        // (4)Out: LED 1 Red
#define RELAY_3         0b00001000        // (3)Out: Accessory relay 3
#define RELAY_2         0b00000100        // (3)Out: Accessory relay 2
#define RELAY_1         0b00000010        // (3)Out: Accessory relay 1
#define RELAY_LIGHTS    0b00000001        // (0)Out: Lights relay
   
// Port B
#define SPEAKER         0b11000000        // (7)Out: Piezo sound
#define LIGHTS_SW       0b01000000        // (6)In:  Lights switch in
#define VALET_SW        0b01100000        // (5)In:  Valet switch in 
#define DOORS_SW        0b01010000        // (4)In:  Doors switch in 
#define VALET_S_L       0b01001000        // (3)Out: Valet switch light (PWM))
#define DOORS_S_L       0b01000100        // (3)Out: Doors switch light
#define ACC_S_L         0b01000010        // (3)Out: Accessory switch light
#define LIGHTS_S_L      0b01000001        // (0)Out: Lights switch light

#define ON              1
#define OFF             0
#define MAX_SW          0xA0              // Max switch counter value
#define MAX_BLINKPACE   0x04B0            // Blinkcheck pace       

typedef unsigned char BYTE;

//*********************************************************************************************************************
// Globals  
//*********************************************************************************************************************
//

BYTE              bPortA = 0x10;                       // Port value for writing 
BYTE              bPortB = 0x00;                       // Port value for writing 
BYTE              bLights_sw = 0x00;                   // Lights switch debounce counter
BYTE              bValet_sw = 0x00;                    // Valet switch debounce counter
BYTE              bDoors_sw = 0x00;                    // Doors switch debounce counter
BYTE              bAcc_sw = 0x00;                      // Acc switch debounce counter

BYTE              bLights_hold = 0x00;                 // Lights switch being held
BYTE              bValet_hold = 0x00;                  // Valet switch being held
BYTE              bDoors_hold = 0x00;                  // Doors switch being held
BYTE              bAcc_hold = 0x00;                    // Acc switch being held

BYTE              bLights_oldhold = 0x00;              // Lights switch being held
BYTE              bValet_oldhold = 0x00;               // Valet switch being held
BYTE              bDoors_oldhold = 0x00;               // Doors switch being held
BYTE              bAcc_oldhold = 0x00;                 // Acc switch being held

BYTE              bLights_state = 0x00;                // Lights state
BYTE              bValet_state = 0x00;                 // Valet state
BYTE              bDoors_state = 0x00;                 // Doors state
BYTE              bAcc_state = 0x00;                   // Acc state (0, 1, 2, 3)
BYTE              bAcc_state2 = 0x00;                  // Acc2 state (waiting, synced)
BYTE              bAcc_state3 = 0x00;                  // Acc3 state (reset, wait for release to enable button)

// Blink counters
BYTE              bLights_b = 0x00;                    // Lights blink counter  MSB signifies slow
BYTE              bValet_b = 0x00;                     // Valet blink counter  
BYTE              bDoors_b = 0x00;                     // Doors blink counter   
BYTE              bAcc_b = 0x00;                       // Accessory blink counter   
BYTE              bBlinkstate = 0x00;                  // Blink state
unsigned long     ulBlinkpace = 0x0000;                // Blink pacer
unsigned long     ulAcc = 0x0000;                      // Acc delay counter


//*********************************************************************************************************************
// 
//*********************************************************************************************************************
//

void init_ports(void) 
{
   TRISA = 0x20;
   TRISB = 0x70;
   PORTA = 0x10;              // Red LED off (OC))
}

//*********************************************************************************************************************
//  
//*********************************************************************************************************************
//

void vToggle(unsigned char ucI, unsigned char ucState)
{
   if ((ucI & LIGHTS_SW) && (ucI & 0b10111111))  // Port B
   {
      ucI = (unsigned char)(ucI & 0b10111111);   // Mask port B identifier
      if (ucState)      
         bPortB |= ucI;
      else
         bPortB &= (unsigned char)(~ucI);      
      PORTB = bPortB;
   }   
   else
   {
      if (ucState)      
         bPortA |= ucI;
      else
         bPortA &= (unsigned char)(~ucI);        
      PORTA = bPortA;
   }       
}
 
//*********************************************************************************************************************
//  
//*********************************************************************************************************************
//
 
/*
void vShortDelay(unsigned long ulD) 
{
      __delay_ms(ulD);
}
*/
 
//*********************************************************************************************************************
//  
//*********************************************************************************************************************
//
 
void vLongDelay(unsigned long ulD) 
{
   unsigned long                 i                    = 0;
    
   for (i = 0; i < ulD; i++)
   {  
      asm("CLRWDT");
      __delay_ms(100);
   }   
}

//*********************************************************************************************************************
//  Blink for testing 
//*********************************************************************************************************************
//
 
void vBlink(unsigned char ucC, unsigned long ulB) 
{
   unsigned long                 i                    = 0;
    
   for (i = 0; i < ulB; i++)
   {   
      vToggle(ucC, ON);
      vLongDelay(20);              
      vToggle(ucC, OFF); 
      vLongDelay(20);        
   }    
}

//*********************************************************************************************************************
//  
//*********************************************************************************************************************
//
 
void vFastBlink(unsigned char ucC, unsigned char ucB) 
{
   if (ucC == LIGHTS_S_L)
      bLights_b = (BYTE)(ucB * 2);
   else if (ucC == VALET_S_L)
      bValet_b = (BYTE)(ucB * 2);
   else if (ucC == DOORS_S_L)
      bDoors_b = (BYTE)(ucB * 2);
   else if (ucC == ACC_S_L)
      bAcc_b = (BYTE)(ucB * 2);    
}

//*********************************************************************************************************************
//  
//*********************************************************************************************************************
//
 
void vSlowBlink(unsigned char ucC, unsigned char ucI) 
{
   unsigned char ucB;
   
   ucB = (BYTE)(ucI * 2);    
   if (ucC == LIGHTS_S_L)
      bLights_b = (BYTE)(ucB | 0x80);
   else if (ucC == VALET_S_L)
      bValet_b = (BYTE)(ucB | 0x80);
   else if (ucC == DOORS_S_L)
      bDoors_b = (BYTE)(ucB | 0x80);
   else if (ucC == ACC_S_L)
      bAcc_b = (BYTE)(ucB | 0x80);    
}

//*********************************************************************************************************************
// 
//*********************************************************************************************************************
//

void vBeep(unsigned char ucB) 
{
   int                           i                    = 0;
   int                           j                    = 0;
   
   switch (ucB)
   {
      case 1:                 // Beep-beep
         asm("CLRWDT");
         for (j = 0; j < 3; j++)
         {   
            for (i = 0; i < 150; i++)
            {
               bPortB |= 0x80; PORTB = bPortB;
               __delay_ms(1);
               bPortB &= (unsigned char)(~0x80); PORTB = bPortB; 
               __delay_ms(0);
            }
            __delay_ms(200);
         }      
      break;
      case 2:
         asm("CLRWDT");
         for (i = 0; i < 50; i++)
         {
            bPortB |= 0x80; PORTB = bPortB;
            __delay_ms(3);
            bPortB &= (unsigned char)(~0x80); PORTB = bPortB;
            __delay_ms(3);
         }
      break;
      case 3:
         asm("CLRWDT");
         for (i = 0; i < 30; i++)
         {
            bPortB |= 0x80; PORTB = bPortB;
            __delay_ms(4);
            bPortB &= (unsigned char)(~0x80); PORTB = bPortB;
            __delay_ms(4);
         }
      break;
      case 4:
         asm("CLRWDT");
         for (i = 0; i < 10; i++)
         {
            bPortB |= 0x80; PORTB = bPortB;
            __delay_ms(1);
            bPortB &= (unsigned char)(~0x80); PORTB = bPortB;
            __delay_ms(0);
         }
      break;
      default:                            // Beep
         asm("CLRWDT");
         for (i = 0; i < 200; i++)
         {
            bPortB |= 0x80; PORTB = bPortB;
            __delay_ms(1);
            bPortB &= (unsigned char)(~0x80); PORTB = bPortB;
            __delay_ms(0);
         }
      break;   
   }        
}

//*********************************************************************************************************************
// 
//*********************************************************************************************************************
//

void vButtonpoll(void) 
{   
   if (PORTB & LIGHTS_SW)
   {
      if (bLights_sw < MAX_SW)
         bLights_sw++;
      else
         bLights_hold = 1;
   }
   else if (bLights_sw)
   {
      if (bLights_sw < 10)
      {   
         bLights_hold = 0;      
         bLights_oldhold = 0;      
      }   
      bLights_sw--;
   }  
   
   if (!(PORTB & VALET_SW))      // Valet is normally high  
   {
      if (bValet_sw < MAX_SW)
         bValet_sw++;      
      else
         bValet_hold = 1;
   }   
   else if (bValet_sw)
   {   
      if (bValet_sw < 10)
      {   
         bValet_hold = 0;
         bValet_oldhold = 0;
      }   
      bValet_sw--;
   }
   
   if (PORTB & 0x10)
   {
      if (bDoors_sw < MAX_SW)
         bDoors_sw++; 
      else
         bDoors_hold = 1;
   }   
   else if (bDoors_sw)
   {
      if (bDoors_sw < 10)
         bDoors_hold = 0;  
      bDoors_sw--;
   }
   
   if (PORTA & ACC_SW)
   {
      if (bAcc_sw < MAX_SW)
         bAcc_sw++;
      else
         bAcc_hold = 1;
   }      
   else if (bAcc_sw)
   {
      if (bAcc_sw < 10)
      {   
         bAcc_hold = 0; 
         bAcc_oldhold = 0;
      }   
      bAcc_sw--;
   }   
}

//*********************************************************************************************************************
// 
//*********************************************************************************************************************
//

void vButtonaction(void) 
{
   // Lights **************************************************
   if (bLights_hold & !bLights_oldhold)
   {   
      bLights_oldhold = bLights_hold;
      bLights_sw = 0;

      if (bLights_state)
      {   
         vBeep(3);
         vSlowBlink(LIGHTS_S_L, 2);
         bLights_state = 0;
         vToggle(RELAY_LIGHTS, OFF);
      }   
      else
      {  
         vBeep(0);
         vFastBlink(LIGHTS_S_L, 4);
         bLights_state = 1;
         vToggle(RELAY_LIGHTS, ON);
      }         
   }

   // Accessories *********************************************
   if (bAcc_hold & !bAcc_oldhold & (bAcc_state2 != 2))
   {  
      bAcc_oldhold = bAcc_hold;
      vFastBlink(ACC_S_L, 1);
      
      if (bAcc_state2 == 1)
      {   
         // In a state, reset to 0
         vToggle(LED_1, OFF); vToggle(LED_2, OFF); vToggle(LED_3, OFF);
         vBeep(3);
         bAcc_state = 0;
         bAcc_state2 = 2;
         bAcc_oldhold = 0;
         // Reset relays
         vToggle(RELAY_1, OFF);
         vToggle(RELAY_2, OFF);
         vToggle(RELAY_3, OFF);                                
      }     
      else     // Not committed yet
      {
         bAcc_state++;        // 0, 1, 2, 3
         if (bAcc_state > 3)
            bAcc_state = 0;

         if (bAcc_state == 1)
         {
            vToggle(LED_1, ON); vToggle(LED_2, OFF); vToggle(LED_3, OFF);
            vBeep(0);
            ulAcc = 0;           // Start delay from 0
         }   
         else if (bAcc_state == 2)
         {
            vToggle(LED_1, OFF); vToggle(LED_2, ON); vToggle(LED_3, OFF);
            vBeep(0); vLongDelay(2); vBeep(0);
            ulAcc = 0;           // Start delay from 0
             
            bAcc_state2 = 0;             
         }
         else if (bAcc_state == 3)
         {
            vToggle(LED_1, OFF); vToggle(LED_2, OFF); vToggle(LED_3, ON);
            vBeep(0); vLongDelay(2); vBeep(0); vLongDelay(2); vBeep(0); 
            ulAcc = 0;           // Start delay from 0
         }           
         else
         {
            vToggle(LED_1, OFF); vToggle(LED_2, OFF); vToggle(LED_3, OFF);            
            vBeep(3); 
         }     
      }
   }   
   
   // No accessory push, see if we're waiting for commit
   if (bAcc_state2)
   {            
   }   
   else if (bAcc_state)
   {        
      // Check if pending state, commit
      if (ulAcc > 10)
      {
         bAcc_state2 = 1;
         if (bAcc_state == 1)
         {   
            vBlink(LED_1, 1); vToggle(LED_1, ON);
            vToggle(RELAY_1, ON);
         }   
         else if (bAcc_state == 2)
         {            
            vBlink(LED_2, 1); vToggle(LED_2, ON);
            vToggle(RELAY_2, ON);
         }
         else if (bAcc_state == 3)
         {
            vBlink(LED_3, 1); vToggle(LED_3, ON);
            vToggle(RELAY_3, ON);
         }   
         vBeep(2);           
      }
   }
   
   // Release 0 button
   if (!bAcc_hold & (bAcc_state2 == 2))
      bAcc_state2 = 0;
   
      
   // Valet ***************************************************
   if (bValet_hold & !bValet_oldhold)
   {    
      vBeep(2);
      vSlowBlink(VALET_S_L, 10);  
      bValet_oldhold = bValet_hold;
   }
   
   // Doors
   if (bDoors_hold & (!bDoors_oldhold))
   {     
      vBeep(1);
      vFastBlink(DOORS_S_L, 8);
      bDoors_state = 0;
      bDoors_oldhold = bDoors_hold;
   }   
   else if ((!bDoors_hold) & bDoors_oldhold)
   {  
      vBeep(3);
      vSlowBlink(DOORS_S_L, 3);
      bDoors_oldhold = bDoors_hold;
   }                     
}

//*********************************************************************************************************************
// 
//*********************************************************************************************************************
//

void vBlinkcheck(void) 
{
   ulBlinkpace++;
   if (ulBlinkpace > MAX_BLINKPACE)//MAX_BLINKPACE)
   {
      ulAcc++;
      ulBlinkpace = 0;
      bBlinkstate++; 
      
      if (bLights_b & 0x7F)
      {            
         if ((bLights_b & 0x80) && (bBlinkstate % 2))   // Slow
         {         
         }
         else
         {   
            if ((bLights_b & 0x7F) % 2)   // Off
               vToggle(LIGHTS_S_L, OFF);
            else
               vToggle(LIGHTS_S_L, ON);
            bLights_b--;                        
         }               
      }
      
      if (bValet_b & 0x7F)
      {   
         if ((bValet_b & 0x80) && (bBlinkstate % 2))   // Slow
         {          
         }
         else
         {   
            if ((bValet_b & 0x7F) % 2)   // Off
               vToggle(VALET_S_L, OFF);
            else
               vToggle(VALET_S_L, ON);
            bValet_b--;                        
         }               
      }
        
      if (bDoors_b & 0x7F)
      {   
         if ((bDoors_b & 0x80) && (bBlinkstate % 2))   // Slow
         {          
         }
         else
         {   
            if ((bDoors_b & 0x7F) % 2)   // Off
               vToggle(DOORS_S_L, OFF);
            else
               vToggle(DOORS_S_L, ON);
            bDoors_b--;                        
         }               
      }
      
      if (bAcc_b & 0x7F)
      {   
         if ((bAcc_b & 0x80) && (bBlinkstate % 2))   // Slow
         {          
         }
         else
         {   
            if ((bAcc_b & 0x7F) % 2)   // Off
               vToggle(ACC_S_L, OFF);
            else
               vToggle(ACC_S_L, ON);
            bAcc_b--;                        
         }               
      }                          
   }   
}

//*********************************************************************************************************************
// 
//*********************************************************************************************************************
//

void vTest(void) 
{
   int                           bBusy                = 1;

   vLongDelay(80);  
   while (bBusy)                     // Run until power off 
   {
      vBeep(0);
      asm("CLRWDT");
      vBlink(LED_1, 1);  
      asm("CLRWDT");
      vBeep(1);
      asm("CLRWDT");
      vBlink(LED_2, 1);            
      vBeep(2);
      asm("CLRWDT");
      vBlink(LED_3, 1);       
      vBeep(3);
      vBlink(LIGHTS_S_L, 2);             
      vBlink(ACC_S_L, 2);       
      asm("CLRWDT");
      vBlink(VALET_S_L, 2);       
      vBlink(DOORS_S_L, 2);   
      asm("CLRWDT");
      vBlink(RELAY_LIGHTS, 2);            
      asm("CLRWDT");
      vBlink(RELAY_1, 2);       
      vBlink(RELAY_2, 2);       
      asm("CLRWDT");
      vBlink(RELAY_3, 2);                       
   }
}

//*********************************************************************************************************************
// 
//*********************************************************************************************************************
//

void main() 
{
   unsigned char ucI       = 0;
      
   // Initialize device
   OPTION_REG = 0x8F; 
   PCON = 0x08;
   VRCON = 0x00;
   INTCON = 0x00;
   PIE1 = 0x00;
   PIR1 = 0x00; 
   
   //CMCON = 0x07;
     
   // Configure ports
   init_ports();
   vToggle(LED_1, OFF); 
   vToggle(LIGHTS_S_L, OFF); 
   
   // Check for test routine
   if (!(PORTB & VALET_SW))
      vTest();
   
   vLongDelay(50);
   vToggle(LED_2, ON);
   vToggle(LED_3, ON);         
   vLongDelay(50);
   vToggle(LED_2, OFF);
   vToggle(LED_3, OFF);         
   vLongDelay(50);
        
//*********************************************************************************************************************
// Main loop 
   
   while (1)
   {   
      vButtonpoll();
      vButtonaction();     
      vBlinkcheck();
      asm("CLRWDT");                   
   }   
  
//*********************************************************************************************************************  
// Halt  Should never reach here

   while(1)
   {   
            
   }   
}

//*********************************************************************************************************************
//










