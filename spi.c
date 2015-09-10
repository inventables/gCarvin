#include "spi.h"
#include <avr/io.h>
#include <avr/interrupt.h>


// Mega
#define PORT_SPI    PORTB
#define DDR_SPI     DDRB
#define DD_MISO     DDB3
#define DD_MOSI     DDB2
#define DD_SCK      DDB1
#define DD_SS       DDB0


void spi_init()
// Initialize pins for spi communication
{
    DDR_SPI &= ~((1<<DD_MOSI)|(1<<DD_MISO)|(1<<DD_SS)|(1<<DD_SCK));
    
    // Define the following pins as output
    DDR_SPI |= ((1<<DD_MOSI)|(1<<DD_SS)|(1<<DD_SCK));
   
    SPCR = ((1<<SPE)|               // SPI Enable
            (0<<SPIE)|              // SPI Interupt Enable
            (0<<DORD)|              // Data Order (0:MSB first / 1:LSB first)
            (1<<MSTR)|              // Master/Slave select   
            (0<<SPR1)|(1<<SPR0)|    // SPI Clock Rate .....  fosc/128
            (1<<CPOL)|              // Clock Polarity (0:SCK low / 1:SCK hi when idle)
            (1<<CPHA));             // Clock Phase (0:leading / 1:trailing edge sampling)

    SPSR = (1<<SPI2X);              // Double Clock Rate
}


uint8_t spi_fast_shift (uint8_t data)
// Clocks only one byte to target device and returns the received one
{
    SPDR = data;
    while((SPSR & (1<<SPIF))==0);
    return SPDR;
}


// Send a 20 bit value via spi
unsigned long spi_send20bit(unsigned long datagram, volatile uint8_t *cs_port, uint8_t cs_bit)
{
  unsigned long i_datagram;  // this is the returned value
  
  *cs_port &= ~(1<<cs_bit);  // chip select
  
  datagram &= 0xFFFFFul; // force to 20 bit
  
  i_datagram = spi_fast_shift((datagram >> 16) & 0xff);
  i_datagram <<= 8;
  i_datagram |= spi_fast_shift((datagram >>  8) & 0xff);
  i_datagram <<= 8;
  i_datagram |= spi_fast_shift((datagram) & 0xff);
  i_datagram >>= 4;
  
   *cs_port |= (1<<cs_bit);   // deselect chip
  
  return i_datagram;
}
