/*
  cpu_map.h - CPU and pin mapping configuration file
  Part of Grbl

  Copyright (c) 2012-2015 Sungeun K. Jeon

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

/* The cpu_map.h file serves as a central pin mapping settings file for different processor
   types, i.e. AVR 328p or AVR Mega 2560. Grbl officially supports the Arduino Uno, but the 
   other supplied pin mappings are supplied by users, so your results may vary. */

// NOTE: This is still a work in progress. We are still centralizing the configurations to
// this file, so your success may vary for other CPUs.

#ifndef cpu_map_h
#define cpu_map_h


//----------------------------------------------------------------------------------------

#ifdef CPU_MAP_CARVIN // The Inventables Carvin Controller for Carvey

  #define CARVIN           // Allows other modules to conditionally compile for CARVIN
	//#define USE_BUTTON_FOR_ON  // can be used for test purposes
  // Increase Buffers to make use of extra SRAM
//#define RX_BUFFER_SIZE		  256
//#define TX_BUFFER_SIZE		  128
//#define BLOCK_BUFFER_SIZE	  36
//#define LINE_BUFFER_SIZE	  100
//#define SEGMENT_BUFFER_SIZE 10

  // Serial port pins
  #define SERIAL_RX USART0_RX_vect
  #define SERIAL_UDRE USART0_UDRE_vect


  // Define step pulse output pins. NOTE: All step bit pins must be on the same port.
  #define STEP_DDR      DDRA
  #define STEP_PORT     PORTA
  #define STEP_PIN      PINA
  #define X_STEP_BIT    5
  #define Y_STEP_BIT    6
  #define Z_STEP_BIT    7
  #define STEP_MASK ((1<<X_STEP_BIT)|(1<<Y_STEP_BIT)|(1<<Z_STEP_BIT)) // All step bits

  // Define step direction output pins. NOTE: All direction pins must be on the same port.
  #define DIRECTION_DDR      DDRA
  #define DIRECTION_PORT     PORTA
  #define DIRECTION_PIN      PINA
  #define X_DIRECTION_BIT   2
  #define Y_DIRECTION_BIT   3
  #define Z_DIRECTION_BIT   4
  #define DIRECTION_MASK ((1<<X_DIRECTION_BIT)|(1<<Y_DIRECTION_BIT)|(1<<Z_DIRECTION_BIT)) // All direction bits

  // Define stepper driver enable/disable output pin.
  #define STEPPERS_DISABLE_DDR   DDRB
  #define STEPPERS_DISABLE_PORT  PORTB
  #define STEPPERS_DISABLE_BIT   7 
  #define STEPPERS_DISABLE_MASK (1<<STEPPERS_DISABLE_BIT)
  
  
  //  ========================  Start of PWM Ports ======================= 
  #define BUTTON_LED_DDR      DDRH  // H3 is Timer 4
  #define BUTTON_LED_PORT     PORTH
  #define BUTTON_LED_BIT      3  
  #define BUTTON_LED_OCR 	  OCR4A
	/*
	// Schematic has an error that port it on port E see below 
	
  #define DOOR_LED_DDR     	  DDRH
  #define DOOR_LED_PORT    	  PORTH
  #define DOOR_LED_BIT     	  4
	
	// more human readable names for the output compare registers (PWM duty)

//#define DOOR_LED_OCR OCR4B  //error on schematic see next line

 
  
	*/
	
	#define DOOR_LED_DDR     DDRE  // name the direction register
    #define DOOR_LED_PORT    PORTE // name the port register
    #define DOOR_LED_BIT     4   // what bit on the port is it?   E4 is Timer 3
	#define DOOR_LED_OCR     OCR3B  // temp fix
	
	#define STEPPER_VREF_DDR     DDRE
    #define STEPPER_VREF_PORT    PORTE
    #define STEPPER_VREF_BIT     3   // E3 is Timer 3
	#define STEPPER_VREF_OCR     OCR3A
	#define I_SENSE_RESISTOR     0.27
	#define STEPPER_RUN_CURRENT  2.2
	#define STEPPER_HOMING_CURRENT  0.75
	
    #define SPINDLE_LED_DDR     DDRH
    #define SPINDLE_LED_PORT    PORTH
    #define SPINDLE_LED_BIT     5     // H5 is Timer 4
	#define SPINDLE_LED_OCR     OCR4C
	
	//  ========================  End of PWM Ports ======================= 
	

	
  // NOTE: All limit bit pins must be on the same port
  #define LIMIT_DDR       DDRB
  #define LIMIT_PORT      PORTB
  #define LIMIT_PIN       PINB
  #define X_LIMIT_BIT     4
  #define Y_LIMIT_BIT     5
  #define Z_LIMIT_BIT     6
  #define LIMIT_INT       PCIE0  // Pin change interrupt enable pin
  #define LIMIT_INT_vect  PCINT0_vect 
  #define LIMIT_PCMSK     PCMSK0 // Pin change interrupt register
  #define LIMIT_MASK ((1<<X_LIMIT_BIT)|(1<<Y_LIMIT_BIT)|(1<<Z_LIMIT_BIT)) // All limit bits

  // Define spindle enable and spindle direction output pins.
  #define SPINDLE_ENABLE_DDR   DDRH
  #define SPINDLE_ENABLE_PORT  PORTH
  #define SPINDLE_ENABLE_BIT   6 
  #define SPINDLE_DIRECTION_DDR   DDRE
  #define SPINDLE_DIRECTION_PORT  PORTE
  #define SPINDLE_DIRECTION_BIT   3
  
  
   
  // These are not actually used
  #define COOLANT_FLOOD_DDR   DDRG
  #define COOLANT_FLOOD_PORT  PORTG
  #define COOLANT_FLOOD_BIT   5
  
	// control i/o
  #define CONTROL_DDR       DDRK
  #define CONTROL_PIN       PINK
  #define CONTROL_PORT      PORTK
  #define RESET_BIT         0  // Not Used
  #define FEED_HOLD_BIT     1  // Not Used
  #define CYCLE_START_BIT   2  // Front Button
  #define SAFETY_DOOR_BIT   1  // Cover Door   Inverted
  #define CLAMP_CHECK_BIT   4   // clamp check
  #define CONTROL_INT       PCIE2  // Pin change interrupt enable pin
  #define CONTROL_INT_vect  PCINT2_vect
  #define CONTROL_PCMSK     PCMSK2 // Pin change interrupt register
  
  
  // all these switches use pull ups the "normal" is high or inverted    
  #define CONTROL_MASK      	((1<<CYCLE_START_BIT)|(1<<SAFETY_DOOR_BIT))  // the mask of all switches
  #define INVERT_CONTROL_MASK 	(1<<CYCLE_START_BIT)  // the mask of ones that are inverted.
  
	
  // Define probe switch input pin.  (Probe is smart clamp in gCarvin)
  #define PROBE_DDR       DDRK
  #define PROBE_PIN       PINK
  #define PROBE_PORT      PORTK
  #define PROBE_BIT       3    // smart clamp
  #define PROBE_MASK      (1<<PROBE_BIT)

    
	// this is spindle control stuff
	#define TCCRA_REGISTER		TCCR2A
	#define TCCRB_REGISTER		TCCR2B
	#define SPINDLE_MOTOR_OCR	OCR2B

	#define COMB_BIT			COM2B1
	#define WAVE0_REGISTER		WGM20
	#define WAVE1_REGISTER		WGM21
	#define WAVE2_REGISTER		WGM22
	#define WAVE3_REGISTER		WGM23

	#define PWM_MAX_VALUE 255    // timer2 on a mega2650 is an 8 bit timer
	#define SPINDLE_PWM_DDR		DDRH
	#define SPINDLE_PWM_PORT    PORTH
	#define SPINDLE_PWM_BIT		6
	
	// spindle need a soft start to prevent drawing too much current
	#define SPINDLE_SPINUP_RATE 2  // seconds
	#define SPINDLE_SPINDOWN_RATE 3  // seconds
    #define EXTEND_SPINDLE_LED_FADE 2 // seconds - it looks a little better if the cross fade associated with the spindle lasts a little longer

#endif



#endif
