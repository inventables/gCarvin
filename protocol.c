/*
  protocol.c - controls Grbl execution protocol and procedures
  Part of Grbl
  
  Copyright (c) 2011-2015 Sungeun K. Jeon  
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

// Define line flags. Includes comment type tracking and line overflow detection.
#define LINE_FLAG_OVERFLOW bit(0)
#define LINE_FLAG_COMMENT_PARENTHESES bit(1)
#define LINE_FLAG_COMMENT_SEMICOLON bit(2)


static char line[LINE_BUFFER_SIZE]; // Line to be executed. Zero-terminated.

static void protocol_exec_rt_suspend();


/* 
  GRBL PRIMARY LOOP:
*/
void protocol_main_loop()
{
  // ------------------------------------------------------------
  // Complete initialization procedures upon a power-up or reset.
  // ------------------------------------------------------------
  
  // Print welcome message   
  report_init_message();

  // Check for and report alarm state after a reset, error, or an initial power up.
  if (sys.state == STATE_ALARM) {
    report_feedback_message(MESSAGE_ALARM_LOCK); 
  } else {
    // All systems go! But first check for safety door.
    sys.state = STATE_IDLE;
    if (system_check_safety_door_ajar()) {
      bit_true(sys_rt_exec_state, EXEC_SAFETY_DOOR);
      protocol_execute_realtime(); // Enter safety door mode. Should return as IDLE state.
    } 
    system_execute_startup(line); // Execute startup script.
  }
    
  // ---------------------------------------------------------------------------------  
  // Primary loop! Upon a system abort, this exits back to main() to reset the system. 
  // This is also where Grbl idles while waiting for something to do.
  // ---------------------------------------------------------------------------------  
  
  uint8_t line_flags = 0;
  uint8_t char_counter = 0;
  uint8_t c;
  for (;;) {

    // Process one line of incoming serial data, as the data becomes available. Performs an
    // initial filtering by removing spaces and comments and capitalizing all letters.    
    while((c = serial_read()) != SERIAL_NO_DATA) {
      if ((c == '\n') || (c == '\r')) { // End of line reached

        protocol_execute_realtime(); // Runtime command check point.
        if (sys.abort) { return; } // Bail to calling function upon system abort  

        line[char_counter] = 0; // Set string termination character.
        #ifdef REPORT_ECHO_LINE_RECEIVED
          report_echo_line_received(line);
        #endif

        // Direct and execute one line of formatted input, and report status of execution.
        if (line_flags & LINE_FLAG_OVERFLOW) {
          // Report line overflow error.
          report_status_message(STATUS_OVERFLOW); 
        } else if (line[0] == 0) {
          // Empty or comment line. For syncing purposes.
          report_status_message(STATUS_OK);
        } else if (line[0] == '$') {
          // Grbl '$' system command
          report_status_message(system_execute_line(line));
        } else if (sys.state == STATE_ALARM) {
          // Everything else is gcode. Block if in alarm mode.
          report_status_message(STATUS_ALARM_LOCK); 
        } else {
          // Parse and execute g-code block.
          report_status_message(gc_execute_line(line)); 
        }
        
        // Reset tracking data for next line.
        line_flags = 0;
        char_counter = 0;
        
      } else {
      
        if (line_flags) {
          // Throw away all (except EOL) comment characters and overflow characters.
          if (c == ')') {
            // End of '()' comment. Resume line allowed.
            if (line_flags & LINE_FLAG_COMMENT_PARENTHESES) { line_flags &= ~(LINE_FLAG_COMMENT_PARENTHESES); }
          }
        } else {
          if (c <= ' ') { 
            // Throw away whitepace and control characters  
          } else if (c == '/') { 
            // Block delete NOT SUPPORTED. Ignore character.
            // NOTE: If supported, would simply need to check the system if block delete is enabled.
          } else if (c == '(') {
            // Enable comments flag and ignore all characters until ')' or EOL.
            // NOTE: This doesn't follow the NIST definition exactly, but is good enough for now.
            // In the future, we could simply remove the items within the comments, but retain the
            // comment control characters, so that the g-code parser can error-check it.
            line_flags |= LINE_FLAG_COMMENT_PARENTHESES;
          } else if (c == ';') {
            // NOTE: ';' comment to EOL is a LinuxCNC definition. Not NIST.
            line_flags |= LINE_FLAG_COMMENT_SEMICOLON;
          // TODO: Install '%' feature 
          // } else if (c == '%') {
            // Program start-end percent sign NOT SUPPORTED.
            // NOTE: This maybe installed to tell Grbl when a program is running vs manual input,
            // where, during a program, the system auto-cycle start will continue to execute 
            // everything until the next '%' sign. This will help fix resuming issues with certain
            // functions that empty the planner buffer to execute its task on-time.
          } else if (char_counter >= (LINE_BUFFER_SIZE-1)) {
            // Detect line buffer overflow and set flag.
            line_flags |= LINE_FLAG_OVERFLOW;
          } else if (c >= 'a' && c <= 'z') { // Upcase lowercase
            line[char_counter++] = c-'a'+'A';
          } else {
            line[char_counter++] = c;
          }
        }
        
      }
    }
    
    // If there are no more characters in the serial read buffer to be processed and executed,
    // this indicates that g-code streaming has either filled the planner buffer or has 
    // completed. In either case, auto-cycle start, if enabled, any queued moves.
    protocol_auto_cycle_start();

    protocol_execute_realtime();  // Runtime command check point.
    if (sys.abort) { return; } // Bail to main() program loop to reset system.
              
    #ifdef SLEEP_ENABLE
      // Check for sleep conditions and execute auto-park, if timeout duration elapses.
      sleep_check();    
    #endif
  }
  
  return; /* Never reached */
}


