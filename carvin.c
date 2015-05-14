/*
  carvin.c - Handles Carvin Controller specific items
  
  Copyright (c) 2014,2015 Bart Dring / Inventables  

*/

#include "system.h"
#include "settings.h"
#include "carvin.h"

int off_button_counter = 0;   // this times how long the button has been held in to turn the machine off

// setup routine for a Carvin Controller
void carvin_init()
{
  
	
	// make sure steppers are disabled at startup.	
	STEPPERS_DISABLE_PORT |= (1<<STEPPERS_DISABLE_BIT);
	
  // setup the PWM pins as outputs
  BUTTON_LED_DDR |= (1<<BUTTON_LED_BIT);
  DOOR_LED_DDR |= (1<<DOOR_LED_BIT);
  SPINDLE_LED_DDR |= (1<<SPINDLE_LED_BIT);
	STEPPER_VREF_DDR |= (1<<STEPPER_VREF_BIT);
	
	set_stepper_current(0);
	
	
	
	#ifdef USE_BUTTON_FOR_ON
	
	long switch_open_count = 0;
	
	// -------------------- Be sure button is not pushed in at startup ----------------------------
	// the button may still be pushed from a prevoius power down
	// wait until it comes up for a while
	// wait for switch to open for given time	
	while (switch_open_count < BUTTON_UP_WAIT_TIME)
	{
		if (bit_istrue(CONTROL_PIN,bit(CYCLE_START_BIT)))
			switch_open_count++;   // count up only while button is not pushed
		else
			switch_open_count = 0; // reset count
	}
	
  // --------------------- Wait for turn on button push ---------------------------------
	

  // Wait in this loop until front "on" button is pushed
  while (bit_istrue(CONTROL_PIN,bit(CYCLE_START_BIT)))
  {
	// do nothing until the button is pushed
  }
	#endif
	
	
  
	// -------------- Setup PWM on Timer 4 ------------------------------
	
  //  Setup PWM For LEDs
  //  --------- Timer 4 ... it controls the following pins ----------
  //  Mega pin 6, PORTH BIT3, OCR4A (Button LED)
  //  Mega pin 7, PORTH BIT4, OCR4B (Door Light)  !!!! schem error
  //  Mega pin 8, PORTH BIT5, OCR4C (Spidle Light) 
  TCCR4A = (1<<COM4A1) | (1<<COM4B1) | (1<<COM4C1) |(1<<WGM41) | (1<<WGM40);
  TCCR4B = (TCCR4B & 0b11111000) | 0x02; // set to 1/8 Prescaler
  //  Set initial duty cycles
  BUTTON_LED_OCR = 0; 
  //DOOR_LED_OCR = 0;
  SPINDLE_LED_OCR = 0;
	
	
	
	
	// -------------- Setup PWM on Timer 3 ------------------------------
	
  //  Setup PWM For Door LED
  //  --------- Timer 3 ... it controls the following pins ----------
  //  PORTE BIT 3, OCR3A (stepper driver current)
  //  PORTE BIT4, OCR3B (Door Light)  !!!! schem error
  
	TCCR3A = (1<<COM3A1) | (1<<COM3B1) | (1<<WGM31) | (1<<WGM30);
  TCCR3B = (TCCR3B & 0b11111000) | 0x02; // set to 1/8 Prescaler
  //  Set initial duty cycles 
  DOOR_LED_OCR = 700;
	
	
  
  
  // ---------------- TIMER5 ISR SETUP --------------------------
  
  // Setup a timer5 interrupt to handle timing of things like LED animations in the background
  TCCR5A = 0;     // Clear entire TCCR1A register
  TCCR5B = 0;     // Clear entire TCCR1B register
  
  TCCR5B |= (1 << WGM52);  // turn on CTC mode:
  TCCR5B |= (1 << CS52);   // divide clock/256 
  
  OCR5A = CARVIN_TIMING_CTC;  // set compare match register to desired timer count:
  
  TIMSK5 |= (1 << OCIE5A);  // enable timer compare interrupt (grbl turns on interrupts)
  
  // ----------------Initial LED SETUP -----------------------------
  
  // setup LEDs
  init_led(&button_led);
	init_led(&door_led);
	init_led(&spindle_led);
  
  // fade on the button and door LEDs at startup	
  set_led(&button_led, 255,3);
	set_led(&door_led, 255,3);
	
	// done initializing Carvin specific things
	
	set_stepper_current(STEPPER_RUN_CURRENT);
	
}

