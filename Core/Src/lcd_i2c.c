#include "lcd_i2c.h"

extern I2C_HandleTypeDef hi2c1;

#define LCD_ADDR (0x27 << 1)   // đổi 0x3F nếu không chạy

#define LCD_BACKLIGHT 0x08
#define LCD_ENABLE    0x04

// ==== GỬI 4 BIT ====
void lcd_send_4bit(uint8_t data)
{
    uint8_t data_t[2];

    data_t[0] = data | LCD_ENABLE | LCD_BACKLIGHT;
    data_t[1] = data | LCD_BACKLIGHT;

    HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, data_t, 2, 100);
}

// ==== GỬI LỆNH ====
void lcd_send_cmd(char cmd)
{
    uint8_t high = cmd & 0xF0;
    uint8_t low  = (cmd << 4) & 0xF0;

    lcd_send_4bit(high);
    lcd_send_4bit(low);

    HAL_Delay(2);
}

// ==== GỬI DATA ====
void lcd_send_data(char data)
{
    uint8_t high = data & 0xF0;
    uint8_t low  = (data << 4) & 0xF0;

    lcd_send_4bit(high | 0x01);   // RS = 1
    lcd_send_4bit(low  | 0x01);

    HAL_Delay(2);
}

// ==== KHỞI TẠO ====
void lcd_init(void)
{
    HAL_Delay(50);

    // reset 4-bit
    lcd_send_4bit(0x30);
    HAL_Delay(5);
    lcd_send_4bit(0x30);
    HAL_Delay(1);
    lcd_send_4bit(0x30);
    HAL_Delay(10);
    lcd_send_4bit(0x20);  // 4-bit mode

    // cấu hình
    lcd_send_cmd(0x28); // 2 dòng, font 5x8
    lcd_send_cmd(0x08); // display off
    lcd_send_cmd(0x01); // clear
    HAL_Delay(2);
    lcd_send_cmd(0x06); // entry mode
    lcd_send_cmd(0x0C); // display on, cursor off
}

// ==== GỬI CHUỖI ====
void lcd_send_string(char *str)
{
    while (*str)
        lcd_send_data(*str++);
}

// ==== SET CURSOR ====
void lcd_put_cur(int row, int col)
{
    uint8_t pos;

    if(row == 0)
        pos = 0x80 + col;
    else
        pos = 0xC0 + col;

    lcd_send_cmd(pos);
}

void lcd_create_char(uint8_t location, uint8_t *data)
{
    location &= 0x07;
    lcd_send_cmd(0x40 | (location << 3));

    for(int i=0;i<8;i++)
    {
        lcd_send_data(data[i]);
    }
}
// ==== CLEAR ====
void lcd_clear(void)
{
    lcd_send_cmd(0x01);
    HAL_Delay(2);
}
