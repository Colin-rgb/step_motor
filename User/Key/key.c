#include "key.h"
#include "gpio.h"
#include "step_motor.h"
#include "stdio.h"

void Key_Init(void)
{
    // 配置 EXTI 中断优先级（如果未在 CubeMX 设置）
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

// // 按键中断触发：切换步进电机启停
// void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
// {
//     if (GPIO_Pin == KEY_motor_Pin)  // 确保是你的按键引脚
//     {
//         static uint8_t state = 0;
//         if (state == 0)
//         {
//             // 电机 A、B 启动
//             StepMotor_SetSleep(STEP_MOTOR_A, GPIO_PIN_RESET);
//             StepMotor_SetSleep(STEP_MOTOR_B, GPIO_PIN_SET);
//
//             StepMotor_SetDir(STEP_MOTOR_A, GPIO_PIN_RESET);  // 顺时针
//             StepMotor_SetDir(STEP_MOTOR_B, GPIO_PIN_RESET);
//
//             StepMotor_SetDuty(STEP_MOTOR_A, 50.0f);
//             StepMotor_SetDuty(STEP_MOTOR_B, 50.0f);
//
//             StepMotor_Start(STEP_MOTOR_A);
//             StepMotor_Start(STEP_MOTOR_B);
//
//             printf("步进电机 A/B 启动\n");
//         }
//         else
//         {
//             // 电机 A、B 停止
//             StepMotor_Stop(STEP_MOTOR_A);
//             StepMotor_Stop(STEP_MOTOR_B);
//
//             StepMotor_SetSleep(STEP_MOTOR_A, GPIO_PIN_RESET);  // 睡眠
//             StepMotor_SetSleep(STEP_MOTOR_B, GPIO_PIN_RESET);
//
//             printf("步进电机 A/B 停止\n");
//         }
//
//         state = !state;  // 状态翻转
//     }
// }


// ✅ 按键单击检测（用于 main() 轮询）
uint8_t click(void)
{
    static uint8_t flag_key = 1;
    if (flag_key && KEY == 0)
    {
        flag_key = 0;
        return 1;
    }
    else if (KEY == 1)
        flag_key = 1;

    return 0;
}

uint8_t click_N_Double(uint8_t time)
{
    static uint8_t flag_key, count_key, double_key;
    static uint16_t count_single, Forever_count;

    if (KEY == 0) Forever_count++;
    else          Forever_count = 0;

    if (KEY == 0 && flag_key == 0) flag_key = 1;

    if (count_key == 0)
    {
        if (flag_key == 1)
        {
            double_key++;
            count_key = 1;
        }

        if (double_key == 2)
        {
            double_key = 0;
            count_single = 0;
            return 2;  // 双击
        }
    }

    if (KEY == 1)
    {
        flag_key = 0;
        count_key = 0;
    }

    if (double_key == 1)
    {
        count_single++;
        if (count_single > time && Forever_count < time)
        {
            double_key = 0;
            count_single = 0;
            return 1;  // 单击
        }
        if (Forever_count > time)
        {
            double_key = 0;
            count_single = 0;
        }
    }
    return 0;
}

uint8_t Long_Press(void)
{
    static uint16_t Long_Press_count = 0;
    static uint8_t Long_Press_flag = 0;

    if (Long_Press_flag == 0 && KEY == 0)
        Long_Press_count++;
    else
        Long_Press_count = 0;

    if (Long_Press_count > 200)
    {
        Long_Press_flag = 1;
        Long_Press_count = 0;
        return 1;
    }

    if (Long_Press_flag == 1)
        Long_Press_flag = 0;

    return 0;
}


// // 单击检测
// uint8_t click(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
// {
//     static uint8_t flag_key1 = 1, flag_key2 = 1;
//
//     uint8_t* flag_key = (GPIO_Pin == GPIO_PIN_13) ? &flag_key1 : &flag_key2;
//
//     if (*flag_key && HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_RESET)
//     {
//         *flag_key = 0;
//         return 1;
//     }
//     else if (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_SET)
//     {
//         *flag_key = 1;
//     }
//
//     return 0;
// }
//
// // 单/双击检测
// uint8_t click_N_Double(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint16_t time)
// {
//     static uint8_t flag_key1 = 0, flag_key2 = 0;
//     static uint8_t count_key1 = 0, count_key2 = 0;
//     static uint8_t double_key1 = 0, double_key2 = 0;
//     static uint16_t count_single1 = 0, count_single2 = 0;
//     static uint16_t Forever_count1 = 0, Forever_count2 = 0;
//
//     uint8_t* flag_key        = (GPIO_Pin == GPIO_PIN_13) ? &flag_key1 : &flag_key2;
//     uint8_t* count_key       = (GPIO_Pin == GPIO_PIN_13) ? &count_key1 : &count_key2;
//     uint8_t* double_key      = (GPIO_Pin == GPIO_PIN_13) ? &double_key1 : &double_key2;
//     uint16_t* count_single   = (GPIO_Pin == GPIO_PIN_13) ? &count_single1 : &count_single2;
//     uint16_t* Forever_count  = (GPIO_Pin == GPIO_PIN_13) ? &Forever_count1 : &Forever_count2;
//
//     GPIO_PinState state = HAL_GPIO_ReadPin(GPIOx, GPIO_Pin);
//
//     if (state == GPIO_PIN_RESET) (*Forever_count)++;
//     else                         *Forever_count = 0;
//
//     if (state == GPIO_PIN_RESET && *flag_key == 0) *flag_key = 1;
//
//     if (*count_key == 0)
//     {
//         if (*flag_key == 1)
//         {
//             (*double_key)++;
//             *count_key = 1;
//         }
//
//         if (*double_key == 2)
//         {
//             *double_key = 0;
//             *count_single = 0;
//             return 2;  // 双击
//         }
//     }
//
//     if (state == GPIO_PIN_SET)
//     {
//         *flag_key = 0;
//         *count_key = 0;
//     }
//
//     if (*double_key == 1)
//     {
//         (*count_single)++;
//         if (*count_single > time && *Forever_count < time)
//         {
//             *double_key = 0;
//             *count_single = 0;
//             return 1;  // 单击
//         }
//         if (*Forever_count > time)
//         {
//             *double_key = 0;
//             *count_single = 0;
//         }
//     }
//     return 0;
// }
//
// // 长按检测
// uint8_t Long_Press(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint16_t press_time)
// {
//     static uint16_t Long_Press_count1 = 0, Long_Press_count2 = 0;
//     static uint8_t Long_Press_flag1 = 0, Long_Press_flag2 = 0;
//
//     uint16_t* count = (GPIO_Pin == GPIO_PIN_13) ? &Long_Press_count1 : &Long_Press_count2;
//     uint8_t*  flag  = (GPIO_Pin == GPIO_PIN_13) ? &Long_Press_flag1  : &Long_Press_flag2;
//
//     if (*flag == 0 && HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_RESET)
//         (*count)++;
//     else
//         *count = 0;
//
//     if (*count > press_time)
//     {
//         *flag = 1;
//         *count = 0;
//         return 1;
//     }
//
//     if (*flag == 1)
//         *flag = 0;
//
//     return 0;
// }