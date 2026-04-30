/*
 * dht11.h
 *
 *  Created on: Apr 24, 2026
 *      Author: PC ACER
 */

#ifndef DHT11_H
#define DHT11_H

#include "stm32f1xx_hal.h"

// ==== CONFIG ====
// đổi PORT + PIN nếu bạn dùng chân khác
#define DHT11_PORT GPIOA
#define DHT11_PIN  GPIO_PIN_2

// ==== FUNCTION ====
uint8_t DHT11_Read(float *temperature, float *humidity);

#endif
