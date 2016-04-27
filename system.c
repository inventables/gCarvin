/*
  system.c - Handles system level commands and real-time processes
  Part of Grbl

  Copyright (c) 2014-2015 Sungeun K. Jeon  

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


void system_init()
{
  CONTROL_DDR &= ~(CONTROL_MASK); // Configure as input pins
  #ifdef DISABLE_CONTROL_PIN_PULL_UP
    CONTROL_PORT &= ~(CONTROL_MASK); // Normal low operation. Requires external pull-down.
  #else
    CONTROL_PORT |= CONTROL_MASK;   // Enable internal pull-up resistors. Normal high operation.
  #endif
  CONTROL_PCMSK |= CONTROL_MASK;  // Enable specific pins of the Pin Change Interrupt
  PCICR |= (1 << CONTROL_INT);   // Enable Pin Change Interrupt
}


// Returns control pin state as a uint8 bitfield. Each bit indicates the input pin state, where 
// triggered is 1 and not triggered is 0. Invert mask is applied. Bitfield organization is
// defined by the CONTROL_PIN_INDEX in the header file.
uint8_t system_control_get_state()
{
  uint8_t control_state = 0;
  uint8_t pin = (CONTROL_PIN & CONTROL_MASK);
  #ifdef INVERT_CONTROL_PIN_MASK
    pin ^= INVERT_CONTROL_PIN_MASK;
  #endif
  if (pin) {
    if (bit_istrue(pin,(1<<CONTROL_SAFETY_DOOR_BIT))) { control_state |= CONTROL_PIN_INDEX_SAFETY_DOOR; }
    if (bit_istrue(pin,(1<<CONTROL_RESET_BIT))) { control_state |= CONTROL_PIN_INDEX_RESET; }
    if (bit_istrue(pin,(1<<CONTROL_FEED_HOLD_BIT))) { control_state |= CONTROL_PIN_INDEX_FEED_HOLD; }
    if (bit_istrue(pin,(1<<CONTROL_CYCLE_START_BIT))) { control_state |= CONTROL_PIN_INDEX_CYCLE_START; }
  }
  return(control_state);
}


// Pin change interrupt for pin-out commands, i.e. cycle start, feed hold, and reset. Sets
// only the realtime command execute variable to have the main program execute these when 
// its ready. This works exactly like the character-based realtime commands when picked off
// directly from the incoming serial data stream.
ISR(CONTROL_INT_vect) 
{
  #ifdef CARVIN
	/* Carvey has to deal with the button differently because the front button
	   does two function hold/reset and therefore needs to be debounced
	*/
	control_button_counter = CONTROL_DEBOUNCE_COUNT;  // the inital count is set here
	return;
  #endif
  
  uint8_t pin = system_control_get_state();
  if (pin) { 
    if (bit_istrue(pin,CONTROL_PIN_INDEX_RESET)) {
      mc_reset();
    } else if (bit_istrue(pin,CONTROL_PIN_INDEX_CYCLE_START)) {
      bit_true(sys_rt_exec_state, EXEC_CYCLE_START);
    } else if (bit_istrue(pin,CONTROL_PIN_INDEX_FEED_HOLD)) {
      bit_true(sys_rt_exec_state, EXEC_FEED_HOLD); 
    } else if (bit_istrue(pin,CONTROL_PIN_INDEX_SAFETY_DOOR)) {
      bit_true(sys_rt_exec_state, EXEC_SAFETY_DOOR);
    } 
  }
}

void checkControlPins()
{
  /*
	CARVIN There are only 2 control pins
		The front button, which is CONTROL_CYCLE_START_BIT
	    The door, which is CONTROL_SAFETY_DOOR_BIT
		
		The front button
			Does nothing in IDLE
			Does Cycle Start when in hold
			Does hold when not in hold		
			
			
  */	
  
  
  
  uint8_t pin = system_control_get_state();  // this returns the control pin corrected with mask and inverts
  
  //print_uint8_base2(pin);
  //printPgmString(PSTR("=Pin\r\n"));
  
  if (pin) { 
  
  
	    //print_uint8_base2(sys.state);
		//printPgmString(PSTR("=State\r\n"));
		
		//print_uint8_base2(sys.suspend);
		//printPgmString(PSTR("=Suspend\r\n"));
  
    if (bit_istrue(pin,CONTROL_PIN_INDEX_CYCLE_START))		
	{
	   
		
	  if (sys.state != STATE_IDLE)  // button only does something when not in idle
	  {
		if (sys.state == STATE_SAFETY_DOOR)
		{								
			bit_true(sys_rt_exec_state, EXEC_CYCLE_START);			
		}
		else
		{
			bit_true(sys_rt_exec_state, EXEC_SAFETY_DOOR);			
		}
	  }
	  
	  
      
	} 
	else if (bit_istrue(pin,CONTROL_PIN_INDEX_SAFETY_DOOR))
	{	
      bit_true(sys_rt_exec_state, EXEC_SAFETY_DOOR);
    } 
  }
	
	
  
}