// Block until all buffered steps are executed or in a cycle state. Works with feed hold
// during a synchronize call, if it should happen. Also, waits for clean cycle end.
void protocol_buffer_synchronize()
{
  // If system is queued, ensure cycle resumes if the auto start flag is present.
  protocol_auto_cycle_start();
  do {
    protocol_execute_realtime();   // Check and execute run-time commands
    if (sys.abort) { return; } // Check for system abort
  } while (plan_get_current_block() || (sys.state == STATE_CYCLE));
}


// Auto-cycle start has two purposes: 1. Resumes a plan_synchronize() call from a function that
// requires the planner buffer to empty (spindle enable, dwell, etc.) 2. As a user setting that 
// automatically begins the cycle when a user enters a valid motion command manually. This is 
// intended as a beginners feature to help new users to understand g-code. It can be disabled
// as a beginner tool, but (1.) still operates. If disabled, the operation of cycle start is
// manually issuing a cycle start command whenever the user is ready and there is a valid motion 
// command in the planner queue.
// NOTE: This function is called from the main loop, buffer sync, and mc_line() only and executes 
// when one of these conditions exist respectively: There are no more blocks sent (i.e. streaming 
// is finished, single commands), a command that needs to wait for the motions in the buffer to 
// execute calls a buffer sync, or the planner buffer is full and ready to go.
void protocol_auto_cycle_start() 
{
  if (plan_get_current_block() != NULL) { // Check if there are any blocks in the buffer.
    system_set_exec_state_flag(EXEC_CYCLE_START); // If so, execute them!
  } 
}


// This function is the general interface to Grbl's real-time command execution system. It is called
// from various check points in the main program, primarily where there may be a while loop waiting 
// for a buffer to clear space or any point where the execution time from the last check point may 
// be more than a fraction of a second. This is a way to execute realtime commands asynchronously 
// (aka multitasking) with grbl's g-code parsing and planning functions. This function also serves
// as an interface for the interrupts to set the system realtime flags, where only the main program
// handles them, removing the need to define more computationally-expensive volatile variables. This
// also provides a controlled way to execute certain tasks without having two or more instances of 
// the same task, such as the planner recalculating the buffer upon a feedhold or overrides.
// NOTE: The sys_rt_exec_state variable flags are set by any process, step or serial interrupts, pinouts,
// limit switches, or the main program.
void protocol_execute_realtime()
{
  protocol_exec_rt_system();
  if (sys.suspend) { protocol_exec_rt_suspend(); }
}


