/*
  carvin.h - Handles Carvin Controller specific items
  Part of Grbl v0.9

  Copyright (c) 2014 Bart Dring / Inventables  

  GNU General Public License
  See <http://www.gnu.org/licenses/>.
*/

#ifndef carvin_h
#define carvin_h

// Note Carvin uses Mega 2560 resources (timers, PWM) so it cannot run on UNO hardware

#define BUTTON_UP_WAIT_TIME 6000
#define OFF_BUTTON_COUNT 1000 // about 2 seconds



#define LED_FULL_ON 255
#define LED_FULL_OFF 0

#define CARVIN_TIMING_CTC 120  // timer interrupt compare value...set this for a roughly 512 hz interrupt, so we can fade 256 levels in 1/2 second

struct led_analog{
  unsigned char target;        // what is the desired brightness
  unsigned char current_level; // how bright is is now
  unsigned char duration;      // time in 1/2 seconds to get there (3 = 1.5 seconds)...0 = on right away
  unsigned char dur_counter;   // where are we in the duration delay
  unsigned char throb;         // 1 = light will throb
  unsigned char throb_min;     // what is the minimum brightness of the throb.  It can look harsh if it goes off or nearly off
};

void carvin_init();

// functions to work with the LEDs
void init_led(struct led_analog * led);
void set_led(struct led_analog * led, unsigned char target_level, unsigned char duration);
int led_level_change(struct led_analog * led);  // checks to see if a level change is needed

void print_sw_states(); // print the switch state like "SW XYZPD"  X,Y,Z, Probe, Door  

void reset_cpu();   // software full reset of the CPU

// the LEDs
struct led_analog button_led;
struct led_analog door_led;
struct led_analog spindle_led;  
#endif