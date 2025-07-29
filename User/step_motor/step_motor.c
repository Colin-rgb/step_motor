#include "step_motor.h"
#include "tim.h"
#include "gpio.h"
#include "usart_user.h"

// 内部函数：获取对应 PWM 通道
static uint32_t StepMotor_GetChannel(StepMotorId motor)
{
    return (motor == STEP_MOTOR_A) ? TIM_CHANNEL_3 : TIM_CHANNEL_4;
}

// 获取 PWM 用的 TIM 句柄
static TIM_HandleTypeDef* StepMotor_GetPWMHandle(StepMotorId motor)
{
    return (motor == STEP_MOTOR_A) ? &htim8 : &htim1;
}

// 获取 角度计数用的 TIM 句柄
static TIM_HandleTypeDef* StepMotor_GetCounterHandle(StepMotorId motor)
{
    return (motor == STEP_MOTOR_A) ? &htim2 : &htim3;
}

// 初始化方向与使能引脚
void StepMotor_Init(void)
{
    HAL_GPIO_WritePin(A_DIR_GPIO_Port, A_DIR_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(B_DIR_GPIO_Port, B_DIR_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(A_SLEEP_GPIO_Port, A_SLEEP_Pin, GPIO_PIN_SET); // 默认休眠
    HAL_GPIO_WritePin(B_SLEEP_GPIO_Port, B_SLEEP_Pin, GPIO_PIN_SET);
}

// 设置方向：0=顺时针，1=逆时针
void StepMotor_SetDir(StepMotorId motor, GPIO_PinState dir)
{
    if (motor == STEP_MOTOR_A)
        HAL_GPIO_WritePin(A_DIR_GPIO_Port, A_DIR_Pin, dir);
    else
        HAL_GPIO_WritePin(B_DIR_GPIO_Port, B_DIR_Pin, dir);
}

// 设置休眠（EN）
void StepMotor_SetSleep(StepMotorId motor, GPIO_PinState state)
{
    if (motor == STEP_MOTOR_A)
        HAL_GPIO_WritePin(A_SLEEP_GPIO_Port, A_SLEEP_Pin, state);
    else
        HAL_GPIO_WritePin(B_SLEEP_GPIO_Port, B_SLEEP_Pin, state);
}

// 设置占空比
void StepMotor_SetDuty(StepMotorId motor, float percent)
{
    TIM_HandleTypeDef* pwm_htim = StepMotor_GetPWMHandle(motor);
    uint32_t channel = StepMotor_GetChannel(motor);
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(pwm_htim);
    uint16_t pulse = (uint16_t)(percent / 100.0f * arr);
    __HAL_TIM_SET_COMPARE(pwm_htim, channel, pulse);
}

// 启动 PWM
void StepMotor_Start(StepMotorId motor)
{
    HAL_TIM_PWM_Start(StepMotor_GetPWMHandle(motor), StepMotor_GetChannel(motor));
}

// 停止 PWM
void StepMotor_Stop(StepMotorId motor)
{
    HAL_TIM_PWM_Stop(StepMotor_GetPWMHandle(motor), StepMotor_GetChannel(motor));
}

// 步进角度控制函数：主从定时器自动计步
void StepMotor_Turn(StepMotorId motor, float angle, float subdivide, uint8_t dir, float rpm)
{
    float step_angle = 1.8f / subdivide;
    uint32_t total_steps = (uint32_t)(angle / step_angle);

    // 设置方向和唤醒
    StepMotor_SetDir(motor, dir ? GPIO_PIN_SET : GPIO_PIN_RESET);
    StepMotor_SetSleep(motor, GPIO_PIN_SET);  // 唤醒

    // 设置 PWM（ARR）频率
    float steps_per_rev = 200.0f * subdivide;
    float freq = (rpm * steps_per_rev) / 60.0f;
    uint32_t arr = (uint32_t)(72000000.0f / freq);
    if (arr > 0xFFFF) arr = 0xFFFF;

    TIM_HandleTypeDef* pwm_htim = StepMotor_GetPWMHandle(motor);
    __HAL_TIM_SET_PRESCALER(pwm_htim, 0);
    __HAL_TIM_SET_AUTORELOAD(pwm_htim, arr);
    StepMotor_SetDuty(motor, 50.0f);

    // 配置从定时器（TIM2 或 TIM3）
    TIM_HandleTypeDef* cnt_htim = StepMotor_GetCounterHandle(motor);
    HAL_TIM_Base_Stop_IT(cnt_htim);  // 先关闭防止误中断
    __HAL_TIM_SET_COUNTER(cnt_htim, 0);
    __HAL_TIM_SET_AUTORELOAD(cnt_htim, total_steps - 1);
    __HAL_TIM_CLEAR_FLAG(cnt_htim, TIM_FLAG_UPDATE);

    // 启动 PWM 与从定时器
    StepMotor_Start(motor);
    HAL_TIM_Base_Start_IT(cnt_htim);

    usart_printf("motor=%d, ARR=%lu, total_steps=%lu, CCR=%lu\n", motor, arr, total_steps, (uint32_t)(arr * 0.5f));
}
