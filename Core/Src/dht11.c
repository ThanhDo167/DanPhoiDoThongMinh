/*
 * dht11.c
 *
 *  Created on: Apr 24, 2026
 *      Author: PC ACER
 */
#include "dht11.h"

extern TIM_HandleTypeDef htim2;

// ==== DELAY US ====
static void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

// ==== SET PIN OUTPUT ====
static void DHT11_SetOutput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

// ==== SET PIN INPUT ====
static void DHT11_SetInput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

// ==== READ ====
uint8_t DHT11_Read(float *temperature, float *humidity)
{
    uint8_t data[5] = {0};

    // ===== START SIGNAL =====
    DHT11_SetOutput();
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);

    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
    delay_us(30);

    DHT11_SetInput();

    // ===== RESPONSE =====
    if(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))
        return 1;

    while(!HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN));
    while(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN));
    while(!HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN));

    // ===== READ 40 BIT =====
    for(int j = 0; j < 5; j++)
    {
        for(int i = 0; i < 8; i++)
        {
            while(!HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN));
            delay_us(40);

            if(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))
                data[j] |= (1 << (7 - i));

            while(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN));
        }
    }

    // ===== CHECK =====
    if((data[0] + data[1] + data[2] + data[3]) != data[4])
        return 1;

    *humidity = data[0];
    *temperature = data[2];

    return 0;
}

