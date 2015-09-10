#ifndef _SPI_H_
#define _SPI_H_
#include <avr/io.h>

extern void spi_init();
extern uint8_t spi_fast_shift (uint8_t data);

#endif /* _SPI_H_ */