// Timer3 Interrupt
// keep this fast to not bother the stepper timing
// Things done here......
// 	LED animations
//  Timing the off button push

ISR(TIMER5_COMPA_vect)
{
  // see if the led values need to change
	if (led_level_change(&button_led))
	  BUTTON_LED_OCR = button_led.current_level;
		
	if (led_level_change(&door_led))
	  DOOR_LED_OCR = door_led.current_level;
		
	if (led_level_change(&spindle_led))
	  SPINDLE_LED_OCR = spindle_led.current_level;
	
	#ifdef USE_BUTTON_FOR_ON
  // if the button is pushed, count up to see if it is held long enough to reset cpu
  if (bit_isfalse(CONTROL_PIN,bit(CYCLE_START_BIT)))
  {
		if (off_button_counter == OFF_BUTTON_COUNT)
			reset_cpu();
		else
			off_button_counter++;
  }
  else
  {
		off_button_counter = 0;  // reset the count
  }
	#endif
	
}

// init or reset the led values
void init_led(struct led_analog * led)
{
  (* led).current_level = 0;   
  (* led).duration = 0;        
  (* led).dur_counter = 0;     
  (* led).throb = 0;					 
  (* led).throb_min = 0;
  (* led).target = 0;
}

// setup an LED with a new brightness level ... change is done via ISR
void set_led(struct led_analog * led, uint8_t target_level, uint8_t duration)
{
	(* led).duration = duration;
	(* led).throb = false;
	(* led).target = target_level;
	
}

// setup an LED for throb ... throbing is done via ISR
void throb_led(struct led_analog * led, uint8_t min_throb, uint8_t duration)
{
	(* led).duration = duration;	
	(* led).throb = true;	
	(* led).target = LED_FULL_ON; // asume throb on first
}

// Adjusts the level of an LED
// Returns true if the value changed
// This is called by the timer ISR...there is no reason to call it outside the ISR
int led_level_change(struct led_analog * led)
{
  // see if the value needs to change
	if ((* led).target == (* led).current_level)
		return false;
	
	// if duration is 0 change the level right away
	if ((* led).duration == 0)  
	{
		(* led).current_level = (* led).target;
		return true;
	}
	
	
	// the duration counter causes the function to be called more than once before it makes a change by counting down to 1
	if ((* led).dur_counter > 1)
	{
	  (* led).dur_counter--;  // count down to 1
	}
	else
	{
		if ((* led).current_level < (* led).target)
		{
			(* led).current_level++;
			if ((* led).throb && (* led).current_level == LED_FULL_ON)  // check to see if we need to reverse the throb
				(* led).target = (* led).throb_min;
		}
		else
		{
			(* led).current_level--;
			if ((* led).throb && (* led).current_level <= (* led).throb_min)  // check to see if we need to reverse the throb
				(* led).target = LED_FULL_ON;
		}
		
				
		(* led).dur_counter = (* led).duration;  // reset the duration counter
		return true;
	 
  }
	
	return false;
	
}

void set_stepper_current(float current)
{
	
	float vref = 0;
  
  // current = VREF /(8× RS)  from driver datasheet
  vref = current * (8 * I_SENSE_RESISTOR);  
 
  // vref goes through a resistor dividor that cuts the voltage in half
  vref = (vref /2.5) * 1023 / 2;
  
  STEPPER_VREF_OCR = (int)vref;
	
	
}

// This is a software reset using the watchdog timer
void reset_cpu()
{
  STEPPERS_DISABLE_PORT |= (1<<STEPPERS_DISABLE_BIT); // turn off the stepper drivers
	
  wdt_enable(WDTO_15MS);
  while(1)
  {
		// wait for it...boom
  }
}

