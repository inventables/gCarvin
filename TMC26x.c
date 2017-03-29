#include <avr/io.h>
#include "spi.h"
#include "grbl.h"

uint32_t setTMC26xCHOPCONF(uint8_t blankingTime, uint8_t chopModeStd, uint8_t randomTimeOff);
uint32_t setTMC26xSMARTEN(char minCoolCurrent);
uint32_t setTMC26xSGSCONF(uint8_t filterEnable, int8_t stallGuardThreshold, uint8_t currentScale);
uint32_t setTMC26xDRVCONF(uint8_t readItem);
uint32_t setTMC26xDRVCTRL(uint8_t interpol, uint8_t doubleEdge, uint16_t microstepMode);

#define DRIVER_CONTROL_REGISTER 0x0ul
#define STEP_INTERPOLATION 0x200ul
#define DOUBLE_EDGE_STEP 0x100ul
#define STEP_ITERPOL_DISABLE 0
#define STEP_ITERPOL_ENABLE 1
#define DOUBLE_EDGE_DISABLE 0
#define DOUBLE_EDGE_ENAABLE 1
  
#define CHOPPER_CONFIG_REGISTER 0x80000ul
#define CHOPPER_MODE_STANDARD 1
#define CHOPPER_MODE_T_OFF_FAST_DECAY 0x4000ul
   
#define T_OFF_PATTERN 0xful
#define RANDOM_TOFF_TIME 0x2000ul
#define YES_RANDOM_TIME_OFF 1
#define NO_RANDOM_TIME_OFF 0

#define BLANK_TIMING_PATTERN 0x18000ul
#define BLANK_TIMING_SHIFT 15

#define HYSTERESIS_DECREMENT_PATTERN 0x1800ul
#define HYSTERESIS_DECREMENT_SHIFT 11
#define HYSTERESIS_LOW_VALUE_PATTERN 0x780ul
#define HYSTERESIS_LOW_SHIFT 7
#define HYSTERESIS_START_VALUE_PATTERN 0x78ul
#define HYSTERESIS_START_VALUE_SHIFT 4
#define T_OFF_TIMING_PATERN 0xFul

#define COOL_STEP_REGISTER  0xA0000ul
#define MIN_COOL_CURRENT_HALF 0
#define MIN_COOL_CURRENT_QUARTER 1
#define MIN_COOL_CURRENT_SHIFT 15
#define CURRENT_DECREMENT_SPEED_SHIFT 12
#define COOLSTEP_UPPER_THRESHOLD_SHIFT 8 
#define CURRENT_INC_SHIFT 4
#define COOLSTEP_LOWER_THRESHOLD_SHIFT 8

#define STALL_GUARD2_LOAD_MEASURE_REGISTER 0xC0000ul

#define DRIVER_CONFIG_REGISTER 0xE0000ul
#define STEP_DIR_ENABLE 0x0ul // 0 = STEP/DIR is on
#define STEP_DIR_DISABLE 0x0ul // 0 = STEP/DIR is on
#define STEP_DIR_SHIFT 7  // bit shift ammount
#define VSENSE_165MV 0x1ul // 0 = 305mV, 1 = 165mV
#define VSENSE_305MV 0x0ul // 0 = 305mV, 1 = 165mV
#define VSENSE_SHIFT 6  // bit shift ammount
#define READOUT_VALUE_MS_POS 0 // 0 = microstep position, 1 = Stallguard2, 2 = SG2 + Coolstep, 3 = don't use 
#define READOUT_VALUE_SG2 1 
#define READOUT_VALUE_SG2_COOLSTEP 2
#define READOUT_VALUE_SHIFT 4 // bit shift ammount
#define SG2_FILTER_DISABLE 0
#define SG2_FILTER_ENABLE 1


// Chip Select pins for the 3 stepper drivers
#define CS_DDR        DDRC
#define CS_PORT       PORTC
#define CS_PIN        PINC
#define CS_X_BIT      0
#define CS_Y_BIT      1
#define CS_Z_BIT      2
#define CS_MASK       ((1<<CS_X_BIT) | (1<<CS_Y_BIT) | (1<<CS_Z_BIT))

// for the SPI currents in motors (1 to 32 = 0 to full scale)  check temps after changing!!!
#define X_RUN_CURRENT 22
#define Y_RUN_CURRENT 20
#define Z_RUN_CURRENT 20

#define X_IDLE_CURRENT 20
#define Y_IDLE_CURRENT 20
#define Z_IDLE_CURRENT 20

