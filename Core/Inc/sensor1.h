#ifndef SENSOR_H
#define SENSOR_H

/* ================================================================
 * sensor.h
 * Module đọc cảm biến: mưa (digital), ánh sáng (ADC/LDR), nhiệt độ
 * và độ ẩm (DHT11).
 * ================================================================ */

#include "main.h"
#include <stdint.h>

/* ---------- Pin cảm biến mưa ---------- */
#define RAIN_PORT       GPIOA
#define RAIN_PIN        GPIO_PIN_1

/* ---------- LDR qua ADC1 channel 0 (PA0) ---------- */
#define LDR_CHANNEL     ADC_CHANNEL_0
#define LIGHT_THRESHOLD 2000U   /* ADC < threshold => sáng */

/* ---------- API ---------- */

/**
 * @brief  Đọc và lọc cảm biến mưa (majority filter 5 lần đọc).
 * @return 1 = đang mưa, 0 = không mưa
 */
uint8_t  Sensor_RainDetect(void);

/**
 * @brief  Khởi động ADC, đọc giá trị LDR, dừng ADC.
 * @return Giá trị ADC thô (0–4095)
 */
uint16_t Sensor_ReadLDR(void);

/**
 * @brief  Đọc DHT11, ghi kết quả vào con trỏ ngoài.
 *         Nếu đọc lỗi, ghi 0.0 cho cả hai giá trị.
 * @param  out_temp  Con trỏ nhận nhiệt độ (°C)
 * @param  out_humi  Con trỏ nhận độ ẩm (%)
 */
void     Sensor_ReadDHT11(float *out_temp, float *out_humi);

#endif /* SENSOR_H */
