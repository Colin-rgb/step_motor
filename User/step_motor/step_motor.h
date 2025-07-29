#ifndef __STEP_MOTOR_H__
#define __STEP_MOTOR_H__

#include "main.h"

// 电机 ID 枚举
typedef enum {
    STEP_MOTOR_A = 0,  // TIM8_CH3 控制，TIM2 计步
    STEP_MOTOR_B = 1   // TIM1_CH2N 控制，TIM3 计步
} StepMotorId;

// 初始化 GPIO（方向、SLEEP）
void StepMotor_Init(void);

// 设置方向：GPIO_PIN_RESET = 顺时针，GPIO_PIN_SET = 逆时针
void StepMotor_SetDir(StepMotorId motor, GPIO_PinState dir);

// 设置使能（SLEEP引脚控制）：GPIO_PIN_SET = 唤醒，GPIO_PIN_RESET = 休眠
void StepMotor_SetSleep(StepMotorId motor, GPIO_PinState state);

// 启动 PWM（开启脉冲输出）
void StepMotor_Start(StepMotorId motor);

// 停止 PWM（停止脉冲输出）
void StepMotor_Stop(StepMotorId motor);

// 设置 PWM 占空比（百分比 0~100）
void StepMotor_SetDuty(StepMotorId motor, float percent);

// 控制电机转动固定角度
void StepMotor_Turn(StepMotorId motor,
                    float angle,        // 角度（单位：度）
                    float subdivide,    // 细分数（如 1, 2, 4, 8...）
                    uint8_t dir,        // 方向：0 = 顺时针，1 = 逆时针
                    float rpm);         // 转速（单位：RPM）

#endif // __STEP_MOTOR_H__