// Returns if safety door is ajar(T) or closed(F), based on pin state.
uint8_t system_check_safety_door_ajar()
{
	#ifdef CARVIN
		if (! use_door_feature)
			return false;
	#endif
    return(system_control_get_state() & CONTROL_PIN_INDEX_SAFETY_DOOR);
	
}


// Executes user startup script, if stored.
void system_execute_startup(char *line) 
{
  uint8_t n;
  for (n=0; n < N_STARTUP_LINE; n++) {
    if (!(settings_read_startup_line(n, line))) {
      report_status_message(STATUS_SETTING_READ_FAIL);
    } else {
      if (line[0] != 0) {
        printString(line); // Echo startup line to indicate execution.
        report_status_message(gc_execute_line(line));
      }
    } 
  }  
}


// Directs and executes one line of formatted input from protocol_process. While mostly
// incoming streaming g-code blocks, this also executes Grbl internal commands, such as 
// settings, initiating the homing cycle, and toggling switch states. This differs from
// the realtime command module by being susceptible to when Grbl is ready to execute the 
// next line during a cycle, so for switches like block delete, the switch only effects
// the lines that are processed afterward, not necessarily real-time during a cycle, 
// since there are motions already stored in the buffer. However, this 'lag' should not
// be an issue, since these commands are not typically used during a cycle.
uint8_t system_execute_line(char *line) 
{   
  uint8_t char_counter = 1; 
  uint8_t helper_var = 0; // Helper variable
  float parameter, value;  
  switch( line[char_counter] ) {
    case 0 : report_grbl_help(); break;
    case '$': case 'G': case 'C': case 'X':
      if ( line[(char_counter+1)] != 0 ) { return(STATUS_INVALID_STATEMENT); }
      switch( line[char_counter] ) {
        case '$' : // Prints Grbl settings
          if ( sys.state & (STATE_CYCLE | STATE_HOLD) ) { return(STATUS_IDLE_ERROR); } // Block during cycle. Takes too long to print.
          else { report_grbl_settings(); }
          break;
        case 'G' : // Prints gcode parser state
          // TODO: Move this to realtime commands for GUIs to request this data during suspend-state.
          report_gcode_modes();
          break;   
        case 'C' : // Set check g-code mode [IDLE/CHECK]
          // Perform reset when toggling off. Check g-code mode should only work if Grbl
          // is idle and ready, regardless of alarm locks. This is mainly to keep things
          // simple and consistent.
          if ( sys.state == STATE_CHECK_MODE ) { 
            mc_reset(); 
            report_feedback_message(MESSAGE_DISABLED);
          } else {
            if (sys.state) { return(STATUS_IDLE_ERROR); } // Requires no alarm mode.
            sys.state = STATE_CHECK_MODE;
            report_feedback_message(MESSAGE_ENABLED);
          }
          break; 
        case 'X' : // Disable alarm lock [ALARM]
          if (sys.state == STATE_ALARM) { 
            // Block if safety door is ajar.
            if (system_check_safety_door_ajar()) { return(STATUS_CHECK_DOOR); }
            report_feedback_message(MESSAGE_ALARM_UNLOCK);
            sys.state = STATE_IDLE;
            // Don't run startup script. Prevents stored moves in startup from causing accidents.
          } // Otherwise, no effect.
          break;                   
    //  case 'J' : break;  // Jogging methods
          // TODO: Here jogging can be placed for execution as a seperate subprogram. It does not need to be 
          // susceptible to other realtime commands except for e-stop. The jogging function is intended to
          // be a basic toggle on/off with controlled acceleration and deceleration to prevent skipped 
          // steps. The user would supply the desired feedrate, axis to move, and direction. Toggle on would
          // start motion and toggle off would initiate a deceleration to stop. One could 'feather' the
          // motion by repeatedly toggling to slow the motion to the desired location. Location data would 
          // need to be updated real-time and supplied to the user through status queries.
          //   More controlled exact motions can be taken care of by inputting G0 or G1 commands, which are 
          // handled by the planner. It would be possible for the jog subprogram to insert blocks into the
          // block buffer without having the planner plan them. It would need to manage de/ac-celerations 
          // on its own carefully. This approach could be effective and possibly size/memory efficient.  
      }
      break;
    default : 
      // Block any system command that requires the state as IDLE/ALARM. (i.e. EEPROM, homing)
	  
      if ( !(sys.state == STATE_IDLE || sys.state == STATE_ALARM) ) { return(STATUS_IDLE_ERROR); }
      switch( line[char_counter] ) {
        case '#' : // Print Grbl NGC parameters
          if ( line[++char_counter] != 0 ) { return(STATUS_INVALID_STATEMENT); }
          else { report_ngc_parameters(); }
          break;          
        case 'H' : // Perform homing cycle [IDLE/ALARM]
		  //printPgmString(PSTR("Home req"));	
          if (bit_istrue(settings.flags,BITFLAG_HOMING_ENABLE)) { 
            // Block if safety door is ajar.
            if (system_check_safety_door_ajar()) { return(STATUS_CHECK_DOOR); }
            sys.state = STATE_HOMING; // Set system state variable
            mc_homing_cycle(); 
            if (!sys.abort) {  // Execute startup scripts after successful homing.
              sys.state = STATE_IDLE; // Set to IDLE when complete.
              st_go_idle(); // Set steppers to the settings idle state before returning.
              system_execute_startup(line); 
            }
          } else { return(STATUS_SETTING_DISABLED); }
          break;
        case 'I' : // Print or store build info. [IDLE/ALARM]
          if ( line[++char_counter] == 0 ) { 
            settings_read_build_info(line);
            report_build_info(line);
          } else { // Store startup line [IDLE/ALARM]
            if(line[char_counter++] != '=') { return(STATUS_INVALID_STATEMENT); }
            helper_var = char_counter; // Set helper variable as counter to start of user info line.
            do {
              line[char_counter-helper_var] = line[char_counter];
            } while (line[char_counter++] != 0);
            if (char_counter > (EEPROM_LINE_SIZE-helper_var)) { return(STATUS_LINE_LENGTH_EXCEEDED); }
            settings_store_build_info(line);
          }
          break; 
		  
		#ifdef CARVIN
		case 'K':
			reset_cpu();  // hard reset
		break;
		case 'L':
		    if(line[++char_counter] == '1')
			{
				throb_pwm(&button_led, BUTTON_LED_THROB_MIN,2);
				throb_pwm(&door_led, DOOR_LED_THROB_MIN,2);
				throb_pwm(&spindle_led, SPINDLE_LED_THROB_MIN,2);
			}
			else
			{
				//turn all LEDS off
				set_pwm(&button_led, 0,4);
				set_pwm(&door_led, 0,4);
				set_pwm(&spindle_led, 0,4);
			}
		break;
		case 'D':
			use_door_feature = (! use_door_feature);
			use_sleep_feature = (! use_sleep_feature);
			printPgmString(PSTR("[Door Feature "));
			print_uint8_base10(use_door_feature);
			printPgmString(PSTR(" Sleep Feature "));
			print_uint8_base10(use_sleep_feature);
			printPgmString(PSTR(" ]\r\n"));			
			
		break;
		case 'S':
			print_switch_states();
		break;
		#endif  
		  
        case 'R' : // Restore defaults [IDLE/ALARM]
          if (line[++char_counter] != 'S') { return(STATUS_INVALID_STATEMENT); }
          if (line[++char_counter] != 'T') { return(STATUS_INVALID_STATEMENT); }
          if (line[++char_counter] != '=') { return(STATUS_INVALID_STATEMENT); }
          if (line[char_counter+2] != 0) { return(STATUS_INVALID_STATEMENT); }                        
          switch (line[++char_counter]) {
            case '$': settings_restore(SETTINGS_RESTORE_DEFAULTS); break;
            case '#': settings_restore(SETTINGS_RESTORE_PARAMETERS); break;
            case '*': settings_restore(SETTINGS_RESTORE_ALL); break;
            default: return(STATUS_INVALID_STATEMENT);
          }
          report_feedback_message(MESSAGE_RESTORE_DEFAULTS);
          mc_reset(); // Force reset to ensure settings are initialized correctly.
          break;
        case 'N' : // Startup lines. [IDLE/ALARM]
          if ( line[++char_counter] == 0 ) { // Print startup lines
            for (helper_var=0; helper_var < N_STARTUP_LINE; helper_var++) {
              if (!(settings_read_startup_line(helper_var, line))) {
                report_status_message(STATUS_SETTING_READ_FAIL);
              } else {
                report_startup_line(helper_var,line);
              }
            }
            break;
          } else { // Store startup line [IDLE Only] Prevents motion during ALARM.
            if (sys.state != STATE_IDLE) { return(STATUS_IDLE_ERROR); } // Store only when idle.
            helper_var = true;  // Set helper_var to flag storing method. 
            // No break. Continues into default: to read remaining command characters.
          }
        default :  // Storing setting methods [IDLE/ALARM]
          if(!read_float(line, &char_counter, &parameter)) { return(STATUS_BAD_NUMBER_FORMAT); }
          if(line[char_counter++] != '=') { return(STATUS_INVALID_STATEMENT); }
          if (helper_var) { // Store startup line
            // Prepare sending gcode block to gcode parser by shifting all characters
            helper_var = char_counter; // Set helper variable as counter to start of gcode block
            do {
              line[char_counter-helper_var] = line[char_counter];
            } while (line[char_counter++] != 0);
            if (char_counter > EEPROM_LINE_SIZE) { return(STATUS_LINE_LENGTH_EXCEEDED); }
            // Execute gcode block to ensure block is valid.
            helper_var = gc_execute_line(line); // Set helper_var to returned status code.
            if (helper_var) { return(helper_var); } // Return error.
            else { 
              helper_var = trunc(parameter); // Set helper_var to int value of parameter
              settings_store_startup_line(helper_var,line);
            }
          } else { // Store global setting.
            if(!read_float(line, &char_counter, &value)) { return(STATUS_BAD_NUMBER_FORMAT); }
            if((line[char_counter] != 0) || (parameter > 255)) { return(STATUS_INVALID_STATEMENT); }
            return(settings_store_global_setting((uint8_t)parameter, value));
          }
      }    
  }
  return(STATUS_OK); // If '$' command makes it to here, then everything's ok.
}


