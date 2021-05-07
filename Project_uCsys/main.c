/*	
   ___           _           _       ___ _             _                     
  / _ \_ __ ___ (_) ___  ___| |_    / _ \ | __ _ _ __ | |_ ___ __ _ _ __ ___ 
 / /_)/ '__/ _ \| |/ _ \/ __| __|  / /_)/ |/ _` | '_ \| __/ __/ _` | '__/ _ \
/ ___/| | | (_) | |  __/ (__| |_  / ___/| | (_| | | | | || (_| (_| | | |  __/
\/    |_|  \___// |\___|\___|\__| \/    |_|\__,_|_| |_|\__\___\__,_|_|  \___|
              |__/  -MADE BY EMMA, HÅVARD, JOHAN & JONAS                                          
			           																						*/

#define F_CPU 16000000UL
#define BAUDRATE 9600
#define UBRR_VALUE ((F_CPU / (BAUDRATE * 16UL)) - 1)
#define A0 0
#define A1 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "USART.h"
#include "lcdpcf8574.h"

/* Function declarations */

void initLCD(void);

void initADC(void);

void initInterupts(void);

void initTimer(void);

void printInt(int32_t val);

uint8_t analogRead(int Pin);

int moistureLevel();

int lightLevel();

int doEveryS(int s);

void LCDMenu(void);

/* Struct & variable declaration */

volatile uint8_t menu;

typedef struct{
	
	uint16_t moisture;
	uint16_t ambient;
	
}data;

data sensor;

int main(void) {
	
	/* Inits */
	
	initUSART();
	initTimer();
	initInterupts();
	initADC();
	initLCD();

	while (1) { /* Main loop */
		
		if(doEveryS(1)) {
			
			moistureLevel(); //reads moisture sensor and prints comment from plant on serial
			lightLevel();    //reads light sensor
						
			/*	Prints to serial monitor */			
			printInt(sensor.ambient);
			printString(" % Light, moisture level ");
			printInt(sensor.moisture);
			printString("\n\n");
		}
		
	LCDMenu(); 

	}
	return 0;
}


ISR(INT0_vect) /* Interupt subroutine triggered by button*/
{	
	menu++; //Used to scroll the menu

	if (menu >= 3){ //resets menu to zero if it exceeds 2.
		menu = 0;
	}

}

void initADC(void) {
	DDRD = 0xff; 
	PORTD = 0x00;
	ADMUX = (1<<REFS0) | (1<< ADLAR); 
	ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
}

void initInterupts(void) {
	EICRA = (1<<ISC01); //Trigger on negative flank
	EIMSK = (1<<INT0); // Enable INT0 vector
	sei(); // enable global interrupts
}

void initTimer(void) {
	TCCR1B = (1<<CS10) | (1<<CS12); //set the pre-scalar as 1024
	OCR1A = 15620;        //1000ms delay
	TCNT1 = 0;
}

void initLCD(void) {
	
	DDRC |= (1<<DDC5) | (1<<DDC4);	// PC5 and PC4 as outputs for SCL and SDA
	PORTC |= (1<<DDC5) | (1<<DDC4);	// Start as high
		
	lcd_init(LCD_DISP_ON);				// Initialize LCD with display on, no cursor.
	lcd_home();							// Go to start position, alternative: lcd_gotoxy(0,0);
	lcd_led(0);							// Set LCD backlight ON (0)
	lcd_puts("* $ # Project");			// Put string onto LCD display
	lcd_gotoxy(0,1);
	lcd_puts("| | | Plantcare");
	_delay_ms(2500);
	lcd_clrscr();
}

void LCDMenu(void) {
	
	static int prev_menu;
	
	if (!(menu == prev_menu)) {
		lcd_clrscr(); //clears LCD if menu has changed
	}
	
	prev_menu = menu;
	
	char moist_string[3];
	itoa(sensor.moisture,moist_string,10);			// int to str/char
	
	char light_string[3];
	itoa(sensor.ambient,light_string,10);			// int to str/char
	
	switch(menu)
	{
		case 0:
			lcd_home();
			lcd_puts("Moisture Level:");	// Put string onto LCD display
			lcd_gotoxy(0,1);
			lcd_puts(moist_string);
			lcd_gotoxy(2,1);
			lcd_puts("(0 to 5)");
			break;

		case 1:
			lcd_home();
			lcd_puts("Light Level:");	// Put string onto LCD display
			lcd_gotoxy(0,1);
			lcd_puts(light_string);
			lcd_gotoxy(strlen(light_string),1); //Moves the percentage depending on the number
			lcd_puts("%");
			lcd_gotoxy(strlen(light_string) + 1,1); //Clears trailing % sign
			lcd_puts("    ");
			break;

		case 2:
			lcd_home();
			lcd_puts("Flower:");
			lcd_gotoxy(0,1);
			
			switch(sensor.moisture) //Antropomorphized feedback from Mr. Flower
			{
				case 0:
				lcd_puts("Practically dead");
				break;

				case 1:
				lcd_puts("I'm dehydrated! ");
				break;

				case 2:
				lcd_puts("I need water..  ");
				break;

				case 3:
				lcd_puts("one Drink please");
				break;
				
				case 4:
				lcd_puts("Life is good    ");
				break;
				
				case 5:
				lcd_puts("I cant swim!!!  ");
				break;
				
				default:
				break;
			}
			break;
		
		
		default:
		break;
	}

}

int doEveryS(int s) {
	
	static int i;
	OCR1A = 15620; //flags every 1000ms
	
	if (TIFR1 & (1<<OCF1A)) {
		TCNT1 = 0;
		TIFR1 |= (1<<OCF1A) ; //clear timer1 overflow flag
		i++;
	}
	
	if (i >= s){
		i = 0;
		return 1;
	}
	else{
		return 0;
	}
	
}

void printInt(int32_t val) {
	
	int32_t num = val;
	char snum[100];

	// convert 123 to string [buf]
	itoa(num, snum, 10);
	
	printString(snum);
}

uint8_t analogRead(int Pin) {
	
	if(Pin == A0) {
		ADMUX &= ~(1<<MUX0); //Enables A0 pin for ADC
	}
	if(Pin == A1) {
		ADMUX |= (1<<MUX0); //Enables A1 pin for ADC
	}
	
	ADCSRA |= (1<<ADSC); //Starts read
	loop_until_bit_is_clear(ADCSRA, ADSC); //Wait until finished.
	
	return ADCH;
}

int moistureLevel() {
	
	sensor.moisture = analogRead(A1) * 5 / 255; //sets range to 0-5
	
	switch(sensor.moisture)
	{
		case 0:
		printString("Im practically dead! -Flower. \n");
		break;

		case 1:
		printString("I'm severely dehydrated! -Flower. \n");
		break;

		case 2:
		printString("I need water.. NOW!! -Flower. \n");
		break;

		case 3:
		printString("Wouldnt mind a drink! -Flower. \n");
		break;
	
		case 4:
		printString("Life is good indeed! -Flower. \n");
		break;
		
		case 5:
		printString("Who dropped me into a pool! \n");
		printString("I cant swim!!! -Flower \n");
		break;
		
		default:
		break;
	}
	return sensor.moisture;
}

int lightLevel() {
	sensor.ambient = (analogRead(A0) * 100) / 255; //Scales photoresistor reading in percentages
	return sensor.ambient;
}