#define X_MICROSTEPS 16
#define Y_MICROSTEPS 16
#define Z_MICROSTEPS 2

#define CARVEY_CHOPPER_BLANKING_TIME 16 // use 16,24,36 0r 54 only ... low is quiet high is accurate
#define CARVEY_RANDOM_TIME_OFF NO_RANDOM_TIME_OFF // is the random chopper off timing used.
#define CARVEY_STALL_GAURD_THRESHOLD 10 


void tmc26x_init()
{
  uint8_t csBit;
  
  CS_DDR |= CS_MASK;  // set them as outputs
  CS_PORT |= (1<<CS_MASK); // make them high (chips not selected)
  
  
  spi_init();  // initialize the SPI port
  
 
  
  // setup x axis driver
  csBit = CS_X_BIT;
  delay_ms(20);  // give a little time before SPI starts
  spi_send20bit(setTMC26xCHOPCONF(CARVEY_CHOPPER_BLANKING_TIME, CHOPPER_MODE_STANDARD, CARVEY_RANDOM_TIME_OFF), &CS_PORT, csBit);
  spi_send20bit(setTMC26xSMARTEN(MIN_COOL_CURRENT_HALF), &CS_PORT, csBit);
  spi_send20bit(setTMC26xSGSCONF(SG2_FILTER_ENABLE, CARVEY_STALL_GAURD_THRESHOLD, X_RUN_CURRENT), &CS_PORT, csBit);
  spi_send20bit( setTMC26xDRVCONF(READOUT_VALUE_SG2), &CS_PORT, csBit);
  spi_send20bit( setTMC26xDRVCTRL(STEP_ITERPOL_DISABLE,DOUBLE_EDGE_DISABLE,X_MICROSTEPS), &CS_PORT, csBit);
  
  // setup Y axis driver
  csBit = CS_Y_BIT;
  delay_ms(20); // give a little time before SPI starts
  spi_send20bit(setTMC26xCHOPCONF(CARVEY_CHOPPER_BLANKING_TIME, CHOPPER_MODE_STANDARD, CARVEY_RANDOM_TIME_OFF), &CS_PORT, csBit);
  spi_send20bit(setTMC26xSMARTEN(MIN_COOL_CURRENT_HALF), &CS_PORT, csBit);
  spi_send20bit(setTMC26xSGSCONF(SG2_FILTER_ENABLE, CARVEY_STALL_GAURD_THRESHOLD, Y_RUN_CURRENT), &CS_PORT, csBit);
  spi_send20bit( setTMC26xDRVCONF(READOUT_VALUE_SG2), &CS_PORT, csBit);
  spi_send20bit( setTMC26xDRVCTRL(STEP_ITERPOL_DISABLE,DOUBLE_EDGE_DISABLE,Y_MICROSTEPS), &CS_PORT, csBit);
  
 
  
  // setup x axis driver
  csBit = CS_Z_BIT;
  delay_ms(20); // give a little time before SPI starts
  spi_send20bit(setTMC26xCHOPCONF(CARVEY_CHOPPER_BLANKING_TIME, CHOPPER_MODE_STANDARD, CARVEY_RANDOM_TIME_OFF), &CS_PORT, csBit);
  spi_send20bit(setTMC26xSMARTEN(MIN_COOL_CURRENT_HALF), &CS_PORT, csBit);
  spi_send20bit(setTMC26xSGSCONF(SG2_FILTER_ENABLE, CARVEY_STALL_GAURD_THRESHOLD, Z_RUN_CURRENT), &CS_PORT, csBit);
  spi_send20bit( setTMC26xDRVCONF(READOUT_VALUE_SG2), &CS_PORT, csBit);
  spi_send20bit( setTMC26xDRVCTRL(STEP_ITERPOL_DISABLE,DOUBLE_EDGE_DISABLE,Z_MICROSTEPS), &CS_PORT, csBit);
  
}


