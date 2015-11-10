/*
  spindle_control.c - spindle control methods
  Part of Grbl

  Copyright (c) 2012-2015 Sungeun K. Jeon
  Copyright (c) 2009-2011 Simen Svale Skogsrud

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "grbl.h"


void spindle_init()
{    
  SPINDLE_PWM_DDR |= (1<<SPINDLE_PWM_BIT); // Configure as PWM output pin.
  
  // setup PWM
  TCCRA_REGISTER = (1<<COMB_BIT) | (1<<WAVE1_REGISTER) | (1<<WAVE0_REGISTER);
  TCCRB_REGISTER = (TCCRB_REGISTER & 0b11111000) | 0x02; // set to 1/8 Prescaler
    
  // start with spindle off
  set_pwm(&spindle_motor, 0,0);
}


void spindle_stop()
{
  // On the Uno, spindle enable and PWM are shared. Other CPUs have seperate enable pin.
  set_pwm(&spindle_motor, 0,SPINDLE_SPINDOWN_RATE);  // fade off spindle
	
  // LED cross fade
  set_pwm(&spindle_led, 0, SPINDLE_SPINDOWN_RATE + EXTEND_SPINDLE_LED_FADE); // fade off spindle LED
  set_pwm(&door_led, DOOR_LED_LEVEL_IDLE, SPINDLE_SPINDOWN_RATE + EXTEND_SPINDLE_LED_FADE); // fade on door LED
  
  
  
}


void spindle_set_state(uint8_t state, float rpm)
{
	
	
  // Halt or set spindle direction and rpm. 
  if (state == SPINDLE_DISABLE) {

    spindle_stop();

  } 
  else 
  {

   // convert the desired speed to the 8 bit pwm value
  uint8_t current_pwm;
  // Calculate PWM register value based on rpm max/min settings and programmed rpm.
  if (settings.rpm_max <= settings.rpm_min) {
	// No PWM range possible. Set simple on/off spindle control pin state.
	current_pwm = PWM_MAX_VALUE;
  } else {
	if (rpm > settings.rpm_max) { rpm = settings.rpm_max; }
	if (rpm < settings.rpm_min) { rpm = settings.rpm_min; }
	#ifdef SPINDLE_MINIMUM_PWM
	  float pwm_gradient = (PWM_MAX_VALUE-SPINDLE_MINIMUM_PWM)/(settings.rpm_max-settings.rpm_min);
	  current_pwm = floor( (rpm-settings.rpm_min)*pwm_gradient + (SPINDLE_MINIMUM_PWM+0.5));
	#else
	  float pwm_gradient = (PWM_MAX_VALUE)/(settings.rpm_max-settings.rpm_min);
	  current_pwm = floor( (rpm-settings.rpm_min)*pwm_gradient + 0.5);
	#endif
  }  
  
	set_pwm(&spindle_motor, current_pwm,SPINDLE_SPINUP_RATE);  // fade on spindle
	set_pwm(&spindle_led, SPINDLE_LED_LEVEL_RUN,SPINDLE_SPINUP_RATE + EXTEND_SPINDLE_LED_FADE);    // fade on spindle LED
	set_pwm(&door_led, DOOR_LED_LEVEL_RUN, SPINDLE_SPINUP_RATE + EXTEND_SPINDLE_LED_FADE);      // fade off door
     
  }
}

void spindle_run(uint8_t state, float rpm)
{
  if (sys.state == STATE_CHECK_MODE) { return; }
  protocol_buffer_synchronize(); // Empty planner buffer to ensure spindle is set when programmed.  
  spindle_set_state(state, rpm);
}