// Executes run-time commands, when required. This function primarily operates as Grbl's state 
// machine and controls the various real-time features Grbl has to offer. 
// NOTE: Do not alter this unless you know exactly what you are doing!
void protocol_exec_rt_system()
{
  uint8_t rt_exec; // Temp variable to avoid calling volatile multiple times.
  rt_exec = sys_rt_exec_alarm; // Copy volatile sys_rt_exec_alarm.
  if (rt_exec) { // Enter only if any bit flag is true
    // System alarm. Everything has shutdown by something that has gone severely wrong. Report
    // the source of the error to the user. If critical, Grbl disables by entering an infinite
    // loop until system reset/abort.
    sys.state = STATE_ALARM; // Set system alarm state
    if (rt_exec & EXEC_ALARM_HARD_LIMIT) {
      report_alarm_message(ALARM_HARD_LIMIT_ERROR); 
    } else if (rt_exec & EXEC_ALARM_SOFT_LIMIT) {
      report_alarm_message(ALARM_SOFT_LIMIT_ERROR);
    } else if (rt_exec & EXEC_ALARM_ABORT_CYCLE) {      
      report_alarm_message(ALARM_ABORT_CYCLE);
    } else if (rt_exec & EXEC_ALARM_PROBE_FAIL) {
      report_alarm_message(ALARM_PROBE_FAIL);
    } else if (rt_exec & EXEC_ALARM_HOMING_FAIL) {
      report_alarm_message(ALARM_HOMING_FAIL);
    }
    // Halt everything upon a critical event flag. Currently hard and soft limits flag this.
    if (rt_exec & EXEC_CRITICAL_EVENT) {
      report_feedback_message(MESSAGE_CRITICAL_EVENT);
      system_clear_exec_state_flag(EXEC_RESET); // Disable any existing reset
      do {       
        // Block everything, except reset and status reports, until user issues reset or power 
        // cycles. Hard limits typically occur while unattended or not paying attention. Gives 
        // the user and a GUI time to do what is needed before resetting, like killing the
        // incoming stream. The same could be said about soft limits. While the position is not 
        // lost, streaming could cause a serious crash if it continues afterwards.
        
// TODO: Allow status reports during a critical alarm. Still need to think about implications of this.
        // if (sys_rt_exec_state & EXEC_STATUS_REPORT) { 
        //   report_realtime_status();
        //   system_clear_exec_state_flag(EXEC_STATUS_REPORT); 
        // }

      } while (bit_isfalse(sys_rt_exec_state,EXEC_RESET));
    }
    system_clear_exec_alarm_flag(0xFF); // Clear all alarm flags
  }
  
  rt_exec = sys_rt_exec_state; // Copy volatile sys_rt_exec_state.
  if (rt_exec) {
  
    // Execute system abort. 
    if (rt_exec & EXEC_RESET) {
      sys.abort = true;  // Only place this is set true.
      return; // Nothing else to do but exit.
    }

    // Execute and serial print status
    if (rt_exec & EXEC_STATUS_REPORT) { 
      report_realtime_status();
      system_clear_exec_state_flag(EXEC_STATUS_REPORT);
    }
  
    // NOTE: The math involved to calculate the hold should be low enough for most, if not all, 
    // operational scenarios. Once hold is initiated, the system enters a suspend state to block
    // all main program processes until either reset or resumed.
    if (rt_exec & (EXEC_MOTION_CANCEL | EXEC_FEED_HOLD | EXEC_SAFETY_DOOR | EXEC_SLEEP)) {
  
      // TODO: CHECK MODE? How to handle this? Likely nothing, since it only works when IDLE and then resets Grbl.
            
      // State check for allowable states for hold methods.
      if (!(sys.state & (STATE_ALARM | STATE_CHECK_MODE))) {

        // If in CYCLE state, all hold states immediately initiate a motion HOLD.
        if (sys.state == STATE_CYCLE) {
          st_update_plan_block_parameters(); // Notify stepper module to recompute for hold deceleration.
          sys.step_control = STEP_CONTROL_EXECUTE_HOLD; // Initiate suspend state with active flag.
		  if(rt_exec & EXEC_SAFETY_DOOR) {
				set_pwm(&spindle_motor, 0, 0);  // added by Brian R.
		  }
        }
        // If IDLE, Grbl is not in motion. Simply indicate suspend state and hold is complete.
        if (sys.state == STATE_IDLE) { 
          sys.suspend = SUSPEND_HOLD_COMPLETE;
          sys.step_control = STEP_CONTROL_END_MOTION; 
        }
    
        // Execute and flag a motion cancel with deceleration and return to idle. Used primarily by probing cycle
        // to halt and cancel the remainder of the motion.
        if (rt_exec & EXEC_MOTION_CANCEL) {
          // MOTION_CANCEL only occurs during a CYCLE, but a HOLD and SAFETY_DOOR may been initiated beforehand
          // to hold the CYCLE. If so, only flag that motion cancel is complete.
          if (sys.state == STATE_CYCLE) { sys.state = STATE_MOTION_CANCEL; }
          // NOTE: Ensures the motion cancel is handled correctly if it is active during a HOLD or DOOR state.
          sys.suspend |= SUSPEND_MOTION_CANCEL;  // Indicate motion cancel when resuming.
        }

        // Execute a feed hold with deceleration, if required. Then, suspend system.
        if (rt_exec & EXEC_FEED_HOLD) {
          // Block SAFETY_DOOR and SLEEP states from changing to HOLD state.
          if (!(sys.state & (STATE_SAFETY_DOOR|STATE_SLEEP))) { sys.state = STATE_HOLD; }
        }

        // Execute a safety door stop with a feed hold and disable spindle/coolant.
        // NOTE: Safety door differs from feed holds by stopping everything no matter state, disables powered
        // devices (spindle/coolant), and blocks resuming until switch is re-engaged.
        if (rt_exec & EXEC_SAFETY_DOOR) {
          report_feedback_message(MESSAGE_SAFETY_DOOR_AJAR); 
        
          // Check if the safety re-opened during a restore parking motion only. Ignore if 
          // already retracting, parked or in sleep state.
          if (sys.state == STATE_SAFETY_DOOR) {
            if (sys.suspend & SUSPEND_INITIATE_RESTORE) { // Actively restoring
              #ifdef PARKING_ENABLE 
                // Set hold and reset appropriate control flags to restart parking sequence.
                if (sys.step_control & STEP_CONTROL_EXECUTE_PARK) {
                  st_update_plan_block_parameters(); // Notify stepper module to recompute for hold deceleration.
                  sys.step_control = (STEP_CONTROL_EXECUTE_HOLD | STEP_CONTROL_EXECUTE_PARK);
                  sys.suspend &= ~(SUSPEND_HOLD_COMPLETE);
                } // else NO_MOTION is active.
              #endif
              sys.suspend &= ~(SUSPEND_RETRACT_COMPLETE | SUSPEND_INITIATE_RESTORE | SUSPEND_RESTORE_COMPLETE);
              sys.suspend |= SUSPEND_RESTART_RETRACT;
            }
          }
        
          // NOTE: This flag doesn't change when the door closes, unlike sys.state. Ensures any parking motions
          // are executed if the door switch closes and the state returns to HOLD.          
          sys.suspend |= SUSPEND_SAFETY_DOOR_AJAR; 
          if (sys.state != STATE_SLEEP) { sys.state = STATE_SAFETY_DOOR; }
        }
        
        if (rt_exec & EXEC_SLEEP) {
          sys.state = STATE_SLEEP;
        }
     
      }
  
      system_clear_exec_state_flag((EXEC_MOTION_CANCEL | EXEC_FEED_HOLD | EXEC_SAFETY_DOOR | EXEC_SLEEP));      
    }
    
    // Execute a cycle start by starting the stepper interrupt to begin executing the blocks in queue.
    if (rt_exec & EXEC_CYCLE_START) {
      // Block if called at same time as the hold commands: feed hold, motion cancel, and safety door.
      // Ensures auto-cycle-start doesn't resume a hold without an explicit user-input.
      if (!(rt_exec & (EXEC_FEED_HOLD | EXEC_MOTION_CANCEL | EXEC_SAFETY_DOOR))) { 
        // Resume door state when parking motion has retracted and door has been closed.
        if ((sys.state == STATE_SAFETY_DOOR) && !(sys.suspend & SUSPEND_SAFETY_DOOR_AJAR)) {
          if (sys.suspend & SUSPEND_RESTORE_COMPLETE) {
            sys.state = STATE_IDLE; // Set to IDLE to immediately resume the cycle.
          } else if (sys.suspend & SUSPEND_RETRACT_COMPLETE) {
            // Flag to re-energize powered components and restore original position, if disabled by SAFETY_DOOR.
            // NOTE: For a safety door to resume, the switch must be closed, as indicated by HOLD state, and
            // the retraction execution is complete, which implies the initial feed hold is not active. To 
            // restore normal operation, the restore procedures must be initiated by the following flag. Once,
            // they are complete, it will call CYCLE_START automatically to resume and exit the suspend.
            sys.suspend |= SUSPEND_INITIATE_RESTORE;
			
          }
        } 
        // Cycle start only when IDLE or when a hold is complete and ready to resume.
        if ((sys.state == STATE_IDLE) || ((sys.state & (STATE_HOLD | STATE_MOTION_CANCEL)) && (sys.suspend & SUSPEND_HOLD_COMPLETE))) {
          // Start cycle only if queued motions exist in planner buffer and the motion is not canceled.
          sys.step_control = STEP_CONTROL_NORMAL_OP; // Restore step control to normal operation
          if (plan_get_current_block() && bit_isfalse(sys.suspend,SUSPEND_MOTION_CANCEL)) {
            sys.suspend = SUSPEND_DISABLE; // Break suspend state.
            sys.state = STATE_CYCLE;
            st_prep_buffer(); // Initialize step segment buffer before beginning cycle.
            st_wake_up();
          } else { // Otherwise, do nothing. Set and resume IDLE state.
            sys.suspend = SUSPEND_DISABLE; // Break suspend state.
            sys.state = STATE_IDLE;
          }
		  #ifdef CARVIN		    
		    set_pwm(&button_led, BUTTON_LED_LEVEL_ON,3);
		  #endif
        }
      }    
      system_clear_exec_state_flag(EXEC_CYCLE_START);
    }
    
    if (rt_exec & EXEC_CYCLE_STOP) {
      // Reinitializes the cycle plan and stepper system after a feed hold for a resume. Called by 
      // realtime command execution in the main program, ensuring that the planner re-plans safely.
      // NOTE: Bresenham algorithm variables are still maintained through both the planner and stepper
      // cycle reinitializations. The stepper path should continue exactly as if nothing has happened.   
      // NOTE: EXEC_CYCLE_STOP is set by the stepper subsystem when a cycle or feed hold completes.
      if ((sys.state & (STATE_HOLD | STATE_SAFETY_DOOR | STATE_SLEEP)) && !(sys.soft_limit)) {
        // Hold complete. Set to indicate ready to resume.  Remain in HOLD or DOOR states until user
        // has issued a resume command or reset.
        plan_cycle_reinitialize();
        if (sys.step_control & STEP_CONTROL_EXECUTE_HOLD) { sys.suspend |= SUSPEND_HOLD_COMPLETE; } 
        bit_false(sys.step_control,(STEP_CONTROL_EXECUTE_HOLD | STEP_CONTROL_EXECUTE_PARK));
      } else { 
        // Motion is complete. Includes CYCLE, HOMING, and MOTION_CANCEL states. Also if a soft limit
        // error has occurred, force an IDLE state to indicate the motion has completed.        
        sys.suspend = SUSPEND_DISABLE;
        sys.state = STATE_IDLE;
      }  
      system_clear_exec_state_flag(EXEC_CYCLE_STOP);
    }
  }
  
  // Overrides flag byte (sys.override) and execution should be installed here, since they 
  // are realtime and require a direct and controlled interface to the main stepper program.

  // Reload step segment buffer
  if (sys.state & (STATE_CYCLE|STATE_HOLD|STATE_MOTION_CANCEL|STATE_SAFETY_DOOR|STATE_SLEEP|STATE_HOMING)) { 
    st_prep_buffer(); 
  }

}


