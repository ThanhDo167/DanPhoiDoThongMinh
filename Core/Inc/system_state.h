#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include "main.h"
#include <stdint.h>

/* ================================================================
 * system_state.h
 * Khai báo tất cả biến trạng thái toàn cục dùng chung giữa các module.
 * Mỗi module #include "system_state.h" để truy cập.
 * Định nghĩa thực sự nằm trong system_state.c (một lần duy nhất).
 * ================================================================ */

/* ---------- Kiểu dữ liệu ---------- */
typedef enum
{
    STATE_THU  = 0,   /* Đang thu vào */
    STATE_PHOI = 1    /* Đang phơi ra */
} SystemState;

/* ---------- Trạng thái hệ thống ---------- */
extern SystemState currentState;   /* Vị trí hiện tại của giá */
extern SystemState targetState;    /* Vị trí đích cần đến     */

extern uint8_t manual_mode;        /* 0 = AUTO, 1 = MANUAL    */
extern uint8_t system_locked;      /* 1 = bị khóa do sai mật khẩu */

/* ---------- Cảm biến ---------- */
extern float    temp;
extern float    humi;
extern uint16_t adcValue;
extern uint8_t  last_isRaining;
extern uint8_t  last_isBright;

/* ---------- Quạt ---------- */
extern uint8_t  fan_state;         /* 0 = OFF, 1 = ON  */
extern uint8_t  fan_manual;        /* 0 = AUTO, 1 = MANUAL */
extern uint32_t fan_timer;
extern uint8_t  rain_prev;

/* ---------- Còi ---------- */
extern uint8_t buzzer_off;

/* ---------- Mật khẩu ---------- */
#define MAX_WRONG_PASS     5U
#define CORRECT_PASS       "1234"   /* Mã tắt còi, không liên quan manual */
#define MANUAL_ENTER_CODE  "1"      /* Mã bật MANUAL mode (mở khóa A/B/C/D) */
#define MANUAL_EXIT_CODE   "0"      /* Mã thoát MANUAL mode, về AUTO */
#define UNLOCK_PASS   "9999"        /* Mã mở khóa hệ thống */
extern char    input_pass[5];
extern uint8_t pass_index;
extern uint8_t wrong_count;
extern uint8_t unlock_mode;
/* ---------- Timer cập nhật ---------- */
extern uint32_t lastUpdate;

#endif /* SYSTEM_STATE_H */
