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
// 定义 UART 接收缓存（大小可根据实际命令长度扩展）
#define UART_RX_BUF_LEN 128
uint8_t uart_rx_ch;           // 每次接收1个字节
char uart_cmd_buf[UART_RX_BUF_LEN];  // 临时拼接命令
uint8_t uart_cmd_idx = 0;
volatile uint8_t uart_cmd_ready = 0;

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


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    if (uart_rx_ch == '\n')  // 命令结束
    {
      uart_cmd_buf[uart_cmd_idx] = '\0';
      uart_cmd_ready = 1;
      uart_cmd_idx = 0;
    }
    else if (uart_cmd_idx < UART_RX_BUF_LEN - 1)
    {
      uart_cmd_buf[uart_cmd_idx++] = uart_rx_ch;
    }

    HAL_UART_Receive_IT(&huart2, &uart_rx_ch, 1);  // 继续接收
  }
}


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

  HAL_UART_Receive_IT(&huart2, &uart_rx_ch, 1);  // ✅ 接收1字节



  // 初始化方向与休眠引脚
  StepMotor_Init();
  // 使能 A、B 电机
  StepMotor_SetSleep(STEP_MOTOR_A, GPIO_PIN_SET);
  StepMotor_SetSleep(STEP_MOTOR_B, GPIO_PIN_SET);

  // 设置方向（GPIO_PIN_RESET = 顺时针）
  StepMotor_SetDir(STEP_MOTOR_A, GPIO_PIN_RESET);
  StepMotor_SetDir(STEP_MOTOR_B, GPIO_PIN_RESET);

  // 5. 设置 PWM 占空比（正式开始转动）
  StepMotor_SetDuty(STEP_MOTOR_A, 0.0f);
  StepMotor_SetDuty(STEP_MOTOR_B, 0.0f);

  // 6. 启动 PWM（TIM8）
  StepMotor_Start(STEP_MOTOR_A);
  StepMotor_Start(STEP_MOTOR_B);

  delay_ms(10);

  StepMotor_Turn(STEP_MOTOR_A, 80, 32.0f, 0, 50);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  // 默认状态
  float angle_val = 90.0;               // 默认角度
  float speed_val = 30.0;               // 默认速度RPM
  GPIO_PinState last_run = GPIO_PIN_SET;
  GPIO_PinState last_angle = GPIO_PIN_SET;
  GPIO_PinState last_speed = GPIO_PIN_SET;  // 新增速度切换按键状态
  uint8_t dir_val = 0;              // 方向（0 顺时针，1 逆时针）
  while (1)
  {
    GPIO_PinState run_now = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_14);   // 启动按键
    GPIO_PinState angle_now = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15); // 改变角度按键
    GPIO_PinState speed_now = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13); // 改变速度按键（新增）

    // PC15 从高变低 → 增加角度
    if (last_angle == GPIO_PIN_SET && angle_now == GPIO_PIN_RESET)
    {
      angle_val += 90;
      if (angle_val >= 360) angle_val = 90; // 超过360回到90
      usart_printf("角度增加，当前角度: %.1f 度\r\n", angle_val);
    }

    // PC13 从高变低 → 切换速度 (新增)
    if (last_speed == GPIO_PIN_SET && speed_now == GPIO_PIN_RESET)
    {
      // 速度循环：30 -> 60 -> 100 -> 30
      if (speed_val == 30.0f) {
        speed_val = 60.0f;
      } else if (speed_val == 60.0f) {
        speed_val = 100.0f;
      } else {
        speed_val = 30.0f;
      }
      usart_printf("速度切换，当前速度: %.1f RPM\r\n", speed_val);
    }

    // PC14 从高变低 → 同时启动 A 和 B 电机旋转
    if (last_run == GPIO_PIN_SET && run_now == GPIO_PIN_RESET)
    {
      usart_printf("执行 A/B 电机旋转 %.1f 度，方向: %s，速度: %.1f RPM\r\n",
                   angle_val, dir_val ? "逆时针" : "顺时针", speed_val);
      StepMotor_Turn(STEP_MOTOR_A, angle_val, 32.0f, dir_val, speed_val);
      StepMotor_Turn(STEP_MOTOR_B, angle_val, 32.0f, dir_val, speed_val);
    }

    last_run = run_now;
    last_angle = angle_now;
    last_speed = speed_now;  // 更新速度按键状态

    if (uart_cmd_ready)
    {
      uart_cmd_ready = 0;
      usart_printf("收到: %s\r\n", uart_cmd_buf);  // ✅ 打印拼接完成的命令

      // 🔥 解析命令并支持方向控制
      if (strncmp(uart_cmd_buf, "STOP", 4) == 0) {
        StepMotor_ForceStop(STEP_MOTOR_A);
        StepMotor_ForceStop(STEP_MOTOR_B);
        usart_printf("所有电机已停止\r\n");
      }
      // TURN <angle> [direction] [speed] - 转动指定角度
      // 例：TURN 90     （90度，默认顺时针，30RPM）
      // 例：TURN 180 1  （180度，逆时针，30RPM）
      // 例：TURN 45 0 60（45度，顺时针，60RPM）
      else if (strncmp(uart_cmd_buf, "TURN", 4) == 0) {
        float angle = 90.0f;
        uint8_t direction = 0;  // 默认顺时针
        float speed = 30.0f;    // 默认30RPM

        // 解析参数
        char* token = strtok(uart_cmd_buf + 5, " ");  // 跳过"TURN "
        if (token != NULL) {
          angle = atof(token);

          token = strtok(NULL, " ");  // 获取方向参数
          if (token != NULL) {
            direction = (uint8_t)atoi(token);

            token = strtok(NULL, " ");  // 获取速度参数
            if (token != NULL) {
              speed = atof(token);
            } else {
              speed = speed_val;  // 使用默认速度
            }
          } else {
            speed = speed_val;  // 使用默认速度
          }
        }

        // 限制参数范围
        if (angle <= 0) angle = 90.0f;
        if (angle > 3600) angle = 3600.0f;  // 最大10圈
        if (direction > 1) direction = 0;
        if (speed <= 0) speed = speed_val;  // 使用默认速度
        if (speed > 200) speed = 200.0f;    // 最大200RPM

        usart_printf("执行命令：角度=%.1f° 方向=%s 速度=%.1fRPM\r\n",
                     angle, direction ? "逆时针" : "顺时针", speed);
        StepMotor_Turn(STEP_MOTOR_A, angle, 32.0f, direction, speed);
      }
      // TURNB <angle> [direction] [speed] - 只转动B电机
      else if (strncmp(uart_cmd_buf, "TURNB", 5) == 0) {
        float angle = 90.0f;
        uint8_t direction = 0;
        float speed = 30.0f;

        char* token = strtok(uart_cmd_buf + 6, " ");
        if (token != NULL) {
          angle = atof(token);

          token = strtok(NULL, " ");
          if (token != NULL) {
            direction = (uint8_t)atoi(token);

            token = strtok(NULL, " ");
            if (token != NULL) {
              speed = atof(token);
            } else {
              speed = speed_val;  // 使用默认速度
            }
          } else {
            speed = speed_val;  // 使用默认速度
          }
        }

        if (angle <= 0) angle = 90.0f;
        if (angle > 3600) angle = 3600.0f;
        if (direction > 1) direction = 0;
        if (speed <= 0) speed = speed_val;
        if (speed > 200) speed = 200.0f;

        usart_printf("B电机：角度=%.1f° 方向=%s 速度=%.1fRPM\r\n",
                     angle, direction ? "逆时针" : "顺时针", speed);
        StepMotor_Turn(STEP_MOTOR_B, angle, 32.0f, direction, speed);
      }
      // BOTH <angle> [direction] [speed] - 同时转动AB电机
      else if (strncmp(uart_cmd_buf, "BOTH", 4) == 0) {
        float angle = 90.0f;
        uint8_t direction = 0;
        float speed = 30.0f;

        char* token = strtok(uart_cmd_buf + 5, " ");
        if (token != NULL) {
          angle = atof(token);

          token = strtok(NULL, " ");
          if (token != NULL) {
            direction = (uint8_t)atoi(token);

            token = strtok(NULL, " ");
            if (token != NULL) {
              speed = atof(token);
            } else {
              speed = speed_val;  // 使用默认速度
            }
          } else {
            speed = speed_val;  // 使用默认速度
          }
        }

        if (angle <= 0) angle = 90.0f;
        if (angle > 3600) angle = 3600.0f;
        if (direction > 1) direction = 0;
        if (speed <= 0) speed = speed_val;
        if (speed > 200) speed = 200.0f;

        usart_printf("AB电机：角度=%.1f° 方向=%s 速度=%.1fRPM\r\n",
                     angle, direction ? "逆时针" : "顺时针", speed);
        StepMotor_Turn(STEP_MOTOR_A, angle, 32.0f, direction, speed);
        StepMotor_Turn(STEP_MOTOR_B, angle, 32.0f, direction, speed);
      }
      // SPEED <rpm> - 设置默认速度
      else if (strncmp(uart_cmd_buf, "SPEED", 5) == 0) {
        if (strlen(uart_cmd_buf) > 6) {
          float new_speed = atof(&uart_cmd_buf[6]);
          if (new_speed > 0 && new_speed <= 200) {
            speed_val = new_speed;
            usart_printf("默认速度已改为：%.1f RPM\r\n", speed_val);
          } else {
            usart_printf("速度参数错误，请使用1-200之间的数值\r\n");
          }
        } else {
          usart_printf("当前默认速度：%.1f RPM\r\n", speed_val);
        }
      }
      // STATUS - 显示当前设置状态
      else if (strncmp(uart_cmd_buf, "STATUS", 6) == 0) {
        usart_printf("=== 当前设置状态 ===\r\n");
        usart_printf("默认角度：%.1f 度\r\n", angle_val);
        usart_printf("默认方向：%s\r\n", dir_val ? "逆时针" : "顺时针");
        usart_printf("默认速度：%.1f RPM\r\n", speed_val);
        usart_printf("==================\r\n");
      }
      // HELP - 显示帮助信息
      else if (strncmp(uart_cmd_buf, "HELP", 4) == 0) {
        usart_printf("=== 电机控制命令 ===\r\n");
        usart_printf("STOP                    - 停止所有电机\r\n");
        usart_printf("TURN <角度> [方向] [速度] - A电机转动\r\n");
        usart_printf("TURNB <角度> [方向] [速度]- B电机转动\r\n");
        usart_printf("BOTH <角度> [方向] [速度] - AB电机同时转动\r\n");
        usart_printf("DIR <0/1>               - 设置默认方向\r\n");
        usart_printf("SPEED <rpm>             - 设置默认速度\r\n");
        usart_printf("STATUS                  - 显示当前设置\r\n");
        usart_printf("HELP                    - 显示此帮助\r\n");
        usart_printf("按键说明：\r\n");
        usart_printf("  PC15: 切换角度 (90°->180°->270°->90°)\r\n");
        usart_printf("  PC13: 切换速度 (30->60->100->30 RPM)\r\n");
        usart_printf("  PC14: 执行运动 (使用当前设置)\r\n");
        usart_printf("参数说明：\r\n");
        usart_printf("  角度: 0.1-3600度\r\n");
        usart_printf("  方向: 0=顺时针, 1=逆时针\r\n");
        usart_printf("  速度: 1-200 RPM\r\n");
        usart_printf("示例：\r\n");
        usart_printf("  TURN 90      (90度顺时针当前速度)\r\n");
        usart_printf("  TURN 180 1   (180度逆时针当前速度)\r\n");
        usart_printf("  BOTH 45 0 60 (45度顺时针60RPM)\r\n");
        usart_printf("  SPEED 80     (设置默认速度为80RPM)\r\n");
        usart_printf("  STATUS       (查看当前设置)\r\n");
        usart_printf("================\r\n");
      }
      else {
        usart_printf("未知命令，请输入HELP查看帮助\r\n");
      }
    }

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
