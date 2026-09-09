#ifndef __LCD_SPI_H__
#define __LCD_SPI_H__

#include <stdint.h>
#include <stdbool.h>

typedef void (*lcd_spi_send_finish_callback_t)(void);

void lcd_spi_init(void);
void lcd_spi_write(uint8_t *data, uint16_t length);
void lcd_spi_write_async(uint8_t *data, uint16_t length);
void lcd_spi_send_finish_register(lcd_spi_send_finish_callback_t callback);

#endif 
