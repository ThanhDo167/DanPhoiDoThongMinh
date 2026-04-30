/*
 * code.c
 *
 *  Created on: Apr 24, 2026
 *      Author: PC ACER
 */
#include "main.h"
#include <stdio.h>
#include <string.h>

// ==== DEFINE ====
#define RAIN_PIN GPIO_PIN_1
#define RAIN_PORT GPIOA

#define LDR_PIN GPIO_PIN_0
#define LDR_PORT GPIOA

#define BUZZER_PIN GPIO_PIN_3
#define BUZZER_PORT GPIOA

#define RELAY_PIN GPIO_PIN_4
#define RELAY_PORT GPIOA

// Stepper
#define IN1 GPIO_PIN_5
#define IN2 GPIO_PIN_6
#define IN3 GPIO_PIN_7
#define IN4 GPIO_PIN_8
#define STEPPER_PORT GPIOA

extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim2;

// ==== GLOBAL ====
uint8_t temp, humi;
uint8_t buzzer_off = 0;
char pass[5] = "";
int pass_index = 0;

// ==== DELAY US ====
void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

// ==== DHT11 ====
void DHT11_Start()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, 0);
    HAL_Delay(18);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, 1);
    delay_us(20);

    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

uint8_t DHT11_Read()
{
    uint8_t i, j;
    for(j=0;j<8;j++)
    {
        while(!(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2)));
        delay_us(40);
        if(!(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2)))
            i &= ~(1<<(7-j));
        else
            i |= (1<<(7-j));
        while((HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2)));
    }
    return i;
}

// ==== STEPPER ====
void step(int a,int b,int c,int d)
{
    HAL_GPIO_WritePin(STEPPER_PORT, IN1, a);
    HAL_GPIO_WritePin(STEPPER_PORT, IN2, b);
    HAL_GPIO_WritePin(STEPPER_PORT, IN3, c);
    HAL_GPIO_WritePin(STEPPER_PORT, IN4, d);
}

void stepper_in()
{
    for(int i=0;i<50;i++)
    {
        step(1,0,0,0); HAL_Delay(2);
        step(1,1,0,0); HAL_Delay(2);
        step(0,1,0,0); HAL_Delay(2);
        step(0,1,1,0); HAL_Delay(2);
        step(0,0,1,0); HAL_Delay(2);
        step(0,0,1,1); HAL_Delay(2);
        step(0,0,0,1); HAL_Delay(2);
        step(1,0,0,1); HAL_Delay(2);
    }
}

void stepper_out()
{
    for(int i=0;i<50;i++)
    {
        step(1,0,0,1); HAL_Delay(2);
        step(0,0,0,1); HAL_Delay(2);
        step(0,0,1,1); HAL_Delay(2);
        step(0,0,1,0); HAL_Delay(2);
        step(0,1,1,0); HAL_Delay(2);
        step(0,1,0,0); HAL_Delay(2);
        step(1,1,0,0); HAL_Delay(2);
        step(1,0,0,0); HAL_Delay(2);
    }
}

// ==== LCD (GIẢ LẬP - bạn dùng thư viện ngoài) ====
void LCD_Print(char *str)
{
    // dùng thư viện LCD I2C bạn đã add
}

// ==== KEYPAD (giả lập) ====
char keypad_read()
{
    return 0; // bạn sẽ thay bằng code keypad
}

// ==== MAIN ====
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_TIM2_Init();

    HAL_TIM_Base_Start(&htim2);

    while (1)
    {
        uint8_t rain = HAL_GPIO_ReadPin(RAIN_PORT, RAIN_PIN);
        uint8_t light = HAL_GPIO_ReadPin(LDR_PORT, LDR_PIN);

        DHT11_Start();
        humi = DHT11_Read();
        temp = DHT11_Read();

        // ==== AUTO MODE ====
        if((rain == 0 || humi > 80 || light == 0) && !buzzer_off)
        {
            LCD_Print("CANH BAO MUA");
            HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, 1);
            stepper_in();
        }
        else
        {
            HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, 0);
            stepper_out();
        }

        // ==== KEYPAD ====
        char key = keypad_read();

        if(key != 0)
        {
            if(key == 'A')
            {
                stepper_in();
            }
            else if(key == 'B')
            {
                stepper_out();
            }
            else
            {
                pass[pass_index++] = key;

                if(pass_index == 4)
                {
                    if(strcmp(pass,"1234") == 0)
                    {
                        HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, 0);
                        buzzer_off = 1;
                    }
                    pass_index = 0;
                    memset(pass,0,5);
                }
            }
        }

        HAL_Delay(500);
    }
}
