/*
  config.h - compile time configuration
  Part of Grbl

  Copyright (c) 2012-2016 Sungeun K. Jeon
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
  
// This file contains compile-time configurations for Grbl's internal system. For the most part,
// users will not need to directly modify these, but they are here for specific needs, i.e.
// performance tuning or adjusting to non-typical machines.

// IMPORTANT: Any changes here requires a full re-compiling of the source code to propagate them.

#ifndef config_h
#define config_h
#include "grbl.h" // For Arduino IDE compatibility.


// Define CPU pin map and default settings.
// NOTE: OEMs can avoid the need to maintain/update the defaults.h and cpu_map.h files and use only
// one configuration file by placing their specific defaults and pin map at the bottom of this file.
// If doing so, simply comment out these two defines and see instructions below.
//#define DEFAULTS_GENERIC
//#define CPU_MAP_2560_INITIAL

// Serial baud rate
#define BAUD_RATE 115200

// Define realtime command special characters. These characters are 'picked-off' directly from the
// serial read data stream and are not passed to the grbl line execution parser. Select characters
// that do not and must not exist in the streamed g-code program. ASCII control characters may be 
// used, if they are available per user setup. Also, extended ASCII codes (>127), which are never in 
// g-code programs, maybe selected for interface programs.
// NOTE: If changed, manually update help message in report.c.
#define CMD_STATUS_REPORT '?'
#define CMD_FEED_HOLD '!'
#define CMD_CYCLE_START '~'
#define CMD_RESET 0x18 // ctrl-x.
#define CMD_SAFETY_DOOR '@'
#define CMD_RESET_CPU 0x19

// If homing is enabled, homing init lock sets Grbl into an alarm state upon power up. This forces
// the user to perform the homing cycle (or override the locks) before doing anything else. This is
// mainly a safety feature to remind the user to home, since position is unknown to Grbl.
#define HOMING_INIT_LOCK // Comment to disable

// Define the homing cycle patterns with bitmasks. The homing cycle first performs a search mode
// to quickly engage the limit switches, followed by a slower locate mode, and finished by a short
// pull-off motion to disengage the limit switches. The following HOMING_CYCLE_x defines are executed 
// in order starting with suffix 0 and completes the homing routine for the specified-axes only. If
// an axis is omitted from the defines, it will not home, nor will the system update its position.
// Meaning that this allows for users with non-standard cartesian machines, such as a lathe (x then z,
// with no y), to configure the homing cycle behavior to their needs. 
// NOTE: The homing cycle is designed to allow sharing of limit pins, if the axes are not in the same
// cycle, but this requires some pin settings changes in cpu_map.h file. For example, the default homing
// cycle can share the Z limit pin with either X or Y limit pins, since they are on different cycles.
// By sharing a pin, this frees up a precious IO pin for other purposes. In theory, all axes limit pins
// may be reduced to one pin, if all axes are homed with seperate cycles, or vice versa, all three axes
// on separate pin, but homed in one cycle. Also, it should be noted that the function of hard limits 
// will not be affected by pin sharing.
// NOTE: Defaults are set for a traditional 3-axis CNC machine. Z-axis first to clear, followed by X & Y.
#define HOMING_CYCLE_0 (1<<Z_AXIS)                // REQUIRED: First move Z
#define HOMING_CYCLE_1 ((1<<X_AXIS)|(1<<Y_AXIS))
//#define HOMING_CYCLE_1 (1<<X_AXIS) 
//#define HOMING_CYCLE_2 (1<<Y_AXIS) 

// Number of homing cycles performed after when the machine initially jogs to limit switches.
// This help in preventing overshoot and should improve repeatability. This value should be one or 
// greater.
#define N_HOMING_LOCATE_CYCLE 1 // Integer (1-128)

// After homing, Grbl will set by default the entire machine space into negative space, as is typical
// for professional CNC machines, regardless of where the limit switches are located. Uncomment this 
// define to force Grbl to always set the machine origin at the homed location despite switch orientation.
// #define HOMING_FORCE_SET_ORIGIN // Uncomment to enable.

// Number of blocks Grbl executes upon startup. These blocks are stored in EEPROM, where the size
// and addresses are defined in settings.h. With the current settings, up to 2 startup blocks may
// be stored and executed in order. These startup blocks would typically be used to set the g-code
// parser state depending on user preferences.
#define N_STARTUP_LINE 2 // Integer (1-2)

// Number of floating decimal points printed by Grbl for certain value types. These settings are 
// determined by realistic and commonly observed values in CNC machines. For example, position
// values cannot be less than 0.001mm or 0.0001in, because machines can not be physically more
// precise this. So, there is likely no need to change these, but you can if you need to here.
// NOTE: Must be an integer value from 0 to ~4. More than 4 may exhibit round-off errors.
#define N_DECIMAL_COORDVALUE_INCH 4 // Coordinate or position value in inches
#define N_DECIMAL_COORDVALUE_MM   3 // Coordinate or position value in mm
#define N_DECIMAL_RATEVALUE_INCH  1 // Rate or velocity value in in/min
#define N_DECIMAL_RATEVALUE_MM    0 // Rate or velocity value in mm/min
#define N_DECIMAL_SETTINGVALUE    3 // Decimals for floating point setting values
#define N_DECIMAL_RPMVALUE        0 // RPM value in rotations per min.

// If your machine has two limits switches wired in parallel to one axis, you will need to enable
// this feature. Since the two switches are sharing a single pin, there is no way for Grbl to tell
// which one is enabled. This option only effects homing, where if a limit is engaged, Grbl will 
// alarm out and force the user to manually disengage the limit switch. Otherwise, if you have one
// limit switch for each axis, don't enable this option. By keeping it disabled, you can perform a
// homing cycle while on the limit switch and not have to move the machine off of it.
// #define LIMITS_TWO_SWITCHES_ON_AXES

// Allows GRBL to track and report gcode line numbers and real-time feed rate. Both of these may
// be toggled to be visible when their status report mask setting when enabled.
// NOTE: The option to disable these will be removed soon, as these will be added permanently.
//#define REPORT_REALTIME_LINE_NUMBERS // Enabled by default. Comment to disable.
//#define REPORT_REALTIME_RATE // Enabled by default. Comment to disable.

// Upon a successful probe cycle, this option provides immediately feedback of the probe coordinates
// through an automatically generated message. If disabled, users can still access the last probe
// coordinates through Grbl '$#' print parameters.
#define MESSAGE_PROBE_COORDINATES // Enabled by default. Comment to disable.

// After the safety door switch has been toggled and restored, this setting sets the power-up delay
// between restoring the spindle and coolant and resuming the cycle.
#define SAFETY_DOOR_SPINDLE_DELAY 1.0 // Float (seconds)
#define SAFETY_DOOR_COOLANT_DELAY 1.0 // Float (seconds)

// Enable CoreXY kinematics. Use ONLY with CoreXY machines. 
// IMPORTANT: If homing is enabled, you must reconfigure the homing cycle #defines above to 
// #define HOMING_CYCLE_0 (1<<X_AXIS) and #define HOMING_CYCLE_1 (1<<Y_AXIS)
// NOTE: This configuration option alters the motion of the X and Y axes to principle of operation
// defined at (http://corexy.com/theory.html). Motors are assumed to positioned and wired exactly as
// described, if not, motions may move in strange directions. Grbl assumes the CoreXY A and B motors
// have the same steps per mm internally.
// #define COREXY // Default disabled. Uncomment to enable.

// Inverts pin logic of the control command pins based on a mask. This essentially means you can use
// normally-closed switches on the specified pins, rather than the default normally-open switches.
// NOTE: The top option will mask and invert all control pins. The bottom option is an example of
// inverting only two control pins, the safety door and reset. See cpu_map.h for other macro definitions.
// #define INVERT_CONTROL_PIN_MASK CONTROL_MASK // Default disabled. Uncomment to disable.
// #define INVERT_CONTROL_PIN_MASK ((1<<CONTROL_SAFETY_DOOR_BIT)|(CONTROL_RESET_BIT)) // Default disabled.

// Inverts select limit pin states based on the following mask. This effects all limit pin functions, 
// such as hard limits and homing. However, this is different from overall invert limits setting. 
// This build option will invert only the limit pins defined here, and then the invert limits setting
// will be applied to all of them. This is useful when a user has a mixed set of limit pins with both
// normally-open(NO) and normally-closed(NC) switches installed on their machine.
// NOTE: PLEASE DO NOT USE THIS, unless you have a situation that needs it.
// #define INVERT_LIMIT_PIN_MASK ((1<<X_LIMIT_BIT)|(1<<Y_LIMIT_BIT)) // Default disabled.

// Inverts the spindle enable pin from low-disabled/high-enabled to low-enabled/high-disabled. Useful
// for some pre-built electronic boards.
// NOTE: If VARIABLE_SPINDLE is enabled(default), this option has no effect as the PWM output and 
// spindle enable are combined to one pin. If you need both this option and spindle speed PWM, 
// uncomment the config option USE_SPINDLE_DIR_AS_ENABLE_PIN below.
// #define INVERT_SPINDLE_ENABLE_PIN // Default disabled. Uncomment to enable.

// Inverts the selected coolant pin from low-disabled/high-enabled to low-enabled/high-disabled. Useful
// for some pre-built electronic boards.
// #define INVERT_COOLANT_MIST_PIN // Default disabled. Uncomment to enable.
// #define INVERT_COOLANT_FLOOD_PIN // Default disabled. Uncomment to enable.

// Enable all pin states feedback in status reports. Configurable with Grbl settings to print only
// the desired data, which is presented as simple binary reading of each pin as (0 (low) or 1(high)).
// The fields are printed in a particular order and settings groups are separated by '|' characters. 
// NOTE: This option is here for backward compatibility of the old style of pin state reports, i.e. 
// `Lim:000`. This new `Pin:` report will be the standard going forward.
#define REPORT_ALL_PIN_STATES // Default enabled. Comment to disable.

// When Grbl powers-cycles or is hard reset with the Arduino reset button, Grbl boots up with no ALARM
// by default. This is to make it as simple as possible for new users to start using Grbl. When homing
// is enabled and a user has installed limit switches, Grbl will boot up in an ALARM state to indicate 
// Grbl doesn't know its position and to force the user to home before proceeding. This option forces
// Grbl to always initialize into an ALARM state regardless of homing or not. This option is more for
// OEMs and LinuxCNC users that would like this power-cycle behavior.
// #define FORCE_INITIALIZATION_ALARM // Default disabled. Uncomment to enable.

// ---------------------------------------------------------------------------------------
// ADVANCED CONFIGURATION OPTIONS:

// Realtime reports will be altered soon, and the current proposed report may be used by commenting 
// out the following define.
#define USE_CLASSIC_REALTIME_REPORT // Will be disabled in upcoming releases.

// Enables minimal reporting feedback mode for GUIs, where human-readable strings are not as important.
// This saves nearly 2KB of flash space and may allow enough space to install other/future features.
// GUIs will need to install a look-up table for the error-codes that Grbl sends back in their place.
// NOTE: This feature is new and experimental. Make sure the GUI you are using supports this mode.
// #define REPORT_GUI_MODE // Default disabled. Uncomment to enable.

// The temporal resolution of the acceleration management subsystem. A higher number gives smoother
// acceleration, particularly noticeable on machines that run at very high feedrates, but may negatively
// impact performance. The correct value for this parameter is machine dependent, so it's advised to
// set this only as high as needed. Approximate successful values can widely range from 50 to 200 or more.
// NOTE: Changing this value also changes the execution time of a segment in the step segment buffer. 
// When increasing this value, this stores less overall time in the segment buffer and vice versa. Make
// certain the step segment buffer is increased/decreased to account for these changes.
#define ACCELERATION_TICKS_PER_SECOND 100 

// Adaptive Multi-Axis Step Smoothing (AMASS) is an advanced feature that does what its name implies, 
// smoothing the stepping of multi-axis motions. This feature smooths motion particularly at low step
// frequencies below 10kHz, where the aliasing between axes of multi-axis motions can cause audible 
// noise and shake your machine. At even lower step frequencies, AMASS adapts and provides even better
// step smoothing. See stepper.c for more details on the AMASS system works.
#define ADAPTIVE_MULTI_AXIS_STEP_SMOOTHING  // Default enabled. Comment to disable.

// Sets the maximum step rate allowed to be written as a Grbl setting. This option enables an error 
// check in the settings module to prevent settings values that will exceed this limitation. The maximum
// step rate is strictly limited by the CPU speed and will change if something other than an AVR running
// at 16MHz is used.
#define MAX_STEP_RATE_HZ 30000 // Hz

// By default, Grbl sets all input pins to normal-high operation with their internal pull-up resistors
// enabled. This simplifies the wiring for users by requiring only a switch connected to ground, 
// although its recommended that users take the extra step of wiring in low-pass filter to reduce
// electrical noise detected by the pin. If the user inverts the pin in Grbl settings, this just flips
// which high or low reading indicates an active signal. In normal operation, this means the user 
// needs to connect a normal-open switch, but if inverted, this means the user should connect a 
// normal-closed switch. 
// The following options disable the internal pull-up resistors, sets the pins to a normal-low 
// operation, and switches must be now connect to Vcc instead of ground. This also flips the meaning 
// of the invert pin Grbl setting, where an inverted setting now means the user should connect a 
// normal-open switch and vice versa.
// NOTE: All pins associated with the feature are disabled, i.e. XYZ limit pins, not individual axes.
// WARNING: When the pull-ups are disabled, this requires additional wiring with pull-down resistors!
//#define DISABLE_LIMIT_PIN_PULL_UP
//#define DISABLE_PROBE_PIN_PULL_UP
//#define DISABLE_CONTROL_PIN_PULL_UP

// Sets which axis the tool length offset is applied. Assumes the spindle is always parallel with 
// the selected axis with the tool oriented toward the negative direction. In other words, a positive
// tool length offset value is subtracted from the current location.
#define TOOL_LENGTH_OFFSET_AXIS Z_AXIS // Default z-axis. Valid values are X_AXIS, Y_AXIS, or Z_AXIS.

// Used by variable spindle output only. This forces the PWM output to a minimum duty cycle when enabled.
// The PWM pin will still read 0V when the spindle is disabled. Most users will not need this option, but
// it may be useful in certain scenarios. This minimum PWM settings coincides with the spindle rpm minimum
// setting, like rpm max to max PWM. So the variable spindle pin will not output the voltage range between 
// 0V for disabled and the voltage set by the minimum PWM for minimum rpm.
// NOTE: Compute duty cycle at the minimum PWM by this equation: (% duty cycle)=(SPINDLE_MINIMUM_PWM/256)*100
// #define SPINDLE_MINIMUM_PWM 5 // Default disabled. Uncomment to enable. Integer (0-255)

// With this enabled, Grbl sends back an echo of the line it has received, which has been pre-parsed (spaces
// removed, capitalized letters, no comments) and is to be immediately executed by Grbl. Echoes will not be 
// sent upon a line buffer overflow, but should for all normal lines sent to Grbl. For example, if a user 
// sendss the line 'g1 x1.032 y2.45 (test comment)', Grbl will echo back in the form '[echo: G1X1.032Y2.45]'.
// NOTE: Only use this for debugging purposes!! When echoing, this takes up valuable resources and can effect
// performance. If absolutely needed for normal operation, the serial write buffer should be greatly increased
// to help minimize transmission waiting within the serial write protocol.
// #define REPORT_ECHO_LINE_RECEIVED // Default disabled. Uncomment to enable.

// Minimum planner junction speed. Sets the default minimum junction speed the planner plans to at
// every buffer block junction, except for starting from rest and end of the buffer, which are always
// zero. This value controls how fast the machine moves through junctions with no regard for acceleration
// limits or angle between neighboring block line move directions. This is useful for machines that can't
// tolerate the tool dwelling for a split second, i.e. 3d printers or laser cutters. If used, this value
// should not be much greater than zero or to the minimum value necessary for the machine to work.
#define MINIMUM_JUNCTION_SPEED 0.0 // (mm/min)

// Sets the minimum feed rate the planner will allow. Any value below it will be set to this minimum
// value. This also ensures that a planned motion always completes and accounts for any floating-point
// round-off errors. Although not recommended, a lower value than 1.0 mm/min will likely work in smaller
// machines, perhaps to 0.1mm/min, but your success may vary based on multiple factors.
#define MINIMUM_FEED_RATE 1.0 // (mm/min)

// Number of arc generation iterations by small angle approximation before exact arc trajectory 
// correction with expensive sin() and cos() calcualtions. This parameter maybe decreased if there 
// are issues with the accuracy of the arc generations, or increased if arc execution is getting
// bogged down by too many trig calculations. 
#define N_ARC_CORRECTION 12 // Integer (1-255)

// The arc G2/3 g-code standard is problematic by definition. Radius-based arcs have horrible numerical 
// errors when arc at semi-circles(pi) or full-circles(2*pi). Offset-based arcs are much more accurate 
// but still have a problem when arcs are full-circles (2*pi). This define accounts for the floating 
// point issues when offset-based arcs are commanded as full circles, but get interpreted as extremely
// small arcs with around machine epsilon (1.2e-7rad) due to numerical round-off and precision issues.
// This define value sets the machine epsilon cutoff to determine if the arc is a full-circle or not.
// NOTE: Be very careful when adjusting this value. It should always be greater than 1.2e-7 but not too
// much greater than this. The default setting should capture most, if not all, full arc error situations.
#define ARC_ANGULAR_TRAVEL_EPSILON 5E-7 // Float (radians)

// Time delay increments performed during a dwell. The default value is set at 50ms, which provides
// a maximum time delay of roughly 55 minutes, more than enough for most any application. Increasing
// this delay will increase the maximum dwell time linearly, but also reduces the responsiveness of 
// run-time command executions, like status reports, since these are performed between each dwell 
// time step. Also, keep in mind that the Arduino delay timer is not very accurate for long delays.
#define DWELL_TIME_STEP 50 // Integer (1-255) (milliseconds)

// Creates a delay between the direction pin setting and corresponding step pulse by creating
// another interrupt (Timer2 compare) to manage it. The main Grbl interrupt (Timer1 compare) 
// sets the direction pins, and does not immediately set the stepper pins, as it would in 
// normal operation. The Timer2 compare fires next to set the stepper pins after the step 
// pulse delay time, and Timer2 overflow will complete the step pulse, except now delayed 
// by the step pulse time plus the step pulse delay. (Thanks langwadt for the idea!)
// NOTE: Uncomment to enable. The recommended delay must be > 3us, and, when added with the
// user-supplied step pulse time, the total time must not exceed 127us. Reported successful
// values for certain setups have ranged from 5 to 20us.
// #define STEP_PULSE_DELAY 10 // Step pulse delay in microseconds. Default disabled.

// The number of linear motions in the planner buffer to be planned at any give time. The vast
// majority of RAM that Grbl uses is based on this buffer size. Only increase if there is extra 
// available RAM, like when re-compiling for a Mega or Sanguino. Or decrease if the Arduino
// begins to crash due to the lack of available RAM or if the CPU is having trouble keeping
// up with planning new incoming motions as they are executed. 
// #define BLOCK_BUFFER_SIZE 36  // Uncomment to override default in planner.h.

// Governs the size of the intermediary step segment buffer between the step execution algorithm
// and the planner blocks. Each segment is set of steps executed at a constant velocity over a
// fixed time defined by ACCELERATION_TICKS_PER_SECOND. They are computed such that the planner
// block velocity profile is traced exactly. The size of this buffer governs how much step 
// execution lead time there is for other Grbl processes have to compute and do their thing 
// before having to come back and refill this buffer, currently at ~50msec of step moves.
// #define SEGMENT_BUFFER_SIZE 10 // Uncomment to override default in stepper.h.

// Line buffer size from the serial input stream to be executed. Also, governs the size of 
// each of the startup blocks, as they are each stored as a string of this size. Make sure
// to account for the available EEPROM at the defined memory address in settings.h and for
// the number of desired startup blocks.
// NOTE: 80 characters is not a problem except for extreme cases, but the line buffer size 
// can be too small and g-code blocks can get truncated. Officially, the g-code standards 
// support up to 256 characters. In future versions, this default will be increased, when 
// we know how much extra memory space we can re-invest into this.
// #define LINE_BUFFER_SIZE 256  // Uncomment to override default in protocol.h
  
// Serial send and receive buffer size. The receive buffer is often used as another streaming
// buffer to store incoming blocks to be processed by Grbl when its ready. Most streaming
// interfaces will character count and track each block send to each block response. So, 
// increase the receive buffer if a deeper receive buffer is needed for streaming and avaiable
// memory allows. The send buffer primarily handles messages in Grbl. Only increase if large
// messages are sent and Grbl begins to stall, waiting to send the rest of the message.
// NOTE: Buffer size values must be greater than zero and less than 256.
// #define RX_BUFFER_SIZE 256 // Uncomment to override defaults in serial.h
// #define TX_BUFFER_SIZE 128

// The maximum line length of a data string stored in EEPROM. Used by startup lines and build
// info. This size differs from the LINE_BUFFER_SIZE as the EEPROM is usually limited in size.
// NOTE: Be very careful when changing this value. Check EEPROM address locations to make sure
// these string storage locations won't corrupt one another.
// #define EEPROM_LINE_SIZE 80 // Uncomment to override defaults in settings.h
  
// Toggles XON/XOFF software flow control for serial communications. Not officially supported
// due to problems involving the Atmega8U2 USB-to-serial chips on current Arduinos. The firmware
// on these chips do not support XON/XOFF flow control characters and the intermediate buffer 
// in the chips cause latency and overflow problems with standard terminal programs. However, 
// using specifically-programmed UI's to manage this latency problem has been confirmed to work.
// As well as, older FTDI FT232RL-based Arduinos(Duemilanove) are known to work with standard
// terminal programs since their firmware correctly manage these XON/XOFF characters. In any
// case, please report any successes to grbl administrators!
// #define ENABLE_XONXOFF // Default disabled. Uncomment to enable.

// A simple software debouncing feature for hard limit switches. When enabled, the interrupt 
// monitoring the hard limit switch pins will enable the Arduino's watchdog timer to re-check 
// the limit pin state after a delay of about 32msec. This can help with CNC machines with 
// problematic false triggering of their hard limit switches, but it WILL NOT fix issues with 
// electrical interference on the signal cables from external sources. It's recommended to first
// use shielded signal cables with their shielding connected to ground (old USB/computer cables 
// work well and are cheap to find) and wire in a low-pass circuit into each limit pin.
// #define ENABLE_SOFTWARE_DEBOUNCE // Default disabled. Uncomment to enable.

// Force Grbl to check the state of the hard limit switches when the processor detects a pin
// change inside the hard limit ISR routine. By default, Grbl will trigger the hard limits
// alarm upon any pin change, since bouncing switches can cause a state check like this to 
// misread the pin. When hard limits are triggered, they should be 100% reliable, which is the
// reason that this option is disabled by default. Only if your system/electronics can guarantee
// that the switches don't bounce, we recommend enabling this option. This will help prevent
// triggering a hard limit when the machine disengages from the switch.
// NOTE: This option has no effect if SOFTWARE_DEBOUNCE is enabled.
// #define HARD_LIMIT_FORCE_STATE_CHECK // Default disabled. Uncomment to enable.

// Adjusts homing cycle search and locate scalars. These are the multipliers used by Grbl's
// homing cycle to ensure the limit switches are engaged and cleared through each phase of 
// the cycle. The search phase uses the axes max-travel setting times the SEARCH_SCALAR to
// determine distance to look for the limit switch. Once found, the locate phase begins and
// uses the homing pull-off distance setting times the LOCATE_SCALAR to pull-off and re-engage
// the limit switch.
// NOTE: Both of these values must be greater than 1.0 to ensure proper function.
#define HOMING_AXIS_SEARCH_SCALAR  1.1 // Uncomment to override defaults in limits.c.
#define HOMING_AXIS_LOCATE_SCALAR  10.0 // Uncomment to override defaults in limits.c.

// Enables and configures parking motion methods upon a safety door state. Primarily for OEMs
// that desire this feature for their integrated machines. At the moment, Grbl assumes that 
// the parking motion only involves one axis, although the parking implementation was written
// to be easily refactored for any number of motions on different axes by altering the parking 
// source code. At this time, Grbl only supports parking one axis (typically the Z-axis) that 
// moves in the positive direction upon retracting and negative direction upon restoring position.
// The motion executes with a slow pull-out retraction motion, power-down, and a fast park. 
// Restoring to the resume position follows these set motions in reverse: fast restore to 
// pull-out position, power-up with a time-out, and plunge back to the original position at the
// slower pull-out rate.
// NOTE: Still a work-in-progress. Machine coordinates must be in all negative space and 
// does not work with HOMING_FORCE_SET_ORIGIN enabled. Parking motion also moves only in 
// positive direction.
#define PARKING_ENABLE  // Default disabled. Uncomment to enable

// Configure options for the parking motion, if enabled.
#define PARKING_AXIS Z_AXIS // Define which axis that performs the parking motion
#define PARKING_TARGET -5.0 // Parking axis target. In mm, as machine coordinate [-max_travel,0].
#define PARKING_RATE -1.0 // Parking fast rate after pull-out. In mm/min or (-1.0) for seek rate.
#define PARKING_PULLOUT_RATE 500.0 // Pull-out/plunge slow feed rate in mm/min.
#define PARKING_PULLOUT_INCREMENT 5.0 // Spindle pull-out and plunge distance in mm. Incremental distance.
                                      // Must be positive value or equal to zero.

// Enables and configures Grbl's sleep mode feature. If the spindle or coolant are powered and Grbl 
// is not actively moving or receiving any commands, a sleep timer will start. If any data or commands
// are received, the sleep timer will reset and restart until the above condition are not satisfied.
// If the sleep timer elaspes, Grbl will immediately execute the sleep mode by shutting down the spindle
// and coolant and entering a safe sleep state. If parking is enabled, Grbl will park the machine as
// well. While in sleep mode, only a hard/soft reset will exit it and the job will be unrecoverable.
// NOTE: Sleep mode is a safety feature, primarily to address communication disconnect problems. To 
// keep Grbl from sleeping, employ a stream of '?' status report commands as a connection "heartbeat".
#define SLEEP_ENABLE  // Default disabled. Uncomment to enable.
#define SLEEP_DURATION 2.0 // Float (0.25 - 61.0) seconds before sleep mode is executed.


// ---------------------------------------------------------------------------------------
// COMPILE-TIME ERROR CHECKING OF DEFINE VALUES:

#ifndef HOMING_CYCLE_0
  #error "Required HOMING_CYCLE_0 not defined."
#endif

#if defined(PARKING_ENABLE)
  #if defined(HOMING_FORCE_SET_ORIGIN)
    #error "HOMING_FORCE_SET_ORIGIN is not supported with PARKING_ENABLE at this time."
  #endif
#endif

/* ---------------------------------------------------------------------------------------
   OEM Single File Configuration Option
   
   Instructions: Paste the cpu_map and default setting definitions below without an enclosing
   #ifdef. Comment out the CPU_MAP_xxx and DEFAULT_xxx defines at the top of this file, and
   the compiler will ignore the contents of defaults.h and cpu_map.h and use the definitions
   below.
*/

