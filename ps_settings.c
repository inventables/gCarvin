/*
  ps_settings.c - Handles storing and retrieving product specific settings from
  Non Volatile Storage (EEPROM)
  
  Copyright (c) 2017 Inventables Inc.
*/

#include "ps_settings.h"
#include "string.h"
#include "stdlib.h"

#include "eeprom.h"
#include "nuts_bolts.h"

#include <avr/pgmspace.h>

#define PS_SETTINGS_EEPROM_REVISION_OFFSET   0x1E
#define PS_SETTINGS_EEPROM_PARAM_SIZE_OFFSET 0x1F
#define PS_SETTINGS_EEPROM_PARAMETERS_OFFSET 0x20

const ps_settings_map_element gCarvinParameterMap[] =
{ // {Offset, Size, default value}
  { 0U, 4U, 1.05f }, // Spindle Current Overload Threshold
};

static uint8_t* ps_settings_ram_storage = 0U;
static const ps_settings_map_element* ps_settings_metadata = gCarvinParameterMap;
static const char* ps_settings_header = "Inventables Settings          ";
static uint8_t storage_size = 0U;

static void set_default_values( uint8_t* data );
static void set_eeprom_storage_size( uint8_t size );
static void set_eeprom_header_and_revision( void );
static void write_ps_settings_to_eeprom( uint8_t* data, uint8_t data_size );
static uint8_t validate_eeprom_image( const uint8_t* data );
static void* my_memcpy(void *dest, const void *src, unsigned int n);

void ps_settings_init( void )
{
  uint8_t error = 0U;
  
  if ( PS_SETTINGS_NUM_PARAMETERS > 0 )
  {
    uint8_t eeprom_data[ 256U ];
    uint8_t eeprom_data_size = eeprom_get_char( PS_SETTINGS_EEPROM_OFFSET + PS_SETTINGS_EEPROM_PARAM_SIZE_OFFSET );
    uint8_t eeprom_revision = eeprom_get_char( PS_SETTINGS_EEPROM_OFFSET + PS_SETTINGS_EEPROM_REVISION_OFFSET );
    uint8_t eeprom_checksum = 0U;
    uint8_t element = 0U;
    
    storage_size = 0U;
        
    for ( ; element < PS_SETTINGS_NUM_PARAMETERS; ++element )
    {
      storage_size += ps_settings_metadata[ element ].size;
    }
    
    ps_settings_ram_storage = malloc( storage_size );
    
    // set RAM storage to defaults    
    set_default_values( ps_settings_ram_storage );
    
    // recall from EEPROM
    // header "Inventables Settings      "
    memcpy_from_eeprom_no_checksum( eeprom_data,
                                    PS_SETTINGS_EEPROM_OFFSET, 
                                    30U );
                                      
    if ( (eeprom_data_size > 0U) && 
         (eeprom_data_size < 255U) &&
         (memcmp( ps_settings_header, eeprom_data, 30U ) == 0 ) )
    {
      eeprom_checksum = memcpy_from_eeprom_with_checksum( eeprom_data,
                                                          PS_SETTINGS_EEPROM_OFFSET + PS_SETTINGS_EEPROM_PARAMETERS_OFFSET,
                                                          eeprom_data_size );
      if ( eeprom_checksum )
      {
        my_memcpy( ps_settings_ram_storage,
                   eeprom_data,
                   min( eeprom_data_size, storage_size ) );
      }
      
      if ( eeprom_data_size != storage_size )
      {
        if ( eeprom_data_size > storage_size )
        {
          // more data than expected --> downgrade
          set_eeprom_storage_size( storage_size );
        }
        else if ( eeprom_data_size < storage_size )
        {
          // less data than expected --> upgrade
          set_eeprom_storage_size( storage_size );
          write_ps_settings_to_eeprom( ps_settings_ram_storage, storage_size );
        }
      }
    }
    else
    {
      ps_settings_restore();
    }    
  }
}

void ps_settings_restore( void )
{
  set_eeprom_header_and_revision();
  
  set_eeprom_storage_size( storage_size );
  
  set_default_values( ps_settings_ram_storage );
  
  write_ps_settings_to_eeprom( ps_settings_ram_storage, storage_size );
}

uint8_t ps_settings_store_setting( uint8_t parameter, uint8_t* value )
{
  uint8_t success = 1U;
  
  if ( parameter <= PS_SETTINGS_NUM_PARAMETERS )
  {
    success = 0U; //(STATUS_OK)
    my_memcpy( ps_settings_ram_storage + ps_settings_metadata[ parameter ].offset,
               value,
               ps_settings_metadata[ parameter ].size );
            
    write_ps_settings_to_eeprom( ps_settings_ram_storage, storage_size );
  }
  
  return success;
}

uint8_t ps_settings_get_setting( uint8_t parameter, uint8_t* value )
{
  uint8_t success = 1U;
  
  if ( parameter < PS_SETTINGS_NUM_PARAMETERS )
  {
    if ( ps_settings_ram_storage )
    {
      success = 0U; //(STATUS_OK)
      my_memcpy( value, 
                 ps_settings_ram_storage + ps_settings_metadata[ parameter ].offset, 
                 ps_settings_metadata[ parameter ].size );
    }
  }
  
  return success;
}

void set_default_values( uint8_t* data )
{
  uint8_t element = 0U;
  uint8_t size = 0U;
  
  for ( ; element <= PS_SETTINGS_NUM_PARAMETERS; ++element )
  {
    my_memcpy( &data[ ps_settings_metadata[ element ].offset ], 
               &ps_settings_metadata[ element ].default_value.integer,
               ps_settings_metadata[ element ].size );
  }
}

void set_eeprom_storage_size( uint8_t size )
{
  eeprom_put_char( PS_SETTINGS_EEPROM_OFFSET + PS_SETTINGS_EEPROM_PARAM_SIZE_OFFSET, size );  
}

void write_ps_settings_to_eeprom( uint8_t* data, uint8_t data_size )
{
  if ( data && data_size )
  {
    memcpy_to_eeprom_with_checksum( PS_SETTINGS_EEPROM_OFFSET + PS_SETTINGS_EEPROM_PARAMETERS_OFFSET,
                                    data,
                                    data_size );
  }
}

void set_eeprom_header_and_revision( void )
{
  memcpy_to_eeprom_no_checksum( PS_SETTINGS_EEPROM_OFFSET, ps_settings_header, 30U );
  eeprom_put_char( PS_SETTINGS_EEPROM_OFFSET + PS_SETTINGS_EEPROM_REVISION_OFFSET, PS_SETTINGS_VERSION );
}

void* my_memcpy( void *dest, const void *src, unsigned int n )
{
  void * retVal = dest;
  if ( dest && src )
  {
    while ( n > 0 )
    {
      *(char*)src++ = *(char*)dest++;
      --n;
    }
  }
  return ( retVal );
}
