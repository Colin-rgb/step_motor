/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "step_motor.h"
#include "delay.h"
#include "key.h"
#include "usart_user.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  delay_init();  // 初始化 DWT/SysTick 延时
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  MX_TIM8_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  // 初始化按键中断
  // Key_Init();

  // 初始化方向与休眠引脚
  StepMotor_Init();
  // 使能 A、B 电机（退出 SLEEP）
  StepMotor_SetSleep(STEP_MOTOR_A, GPIO_PIN_RESET);
  StepMotor_SetSleep(STEP_MOTOR_B, GPIO_PIN_RESET);

  // 设置方向（GPIO_PIN_RESET = 顺时针）
  StepMotor_SetDir(STEP_MOTOR_A, GPIO_PIN_RESET);
  StepMotor_SetDir(STEP_MOTOR_B, GPIO_PIN_RESET);

  // 设置占空比为 50%
  // StepMotor_SetDuty(STEP_MOTOR_A, 50.0f);  // 百分比
  // StepMotor_SetDuty(STEP_MOTOR_B, 50.0f);
  StepMotor_SetDuty(STEP_MOTOR_A, 0.0f);  // 百分比
  StepMotor_SetDuty(STEP_MOTOR_B, 0.0f);

  // 启动 PWM 输出
  StepMotor_Start(STEP_MOTOR_A);
  StepMotor_Start(STEP_MOTOR_B);
  /* USER CODE END 2 */

  /* Infinite loop */
  // 初始状态
  GPIO_PinState last_run = GPIO_PIN_SET;  // PC14上次状态（运行）
  GPIO_PinState last_angle = GPIO_PIN_SET;  // PC15上次状态（改变角度）

  int angle_val = 90;   // 初始角度
  uint8_t dir_val = 0; // 固定方向，若不需要可以扩展
  /* USER CODE BEGIN WHILE */
  // 默认状态
  while (1)
  {
    GPIO_PinState run_now = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_14);   // 启动按键
    GPIO_PinState angle_now = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15); // 改变角度按键

    // 如果 PC15 从高变低 → 增加角度
    if (last_angle == GPIO_PIN_SET && angle_now == GPIO_PIN_RESET)
    {
      angle_val += 90;
      if (angle_val >= 360) angle_val = 90; // 超过360后回到90
      usart_printf("角度增加，当前角度: %d 度\r\n", angle_val);
    }

    // 如果 PC14 从高变低 → 运行一次
    if (last_run == GPIO_PIN_SET && run_now == GPIO_PIN_RESET)
    {
      usart_printf("执行电机旋转 %d 度\r\n", angle_val);
      StepMotor_Turn(STEP_MOTOR_B, angle_val, 32.0f, dir_val, 20.0f);  // 细分32，20转/分
    }

    last_run = run_now;
    last_angle = angle_now;

    HAL_Delay(10);  // 简单防抖
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
#ifdef USE_FULL_ASSERT
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
