/*        Salem Guevara M. & sguev010@ucr.edu

 *         Discussion Section: 022

 *         Assignment: Custom Lab Demo 3

 *         I acknowledge all content contained herein, excluding template or example code, is my own original work.

 *         Demo Link: https://www.youtube.com/watch?v=p_L2HtBmQdw

 */

#include "timerISR.h"
#include "helper.h"
#include "periph.h"
#include "serialATmega.h"
#include "spiAVR.h"
#include "LCD.h"
#include <avr/eeprom.h>

#define EEADDR 0 //fixed address to write the HI score count 
#define EEDATA 0 //data to be written (Hi Score) we can replace this with the Hi Score data /
#define NUM_TASKS 4 //TODO: Change to the number of tasks being used

int score = 0;
int press = 0;
int curr_Row = 0; char rowDat = 0; char colDat = 0;
int j;
unsigned char row = 254;
unsigned char col = 32;
int HS = 0; int W; int L = 0;
//put EEPROM data to this variable
uint16_t EEByte;

//Task struct for concurrent synchSMs implmentations
typedef struct _task{
	signed 	 char state; 		//Task's current state
	unsigned long period; 		//Task period
	unsigned long elapsedTime; 	//Time elapsed since last task tick
	int (*TickFct)(int); 		//Task tick function
} task;

//TODO: Define Periods for each task
const unsigned long CONTROLLER_PERIOD = 200;
const unsigned long LCD_PERIOD = 1000;
const unsigned long BUTTON_PERIOD = 5;
const unsigned long GAME_PERIOD = 10;
const unsigned long GCD_PERIOD = 5;//TODO:Set the GCD Period

task tasks[NUM_TASKS]; // declared task array with 5 tasks
char* intToStr(int num) {
    static char result[2];
    int i = sizeof(result) - 1;
    do {
        result[i--] = (num % 10) + '0';
        num /= 10;
    } while (num > 0);
    return &result[i + 1];
}

void shiftOut(char row, char col) {
  for (int i = 0; i < 8; i++) {
    // Set the data pin to the correct bit
    PORTB = SetBit(PORTB, 0, GetBit(row, i)); 
    PORTB = SetBit(PORTB, 4, GetBit(col, i));
    PORTB = SetBit(PORTB, 2, 1); //Shift
    PORTB = SetBit(PORTB, 5, 1);
    _delay_us(1);
    PORTB = SetBit(PORTB, 2, 0);
    PORTB = SetBit(PORTB, 5, 0);
    _delay_us(1);
  }

  PORTB = SetBit(PORTB, 1, 1); //Store
  PORTB = SetBit(PORTB, 3, 1);
  _delay_us(1);
  PORTB = SetBit(PORTB, 1, 0);
  PORTB = SetBit(PORTB, 3, 0);
  _delay_us(1);
}
const int L_bitmap[8] = {

    0b00000000, // Row 0
    0b00000000, // Row 1
    0b00100000, // Row 2
    0b00100000, // Row 3 (initially off)
    0b00100000, // Row 4 (initially off)
    0b00100000, // Row 5 (initially off)
    0b00100000, // Row 6 (initially off)
    0b00111110  // Row 7 (initially off)

};
const int W_bitmap[8] = {

    0b10000010, // Row 0
    0b10000010, // Row 1
    0b10000010, // Row 2
    0b10000010, // Row 3 (initially off)
    0b10010010, // Row 4 (initially off)
    0b10101010, // Row 5 (initially off)
    0b11000110, // Row 6 (initially off)
    0b10000010  // Row 7 (initially off)

};
const int pattern[8] = {

    0b10001111, // Row 0
    0b11000111, // Row 1
    0b11100011, // Row 2
    0b11110001, // Row 3 (initially off)
    0b11111000, // Row 4 (initially off)
    0b11110001, // Row 5 (initially off)
    0b11100011, // Row 6 (initially off)
    0b10000111  // Row 7 (initially off)
};
uint8_t Happy[8] = {
  0b00000,
  0b01010,
  0b01010,
  0b01010,
  0b00000,
  0b11111,
  0b10001,
  0b01110
};
uint8_t Sad[8] = {
  0b00000,
  0b10001,
  0b01010,
  0b01010,
  0b00000,
  0b01110,
  0b10001,
  0b11111
};
void update_leds(char i) {

    for (curr_Row = 7; curr_Row >= 0; curr_Row--) {

        rowDat = pattern[(curr_Row + i) % 8];

        colDat = ~(1 << curr_Row);

        shiftOut(colDat, rowDat);

    }

    shiftOut(0xf0, 0x00);

    shiftOut(row, col);

}

