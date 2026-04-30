#ifndef LCD_I2C_H
#define LCD_I2C_H

#include "stm32f1xx_hal.h"

void lcd_init(void);
void lcd_send_cmd(char cmd);
void lcd_send_data(char data);
void lcd_send_string(char *str);
void lcd_put_cur(int row, int col);
void lcd_clear(void);
void lcd_create_char(uint8_t location, uint8_t *data);
#endif
