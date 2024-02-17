

#define _XTAL_FREQ 8000000
     
#define TIMER0VALUE 6 // count from 6 to 256 = 250 count x 8 us = 2000 us per interrupt.

#define RS RD0
#define EN RD1
#define D4 RD2
#define D5 RD3
#define D6 RD4
#define D7 RD5

#define MOTORDIR RC4
#define MOTORSTEP RC5
#define BUZZER RC3

#define BUTTONA RB2
#define BUTTONB RB1
#define BUTTONC RB0

#define STASIZESELECT 0
#define STAFLOWSET 1
#define STASELECTRUN 2
#define STARUN 3

#define RADIUS 8.015
#define PI 3.14
#define STEP_PER_REVOLUTION 200
#define MICROSTEP_PER_STEP 1
#define GEER_BOX_RATIO 1
#define ROD_PITCH 1


#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include "lcd.h"
#include <math.h>

#pragma config FOSC = HS        // Oscillator Selection bits (XT oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable bit (BOR disabled)
#pragma config LVP = OFF        // Low-Voltage (Single-Supply) In-Circuit Serial Programming Enable bit (RB3 is digital I/O, HV on MCLR must be used for programming)
#pragma config CPD = OFF        // Data EEPROM Memory Code Protection bit (Data EEPROM code protection off)
#pragma config WRT = OFF        // Flash Program Memory Write Enable bits (Write protection off; all program memory may be written to by EECON control)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)


int Timer0Count=0;
int Size=1;
int Flow=1;
int FlowRun=0;
int TimeRun=0;

int CurrentState=0;

int RunMode=0; // =0 ; RF ; =1 IF

float distance=0;
float revolutions=0;
float remainRevolutions=0;
float revolutions_10cc=0;
float revolutions_10cc_count=0;
float availableRevolutionsIF = 0;
float availableRevolutionsRF = 0;

unsigned long StepBack=0;

void InitTimer0() // time cound second
{
    OPTION_REGbits.T0CS=0; // internal clock for timer 0
    OPTION_REGbits.PSA = 0; // prescal for timer 0
    OPTION_REGbits.PS2=0;  // prescal 1:16 Clock = FOSC /4 and / prescal/ 8Mhz/4/16 = 0.125Mhz -> 8 us per timer count
    OPTION_REGbits.PS1=1;
    OPTION_REGbits.PS0=1;
    TMR0=TIMER0VALUE;// count from 6 to 255 = 250 count x 8 us = 2000 us = 2ms per interrupt.
    INTCONbits.TMR0IF=0; // clear flag
    INTCONbits.TMR0IE=1; // enable timer0 interrupt
    INTCONbits.PEIE=1; // enable preha interrupt
    INTCONbits.GIE=1; // enable global interrupt
}


void __interrupt() ISR()
{ 

    if(INTCONbits.TMR0IF==1) //if interrupt timer 0
    {
        INTCONbits.TMR0IF=0;// clear flag
        TMR0=TIMER0VALUE; // preload value
        Timer0Count++;
         
        if(Timer0Count>=500) // 500x2=1000ms = 1s for time count
        {
            Timer0Count=0;
            // 1 second action
          
        }  
        
    }
    
}


void Step(int StepMove)
{
  int i;

  if(StepMove>0)
  {
    MOTORDIR=1;
    for(i=0;i<StepMove;i++)
    {
       MOTORSTEP=1;
      __delay_us(2500);
       MOTORSTEP=0;
      __delay_us(2500);;
    }
  }
  else if(StepMove<0)
  {
    StepMove=StepMove*(-1);
    MOTORDIR=0;
    for(i=0;i<StepMove;i++)
    {
       MOTORSTEP=1;
      __delay_us(2500);
       MOTORSTEP=0;
      __delay_us(2500);
    }
  }
  
}

void DisplaySize()
{
    Lcd_Set_Cursor(6,1);
    if(Size>=10) Lcd_Write_Char(Size/10+48);
    else Lcd_Write_Char(' ');
    Lcd_Write_Char(Size%10+48);
    Lcd_Write_String("cc");
}

void DisplaySizeSelect()
{
    Lcd_Write_String("       SIZE     ");
    Lcd_Set_Cursor(1,2);
    Lcd_Write_String("    SELECTING   ");
    __delay_ms(1000);
    Lcd_Clear();
    Lcd_Set_Cursor(1,1);
    Lcd_Write_String("Size:");
    Lcd_Set_Cursor(1,2);
    Lcd_Write_String("A. <  B. >  C.OK");   
    DisplaySize();
}

