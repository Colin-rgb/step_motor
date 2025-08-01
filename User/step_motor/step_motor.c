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


// void StepMotor_Turn(StepMotorId motor, float angle, float subdivide, uint8_t dir, float rpm)
// {
//     // 1. 参数检查
//     if (angle <= 0.0f || subdivide <= 0.0f || rpm <= 0.0f) return;
//
//     float step_angle = 1.8f / subdivide;
//     uint32_t total_steps = (uint32_t)(angle / step_angle + 0.5f);  // 四舍五入避免精度丢失
//     if (total_steps == 0) return;  // 防止启动0步
//
//     // 2. 设置方向和唤醒
//     StepMotor_SetDir(motor, dir ? GPIO_PIN_SET : GPIO_PIN_RESET);
//     StepMotor_SetSleep(motor, GPIO_PIN_SET);  // 唤醒电机驱动器
//
//     // 3. 配置 PWM 定时器
//     float steps_per_rev = 200.0f * subdivide;
//     float freq = (rpm * steps_per_rev) / 60.0f;
//     uint32_t arr = (uint32_t)(72000000.0f / freq);
//     if (arr > 0xFFFF) arr = 0xFFFF;
//
//     TIM_HandleTypeDef* pwm_htim = StepMotor_GetPWMHandle(motor);
//     __HAL_TIM_SET_PRESCALER(pwm_htim, 0);
//     __HAL_TIM_SET_AUTORELOAD(pwm_htim, arr);
//     StepMotor_SetDuty(motor, 50.0f);  // 固定占空比 50%
//
//     // 4. 配置从定时器（计步）
//     TIM_HandleTypeDef* cnt_htim = StepMotor_GetCounterHandle(motor);
//     HAL_TIM_Base_Stop_IT(cnt_htim);  // 防止残余中断
//     __HAL_TIM_SET_COUNTER(cnt_htim, 0);
//     __HAL_TIM_SET_AUTORELOAD(cnt_htim, total_steps - 1);  // 注意是 N-1
//     __HAL_TIM_CLEAR_FLAG(cnt_htim, TIM_FLAG_UPDATE);
//
//     // 5. 启动从定时器 → 再启动主PWM（确保从定时器先就绪）
//     HAL_TIM_Base_Start_IT(cnt_htim);     // ✅ 必须先启动它
//     StepMotor_Start(motor);              // ✅ 再启动 PWM
// }

StepMotorRamp_t motor_ramp_A;
StepMotorRamp_t motor_ramp_B;

