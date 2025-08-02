#include "pid.h"
#include "step_motor.h"
#include <math.h>
#include <stdint.h>

#define DEAD_ZONE         1.0f     // 死区阈值，偏差小于此值不移动
#define LOST_TIMEOUT_MS   300      // 丢失目标的最大持续时间（单位：ms）

PID_t pid_x, pid_y;

int last_err_x = 0, last_err_y = 0;
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

void StepMotor_PID_Update(int err_x, int err_y, int valid)
{
    uint32_t now = HAL_GetTick();

    if (valid)
    {
        last_valid_time = now;
        last_err_x = err_x;
        last_err_y = err_y;
    }
    else if ((now - last_valid_time) > LOST_TIMEOUT_MS)
    {
        return;
    }
    else
    {
        err_x = last_err_x;
        err_y = last_err_y;
    }

    if (fabsf(err_x) < DEAD_ZONE && fabsf(err_y) < DEAD_ZONE)
        return;

    // 电机是否运行中？
    // if (motor_ramp_A.run_state == RUN || motor_ramp_B.run_state == RUN)
    //     return;

    float output_x = PID_Compute(&pid_x, err_x);
    float output_y = PID_Compute(&pid_y, err_y);

    int dir_x = (output_x >= 0) ? 1 : 0;
    int dir_y = (output_y >= 0) ? 1 : 0;

    float rpm_x = fabsf(output_x);
    float rpm_y = fabsf(output_y);

    if (rpm_x < 10.0f) rpm_x = 10.0f;
    if (rpm_x > 25.0f) rpm_x = 25.0f;
    if (rpm_y < 10.0f) rpm_y = 10.0f;
    if (rpm_y > 25.0f) rpm_y = 25.0f;

    float angle = 0.12f;
    // float angle = 6.0f;
    float min_angle = 1.8f / 32.0f;

    if (angle < min_angle) angle = min_angle;

    StepMotor_Turn(STEP_MOTOR_B, angle, 32.0f, dir_x, rpm_x);
    StepMotor_Turn(STEP_MOTOR_A, angle, 32.0f, dir_y, rpm_y);
}