// Returns machine position of axis 'idx'. Must be sent a 'step' array.
// NOTE: If motor steps and machine position are not in the same coordinate frame, this function
//   serves as a central place to compute the transformation.
float system_convert_axis_steps_to_mpos(int32_t *steps, uint8_t idx)
{
  float pos;
  #ifdef COREXY
    if (idx==A_MOTOR) { 
      pos = 0.5*((steps[A_MOTOR] + steps[B_MOTOR])/settings.steps_per_mm[idx]);
    } else if (idx==B_MOTOR) {
      pos = 0.5*((steps[A_MOTOR] - steps[B_MOTOR])/settings.steps_per_mm[idx]);
    } else {
      pos = steps[idx]/settings.steps_per_mm[idx];
    }
  #else
    pos = steps[idx]/settings.steps_per_mm[idx];
  #endif
  return(pos);
}


void system_convert_array_steps_to_mpos(float *position, int32_t *steps)
{
  uint8_t idx;
  for (idx=0; idx<N_AXIS; idx++) {
    position[idx] = system_convert_axis_steps_to_mpos(steps, idx);
  }
  return;
}


// Special handlers for setting and clearing Grbl's real-time execution flags.
void system_set_exec_state_flag(uint8_t mask) {
  uint8_t sreg = SREG; 
  cli(); 
  sys_rt_exec_state |= (mask);
  SREG = sreg;
}

void system_clear_exec_state_flag(uint8_t mask) {
  uint8_t sreg = SREG; 
  cli(); 
  sys_rt_exec_state &= ~(mask);
  SREG = sreg;
}

void system_set_exec_alarm_flag(uint8_t mask) {
  uint8_t sreg = SREG; 
  cli(); 
  sys_rt_exec_alarm |= (mask);
  SREG = sreg;
}

void system_clear_exec_alarm_flag(uint8_t mask) {
  uint8_t sreg = SREG; 
  cli(); 
  sys_rt_exec_alarm &= ~(mask);
  SREG = sreg;
}