//TODO: Create your tick functions for each task

enum LCD_States {Init, Display};
int TickFct_LCD (int state) {
  switch (state) {
    case Init:
      lcd_clear();
      state = Display;
      break;
    case Display: 
      lcd_clear();
      state = Display;
      break;
  }
  switch (state) {
    case Init:
      break;
    case Display:
      if (press && !W && !L) {
        lcd_clear();
        lcd_goto_xy(0,1);
        lcd_write_str(const_cast<char*>("LIGHT, APPEAR!"));
        lcd_goto_xy(1,0);
        lcd_write_str(const_cast<char*>("SCORE: "));
        lcd_write_str(const_cast<char*>(intToStr(score)));
      }
      else if (HS) {
        lcd_clear();
        lcd_goto_xy(0,1);
        lcd_write_str(const_cast<char*>("HIGHSCORE: "));
        lcd_write_str(const_cast<char*>(intToStr(eeprom_read_byte((uint8_t*)EEADDR))));
      }
      else if (W) {
        lcd_clear();
        //Create custom character
        lcd_write_custom_char(1, Happy);
        // Move cursor to home position
        lcd_command(0x80);
        // Display custom character
        lcd_data(1);
      }
      else if (L) {
        lcd_clear();
        // Create custom character
        lcd_write_custom_char(1, Sad);
        // Move cursor to home position
        lcd_command(0x80);
        // Display custom character
        lcd_data(1);
      }
      else {
        lcd_clear();
        lcd_goto_xy(0,0);
        lcd_write_str(const_cast<char*>("PRESS TO START"));
        lcd_goto_xy(1,0);
        lcd_write_str(const_cast<char*>("SCORE: "));
        lcd_write_str(const_cast<char*>(intToStr(score)));
      }
      break;
  }
  return state;
}

enum Controller {CINIT, Idle, Horizontal};
int TickFct_Controller (int state) {
  switch(state) {
    case CINIT:
      state = Idle;
      break;

    case Idle:
      if (ADC_read(1) > 600 || ADC_read(1) < 450) {state = Horizontal;}
      else {state = Idle;}
      break;

    case Horizontal:
        if (ADC_read(1) < 600 && ADC_read(1) > 450) {state = Idle;}
        else {state = Horizontal;}
        break;
  }

  switch(state) {
    case CINIT:
      //Intial position: Row 6, Column 4
      shiftOut(row, col);
      break;
    case Idle:
      break;
    case Horizontal:
      if (ADC_read(1) > 600) {
        //Move character to the right
        if ((col >> 1) == 0) {col = col;} 
        else if (col == -128) {col = 64;}
        else {col = col >> 1;}
        shiftOut(row, col);
      }

      else if (ADC_read(1) < 400) {
        //Move character to the left
        if ((col << 1) > 128) {col = 128;}
        else {col = col << 1;}
        shiftOut(row, col);
      }
      break; 
  }

  return state;
}

enum Press {Menu, Hold_Play, Hold_HS, Hold_Menu, Play};
int TickFct_Button(int state) {
  switch (state) {
    case Menu:
      if (GetBit(PINC, 3)) {state = Hold_Play;}
      else if (GetBit(PINC, 0)) {HS = 1; state = Hold_HS;}
      else {state = Menu;}
      break;

    case Hold_Play:
      if (!GetBit(PINC, 3)) {press = 1; state = Play;}
      else {state = Hold_Play;}
      break;
    
    case Hold_HS: 
      if (!GetBit(PINC, 0)) {state = Menu;}
      else {state = Hold_HS;}
      break;

    case Play:
      if (GetBit(PINC, 3)) {press = 0; state = Hold_Menu;}
      else if (GetBit(PINC, 0)) {HS = 1; state = Hold_HS;}
      else {state = Play;}
      break;

    case Hold_Menu:
      if (!GetBit(PINC, 3)) {state = Menu;}
      else if (!GetBit(PINC, 0)) {state = Menu;}
      else {state = Hold_Menu;}
      break;
  }
  switch (state) {
    case Menu:
      HS = 0;
      break;
    case Hold_Play:
      break;
    case Hold_HS:
      HS = 1;
      break;
    case Play:
      break;
    case Hold_Menu:
      break;
  }
  return state;
}