void setTMC26xRunCurrent(uint8_t level)  // 1 = run, 0 = idle
{
	if (level == 1)
	{	
		spi_send20bit(setTMC26xSGSCONF(SG2_FILTER_ENABLE, CARVEY_STALL_GAURD_THRESHOLD, X_RUN_CURRENT), &CS_PORT, CS_X_BIT);
		spi_send20bit(setTMC26xSGSCONF(SG2_FILTER_ENABLE, CARVEY_STALL_GAURD_THRESHOLD, Y_RUN_CURRENT), &CS_PORT, CS_Y_BIT);
		spi_send20bit(setTMC26xSGSCONF(SG2_FILTER_ENABLE, CARVEY_STALL_GAURD_THRESHOLD, Z_RUN_CURRENT), &CS_PORT, CS_Z_BIT);
	}
	else 
	{
		spi_send20bit(setTMC26xSGSCONF(SG2_FILTER_ENABLE, CARVEY_STALL_GAURD_THRESHOLD, X_IDLE_CURRENT), &CS_PORT, CS_X_BIT);
		spi_send20bit(setTMC26xSGSCONF(SG2_FILTER_ENABLE, CARVEY_STALL_GAURD_THRESHOLD, Y_IDLE_CURRENT), &CS_PORT, CS_Y_BIT);
		spi_send20bit(setTMC26xSGSCONF(SG2_FILTER_ENABLE, CARVEY_STALL_GAURD_THRESHOLD, Z_IDLE_CURRENT), &CS_PORT, CS_Z_BIT);
	}
}


unsigned long readValue(uint8_t readItem)
 {
    return  spi_send20bit( setTMC26xDRVCONF(readItem), &CS_PORT, CS_X_BIT);
 }
 
 unsigned long readStallGuard()
 {
   //unsigned long sgVal;
   
   return (( spi_send20bit( setTMC26xDRVCONF(READOUT_VALUE_SG2), &CS_PORT, CS_X_BIT) & 0x7FC00) >> 10);
 }


/*
  ================================= CHOPCONF =============================
  blankingTime = blanking time in system clock intervals (16,24,36 or 54) an invalid value will default to 24
  
  randomTimeOff 0 = not enabled, 1 = enabled
  offTime ... 0 diables all MOSFETS, 1 = use with TBL of minimum 24 clocks, 0r 2...15 (N = 12 + (32 x TOFF) (Minimum time is 64clocks.))
  
*/
uint32_t setTMC26xCHOPCONF(uint8_t blankingTime, uint8_t chopModeStd, uint8_t randomTimeOff)
{
    uint32_t regVal = CHOPPER_CONFIG_REGISTER;
    
    #define HYSTERESIS_DECREMENT 16
    #define HYSTERESIS_LOW 6 // (-3 through +12)
    #define HYSTERESIS_MIN_VAl -3 // (-3 through +12)
    #define HYSTERESIS_START 8 // (1 through 8)
    #define OFF_TIME 2
    
    switch (blankingTime)
    {
      case 16:
        // leave bits at zero
      break;
      case 24:
        regVal |= (0x1ul << BLANK_TIMING_SHIFT);
      break;
      case 36:
        regVal |= (0x2ul << BLANK_TIMING_SHIFT);
      break;
      case 54:
        regVal |= (0x3ul << BLANK_TIMING_SHIFT);
      break;
      default:
        regVal |= (0x1ul << BLANK_TIMING_SHIFT);  // use 24 as default
      break;
    }
    
    if(!chopModeStd)
      regVal |= CHOPPER_MODE_T_OFF_FAST_DECAY;
      
    if (randomTimeOff == YES_RANDOM_TIME_OFF)
      regVal |= RANDOM_TOFF_TIME;
     
    // the rest of the values are based on Chopper mode 
    if (chopModeStd)
    {
      switch (HYSTERESIS_DECREMENT)
      {
        case 16:
          // do nothing
        break;
        case 32:
          regVal |= (0x1ul << HYSTERESIS_DECREMENT_SHIFT);
        break;
        case 48:
          regVal |= (0x2ul << HYSTERESIS_DECREMENT_SHIFT);
        break;
        case 64:
          regVal |= (0x3ul << HYSTERESIS_DECREMENT_SHIFT);
        break;       
      }
      
      //  HYSTERESIS_LOW
      regVal |= ((HYSTERESIS_LOW - HYSTERESIS_MIN_VAl) << HYSTERESIS_LOW_SHIFT);
      
      // HYSTERESIS_START
      regVal |= ((HYSTERESIS_START -1) << 4);
      
    }
    else // ! chopModeStd
    {
        // not implemented yet
    }
    
    regVal |= OFF_TIME;
    
    return regVal;
}


