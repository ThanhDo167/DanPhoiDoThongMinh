#ifndef STEPPER_H
#define STEPPER_H

/* ================================================================
 * stepper.h
 * Module điều khiển động cơ bước 28BYJ-48 qua driver L298N.
 * Sử dụng half-step 8 bước, non-blocking (không HAL_Delay).
 * ================================================================ */

#include "main.h"
#include <stdint.h>

/* ---------- Pin kết nối L298N ---------- */
#define IN1_PORT    GPIOA
#define IN1_PIN     GPIO_PIN_5

#define IN2_PORT    GPIOA
#define IN2_PIN     GPIO_PIN_6

#define IN3_PORT    GPIOA
#define IN3_PIN     GPIO_PIN_7

#define IN4_PORT    GPIOB
#define IN4_PIN     GPIO_PIN_8

/* ---------- Thông số động cơ ---------- */
#define STEPPER_DELAY  1U      /* Thời gian tối thiểu giữa 2 bước (ms) */
#define STEPPER_STEPS  4096U   /* Số bước cho 1 vòng quay đầy đủ       */

/* ---------- API ---------- */

/**
 * @brief  Đặt số bước và chiều quay, bắt đầu chuỗi chuyển động.
 * @param  steps  Số bước cần thực hiện
 * @param  dir    +1 = chiều thuận (THU), -1 = chiều ngược (PHƠI)
 */
void Stepper_Start(int steps, int dir);

/**
 * @brief  Gọi mỗi vòng lặp chính để tiến thêm 1 bước nếu đến lúc.
 *         Non-blocking: dùng HAL_GetTick() thay vì HAL_Delay().
 */
void Stepper_Run(void);

/**
 * @brief  Kiểm tra xem động cơ có đang chạy không.
 * @return 1 = đang chạy, 0 = dừng
 */
uint8_t Stepper_IsBusy(void);

/**
 * @brief  Tắt điện tất cả cuộn dây (giảm nhiệt, tiết kiệm điện).
 */
void Stepper_PowerOff(void);

#endif /* STEPPER_H */
