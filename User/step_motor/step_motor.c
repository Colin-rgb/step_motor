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

void StepMotor_Turn(StepMotorId motor, float angle, float subdivide, uint8_t dir, float rpm_max)
{
    if (angle <= 0.0f || subdivide <= 0.0f || rpm_max <= 0.0f) return;

    StepMotorRamp_t* ramp = (motor == STEP_MOTOR_A) ? &motor_ramp_A : &motor_ramp_B;
    memset(ramp, 0, sizeof(StepMotorRamp_t));
    ramp->motor = motor;

    float step_angle = 1.8f / subdivide;
    ramp->total_steps = (uint32_t)(angle / step_angle + 0.5f);
    if (ramp->total_steps == 0) return;

    float steps_per_rev = 200.0f * subdivide;
    float freq_max = (rpm_max * steps_per_rev) / 60.0f;
    ramp->min_arr = (uint32_t)(72000000.0f / freq_max);
    if (ramp->min_arr > 0xFFFF) ramp->min_arr = 0xFFFF;

    ramp->curr_arr = (uint32_t)(72000000.0f / (freq_max / 10.0f));
    if (ramp->curr_arr > 0xFFFF) ramp->curr_arr = 0xFFFF;

    // ✅ 正确计算 decel 步数
    ramp->decel_start = ramp->total_steps * 6 / 10;
    ramp->decel_val = ramp->total_steps - ramp->decel_start;

    ramp->run_state = ACCEL;
    ramp->accel_count = 0;
    ramp->subdivide = subdivide;

    StepMotor_SetDir(motor, dir ? GPIO_PIN_SET : GPIO_PIN_RESET);
    StepMotor_SetSleep(motor, GPIO_PIN_SET);

    TIM_HandleTypeDef* pwm_htim = StepMotor_GetPWMHandle(motor);
    __HAL_TIM_SET_PRESCALER(pwm_htim, 0);
    __HAL_TIM_SET_AUTORELOAD(pwm_htim, ramp->curr_arr);
    StepMotor_SetDuty(motor, 50.0f);
    StepMotor_Start(motor);

    TIM_HandleTypeDef* cnt_htim = StepMotor_GetCounterHandle(motor);
    __HAL_TIM_SET_COUNTER(cnt_htim, 0);
    // __HAL_TIM_SET_AUTORELOAD(cnt_htim, ramp->total_steps - 1);
    __HAL_TIM_SET_AUTORELOAD(cnt_htim, ramp->total_steps);
    __HAL_TIM_CLEAR_FLAG(cnt_htim, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start_IT(cnt_htim);
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    StepMotorId motor;
    StepMotorRamp_t* ramp;

    // 匹配 PWM 通道对应的电机
    if (htim == &htim8) {
        motor = STEP_MOTOR_A;
        ramp = &motor_ramp_A;
    } else if (htim == &htim1) {
        motor = STEP_MOTOR_B;
        ramp = &motor_ramp_B;
    } else {
        return; // 非法中断源
    }

    // 如果已经停止，不再处理
    if (ramp->run_state == STOP) return;

    // 步数计数
    ramp->step_count++;

    // ✅ 达到总步数，停止电机
    if (ramp->step_count >= ramp->total_steps) {
        StepMotor_Stop(motor);
        ramp->run_state = STOP;

        float actual_angle = ramp->step_count * (1.8f / ramp->subdivide);
        usart_printf("%c 电机完成（减速），角度: %.2f°\r\n",
                     (motor == STEP_MOTOR_A) ? 'A' : 'B',
                     actual_angle);
        return;
    }

    // 状态机更新
    switch (ramp->run_state) {
        case ACCEL:
            ramp->accel_count++;
            if (ramp->accel_count >= ramp->decel_start) {
                ramp->run_state = DECEL;
            } else {
                if (ramp->curr_arr > ramp->min_arr) {
                    ramp->curr_arr--;  // 加速阶段逐步提高频率
                }
            }
            break;

        case DECEL:
            ramp->accel_count++;
            ramp->curr_arr++;  // 减速阶段逐步减低频率
            break;

        case STOP:
        default:
            return;
    }

    // ✅ 实时更新 ARR（自动重装载周期）来调整 PWM 频率
    TIM_HandleTypeDef* pwm_htim = StepMotor_GetPWMHandle(motor);
    __HAL_TIM_SET_AUTORELOAD(pwm_htim, ramp->curr_arr);
}

