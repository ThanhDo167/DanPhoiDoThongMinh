/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <lcd_i2c.h>
#include <dht11.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// ==== DEFINE ====
#define RAIN_PIN GPIO_PIN_1
#define LDR_CHANNEL ADC_CHANNEL_0

#define BUZZER_PIN GPIO_PIN_3
#define RELAY_PIN  GPIO_PIN_4

// Stepper
#define IN1 GPIO_PIN_5
#define IN2 GPIO_PIN_6
#define IN3 GPIO_PIN_7
#define IN4 GPIO_PIN_8


#define LIGHT_THRESHOLD 2000

// ===== STEPPER ULN2003 (NON-BLOCKING) =====
#define STEPPER_DELAY 2   // ms

const uint8_t step_seq[8][4] = {
    {1,0,0,0},
    {1,1,0,0},
    {0,1,0,0},
    {0,1,1,0},
    {0,0,1,0},
    {0,0,1,1},
    {0,0,0,1},
    {1,0,0,1}
};

int step_index = 0;
int step_dir = 0;          // 1: forward, -1: reverse
int step_count = 0;
int step_target = 0;
uint32_t last_step_time = 0;

extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim2;

// ==== GLOBAL ====
float temp = 0, humi = 0;
uint16_t adcValue;
uint32_t lastUpdate = 0;
// ===== LCD BUFFER =====
char line1[16];
char line2[16];
char old_line1[16] = "";
char old_line2[16] = "";



// ===== KEYPAD MAP =====
const char keymap[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

uint16_t row_pins[4] = {GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_10, GPIO_PIN_11};
uint16_t col_pins[4] = {GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15};
uint8_t extended = 0;
uint8_t buzzer_off = 0;

// ================= FAN FSM =================
typedef enum { FAN_AUTO, FAN_MANUAL } FanMode;
typedef enum { FAN_OFF, FAN_ON } FanState;

FanMode fan_mode = FAN_AUTO;
FanState fan_state = FAN_OFF;
uint32_t fan_timer = 0;       // đếm thời gian 10s
uint8_t rain_prev = 0;        // phát hiện cạnh mưa

// ================= SYSTEM =================
typedef enum { STATE_THU, STATE_PHOI } SystemState;

SystemState currentState = STATE_THU;
SystemState targetState  = STATE_THU;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE BEGIN 0 */
void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

// ===== PASSWORD =====
char correct_pass[] = "1234";
char input_pass[5] = "";
char fan_str[5];
uint8_t pass_index = 0;
uint8_t wrong_count = 0;
uint8_t system_locked = 0;

// ===== MODE =====
uint8_t manual_mode = 0; // 0 = auto, 1 = manual

// ====KEYPAD====
char keypad_read()
{
    for(int r=0; r<4; r++)
    {
        for(int i=0;i<4;i++)
            HAL_GPIO_WritePin(GPIOB, row_pins[i], GPIO_PIN_SET);

        HAL_GPIO_WritePin(GPIOB, row_pins[r], GPIO_PIN_RESET);

        for(int c=0; c<4; c++)
        {
            if(HAL_GPIO_ReadPin(GPIOB, col_pins[c]) == GPIO_PIN_RESET)
            {
                HAL_Delay(20);

                if(HAL_GPIO_ReadPin(GPIOB, col_pins[c]) == GPIO_PIN_RESET)
                {
                    while(HAL_GPIO_ReadPin(GPIOB, col_pins[c]) == GPIO_PIN_RESET);
                    return keymap[r][c];
                }
            }
        }
    }
    return 0;
}

// ===== LDR =====
uint16_t read_LDR()
{
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1,100);
    return HAL_ADC_GetValue(&hadc1);
}

// ===== RAIN FILTER =====
uint8_t rain_detect()
{
    int count = 0;

    for(int i=0;i<5;i++)
    {
        if(HAL_GPIO_ReadPin(GPIOA, RAIN_PIN) == 0)
            count++;

        HAL_Delay(5);
    }

    return (count >= 3);
}
// ===== STEPPER =====
void stepper_write(uint8_t s[4])
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, s[0]);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, s[1]);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, s[2]);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, s[3]); // 🔥 PB8 đúng của bạn
}

void stepper_start(int steps, int dir)
{
    step_target = steps;
    step_count = 0;
    step_dir = dir;
}

void stepper_run()
{
    if(step_count >= step_target) return;

    if(HAL_GetTick() - last_step_time >= STEPPER_DELAY)
    {
        last_step_time = HAL_GetTick();

        stepper_write((uint8_t*)step_seq[step_index]);

        step_index += step_dir;
        if(step_index > 7) step_index = 0;
        if(step_index < 0) step_index = 7;

        step_count++;
    }
}

// ===== MOTOR CONTROL =====
void handle_motor()
{
    static uint8_t motor_running = 0;

    if(currentState != targetState && !motor_running)
    {
        if(targetState == STATE_THU)
            stepper_start(1024, -1); // quay vào
        else
            stepper_start(1024, 1);  // quay ra

        motor_running = 1;
    }

    if(motor_running)
    {
        stepper_run();

        if(step_count >= step_target)
        {
            currentState = targetState;
            motor_running = 0;
        }
    }
}
// ===== FAN FSM =====
void fan_apply(FanState s)
{
    fan_state = s;
    HAL_GPIO_WritePin(GPIOA, RELAY_PIN,
                      s==FAN_ON?GPIO_PIN_SET:GPIO_PIN_RESET);
}

