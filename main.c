#include "REG_89C51.h"

#define dcMotorPin     P1_7
#define RS_lcd     P2_0
#define RW_lcd     P2_1
#define E_lcd      P2_2
#define DATA_lcd   P2
#define start     P1_2
#define adr     P3
#define eoc     P1_1

int digital_out=0,ldr_target,ldr_ref;
unsigned char rand;
unsigned char score;
unsigned int t1,t2;
float k;
unsigned char x,r,w2=0,w3=0;


//====================================== delay ===============================================//
void delay(float time_delay)
 {
	for(k=0;k<2.2*time_delay;k++);
 }


//====================================== lcd functions =======================================//
void sendByteToLcd(unsigned char ByteValue)     
 {
	E_lcd =1;//enable high
	DATA_lcd &= 0x87;
	DATA_lcd |= ((ByteValue &0xf0 )>>1);// write higher four bits    
	E_lcd =0; //enable low       it's a high to low transition
	delay(10);
	E_lcd =1;//enable high

	DATA_lcd &= 0x87;
	DATA_lcd |= ((ByteValue &0x0f )<<3) ;// write lower four bits  
	E_lcd =0; //enable low      it's a high to low transition
	delay(10);
	E_lcd =1;//enable high
 }


//====================================== write command byte ==================================//
void instruction_lcd(unsigned char cod)
 {
	RS_lcd=0;
	RW_lcd =0;
	sendByteToLcd(cod); 
 }


//====================================== lcd initialization ==================================//
void init_lcd()
 {
	instruction_lcd(0x33);	// mix of 0x30 and 0x30
	instruction_lcd(0x32); // mix of 0x30 and 0x20
	instruction_lcd(0x28);
	instruction_lcd(0x06);
	instruction_lcd(0x0E);
	instruction_lcd(0x01);
 }


//====================================== write a data byte (askii code) ======================//
void data_out_lcd(unsigned char data_out)
 {
	if(data_out>0x80)
	data_out-=0x40;
	RS_lcd=1;
	RW_lcd =0;
	sendByteToLcd(data_out);
 }


//====================================== write an entire line. first row =====================//
void line1(unsigned char arr[16])
 {
	char j;
	instruction_lcd(0x80); // begin at address 0x80 (first row left end)
	instruction_lcd(0x06);	//  address  increment
	for(j=0;j<16;j++)
	data_out_lcd(arr[j]);
 }


//====================================== write an entire line. second row ====================//
void line2(unsigned char arr[16])
 {
	char j;
	instruction_lcd(0xc0); // begin at address 0xc0 (second row left end)
	instruction_lcd(0x06); //  address  increment
	for(j=0;j<16;j++)
	data_out_lcd(arr[j]);
 }


//====================================== display a number with 3 digits ======================//
void display_lcd(unsigned char num)
 {
	instruction_lcd(0xc7); // begin at address 0xc0 (second row left end)
	instruction_lcd(0x06); //  address  increment
	data_out_lcd(num/100+0x30);
	data_out_lcd(num/10%10+0x30);
	data_out_lcd(num%10+0x30);
 }

	
//====================================== dc motor functions ==================================//
void speed(unsigned char dc) //dc=duty cycle (%)
 {
	t1=65535-dc*10; 
	t2=64535+dc*10;
 }

 
void init_timer1()
 {
	EA=1; 
	ET1=1;
	TMOD|=0x10;//0 0 0 1 0 0 0 0, mode 1
	TH1=252  ;
	TL1=0   ;
	speed(1);//motor stop
	TR1=1   ;//Start the timer
 }


void timer1() interrupt 3
 {
	if (dcMotorPin==1)
	 {
		dcMotorPin=0;
		TH1=t1/256  ;
		TL1=t1%256   ;
	 }
	else
	 {
		dcMotorPin=1;
		TH1=t2/256  ;
		TL1=t2%256  ;
	 }
 }


//====================================== PWM servomotor functions ============================//
void init_pwm()
 {
	CCAPM0= 0x42;//01000010 ECOM0=1,PWM0=1 // pin P1_3
	CCAPM1= 0x42;//01000010 ECOM1=1,PWM1=1 // pin P1_4
	CCAPM2= 0x42;//01000010 ECOM2=1,PWM2=1 // pin P1_5
	CCAPM3= 0x42;//01000010 ECOM3=1,PWM3=1 // pin P1_6
	
    CMOD  = 0x84;//10000000 CIDL=1 // Fclock= overflow timer 0
	CCON= 0x40;//01000000 CR=1  // PCA counter run
 }