// Paste CPU_MAP definitions here.
#define CARVIN           // Setting this allows other modules to conditionally compile for CARVIN
//#define CARVIN_DEBUG     // can be used to include debugging print statements
  
  
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
#define STEPPERS_DISABLE_DDR   DDRH
#define STEPPERS_DISABLE_PORT  PORTH
#define STEPPERS_DISABLE_BIT   7 
#define STEPPERS_DISABLE_MASK (1<<STEPPERS_DISABLE_BIT)


//  ========================  Start of PWM Ports ======================= 
#define BUTTON_LED_DDR      DDRH  // H3 is Timer 4
#define BUTTON_LED_PORT     PORTH
#define BUTTON_LED_BIT      3  
#define BUTTON_LED_OCR 	  OCR4A

// DOOR LED ============
// door LED is defined on deferent ports on the 2 versions of hardware  

#define DOOR_LED_DDR     DDRH  // name the direction register
#define DOOR_LED_PORT    PORTH // name the port register
#define DOOR_LED_BIT     4   // what bit on the port is it?
#define DOOR_LED_OCR     OCR4B // the value of the duty cycle
#define DOOR_LED_MAX     1023

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
#define COOLANT_MIST_DDR    DDRJ
#define COOLANT_MIST_PORT   PORTJ
#define COOLANT_MIST_BIT    1 // MEGA2560 Digital Pin 9

