#ifndef UART_CLI_H
#define UART_CLI_H

/* ================================================================
 * uart_cli.h  —  UART Monitor (read-only dashboard)
 *
 * Chế độ: CHỈ HIỂN THỊ — không nhận lệnh từ PC.
 * Mọi điều khiển thực hiện qua bàn phím vật lý (keypad).
 *
 * Cấu hình PuTTY / terminal:
 *   Baud rate : 115200
 *   Data bits : 8
 *   Stop bits : 1
 *   Parity    : None
 *   Flow ctrl : None
 *   Terminal  : VT100 / xterm  (bật ANSI color)
 *   Implicit CR in every LF: BẬT
 *
 * Chiến lược không nhấp nháy:
 *   - Skeleton (khung, nhãn tĩnh) chỉ vẽ 1 lần lúc init.
 *   - Mỗi giây chỉ nhảy tới đúng cột giá trị và overwrite nội dung.
 *   - Tuyệt đối không dùng ANSI_CLEAR (\033[2J) sau init.
 *   - Con trỏ bị ẩn (\033[?25l) để không nháy trên màn hình.
 * ================================================================ */

#include "main.h"
#include <stdint.h>

extern UART_HandleTypeDef huart1;

/* Khởi tạo UART1 (115200-8N1) và vẽ skeleton dashboard một lần. */
void UART_CLI_Init(void);

/* Gọi mỗi 1 giây: cập nhật CHỈ các ô giá trị thay đổi. */
void UART_CLI_Update(void);

/* Tiện ích gửi chuỗi — dùng nội bộ và từ module khác nếu cần. */
void UART_CLI_Print(const char *str);
void UART_CLI_Println(const char *str);

#endif /* UART_CLI_H */
