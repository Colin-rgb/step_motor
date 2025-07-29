#include "step_motor.h"
#include "tim.h"
#include "gpio.h"

// 内部函数：获取对应通道，A电机为TIM_CHN_3，B电机为TIM_CHN_4
// ********** 云台下面是B电机，上面是A电机 ************ //
static uint32_t StepMotor_GetChannel(StepMotorId motor)
{
    return (motor == STEP_MOTOR_A) ? TIM_CHANNEL_3 : TIM_CHANNEL_4;
}

// 初始化 GPIO：方向和 SLEEP 引脚
void StepMotor_Init(void)
{
    // DIR 设为默认方向（低）
    HAL_GPIO_WritePin(A_DIR_GPIO_Port, A_DIR_Pin, GPIO_PIN_RESET); // A_DIR
    HAL_GPIO_WritePin(B_DIR_GPIO_Port, B_DIR_Pin,  GPIO_PIN_RESET); // B_DIR

    // SLEEP 设为高电平 → 休眠状态
    HAL_GPIO_WritePin(A_SLEEP_GPIO_Port, A_SLEEP_Pin, GPIO_PIN_SET);   // A_SLEEP
    HAL_GPIO_WritePin(B_SLEEP_GPIO_Port, B_SLEEP_Pin, GPIO_PIN_SET);   // B_SLEEP
}

// 设置方向：0为顺时针，1为逆时针
void StepMotor_SetDir(StepMotorId motor, GPIO_PinState dir)
{
    if (motor == STEP_MOTOR_A)
        HAL_GPIO_WritePin(A_DIR_GPIO_Port, A_DIR_Pin, dir); // A_DIR
    else
        HAL_GPIO_WritePin(B_DIR_GPIO_Port, B_DIR_Pin, dir);  // B_DIR
}

// 设置使能（EN）状态：SET=使能，RESET=休眠
void StepMotor_SetSleep(StepMotorId motor, GPIO_PinState state)
{
    if (motor == STEP_MOTOR_A)
        HAL_GPIO_WritePin(A_SLEEP_GPIO_Port, A_SLEEP_Pin, state); // A_SLEEP
    else
        HAL_GPIO_WritePin(B_SLEEP_GPIO_Port, B_SLEEP_Pin, state); // B_SLEEP
}


// 启动 PWM 输出
void StepMotor_Start(StepMotorId motor)
{
    HAL_TIM_PWM_Start(&htim8, StepMotor_GetChannel(motor));
}

// 停止 PWM 输出
void StepMotor_Stop(StepMotorId motor)
{
    HAL_TIM_PWM_Stop(&htim8, StepMotor_GetChannel(motor));
}

// 设置占空比（百分比）
void StepMotor_SetDuty(StepMotorId motor, float percent)
{
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim8);
    uint16_t pulse = (uint16_t)(percent / 100.0f * arr);
    __HAL_TIM_SET_COMPARE(&htim8, StepMotor_GetChannel(motor), pulse);
}

/**
 * @brief 使用 PWM 控制步进电机旋转指定角度
 * @param motor         STEP_MOTOR_A / STEP_MOTOR_B
 * @param angle         需要旋转的角度
 * @param subdivide     细分值（如 1, 2, 4, 8...）
 * @param dir           方向：0=顺时针，1=逆时针
 * @param rpm           目标转速（RPM）
 */
void StepMotor_Turn(StepMotorId motor, float angle, float subdivide, uint8_t dir, float rpm)
{
    // 1. 设置方向
    StepMotor_SetDir(motor, dir == 0 ? GPIO_PIN_RESET : GPIO_PIN_SET);

    // 2. 设置 EN 高电平（使能）
    StepMotor_SetSleep(motor, GPIO_PIN_SET);

    // 3. 计算总步数（以1.8度为基础）
    float step_angle = 1.8f / subdivide;  // 每个子步对应的角度
    uint32_t total_steps = (uint32_t)(angle / step_angle);

    // 4. 计算频率（Hz） = RPM × 步数/圈 ÷ 60
    float steps_per_rev = 200 * subdivide;  // 每圈脉冲数
    float freq = (rpm * steps_per_rev) / 60.0f;

    // 5. 设置 PWM 频率（用ARR和PSC控制）
    // 假设使用 TIM8，APB2时钟=72MHz，预分频后 TIM 时钟为 72MHz
    // 目标 freq → ARR = 72MHz / freq
    uint32_t timer_clk = 72000000;
    uint32_t arr = (uint32_t)(timer_clk / freq);
    if (arr > 0xFFFF) arr = 0xFFFF;  // 限制最大值
    __HAL_TIM_SET_AUTORELOAD(&htim8, arr);
    __HAL_TIM_SET_PRESCALER(&htim8, 0);  // 你也可以按需设置PSC

    StepMotor_SetDuty(motor, 50.0f);  // 设置占空比50%
    StepMotor_Start(motor);          // 启动PWM

    // 6. 延时以输出 total_steps 个脉冲
    // 一个脉冲周期 = 1/freq 秒，n个脉冲时间 = total_steps / freq 秒
    float duration_ms = (float)total_steps / freq * 1000.0f;
    HAL_Delay((uint32_t)duration_ms);

    // 7. 停止
    StepMotor_Stop(motor);
    StepMotor_SetSleep(motor, GPIO_PIN_RESET);  // 关闭 EN
}

