/*
  carvin.h - Handles Carvin Controller specific items
  
  Copyright (c) 2014,2015 Bart Dring / Inventables  

*/

#ifndef carvin_h
#define carvin_h

// Note Carvin uses Mega 2560 resources (timers, PWM) so it cannot run on UNO hardware

#define BUTTON_UP_WAIT_TIME 6000
#define OFF_BUTTON_COUNT 1000 // about 2 seconds


// LED Mode Levels
#define LED_FULL_ON 255
#define LED_FULL_OFF 0

#define DOOR_LED_LEVEL_IDLE 255
#define DOOR_LED_LEVEL_RUN 100
#define DOOR_LED_THROB_MIN 60

#define SPINDLE_LED_LEVEL_IDLE 0
#define SPINDLE_LED_LEVEL_RUN 255
#define SPINDLE_LED_THROB_MIN 60

#define BUTTON_LED_LEVEL_ON 255
#define BUTTON_LED_LEVEL_OFF 0
#define BUTTON_LED_THROB_MIN 60
#define BUTTON_LED_THROB_RATE 1 // 1/2 second

#define CARVIN_TIMING_CTC 120  // timer interrupt compare value...set this for a roughly 512 hz interrupt, so we can fade 256 levels in 1/2 second

#define CONTROL_DEBOUNCE_COUNT 8 // this is count down by timer5

extern int control_button_counter;  // Used to debounce the control button.

struct pwm_analog{
  unsigned char target;        // what is the desired brightness
  unsigned char current_level; // how bright is is now
  unsigned char duration;      // time in 1/2 seconds to get there (3 = 1.5 seconds)...0 = on right away
  unsigned char dur_counter;   // where are we in the duration delay
  unsigned char throb;         // 1 = light will throb...fade on...fade off
  unsigned char throb_min;     // what is the minimum brightness of the throb.  It can look harsh if it goes off or nearly off
};

void carvin_init();

// functions to work with the LEDs
void init_pwm(struct pwm_analog * led);
void set_pwm(struct pwm_analog * led, unsigned char target_level, unsigned char duration);
int pwm_level_change(struct pwm_analog * led);  // checks to see if a level change is needed



void print_switch_states();

void reset_cpu();   // software full reset of the CPU

// the LEDs
struct pwm_analog button_led;
struct pwm_analog door_led;
struct pwm_analog spindle_led;
struct pwm_analog spindle_motor;


#endif