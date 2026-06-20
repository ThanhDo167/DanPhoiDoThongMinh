#ifndef ACTUATOR_H
#define ACTUATOR_H

/* ================================================================
 * actuator.h
 * Module điều khiển quạt (relay) và còi (buzzer).
 * Đây là lớp duy nhất được phép ghi GPIO của relay và buzzer.
 * ================================================================ */

#include "main.h"
#include <stdint.h>

/* ---------- Pin quạt (qua relay) ---------- */
#define RELAY_PORT  GPIOA
#define RELAY_PIN   GPIO_PIN_4

/* ---------- Pin còi ---------- */
#define BUZZER_PORT GPIOA
#define BUZZER_PIN  GPIO_PIN_3

/* ---------- API ---------- */

/**
 * @brief  Bật hoặc tắt quạt qua relay.
 *         Cập nhật biến fan_state trong system_state.
 * @param  state  1 = ON, 0 = OFF
 */
void Actuator_SetFan(uint8_t state);

/**
 * @brief  Điều khiển còi dựa trên trạng thái mưa và các cờ hệ thống.
 *         Tự đọc system_locked, buzzer_off từ system_state.
 * @param  isRaining  1 = đang mưa, 0 = không mưa
 */
void Actuator_HandleBuzzer(uint8_t isRaining);

#endif /* ACTUATOR_H */
