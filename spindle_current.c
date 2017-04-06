/*
  spindle_current.h - Handles monitoring spindle current
  
  Copyright (c) 2016, 2017 Inventables Inc.
*/

#include "spindle_current.h"
#include "system.h"
#include "settings.h"
#include "carvin.h"

#define SPINDLE_I_MULTIPLIER  256ul
#define SPINDLE_I_AVG_CONST   252ul // the constant used to average the current
#define SPINDLE_CURRENT_AMPS_PER_COUNT (2.56/1023.0)
static uint8_t enabled = 0U;
static uint16_t spindle_current;
static uint16_t spindle_I_max;
static float spindle_I_max_amps;
static uint8_t spindle_current_counter;

void spindle_current_init( uint8_t enable )
{
  enabled = enable;
  
  spindle_I_max_amps = SPINDLE_I_THRESHOLD;
  ps_settings_get_setting( 0, (uint8_t*)(&spindle_I_max_amps) );
  spindle_current_set_threshold( spindle_I_max_amps );
  
  if ( enabled )
  {
    // Enable ADC. Free running mode. Prescale=16.  2.56V internal voltage reference.
    ADCSRA = (1<<ADEN)|(1<<ADATE)|(1<<ADPS2);
    ADMUX  = (1<<REFS1 | (1<<REFS0));
    ADCSRA |= (1<<ADSC);
  }
  else
  {
    ADCSRA = 0U;
    ADMUX  = 0U;
    ADCSRA &= ~(1<<ADSC);
  }
  
  spindle_current = 0U;
  spindle_current_counter = SPINDLE_I_COUNT;
}

uint8_t spindle_current_is_enabled( void )
{
  return enabled;
}

uint8_t spindle_current_proc( void )
{
  uint8_t threshold_exceeded = 0U;
  if ( enabled )
  {
    if ( spindle_current_counter == 0U )
    {
      unsigned long measurement = ADCL;
      float previous_spindle_I_max_amps = spindle_I_max_amps;
      measurement += ((long)ADCH) << 8U;
      
      // apply filter algorithm and store result
      spindle_current = (SPINDLE_I_AVG_CONST * (long)spindle_current 
                         + (SPINDLE_I_MULTIPLIER - SPINDLE_I_AVG_CONST) * measurement )
                        / SPINDLE_I_MULTIPLIER;
      
      ps_settings_get_setting( 0, (uint8_t*)(&spindle_I_max_amps) );
      
      if ( spindle_I_max_amps != spindle_I_max_amps )
      {
        spindle_current_set_threshold( spindle_I_max_amps );
      }
      
      // check against threshold
      if ( spindle_current > spindle_I_max )
      {
        threshold_exceeded = 1U;
      }
      
      spindle_current_counter = SPINDLE_I_COUNT;
    }
    else
    {
      --spindle_current_counter;
    }
  }
  
  return threshold_exceeded;
}

uint16_t spindle_current_get_counts( void )
{
  return spindle_current;
}

float spindle_current_get( void )
{
  return ( spindle_current * SPINDLE_CURRENT_AMPS_PER_COUNT );
}

void spindle_current_set_threshold( float threshold )
{
  if ( threshold <= SPINDLE_I_SETTING_MAX )
  {
    spindle_I_max = threshold / SPINDLE_CURRENT_AMPS_PER_COUNT;
  }
  else
  {
    spindle_I_max = SPINDLE_I_SETTING_MAX / SPINDLE_CURRENT_AMPS_PER_COUNT;
  }
}
