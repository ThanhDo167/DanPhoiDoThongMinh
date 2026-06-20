#ifndef LOGIC_H
#define LOGIC_H

/* ================================================================
 * logic.h
 * ================================================================ */

#include <stdint.h>

/**
 * @brief  Cập nhật targetState dựa trên điều kiện môi trường.
 *
 *         Thứ tự ưu tiên:
 *           1. Mưa → LUÔN thu vào (STATE_THU), bất kể AUTO hay MANUAL.
 *           2. Không mưa + MANUAL → giữ nguyên targetState, không can thiệp.
 *           3. Không mưa + AUTO → xét độ ẩm (>80%) và ánh sáng để quyết định.
 *
 *         Guard: humidity = 0 (DHT11 lỗi) không được dùng để quyết định.
 */
void Logic_UpdateRackState(uint8_t isRaining, uint8_t isBright, float humidity);

/**
 * @brief  Xử lý quạt theo mưa. Gọi mỗi 1 giây.
 *
 *         Quạt:
 *           - Bật khi giá đã THU XONG và đang mưa.
 *           - Tiếp tục chạy liên tục trong suốt thời gian còn mưa.
 *           - Khi mưa dừng, đếm thêm 10 giây rồi mới tắt.
 *           - Áp dụng cho mọi chế độ (AUTO hoặc MANUAL).
 *
 *         MANUAL mode là quyền cao nhất: KHÔNG có timeout tự động
 *         quay về AUTO. MANUAL chỉ thoát khi người dùng chủ động
 *         bấm "0#" (xem Keypad_Handle), hoặc khi hệ thống bị khóa
 *         do nhập sai mật khẩu quá số lần cho phép.
 */
void Logic_HandleFanAuto(uint8_t isRaining);

#endif /* LOGIC_H */
