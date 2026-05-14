#ifndef SPI_H
#define SPI_H

#include "gd32f30x.h"

void spi_lcd_init(void);
void SPI2_WriteBytes(uint8_t *pbuffer, uint32_t length);

#endif /* SPI_H */