void init_timer0()
 {
	TMOD|=0X02; //mode 2
	TH0=184; //256-72 //(20msec/256)*11.0592/12 = 72
	TR0=1;
 }
	
		
//====================================== adc functions =======================================//
void init_clock()
 {
	T2MOD|=0x02;
	T2CON&=~0x02;
	RCAP2H=0xff;
	RCAP2L=0x00;
	TH2=0xff;
	TL2=0x00;
	T2CON|=0x04;
 }


unsigned char conversion( unsigned char a)
 {
	start=0;
	adr &= 0x8f;
	adr |= (a<<4) ;
	delay(10);
	start=1;
	delay(10);
	start=0;
	delay(100);
	while(!eoc);
	digital_out=P0;
	return digital_out;
 }


//====================================== bdikat pgiya ========================================//
char target_check(char num)			 
 {
	char hit=0;
	ldr_target=conversion(2*num);
	ldr_ref=conversion(2*num+1);
	if((ldr_ref-ldr_target)>100){hit=1;x=100;score++;}
	return hit;
 }


//====================================== game ================================================//
void game_on()
 {
	speed(50);
	delay(1000);
	rand=0;
	for(r=0;r<30;r++)	 // lifting 30 targets alternately, only once at the time
	 {
		rand=conversion(rand); // the choice of the target is random
		rand=rand%4; // can be 0 or 1 or 2 or 3 
		delay(500);
		if(!w2)
		 {
			switch(rand)
			 {
				case 0:{CCAP0H= 230;for(x=0;x<100;x++)target_check(0);CCAP0H= 243;}break;	  // 100 are 15 seconds	approximatly
				case 1:{CCAP1H= 230;for(x=0;x<100;x++)target_check(1);CCAP1H= 243;}break;
				case 2:{CCAP2H= 230;for(x=0;x<100;x++)target_check(2);CCAP2H= 243;}break;
				case 3:{CCAP3H= 230;for(x=0;x<100;x++)target_check(3);CCAP3H= 243;}break;
			 }
		 }
		display_lcd(score);
		if(w2) {CCAP2H= 230;while(!target_check(2));score--;w2=0;}//if there is a pause, waiting to the end of the pause
	 }
 }


//====================================== external interrupt 0 ================================//
void int_0 () // external interrupt 0
 {
	EA=1; // אפשור כללי של פסיקות
	EX0=1; // אפשור פסיקה חיצונית 0
	IT0=1; //  לאפשר פסיקה חיצונית 0 בירידת שעון
 }


void reset_or_pause() interrupt 0
 {
	EX0=0;
	CCAP0H= 243;
	CCAP1H= 243; 
	CCAP2H= 230;
	CCAP3H= 230;
	do
	 {
		w2=target_check(2);
		w3=target_check(3);
	 }
	while(!w2&&!w3); 
	if(w2){ x=100;score--;} //pause
	if(w3){score--;r=30;CCAP2H= 243;w3=0;}  // reset, back to the beginning
	CCAP3H= 243;
	delay(500); // delay for the end of laser hit
	EX0=1;
 }


//====================================== main function =======================================//
void main()
 {
	delay(1000);
	init_lcd(); 
	init_clock();

	init_pwm();
	init_timer0();
	init_timer1();
	int_0 ();
	CCAP0H= 243;  //0 degree
	CCAP1H= 243;  // 0 degree
	CCAP2H= 243;
	CCAP3H= 243;
	line1("  weapon for    ");	 // 16 characters in english
	line2("combat practice ");	  // 16 characters in english 
	delay(3000);
	line2("                ");
	while(1)
	 {
		w2=0;
		w3=0;
		CCAP0H= 230;  //90 degrees
		CCAP1H= 243;  // 0 degree
		CCAP2H= 243;
		CCAP3H= 243;
		line1(" start practice!");
		while(!target_check(0)); //waiting for the beginning of the game
		line1("                ");
		line2("score:      hits");
		CCAP0H= 243; // 0 deg
		score=0;
		display_lcd(score);

		game_on();
		line1("the game result:");
		display_lcd(score);
		delay(2000);
	 }
 }