void DisplayFlow(int Line)
{
    Lcd_Set_Cursor(10,Line);
    if(Flow>=10) Lcd_Write_Char(Flow/10+48);
    else Lcd_Write_Char(' ');
    Lcd_Write_Char(Flow%10+48);
    Lcd_Write_String("ml/h");
}

void DisPlayFlowSet()
{
    Lcd_Clear();
    Lcd_Set_Cursor(1,1);
    Lcd_Write_String("FlowRate:");
    Lcd_Set_Cursor(1,2);
    Lcd_Write_String("A. +  B. -  C.OK");   
    DisplayFlow(1);
}

void DisplayFlowRun()
{
    Lcd_Set_Cursor(1,1);
    Lcd_Write_Char(FlowRun/10+48);
    Lcd_Write_Char(FlowRun%10+48);
    Lcd_Write_Char('/');
    Lcd_Write_Char(Flow/10+48);
    Lcd_Write_Char(Flow%10+48);
    Lcd_Write_String("ml");
     
}

void DisplayTimeRun()
{
    int MinuteRun=0;
    int SecondRun=0;
    
    MinuteRun=TimeRun/60;
    SecondRun=TimeRun%60;
    Lcd_Set_Cursor(11,1);
    Lcd_Write_Char(MinuteRun/10+48);
    Lcd_Write_Char(MinuteRun%10+48);
    Lcd_Write_Char(':');
    Lcd_Write_Char(SecondRun/10+48);
    Lcd_Write_Char(SecondRun%10+48);
    Lcd_Write_Char('s');
     
}

void DisplayMode()
{
    Lcd_Set_Cursor(13,2);
    if(RunMode==0)
    {
       Lcd_Write_String("RF");
    }
    else
    {
       Lcd_Write_String("IF");
    }
           
}

void DisplaySelectRun()
{
    Lcd_Clear();
    Lcd_Set_Cursor(1,2);
    Lcd_Write_String("A.START   B.");   
    DisplayFlowRun();
    DisplayTimeRun();
    DisplayMode();
}

float calculateDistance ()
{
    float distance = 10 * 1000 / (PI * RADIUS * RADIUS);
    return distance;
}

float setRevol (float distance)
{
    float revolutions = distance * STEP_PER_REVOLUTION * MICROSTEP_PER_STEP * GEER_BOX_RATIO / ROD_PITCH;
    return 18000000 / revolutions;
}

void Beep(int Repeat)
{
    int i;
    for(i=0;i<Repeat;i++)
    {
        BUZZER=1;
        __delay_ms(200);
        BUZZER=0;
        __delay_ms(200);
        
    }
}
float remainingVolume = 0;
void calculateRemainingVolume() {
    float totalVolume = Size; // Assuming Size is the desired total volume
    remainingVolume = fmod(totalVolume, 10.0); // Calculate the remainder of totalVolume / 10
}
void StartRunRF()
{
        unsigned long i;
        for(i=0;i<StepBack;i++)
        {
            if(RunMode==0) Step(-1);
            //else Step(1);
            if(i % 200 == 0)
            {
               TimeRun++;
               DisplayTimeRun();  
            }
            
        }
        if(RunMode==0) RunMode=1;
        else RunMode=0;
        StepBack=0;
        CurrentState=STASELECTRUN;
        Beep(1);
}
 