// control i/o
#define CONTROL_DDR       DDRK
#define CONTROL_PIN       PINK
#define CONTROL_PORT      PORTK
#define CONTROL_RESET_BIT       0  // Not Used
#define CONTROL_FEED_HOLD_BIT   5  // Not Used
#define CONTROL_CYCLE_START_BIT 2 // front button
#define CONTROL_SAFETY_DOOR_BIT 1 // door
#define CLAMP_CHECK_BIT   4   // clamp check not used
#define CONTROL_INT       PCIE2  // Pin change interrupt enable pin
#define CONTROL_INT_vect  PCINT2_vect
#define CONTROL_PCMSK     PCMSK2 // Pin change interrupt register


// all these switches use pull ups the "normal" is high or inverted    
#define CONTROL_MASK      	((1<<CONTROL_CYCLE_START_BIT)|(1<<CONTROL_SAFETY_DOOR_BIT))  // the mask of all switches
#define INVERT_CONTROL_PIN_MASK 	((1<<CONTROL_CYCLE_START_BIT)) // the mask of ones that are inverted.

//#define CONTROL_INVERT_MASK   CONTROL_MASK // May be re-defined to only invert certain control pins.


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

#define SPINDLE_PWM_MAX_VALUE 255
//#define PWM_MAX_VALUE 255    // timer2 on a mega2650 is an 8 bit timer
#define SPINDLE_PWM_DDR		DDRH
#define SPINDLE_PWM_PORT    PORTH
#define SPINDLE_PWM_BIT		6

