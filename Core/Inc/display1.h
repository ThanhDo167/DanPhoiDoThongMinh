#ifndef DISPLAY_H
#define DISPLAY_H

/* ================================================================
 * display.h
 * Module hiển thị LCD 16×2 qua I2C.
 * Chỉ đọc biến từ system_state — không ghi bất kỳ biến nào.
 * Dùng cache (old_line) để tránh ghi LCD không cần thiết.
 * ================================================================ */

#include <stdint.h>

/* Được khai báo ở đây để keypad.c có thể extern và xóa cache */
extern char old_line1[17];
extern char old_line2[17];

/* ---------- API ---------- */

/**
 * @brief  Cập nhật 2 dòng LCD dựa trên trạng thái hiện tại.
 *         Chỉ ghi I2C khi nội dung thực sự thay đổi.
 * @param  isRaining  1 = đang mưa
 * @param  isBright   1 = đủ sáng
 */
void Display_Update(uint8_t isRaining, uint8_t isBright);

#endif /* DISPLAY_H */