// Handles Grbl system suspend procedures, such as feed hold, safety door, and parking motion.
// The system will enter this loop, create local variables for suspend tasks, and return to
// whatever function that invoked the suspend, such that Grbl resumes normal operation.
// This function is written in a way to promote custom parking motions. Simply use this as a
// template 
static void protocol_exec_rt_suspend()
{
	
  	
  
		
		//print_uint8_base2(sys.suspend);
		//printPgmString(PSTR("=Suspend\r\n"));
  
	
  #ifdef PARKING_ENABLE
    // Declare parking local variables
    float restore_target[N_AXIS];
    float parking_target[N_AXIS];
    float retract_waypoint = PARKING_PULLOUT_INCREMENT;
  #endif

  while (sys.suspend) {
  
    if (sys.abort) { return; }
    
    // Parking manager. Handles de/re-energizing, switch state checks, and parking motions for 
    // the safety door and sleep states.
    if ((sys.state & (STATE_SAFETY_DOOR | STATE_SLEEP)) && (sys.suspend & SUSPEND_HOLD_COMPLETE)) { 
  
      // Handles retraction motions and de-energizing.
      if (bit_isfalse(sys.suspend,SUSPEND_RETRACT_COMPLETE)) { 

        #ifndef PARKING_ENABLE
                  
          spindle_stop(); // De-energize
          coolant_stop(); // De-energize
          
        #else 
                 
          // Get current position and store restore location and spindle retract waypoint.
          system_convert_array_steps_to_mpos(parking_target,sys.position);
          if (bit_isfalse(sys.suspend,SUSPEND_RESTART_RETRACT)) {
            memcpy(restore_target,parking_target,sizeof(parking_target));
            retract_waypoint += restore_target[PARKING_AXIS];
            retract_waypoint = min(retract_waypoint,PARKING_TARGET);
          }

          // Execute slow pull-out parking retract motion. Parking requires homing enabled and
          // the current location not exceeding the parking target location.
          // NOTE: State is will remain DOOR, until the de-energizing and retract is complete.
          if ((bit_istrue(settings.flags,BITFLAG_HOMING_ENABLE)) &&
                          (parking_target[PARKING_AXIS] < PARKING_TARGET)) {
                
            // Retract spindle by pullout distance. Ensure retraction motion moves away from 
            // the workpiece and waypoint motion doesn't exceed the parking target location.
            if (parking_target[PARKING_AXIS] < retract_waypoint) {   
              parking_target[PARKING_AXIS] = retract_waypoint;
              mc_parking_motion(parking_target, PARKING_PULLOUT_RATE);
            }
           
            spindle_stop(); // De-energize
            coolant_stop(); // De-energize

            // Execute fast parking retract motion to parking target location.
            if (parking_target[PARKING_AXIS] < PARKING_TARGET) {   
              parking_target[PARKING_AXIS] = PARKING_TARGET;      
              mc_parking_motion(parking_target, PARKING_RATE);
            }
            
          } else {
          
            // Parking motion not possible. Just disable the spindle and coolant.
            spindle_stop(); // De-energize
            coolant_stop(); // De-energize
            
          }
          
        #endif

        sys.suspend &= ~(SUSPEND_RESTART_RETRACT);
        sys.suspend |= SUSPEND_RETRACT_COMPLETE;

      } else {    

        // Allows resuming from parking/safety door. Actively checks if safety door is closed and ready to resume.
        // NOTE: Block the sleep state from resuming a park. User must reset to exit sleep.
        if (sys.state == STATE_SAFETY_DOOR) {
          if (!(system_check_safety_door_ajar())) {
			#ifdef CARVIN
			  if (sys.suspend & SUSPEND_SAFETY_DOOR_AJAR) // prevent doing this more than once
				throb_pwm(&button_led, 40,BUTTON_LED_THROB_RATE);
			#endif			
            sys.suspend &= ~(SUSPEND_SAFETY_DOOR_AJAR); // Reset door ajar flag to denote ready to resume.            
		  }
        }
        
        // Handles parking restore and safety door resume.
        if (sys.suspend & SUSPEND_INITIATE_RESTORE) {
   
          #ifdef PARKING_ENABLE
            // Execute fast restore motion to the pull-out position. Parking requires homing enabled.
            // NOTE: State is will remain DOOR, until the de-energizing and retract is complete.
            if (bit_istrue(settings.flags,BITFLAG_HOMING_ENABLE)) {
              // Check to ensure the motion doesn't move below pull-out position.       
              if (parking_target[PARKING_AXIS] <= PARKING_TARGET) {
                parking_target[PARKING_AXIS] = retract_waypoint;     
                mc_parking_motion(parking_target, PARKING_RATE);
              }
            }
          #endif
     
          // Delayed Tasks: Restart spindle and coolant, delay to power-up, then resume cycle.
          if (gc_state.modal.spindle != SPINDLE_DISABLE) {
            // Block if safety door re-opened during prior restore actions.
            if (bit_isfalse(sys.suspend,SUSPEND_RESTART_RETRACT)) {
              spindle_set_state(gc_state.modal.spindle, gc_state.spindle_speed); 
              delay_sec(SAFETY_DOOR_SPINDLE_DELAY, DELAY_MODE_SAFETY_DOOR);
            }
          }
          if (gc_state.modal.coolant != COOLANT_DISABLE) { 
            // Block if safety door re-opened during prior restore actions.
            if (bit_isfalse(sys.suspend,SUSPEND_RESTART_RETRACT)) {
              coolant_set_state(gc_state.modal.coolant); 
              delay_sec(SAFETY_DOOR_COOLANT_DELAY, DELAY_MODE_SAFETY_DOOR);
            }
          }
     
          #ifdef PARKING_ENABLE
            // Execute slow plunge motion from pull-out position to resume position.
            if (bit_istrue(settings.flags,BITFLAG_HOMING_ENABLE)) {
              // Block if safety door re-opened during prior restore actions.
              if (bit_isfalse(sys.suspend,SUSPEND_RESTART_RETRACT)) {
                // Regardless if the retract parking motion was a valid/safe motion or not, the 
                // restore parking motion should logically be valid, either by returning to the 
                // original position through valid machine space or by not moving at all.
                mc_parking_motion(restore_target, PARKING_PULLOUT_RATE);
              }   
            }
          #endif
          
          if (bit_isfalse(sys.suspend,SUSPEND_RESTART_RETRACT)) {
            sys.suspend |= SUSPEND_RESTORE_COMPLETE;
            system_set_exec_state_flag(EXEC_CYCLE_START); // Set to resume program.
          }
        }
   
      }
    }
    
    #ifdef SLEEP_ENABLE
      // Check for sleep conditions and execute auto-park, if timeout duration elapses.
      // Sleep is valid for both hold and door states, if the spindle or coolant are on or
      // set to be re-enabled.
      sleep_check();
	  
	  
	  
    #endif

    protocol_exec_rt_system();
  }
}
