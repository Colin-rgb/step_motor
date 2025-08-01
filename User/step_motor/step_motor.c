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

void StepMotor_ForceStop(StepMotorId motor)
{
    StepMotorRamp_t* ramp = (motor == STEP_MOTOR_A) ? &motor_ramp_A : &motor_ramp_B;

    // 停止 PWM 输出
    StepMotor_Stop(motor);
    StepMotor_SetDuty(motor, 0);  // 不释放使能，让电机保持锁定但无声

    // 停止计步定时器
    TIM_HandleTypeDef* cnt_htim = StepMotor_GetCounterHandle(motor);
    HAL_TIM_Base_Stop_IT(cnt_htim);

    // 清状态
    ramp->run_state = STOP;
    ramp->step_count = 0;
    ramp->accel_count = 0;
    ramp->curr_arr = 0;
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
// void StepMotor_Start(StepMotorId motor)
// {
//     HAL_TIM_PWM_Start(StepMotor_GetPWMHandle(motor), StepMotor_GetChannel(motor));
// }

void StepMotor_Start(StepMotorId motor)
{
    HAL_TIM_PWM_Start_IT(StepMotor_GetPWMHandle(motor), StepMotor_GetChannel(motor));
}


// 停止 PWM
void StepMotor_Stop(StepMotorId motor)
{
    HAL_TIM_PWM_Stop(StepMotor_GetPWMHandle(motor), StepMotor_GetChannel(motor));
}

StepMotorRamp_t motor_ramp_A;
StepMotorRamp_t motor_ramp_B;

// =================================================================================
//  直接替换这个函数
// =================================================================================
void StepMotor_Turn(StepMotorId motor, float angle, float subdivide, uint8_t dir, float rpm)
{
    // 参数检查 (rpm_max 现在是 target rpm)
    if (angle <= 0.0f || subdivide <= 0.0f || rpm <= 0.0f) return;

    // --- 核心修改部分 ---

    // 1. 仍然需要计算总步数以保证角度精确
    float step_angle = 1.8f / subdivide;
    uint32_t total_steps = (uint32_t)(angle / step_angle + 0.5f);
    if (total_steps == 0) return;

    // 2. 根据目标RPM，只计算一个恒定的ARR值
    float steps_per_rev = 200.0f * subdivide;
    float target_freq = (rpm * steps_per_rev) / 60.0f;
    uint32_t constant_arr = (uint32_t)(72000000.0f / target_freq); // 假设72MHz时钟

    // 检查ARR值是否在有效范围内
    if (constant_arr > 0xFFFF) constant_arr = 0xFFFF; // 防止转速过低导致溢出
    if (constant_arr < 10) constant_arr = 10;           // 限制一个最高频率，防止硬件跟不上

    // --- 移除了所有加减速相关的计算和状态设置 ---
    // StepMotorRamp_t 结构体现在只用于标记状态
    StepMotorRamp_t* ramp = (motor == STEP_MOTOR_A) ? &motor_ramp_A : &motor_ramp_B;
    ramp->run_state = RUN; // 直接设置为运行状态
    ramp->total_steps = total_steps; // 仍然保存总步数，方便调试

    // 3. 硬件设置
    StepMotor_SetDir(motor, dir ? GPIO_PIN_SET : GPIO_PIN_RESET);
    StepMotor_SetSleep(motor, GPIO_PIN_SET);

    // 4. 设置PWM定时器 (TIM8 / TIM1)
    TIM_HandleTypeDef* pwm_htim = StepMotor_GetPWMHandle(motor);
    __HAL_TIM_SET_PRESCALER(pwm_htim, 0);
    __HAL_TIM_SET_AUTORELOAD(pwm_htim, constant_arr); // 使用计算出的恒定ARR值
    StepMotor_SetDuty(motor, 50.0f);

    // 5. 设置计步定时器 (TIM2 / TIM3)
    TIM_HandleTypeDef* cnt_htim = StepMotor_GetCounterHandle(motor);
    __HAL_TIM_SET_COUNTER(cnt_htim, 0);
    // 设置计步器在 'total_steps' 个脉冲后产生中断
    // 因为计数从0到ARR，总共是ARR+1次，所以要设置为 total_steps - 1
    __HAL_TIM_SET_AUTORELOAD(cnt_htim, total_steps - 1);
    __HAL_TIM_CLEAR_FLAG(cnt_htim, TIM_FLAG_UPDATE);

    // 6. 启动电机和计步器
    StepMotor_Start(motor);             // 启动PWM脉冲输出
    HAL_TIM_Base_Start_IT(cnt_htim);    // 启动计步器中断，它只会在结束时触发一次
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    // 检查是否是A电机的计步器 TIM2
    if (htim->Instance == TIM2) {
        StepMotor_Stop(STEP_MOTOR_A);
        motor_ramp_A.run_state = STOP; // 更新状态
        // 可选: 打印信息
        usart_printf("A 电机完成, 已停止\r\n");
    }
    // 检查是否是B电机的计步器 TIM3
    if (htim->Instance == TIM3) {
        StepMotor_Stop(STEP_MOTOR_B);
        motor_ramp_B.run_state = STOP; // 更新状态
        // 可选: 打印信息
        usart_printf("B 电机完成, 已停止\r\n");
    }
}

