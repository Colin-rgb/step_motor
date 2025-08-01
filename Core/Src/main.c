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
#include "pid.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// 定义 UART 接收缓存（大小可根据实际命令长度扩展）
#define UART_RX_BUF_LEN 64
// 全局变量
uint8_t uart_cmd_buf[UART_RX_BUF_LEN] = {0};
uint16_t uart_cmd_idx = 0;
uint32_t last_receive_time = 0;

#define CMD_TIMEOUT_MS 1000  // 命令超时时间

// 命令解析函数
void parse_uart_cmd(const char* cmd)
{
  int dect_flag = 0;
  int x_err = 0, y_err = 0;

  // 添加长度检查
  if (strlen(cmd) < 7) {
    // usart_printf("命令过短: %s\r\n", cmd);
    return;
  }

  if (strncmp(cmd, "0xdect:", 7) != 0) {
    // usart_printf("无效命令头: %s\r\n", cmd);
    return;
  }
  if (sscanf(cmd, "0xdect:%d,err:%d,%d", &dect_flag, &x_err, &y_err) == 3)
  {
    // usart_printf("✅ 解析成功: dect=%d, x=%.2f, y=%.2f\r\n", dect_flag, x_err, y_err);
    StepMotor_PID_Update(x_err, y_err, dect_flag);
  }
  else
  {
    // usart_printf("❌ 解析失败: %s\r\n", cmd);
  }
}
// 清空接收缓冲区
void clear_uart_buffer(void)
{
  memset(uart_cmd_buf, 0, UART_RX_BUF_LEN);
  uart_cmd_idx = 0;
}

void loop_uart_check(void)
{
  uint8_t ch;
  uint32_t current_time = HAL_GetTick();

  // 超时检查：如果长时间没有完整命令，清空缓冲区
  if (uart_cmd_idx > 0 && (current_time - last_receive_time) > CMD_TIMEOUT_MS) {
    // usart_printf("⚠️ 命令接收超时，清空缓冲区\r\n");
    clear_uart_buffer();
  }

  while (HAL_UART_Receive(&huart2, &ch, 1, 100) == HAL_OK)  // 增加超时时间到100ms
  {
    last_receive_time = current_time;

    // 方案1：严格的命令头检测 "0xdect:"
    if (uart_cmd_idx == 0) {
      if (ch != '0') {
        continue; // 丢弃开头不是 '0' 的字符
      }
    }
    // 检查是否是完整的命令头 "0xdect:"
    else if (uart_cmd_idx < 7) {
      const char expected[] = "0xdect:";
      if (ch != expected[uart_cmd_idx]) {
        // 如果不匹配，重新开始
        clear_uart_buffer();
        if (ch == '0') {
          uart_cmd_buf[uart_cmd_idx++] = ch;
        }
        continue;
      }
    }

    // 处理结束符
    if (ch == '\n' || ch == '\r')
    {
      if (uart_cmd_idx > 0) {  // 确保有数据
        uart_cmd_buf[uart_cmd_idx] = '\0';
        // usart_printf("📨 收到命令: %s\r\n", uart_cmd_buf);
        parse_uart_cmd(uart_cmd_buf);
        clear_uart_buffer();
      }
    }
    else
    {
      // 防止缓冲区溢出
      if (uart_cmd_idx < UART_RX_BUF_LEN - 1) {
        uart_cmd_buf[uart_cmd_idx++] = ch;
      } else {
        // usart_printf("⚠️ 缓冲区溢出，清空重新开始\r\n");
        clear_uart_buffer();
      }
    }
  }
}

void MotorControl_Init(void)
{
  // 初始化 PID 参数（可自行调参）
  // PID_Init(&pid_x, 0.001f, 0.01f, 0.001f, 50.0f);
  PID_Init(&pid_x, 10.0f, 0.001f, 0.001f, 50.0f);
  PID_Init(&pid_y, 10.0f, 0.001f, 0.001f, 50.0f);
  // PID_Init(&pid_y, 1.0f, 0.01f, 0.001f, 50.0f);
}



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
  MX_TIM1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  // 初始化按键中断
  // Key_Init();

  // HAL_UART_Receive_IT(&huart2, &uart_rx_ch, 1);  // ✅ 接收1字节

  MotorControl_Init();

  // 初始化方向与休眠引脚
  StepMotor_Init();
  // 使能 A、B 电机
  // 初始化方向与休眠引脚
  StepMotor_Init();
  // 使能 A、B 电机（退出 SLEEP）
  StepMotor_SetSleep(STEP_MOTOR_A, GPIO_PIN_SET);
  StepMotor_SetSleep(STEP_MOTOR_B, GPIO_PIN_SET);

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

  delay_ms(10);

  StepMotor_Turn(STEP_MOTOR_A, 80, 32.0f, 0, 100);
  // delay_ms(500);
  // StepMotor_Turn(STEP_MOTOR_A, 0, 32.0f, 0, 50);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  // 默认状态
  while (1)
  {
    // StepMotor_Turn(STEP_MOTOR_B, 1.8, 32.0f, 0, 1);
    // usart_printf("hello");
    loop_uart_check();  // 检查是否有数据
    // HAL_Delay(100);  // 简单防抖
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
