#ifndef KEYPAD_H
#define KEYPAD_H

/* ================================================================
 * keypad.h
 * Module quét bàn phím ma trận 4×4 và xử lý lệnh người dùng
 * bao gồm: điều khiển thủ công, quạt, và mã số điều khiển.
 *
 * Bố cục phím:
 *   1 2 3 A      A = Thu vào   (chỉ khi đã ở MANUAL mode)
 *   4 5 6 B      B = Phơi ra   (chỉ khi đã ở MANUAL mode)
 *   7 8 9 C      C = Bật quạt  (chỉ khi đã ở MANUAL mode)
 *   * 0 # D      D = Tắt quạt  (chỉ khi đã ở MANUAL mode)
 *                * = Xóa mã đang nhập
 *                # = Xác nhận mã:
 *                      "1"    + # → bật MANUAL mode (mở khóa A/B/C/D)
 *                      "0"    + # → thoát MANUAL, về AUTO ngay
 *                      "1234" + # → tắt còi, không đổi manual_mode
 * ================================================================ */

#include "main.h"
#include <stdint.h>

/* ---------- Pin hàng (OUTPUT) — GPIOB ---------- */
extern uint16_t row_pins[4];   /* PB0, PB1, PB10, PB11 */

/* ---------- Pin cột (INPUT PULLUP) — GPIOB ---------- */
extern uint16_t col_pins[4];   /* PB12, PB13, PB14, PB15 */

/* ---------- API ---------- */

/**
 * @brief  Quét toàn bộ ma trận, trả về ký tự phím đang nhấn.
 *         Bao gồm debounce 20 ms và chờ nhả phím.
 * @return Ký tự phím (ví dụ: '1', 'A', '#') hoặc 0 nếu không có phím.
 */
char Keypad_Read(void);

/**
 * @brief  Xử lý phím vừa quét được và cập nhật trạng thái hệ thống.
 *         Gọi mỗi vòng lặp chính (không blocking).
 */
void Keypad_Handle(void);

#endif /* KEYPAD_H */