// 启动带加减速的步进控制
void StepMotor_Turn(StepMotorId motor, float angle, float subdivide, uint8_t dir, float rpm_max)
{
    if (angle <= 0.0f || subdivide <= 0.0f || rpm_max <= 0.0f) return;

    StepMotorRamp_t* ramp = (motor == STEP_MOTOR_A) ? &motor_ramp_A : &motor_ramp_B;
    memset(ramp, 0, sizeof(StepMotorRamp_t));
    ramp->motor = motor;

    // 步数与参数
    float step_angle = 1.8f / subdivide;
    ramp->total_steps = (uint32_t)(angle / step_angle + 0.5f);
    if (ramp->total_steps == 0) return;

    float steps_per_rev = 200.0f * subdivide;
    float freq_max = (rpm_max * steps_per_rev) / 60.0f;
    ramp->min_arr = (uint32_t)(72000000.0f / freq_max);
    if (ramp->min_arr > 0xFFFF) ramp->min_arr = 0xFFFF;

    // 初始 delay (相当于 c0)，设一个较大的起步周期（慢起步）
    ramp->curr_arr = (uint32_t)(72000000.0f / (freq_max / 10.0f)); // 比 max freq 小10倍
    if (ramp->curr_arr > 0xFFFF) ramp->curr_arr = 0xFFFF;

    // 减速阶段提前步数估计
    ramp->decel_start = ramp->total_steps * 6 / 10;  // 可调
    ramp->decel_val = ramp->decel_start - ramp->total_steps;
    ramp->run_state = ACCEL;
    ramp->accel_count = 0;

    // 设置方向和唤醒
    StepMotor_SetDir(motor, dir ? GPIO_PIN_SET : GPIO_PIN_RESET);
    StepMotor_SetSleep(motor, GPIO_PIN_SET);

    // 设置 PWM 频率与占空比
    TIM_HandleTypeDef* pwm_htim = StepMotor_GetPWMHandle(motor);
    __HAL_TIM_SET_PRESCALER(pwm_htim, 0);
    __HAL_TIM_SET_AUTORELOAD(pwm_htim, ramp->curr_arr);
    StepMotor_SetDuty(motor, 50.0f);
    StepMotor_Start(motor);

    // 启动计步定时器
    TIM_HandleTypeDef* cnt_htim = StepMotor_GetCounterHandle(motor);
    __HAL_TIM_SET_COUNTER(cnt_htim, 0);
    __HAL_TIM_SET_AUTORELOAD(cnt_htim, ramp->total_steps - 1);
    __HAL_TIM_CLEAR_FLAG(cnt_htim, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start_IT(cnt_htim);
}

// void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
// {
//     StepMotorRamp_t* ramp = NULL;
//
//     if (htim == &htim8) ramp = &motor_ramp_A;
//     else if (htim == &htim1) ramp = &motor_ramp_B;
//     else return;
//
//     if (ramp->run_state == STOP) return;
//
//     ramp->step_count++;
//     ramp->accel_count++;
//
//     int new_arr;
//     switch (ramp->run_state)
//     {
//         case ACCEL:
//             new_arr = ramp->curr_arr - ((2 * ramp->curr_arr + ramp->rest) / (4 * ramp->accel_count + 1));
//             ramp->rest = (2 * ramp->curr_arr + ramp->rest) % (4 * ramp->accel_count + 1);
//             if (ramp->step_count >= ramp->decel_start)
//             {
//                 ramp->accel_count = ramp->decel_val;
//                 ramp->run_state = DECEL;
//             }
//             else if (new_arr <= ramp->min_arr)
//             {
//                 new_arr = ramp->min_arr;
//                 ramp->rest = 0;
//                 ramp->run_state = RUN;
//             }
//             break;
//
//         case RUN:
//             new_arr = ramp->min_arr;
//             if (ramp->step_count >= ramp->decel_start)
//             {
//                 ramp->accel_count = ramp->decel_val;
//                 new_arr = ramp->curr_arr;
//                 ramp->run_state = DECEL;
//             }
//             break;
//
//         case DECEL:
//             ramp->accel_count++;
//             new_arr = ramp->curr_arr - ((2 * ramp->curr_arr + ramp->rest) / (4 * ramp->accel_count + 1));
//             ramp->rest = (2 * ramp->curr_arr + ramp->rest) % (4 * ramp->accel_count + 1);
//             if (ramp->accel_count >= 0)
//                 ramp->run_state = STOP;
//             break;
//
//         default:
//             new_arr = ramp->curr_arr;
//             break;
//     }
//
//     ramp->curr_arr = new_arr;
//     __HAL_TIM_SET_AUTORELOAD(htim, new_arr);
//
//     // 调试代码
//     // usart_printf("PWM Pulse Callback hit!\r\n");
// }

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    StepMotorRamp_t* ramp = NULL;

    if (htim == &htim8)       ramp = &motor_ramp_A;
    else if (htim == &htim1)  ramp = &motor_ramp_B;
    else return;

    if (ramp->run_state == STOP) return;

    ramp->step_count++;
    ramp->accel_count++;

    int new_arr;
    switch (ramp->run_state)
    {
        case ACCEL:
            new_arr = ramp->curr_arr - ((2 * ramp->curr_arr + ramp->rest) / (4 * ramp->accel_count + 1));
            ramp->rest = (2 * ramp->curr_arr + ramp->rest) % (4 * ramp->accel_count + 1);

            if (ramp->step_count >= ramp->decel_start)
            {
                ramp->accel_count = ramp->decel_val;
                ramp->run_state = DECEL;
            }
            else if (new_arr <= ramp->min_arr)
            {
                new_arr = ramp->min_arr;
                ramp->rest = 0;
                ramp->run_state = RUN;
            }
            break;

        case RUN:
            new_arr = ramp->min_arr;
            if (ramp->step_count >= ramp->decel_start)
            {
                ramp->accel_count = ramp->decel_val;
                new_arr = ramp->curr_arr;
                ramp->run_state = DECEL;
            }
            break;

        case DECEL:
            ramp->accel_count++;
            new_arr = ramp->curr_arr - ((2 * ramp->curr_arr + ramp->rest) / (4 * ramp->accel_count + 1));
            ramp->rest = (2 * ramp->curr_arr + ramp->rest) % (4 * ramp->accel_count + 1);

            if (ramp->accel_count >= 0)
            {
                ramp->run_state = STOP;

                // ✅ 停止 PWM，但不 sleep，防止抖动与响声
                StepMotor_SetDuty(ramp->motor, 0);         // 占空比清 0，立即无声
                StepMotor_Stop(ramp->motor);               // 停止 PWM 输出
                // StepMotor_SetSleep(ramp->motor, GPIO_PIN_RESET);  // 不 sleep，可选加参数控制
                return;
            }
            break;

        default:
            new_arr = ramp->curr_arr;
            break;
    }

    ramp->curr_arr = new_arr;
    __HAL_TIM_SET_AUTORELOAD(htim, new_arr);
}
