#include "pid.h"
#include "step_motor.h"
#include <math.h>
#include <stdint.h>

#define DEAD_ZONE         1.0f     // 死区阈值，偏差小于此值不移动
#define LOST_TIMEOUT_MS   300      // 丢失目标的最大持续时间（单位：ms）

PID_t pid_x, pid_y;

float last_err_x = 0.0f, last_err_y = 0.0f;
uint32_t last_valid_time = 0;  // 单位：ms（由 HAL_GetTick() 提供）

// 初始化 PID
void PID_Init(PID_t* pid, float kp, float ki, float kd, float imax)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->imax = imax;
    pid->integral = 0;
    pid->last_error = 0;
}

float PID_Compute(PID_t* pid, float error)
{
    pid->integral += error;
    if (pid->integral > pid->imax) pid->integral = pid->imax;
    if (pid->integral < -pid->imax) pid->integral = -pid->imax;

    float derivative = error - pid->last_error;
    pid->last_error = error;

    return pid->kp * error + pid->ki * pid->integral + pid->kd * derivative;
}

// 每帧调用该函数：err_x/err_y 为识别得到的偏差；valid=1 表示识别成功
void StepMotor_PID_Update(float err_x, float err_y, int valid)
{
    uint32_t now = HAL_GetTick();

    if (valid)
    {
        // 正常识别到目标，记录时间和误差
        last_valid_time = now;
        last_err_x = err_x;
        last_err_y = err_y;
    }
    else
    {
        // 丢失目标，是否还在允许时间内
        if ((now - last_valid_time) > LOST_TIMEOUT_MS)
        {
            // 丢失超时，停止运动
            return;
        }

        // 短时丢失，使用上一次的误差继续运行
        err_x = last_err_x;
        err_y = last_err_y;
    }

    // 死区判断：小于 DEAD_ZONE 不再移动
    if (fabsf(err_x) < DEAD_ZONE && fabsf(err_y) < DEAD_ZONE)
    {
        // StepMotor_Turn(STEP_MOTOR_B, 0, 32.0f, 0, 0);
        return;
    }

    float output_x = PID_Compute(&pid_x, err_x);
    float output_y = PID_Compute(&pid_y, err_y);

    int dir_x = (output_x >= 0) ? 1 : 0;
    int dir_y = (output_y >= 0) ? 1 : 0;

    float rpm_x = fabsf(output_x);
    float rpm_y = fabsf(output_y);

    // 限制 RPM
    if (rpm_x < 0.1f) rpm_x = 0.1f;
    if (rpm_x > 200.0f) rpm_x = 200.0f;
    if (rpm_y < 0.1f) rpm_y = 0.1f;
    if (rpm_y > 200.0f) rpm_y = 200.0f;

    // float angle = 1.8f; // 每次移动一个步长角度
    float angle = 0.12f;
    StepMotor_Turn(STEP_MOTOR_B, angle, 32.0f, dir_x, rpm_x);
    StepMotor_Turn(STEP_MOTOR_A, angle, 32.0f, dir_y, rpm_y);
}