// spindle need a soft start to prevent drawing too much current
#define SPINDLE_SPINUP_RATE 2  // seconds
#define SPINDLE_SPINDOWN_RATE 3  // seconds
#define EXTEND_SPINDLE_LED_FADE 2 // seconds - it looks a little better if the cross fade associated with the spindle lasts a little longer

#define DEFAULT_G28_X  -282.5
#define DEFAULT_G28_Y  -199.5
#define DEFAULT_G28_Z  -1.0
#define DEFAULT_G30_X  -150.0
#define DEFAULT_G30_Y  -10.0
#define DEFAULT_G30_Z  -1.0

// Paste default settings definitions here.

// Carvey Default Settings
#define DEFAULT_X_STEPS_PER_MM 88.889
#define DEFAULT_Y_STEPS_PER_MM 88.889
#define DEFAULT_Z_STEPS_PER_MM 377.893  // or 5120 (for older m8 rods)
#define DEFAULT_X_MAX_RATE 5000.0 // mm/min
#define DEFAULT_Y_MAX_RATE 5000.0 // mm/min
#define DEFAULT_Z_MAX_RATE 800.0 // mm/min
#define DEFAULT_X_ACCELERATION (500.0*60*60) // 10*60*60 mm/min^2 = 10 mm/sec^2
#define DEFAULT_Y_ACCELERATION (500.0*60*60) // 10*60*60 mm/min^2 = 10 mm/sec^2
#define DEFAULT_Z_ACCELERATION (60.0*60*60) // 10*60*60 mm/min^2 = 10 mm/sec^2
#define DEFAULT_X_MAX_TRAVEL 300.0 // mm
#define DEFAULT_Y_MAX_TRAVEL 203.2 // mm
#define DEFAULT_Z_MAX_TRAVEL 80.0 // mm
#define DEFAULT_SPINDLE_RPM_MAX 12000.0 // rpm
#define DEFAULT_SPINDLE_RPM_MIN 0.0 // rpm
#define DEFAULT_STEP_PULSE_MICROSECONDS 10
#define DEFAULT_STEPPING_INVERT_MASK 0
#define DEFAULT_DIRECTION_INVERT_MASK 0 // was ((1<<X_AXIS) | (1<<Y_AXIS) | (1<<Z_AXIS)) on GEN1
#define DEFAULT_STEPPER_IDLE_LOCK_TIME 255 // msec (0-254, 255 keeps steppers enabled)
#define DEFAULT_STATUS_REPORT_MASK ((BITFLAG_RT_STATUS_MACHINE_POSITION)|(BITFLAG_RT_STATUS_WORK_POSITION))
#define DEFAULT_JUNCTION_DEVIATION 0.02 // mm
#define DEFAULT_ARC_TOLERANCE 0.002 // mm
#define DEFAULT_REPORT_INCHES 0 // false
#define DEFAULT_AUTO_START 1 // true
#define DEFAULT_INVERT_ST_ENABLE 0 // false
#define DEFAULT_INVERT_LIMIT_PINS 0 // false
#define DEFAULT_SOFT_LIMIT_ENABLE 0 // false
#define DEFAULT_HARD_LIMIT_ENABLE 0  // false
#define DEFAULT_HOMING_ENABLE 1  // false
#define DEFAULT_HOMING_DIR_MASK ((1<<X_AXIS)|(1<<Y_AXIS))
#define DEFAULT_HOMING_FEED_RATE 75.0 // mm/min
#define DEFAULT_HOMING_SEEK_RATE 1500.0 // mm/min
#define DEFAULT_HOMING_DEBOUNCE_DELAY 250 // msec (0-65k)
#define DEFAULT_HOMING_PULLOFF 4.0 // mm

#endif