void fan_fsm(uint8_t isRaining)
{
    if(fan_mode == FAN_MANUAL)
        return;   // 👉 manual thì AUTO cấm động vào

    // ===== AUTO MODE =====
    if(isRaining && !rain_prev)
    {
        fan_apply(FAN_ON);
        fan_timer = HAL_GetTick();
    }

    if(fan_state == FAN_ON)
    {
        if(HAL_GetTick() - fan_timer > 10000)
            fan_apply(FAN_OFF);
    }

    rain_prev = isRaining;
}

void update_logic(uint8_t isRaining, uint8_t isBright, float humi)
{
    if(manual_mode) return; // 🔥 quan trọng

    if(isRaining || humi > 80 || !isBright)
        targetState = STATE_THU;
    else
        targetState = STATE_PHOI;
}
// ===== BUZZER =====
void handle_buzzer(uint8_t isRaining)
{
    if(system_locked) return;

    if(isRaining && !buzzer_off)
        HAL_GPIO_WritePin(GPIOA, BUZZER_PIN, 1);
    else
        HAL_GPIO_WritePin(GPIOA, BUZZER_PIN, 0);
}

// ===== LCD =====
void update_lcd(uint8_t isRaining, uint8_t isBright)
{
    char rain_str[5];
    char light_str[5];
    char state_str[5];

    strcpy(rain_str, isRaining ? "Mua" : "Nang");
    strcpy(light_str, isBright ? "Sang" : "Toi");
    strcpy(state_str, currentState ? "Phoi" : "Thu");

    // ===== DÒNG 1 =====
    snprintf(line1, 16, "T:%d H:%d F:%d E%d",
             (int)temp,
             (int)humi,
             fan_state,
             wrong_count);

    // ===== DÒNG 2 =====
    snprintf(line2, 16, "%s|%s|%s",
             rain_str,
             light_str,
             state_str);

    // ===== UPDATE LCD =====
    if(strcmp(old_line1,line1)!=0)
    {
        lcd_put_cur(0,0);
        lcd_send_string("                ");
        lcd_put_cur(0,0);
        lcd_send_string(line1);
        strcpy(old_line1,line1);
    }

    if(strcmp(old_line2,line2)!=0)
    {
        lcd_put_cur(1,0);
        lcd_send_string("                ");
        lcd_put_cur(1,0);
        lcd_send_string(line2);
        strcpy(old_line2,line2);
    }

    // ===== HIỂN THỊ PASS ***** =====
    lcd_put_cur(1,12);
    for(int i=0;i<pass_index;i++)
        lcd_send_data('*');
}
void handle_keypad()
{
    char key = keypad_read();
    if(!key) return;

    HAL_Delay(150);

    if(system_locked)
    {
        lcd_clear();
        lcd_put_cur(0,0);
        lcd_send_string("BI KHOA!");
        return;
    }

    if(key == 'A')
    {
        targetState = STATE_THU;
    }
    else if(key == 'B')
    {
        targetState = STATE_PHOI;
    }
    // ===== QUẠT =====
    else if(key == 'C')
    {
        fan_mode = FAN_MANUAL;
        fan_apply(FAN_ON);
    }
    else if(key == 'D')
    {
    	fan_mode = FAN_MANUAL;
    	fan_apply(FAN_OFF);
    }
    else if(key >= '0' && key <= '9')
    {
        if(pass_index < 4)
            input_pass[pass_index++] = key;
        	input_pass[pass_index] = '\0';
    }
    else if(key == '*')
    {
        pass_index = 0;
        memset(input_pass,0,5);
    }
    else if(key == '#')
    {
        if(strcmp(input_pass, correct_pass) == 0)
        {
            manual_mode = 0;
            wrong_count = 0;
            fan_mode = FAN_AUTO;

            lcd_clear();
            lcd_put_cur(0,0);
            lcd_send_string("OK");
        }
        else
        {
            wrong_count++;

            if(wrong_count >= 5)
                system_locked = 1;
        }

        pass_index = 0;
        memset(input_pass,0,5);
    }
}




// ===== LOGIC =====


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  lcd_init();
  strcpy(old_line1,"");
  strcpy(old_line2,"");
  lcd_clear();

  HAL_TIM_Base_Start(&htim2);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */


  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  if(HAL_GetTick() - lastUpdate > 1000)
	      {
	          lastUpdate = HAL_GetTick();

	          if(DHT11_Read(&temp,&humi)!=0)
	          {
	              temp = 0;
	              humi = 0;
	          }

	          adcValue = read_LDR();
	          uint8_t isBright = (adcValue < LIGHT_THRESHOLD);
	          uint8_t isRaining = rain_detect();

	          update_logic(isRaining, isBright, humi);
	          fan_fsm(isRaining);
	          handle_buzzer(isRaining);
	          update_lcd(isRaining, isBright);
	      }
	  	  handle_motor();
	      handle_keypad();
  	  }
  }
  /* USER CODE END 3 */


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL8;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */
  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 71;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5
                          |GPIO_PIN_6|GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_8, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA2 PA3 PA4 PA5
                           PA6 PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5
                          |GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB10 PB11
                           PB8 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB12 PB13 PB14 PB15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