// ======================== SMARTEN ========================
uint32_t setTMC26xSMARTEN(char minCoolCurrent)
{
    uint32_t regVal = COOL_STEP_REGISTER;
    
    
    
    #define CURRENT_DECREMENT_SPEED 32
    #define CURRENT_DECREMENT_SPEED_SHIFT 12
    
    #define COOLSTEP_UPPER_THRESHOLD 0
    #define COOLSTEP_UPPER_THRESHOLD_SHIFT 8 
    
    #define CURRENT_INC_SIZE 1
    #define CURRENT_INC_SHIFT 4
    
    #define COOLSTEP_LOWER_THRESHOLD 0
    #define COOLSTEP_LOWER_THRESHOLD_SHIFT 8
    
    regVal |= (minCoolCurrent << MIN_COOL_CURRENT_SHIFT);
    
    switch (CURRENT_DECREMENT_SPEED)
    {
      case 32:
        
      break;
      case 8:
        regVal |= (1 << CURRENT_DECREMENT_SPEED_SHIFT);
      break;
      case 2:
        regVal |= (2 << CURRENT_DECREMENT_SPEED_SHIFT);
      break;
      case 1:
        regVal |= (3 << CURRENT_DECREMENT_SPEED_SHIFT);
      break;
    }
    
    regVal |= (COOLSTEP_UPPER_THRESHOLD << COOLSTEP_UPPER_THRESHOLD_SHIFT);
    
    switch (CURRENT_INC_SIZE)
    {
      case 1:
        
      break;
      case 2:
        regVal |= (1 << CURRENT_INC_SHIFT);
      break;
      case 4:
        regVal |= (2 << CURRENT_INC_SHIFT);
      break;
      case 8:
        regVal |= (3 << CURRENT_INC_SHIFT);
      break;
    }
    
    regVal |= (COOLSTEP_LOWER_THRESHOLD << COOLSTEP_LOWER_THRESHOLD_SHIFT);
    
    return regVal;
    
}



// ============================= DRVCONF =======================



uint32_t setTMC26xDRVCONF(uint8_t readItem)
{
  
  uint32_t regVal = DRIVER_CONFIG_REGISTER;
  
  regVal |= (STEP_DIR_ENABLE << STEP_DIR_SHIFT);
  
  regVal |= (VSENSE_165MV << VSENSE_SHIFT);
  
  

   // If readItem is out of range set to a default value
   if (readItem > READOUT_VALUE_SG2_COOLSTEP)
    readItem = READOUT_VALUE_SG2;
 
   regVal |= (READOUT_VALUE_SG2 << READOUT_VALUE_SHIFT);
 
  
  
  return regVal;
}


// currentScale 0 = 1/32 of full current 31 = 32/32 of full current  ... 0 = min, 31 = max

// =============================== SGSCONGF ==========================
uint32_t setTMC26xSGSCONF(uint8_t filterEnable, int8_t stallGuardThreshold, uint8_t currentScale)
{
  uint32_t regVal = STALL_GUARD2_LOAD_MEASURE_REGISTER;
  
  regVal |= ((unsigned long)filterEnable << 16);
  
  regVal |= (stallGuardThreshold << 8);
  
  regVal |= currentScale;
  
  return regVal;
  
}




/* 
  ============================================ DRVCTRL ========================================
  set drvctrl register (when SDOFF=0)

 interpol = 0 for regular stepping  (std behavior)
 interpol = 1 to multiply steps by 16

 doubleEdge = 0 to step on leading edge of pulse (std behavior)
 doubleEdge = 1 to step on rising and falling edge
 
 microstep = (1,2,4,8,16,32,64,128, 0r 256) defaults to 1 if illegal value given
*/
uint32_t setTMC26xDRVCTRL(uint8_t interpol, uint8_t doubleEdge, uint16_t microstepMode)
{
   unsigned char ms = 1;  // microstep setting
   uint32_t regVal = DRIVER_CONTROL_REGISTER;
   switch(microstepMode)
   {
      case 1:
        ms = 8; 
      break;
      case 2:
        ms = 7;
      break;
      case 4:
        ms = 6;
      break;
      case 8:
        ms = 5;
      break;
      case 16:
        ms = 4;
      break;
      case 32:
        ms = 3;
      break;
      case 64:
        ms = 2;
      break;
      case 128:
        ms = 1;
      break;
      case 256:
        ms = 0;
      break;      
   }
   
   if (interpol)
     regVal |= STEP_INTERPOLATION;
     
   if (doubleEdge)
     regVal |= DOUBLE_EDGE_STEP;
     
   regVal |= ms;
     
   return regVal;
}

unsigned long tmc26xStallGuardReading()
{
  
}