enum Play {Stop, Run, Win, Lose};
int TickFct_Game(int state) {
static int i = 0;
static int j = 0;
  switch (state) {
    case Stop:
      if (press == 1) {score = 0; state = Run;}
      else {state = Stop;}
      break;
    case Run:
      if (press == 1) {state = Run;}
      if (press == 0) {state = Stop;}
      if (pattern[7-i] & col) {press = 0; L = 1; state = Lose;} //Collision Check
      if (score == 5) {press = 0; W = 1; state = Win;}
      break;
    case Win:
      if (press == 1) { 
        row = 254; col = 32;
        shiftOut(row, col); 
        rowDat = 0; colDat = 0; 
        i = 0; j = 0; 
        W = 0; L = 0;
        score = 0;
        state = Run;
      }
      else {state = Win;}
      break;

    case Lose:
      if (press == 1) {
        row = 254; col = 32;
        shiftOut(row, col); 
        rowDat = 0; colDat = 0; 
        i = 0; j = 0; 
        L = 0; W = 0;
        score = 0;
        state = Stop;
      }
      else {state = Lose;}
      break;
  }
  switch (state) {
    case Stop:
      i = 0; j = 0;
      row = 254; col = 32;
      shiftOut(row, col); 
      break;
    case Run:
        update_leds(i);
        j++;
        if (j > 100) {
            i++;
            score++;
            j = 0;
        }
        if (i > 7) i = 0;
      break;
    case Win:
      for (curr_Row = 0; curr_Row < 8; curr_Row++) {
        char rowDat = W_bitmap[curr_Row];
        char colDat = ~(1 << (8 - curr_Row));
        shiftOut(colDat, rowDat);
      }
      shiftOut(0xf0, 0x00);
      EEByte = eeprom_read_byte((uint8_t*)EEADDR);
      if (score > EEByte) {
        eeprom_write_byte ((uint8_t*)EEADDR, score);
      }
      break;

    case Lose:
      for (curr_Row = 0; curr_Row < 8; curr_Row++) {
        char rowDat = L_bitmap[curr_Row];
        char colDat = ~(1 << (8 - curr_Row));
        shiftOut(colDat, rowDat);
      }
      shiftOut(0xf0, 0x00);
      EEByte = eeprom_read_byte((uint8_t*)EEADDR);
      if (score > EEByte) {
        eeprom_write_byte ((uint8_t*)EEADDR, score);
      }
      break;
  }
  return state;
}

void TimerISR() {
	for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {                   // Iterate through each task in the task array
		if ( tasks[i].elapsedTime == tasks[i].period ) {           // Check if the task is ready to tick
			tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and set the next state for this task
			tasks[i].elapsedTime = 0;                          // Reset the elapsed time for the next tick
		}
		tasks[i].elapsedTime += GCD_PERIOD;                        // Increment the elapsed time by GCD_PERIOD
	}
}

int main(void) {
    //TODO: initialize all your inputs and ouputs
    j = 0;
    ADC_init();   // initializes ADC
    lcd_init();
    serial_init(9600);
    eeprom_write_byte ((uint8_t*)EEADDR, EEDATA);

    DDRB = 0xFF; 
    PORTB = 0x00; 
    DDRD = 0xFF; //0000 1100
    PORTD = 0x00;
    DDRC = 0x00; //Setting Port C pins to output
    PORTC = 0x00;
    //TODO: Initialize tasks here

    tasks[j].period = CONTROLLER_PERIOD;
    tasks[j].state = CINIT;
    tasks[j].elapsedTime = tasks[j].period;
    tasks[j].TickFct = &TickFct_Controller;
    j++;

    tasks[j].period = LCD_PERIOD;
    tasks[j].state = Init;
    tasks[j].elapsedTime = tasks[j].period;
    tasks[j].TickFct = &TickFct_LCD;
    j++;

    tasks[j].period = BUTTON_PERIOD;
    tasks[j].state = Menu;
    tasks[j].elapsedTime = tasks[j].period;
    tasks[j].TickFct = &TickFct_Button;
    j++;

    tasks[j].period = GAME_PERIOD;
    tasks[j].state = Stop;
    tasks[j].elapsedTime = tasks[j].period;
    tasks[j].TickFct = &TickFct_Game;
    j++;

    TimerSet(GCD_PERIOD);
    TimerOn();

    while (1) {}
    return 0;
}