void StartRunIF()
{
    unsigned long StepRun=0;
    unsigned long i;
    unsigned long RevolutionsPerFlow=0;
    unsigned long calculateSteps(float volume);
    float Temp;
    if(revolutions_10cc_count==revolutions_10cc-1) // vong cuoi chay so CC con lai
    {
        StepRun=(unsigned long)remainRevolutions;
        Temp=revolutions /Flow;
        RevolutionsPerFlow=(unsigned long)Temp+1; //i tinh tu 0
        for(i=0;i<StepRun;i++)
        {
            if(RunMode==1) Step(1);
            //else Step(-1);
            if(i % 200 == 0)
            {
               TimeRun++;
               DisplayTimeRun();  
             
            }
            if(i%RevolutionsPerFlow==0)
            {
                FlowRun++;
                DisplayFlowRun();
            }
            StepBack++;
        }
        CurrentState=STASELECTRUN;
        Beep(5);
        revolutions_10cc_count++;
        
    }
    else if(revolutions_10cc_count<revolutions_10cc) 
    {
        StepRun= (unsigned long) revolutions * 10 /Flow;;// chay full 10cc
        Temp=revolutions /Flow;
        RevolutionsPerFlow=(unsigned long)Temp+1; //i tinh tu 0
        for(i=0;i<StepRun;i++)
        {
            if(RunMode==1) Step(1);
            //else Step(1);
            if(i % 200 == 0)
            {
               TimeRun++;
               DisplayTimeRun();  
             
            }
            if(i%RevolutionsPerFlow==0)
            {
                FlowRun++;
                DisplayFlowRun();
            }
             StepBack++;
          
        }
        if(RunMode==0) RunMode=1;
        else RunMode=0;
        DisplayMode();
        CurrentState=STASELECTRUN;
        revolutions_10cc_count++;
        remainRevolutions=remainRevolutions-StepRun;
        Beep(3);
        
    }
    else if(remainingVolume > 0) {
        // Calculate the number of steps for the remaining volume
        // You will need to adjust the calculation based on your pump's characteristics
        unsigned long stepsForRemainingVolume = calculateSteps(remainingVolume);

        // Run the pump for the calculated steps
        for(i = 0; i < stepsForRemainingVolume; i++) {
          if(RunMode==1) Step(1);
            //else Step(1);
            if(i % 200 == 0)
            {
               TimeRun++;
               DisplayTimeRun();  
             
            }
            if(i%RevolutionsPerFlow==0)
            {
                FlowRun++;
                DisplayFlowRun();
            }
             StepBack++;
        }
         if(RunMode==0) RunMode=1;
        else RunMode=0;
        DisplayMode();
        CurrentState=STASELECTRUN;
        revolutions_10cc_count++;
        remainRevolutions=remainRevolutions-StepRun;
        unsigned long steps = (unsigned long)(distance * 1000); 
       return steps;
        Beep(3);
    }
}
void main(void) {

    
    
    OPTION_REGbits.nRBPU=0;// enable pull up resistor
    ADCON1 = 0x0F;// disable analog on port A
    CMCON = 7; // disable comparator
    TRISD=0x00;
    TRISC=0x00;
    TRISB=0xFF;
    BUZZER=0;
    InitTimer0();
    Lcd_Init();

    Lcd_Write_String("   DIY SYRINGE  ");
    Lcd_Set_Cursor(1,2);
    Lcd_Write_String("      PUMP     ");
    __delay_ms(1000);
    Lcd_Clear();
    CurrentState=STASIZESELECT;
    DisplaySizeSelect();
    while(1)
    {
        if(BUTTONA==0)
        {
           __delay_ms(250);
       
           if(CurrentState==STASIZESELECT)
           {
               if(Size==1) Size=2;
               else if(Size==2) Size=3;
               else if(Size==3) Size=5;
               else if(Size==5) Size=10;
               else if(Size==10) Size=1;
               DisplaySize();
           }
           else if(CurrentState==STAFLOWSET)
           {
               Flow++;
               if(Flow==100) Flow=1;
                DisplayFlow(1);
           }
           else if(CurrentState==STASELECTRUN)
           {
               CurrentState=STARUN;
               if(RunMode==1) StartRunIF();
               else StartRunRF();
           }
            
        }
        if(BUTTONB==0)
        {
           __delay_ms(250);
       
           if(CurrentState==STASIZESELECT)
           {
               if(Size==1) Size=10;
               else if(Size==10) Size=5;
               else if(Size==5) Size=3;
               else if(Size==3) Size=2;
               else if(Size==2) Size=1;
               DisplaySize();
           }
            else if(CurrentState==STAFLOWSET)
           {
               Flow--;
               if(Flow==0) Flow=99;
                DisplayFlow(1);
           }
            else if(CurrentState==STASELECTRUN)
            {
               
               
                if(RunMode==0) RunMode=1;
                else RunMode=0;
                DisplayMode();
                    
            }
        }
         if(BUTTONC==0)
        {
           __delay_ms(150);
           while(BUTTONC==0);
           if(CurrentState==STASIZESELECT)
           {
                Lcd_Set_Cursor(1,2);
                Lcd_Write_String("Flowrate:       ");
                DisplayFlow(2);
                __delay_ms(1000);
                CurrentState=STAFLOWSET;
                DisPlayFlowSet();
           }
           else if(CurrentState==STAFLOWSET)
           {
               Lcd_Clear();
               Lcd_Set_Cursor(1,1);
               Lcd_Write_String("Size:");
               Lcd_Set_Cursor(1,2);
               Lcd_Write_String("FlowRate:");
               DisplaySize();
               DisplayFlow(2);
               __delay_ms(1000);
               
               CurrentState=STASELECTRUN;
               FlowRun=0;
               TimeRun=0;
               
               distance = calculateDistance();       
               revolutions = setRevol(distance) * Flow / 10;
               revolutions_10cc=(Flow-1)/10+1;
               revolutions_10cc_count=0;
               remainRevolutions=revolutions;
               DisplaySelectRun();
           }
            
        }
        
        
     
     }
    
 
    return;
}
