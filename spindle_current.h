/*
  spindle_current.h - Handles monitoring spindle current
  
  Copyright (c) 2016 Inventables Inc.
*/

#include <stdint.h>

#ifndef SPINDLE_CURRENT_H
#define SPINDLE_CURRENT_H

// scale 2.56V = 1023 on ADC
#define SPINDLE_I_COUNT       10    // in the 512Hz interrupt this will get us 51.2Hz current readings
#define SPINDLE_I_THRESHOLD   1.75  // Default current threshold at which system should alarm
#define SPINDLE_I_SETTING_MAX 3.0

/// Initializes the spindle current measurement subsystem
extern void spindle_current_init( uint8_t enable );

extern uint8_t spindle_current_is_enabled( void );

/// Updates the spindle current state and values and returns true if spindle 
/// current exceeds maximum.
/// @note To be called periodically
/// @return Spindle Current above threshold level
/// @retval 0 Under threshold
/// @retval 1 Above threshold
extern uint8_t spindle_current_proc( void );

/// Gets the average spindle current value in ADC counts
extern uint16_t spindle_current_get_counts( void );

/// Gets the average spindle current value in Amps
extern float spindle_current_get( void );

/// Sets the threshold current level
/// @param[in] threshold A threshold value in Amps used in comparison in 
/// spindle_current_proc to not exceed SPINDLE_I_SETTING_MAX
extern void spindle_current_set_threshold( float threshold );

#endif
