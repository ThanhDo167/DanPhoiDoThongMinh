#include "system_state.h"

/* ================================================================
 * system_state.c
 * Định nghĩa (cấp phát bộ nhớ) cho tất cả biến toàn cục.
 * Chỉ file này được phép định nghĩa — các file khác chỉ dùng extern.
 * ================================================================ */

/* ---------- Trạng thái hệ thống ---------- */
SystemState currentState = STATE_THU;
SystemState targetState  = STATE_THU;

uint8_t manual_mode    = 0;
uint8_t system_locked  = 0;

/* ---------- Cảm biến ---------- */
float    temp          = 0.0f;
float    humi          = 0.0f;
uint16_t adcValue      = 0;
uint8_t  last_isRaining = 0;
uint8_t  last_isBright  = 0;

/* ---------- Quạt ---------- */
uint8_t  fan_state     = 0;
uint8_t  fan_manual    = 0;
uint32_t fan_timer     = 0;
uint8_t  rain_prev     = 0;

/* ---------- Còi ---------- */
uint8_t buzzer_off     = 0;

/* ---------- Mật khẩu ---------- */
char    input_pass[5]  = "";
uint8_t pass_index     = 0;
uint8_t wrong_count    = 0;
uint8_t unlock_mode = 0;

/* ---------- Timer ---------- */
uint32_t lastUpdate    = 0;
