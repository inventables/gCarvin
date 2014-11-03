/*
  carvin.c - Handles Carvin Controller specific items
  Part of Grbl v0.9

  Copyright (c) 2014 Bart Dring / Inventables  

  GNU General Public License
  See <http://www.gnu.org/licenses/>.
*/

#include "system.h"
#include "settings.h"
#include "carvin.h"

int off_button_counter = 0;   // this times how long the button has been held in to turn the machine off

// setup routine for a Carvin Controller
void carvin_init()
{
  
	// -------------------------------------
	
	// make sure steppers are disabled at startup.	
	STEPPERS_DISABLE_PORT |= (1<<STEPPERS_DISABLE_BIT);
	
	

  // setup the LED pins as outputs
  BUTTON_LED_DDR |= (1<<BUTTON_LED_BIT);
  DOOR_LED_DDR |= (1<<DOOR_LED_BIT);
  SPINDLE_LED_DDR |= (1<<SPINDLE_LED_BIT);
	
	
	
	#ifdef WAIT_FOR_BUTTON
	// -------------------- Be sure button is not in at startup ----------------------------
	// the button may still be pushed from a prevoius power down
	// wait until it comes up for a while
	
	long switch_open_count = 0;

	// wait for switch to open for given time
	
	while (switch_open_count < BUTTON_UP_WAIT_TIME)
	{
		if (bit_istrue(PINOUT_PIN,bit(PIN_CYCLE_START)))
			switch_open_count++;
		else
			switch_open_count = 0; // reset from debounce
	}
	
  // --------------------- Wait for turn on button push ---------------------------------
	

  // Wait in this loop until front "on" button is pushed
  // TO DO... make sure the push to turn off does not continue part reset and right through this  
  while (bit_istrue(PINOUT_PIN,bit(PIN_CYCLE_START)))
  {
	// do nothing until the button is pushed
	// TO DO should we filter this so noise cannot turn on thre unit
  }
	#endif
	
  
	// -------------- PWM ------------------------------
	
  //  Setup PWM For LEDs
  //  Timer 4 ... it controls the following pins
  //  Mega pin 6, PORTH BIT3, OCR4A (Button LED)
  //  Mega pin 7, PORTH BIT4, OCR4B (Door Light)
  //  Mega pin 8, PORTH BIT5, OCR4C (Spidle Light) 
  TCCR4A = (1<<COM4A1) | (1<<COM4B1) | (1<<COM4C1) |(1<<WGM41) | (1<<WGM40);
  TCCR4B = (TCCR4B & 0b11111000) | 0x02; // set to 1/8 Prescaler
  //  Set initial duty cycles
  BUTTON_LED_OCR = 0; 
  DOOR_LED_OCR = 0;
  SPINDLE_LED_OCR = 0;
  
  // ---------------- TIMER3 ISR SETUP --------------------------
  
  // Setup a timer3 interrupt to handle timing of things like LED animations in the background
  TCCR3A = 0;     // Clear entire TCCR1A register
  TCCR3B = 0;     // Clear entire TCCR1B register
  
  TCCR3B |= (1 << WGM32);  // turn on CTC mode:
  TCCR3B |= (1 << CS32);   // divide clock/256 
  
  OCR3A = CARVIN_TIMER3_CTC;  // set compare match register to desired timer count:
  
  TIMSK3 |= (1 << OCIE3A);  // enable timer compare interrupt (grbl turns on interrupts)
  
  // ----------------Initial LED SETUP -----------------------------
  
  // setup LEDs
  init_led(&button_led);
	init_led(&door_led);
	init_led(&spindle_led);
  
  // fade on the button and door LEDs at startup	
  set_led(&button_led, 255,3);
	set_led(&door_led, 255,3);
	
	// done initializing Carvin specific things
	
}

// Timer3 Interrupt
// keep this fast to not bother the stepper timing
// Things done here......
// 	LED animations
//  Timing the off button push

ISR(TIMER3_COMPA_vect)
{
  // see if the led values need to change
	if (led_level_change(&button_led))
	  BUTTON_LED_OCR = button_led.current_level;
		
	if (led_level_change(&door_led))
	  DOOR_LED_OCR = door_led.current_level;
		
	if (led_level_change(&spindle_led))
	  SPINDLE_LED_OCR = spindle_led.current_level;
	
  // if the button is pushed, count up to see if it is held long enough to reset cpu
  if (bit_isfalse(PINOUT_PIN,bit(PIN_CYCLE_START)))
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

// the returns the switch states in the form "SW XYZPD:11111" XYZ limits, then probe, then door
// It is called by the "?" command in garble.
void print_sw_states()
{
    printPgmString(PSTR(",SW XYZPD:")); 
		
		if (bit_istrue(LIMIT_PIN,bit(X_LIMIT_BIT)))
			printPgmString(PSTR("0"));
		else
			printPgmString(PSTR("1"));
	  
		if (bit_istrue(LIMIT_PIN,bit(Y_LIMIT_BIT)))
			printPgmString(PSTR("0"));
		else
			printPgmString(PSTR("1"));
			
		if (bit_istrue(LIMIT_PIN,bit(Z_LIMIT_BIT)))
			printPgmString(PSTR("0"));
		else
			printPgmString(PSTR("1"));
			
		if (bit_istrue(PROBE_PIN,bit(PROBE_BIT)))
			printPgmString(PSTR("0"));
		else
			printPgmString(PSTR("1"));
			
		if (bit_istrue(PINOUT_PIN,bit(PIN_FEED_HOLD)))
			printPgmString(PSTR("0"));
		else
			printPgmString(PSTR("1"));
}