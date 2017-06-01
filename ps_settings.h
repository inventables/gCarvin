/*
  ps_settings.h - Handles storing and retrieving product specific settings from
  Non Volatile Storage (EEPROM)
  
  Copyright (c) 2017 Inventables Inc.
*/

#ifndef PS_SETTINGS_H
#define PS_SETTINGS_H

#include <stdint.h>

#define PS_SETTINGS_VERSION 1U
#define PS_SETTINGS_EEPROM_OFFSET 0x0800
#define PS_SETTINGS_NUM_PARAMETERS (sizeof(gCarvinParameterMap)/sizeof(ps_settings_map_element))

typedef struct
{
  uint16_t offset;
  uint16_t size;
  union
  {
      float    real;
      uint32_t integer;
  } default_value;
} ps_settings_map_element;

extern const ps_settings_map_element gCarvinParameterMap[];

/// Initializes the product specific settings subsystem
/// Allocates and initializes a ps_settings_definition_t structure and storage
/// @param[in] parameter_map A map of parameter to storage offset and size
extern void ps_settings_init( void );

/// Restore product specific NVS-backed settings to default
extern void ps_settings_restore( void );

/// Update the value and store a specific parameter in NVS
/// @param[in] parameter The parameter index identifying which parameter to set
/// @param[in] value A pointer to a RAM location storing the value to store
/// @return success
/// @retval 0 Sucessful operation (STATUS_OK)
/// @retval 1 Unsuccessful operation
extern uint8_t ps_settings_store_setting( uint8_t parameter, uint8_t* value );

/// Gets a specified parameter value stored in NVS
/// @param[in] The parameter index identifying which parameter to get
/// @param[out] value A pointer to a RAM location to contain the value
/// @return success
/// @retval 0 Sucessful operation (STATUS_OK)
/// @retval 1 Unsuccessful operation
extern uint8_t ps_settings_get_setting( uint8_t parameter, uint8_t* value );

#endif
