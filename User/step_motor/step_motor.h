#ifndef __STEP_MOTOR_H
#define __STEP_MOTOR_H

#include "main.h"

// 步进电机通道标识
typedef enum {
    STEP_MOTOR_A = 0,
    STEP_MOTOR_B = 1
} StepMotorId;

/**
 * @brief 初始化步进电机方向与休眠引脚（不启动 PWM）
 */
void StepMotor_Init(void);

/**
 * @brief 设置电机方向
 * @param motor 选择 A 或 B
 * @param dir GPIO_PIN_RESET = 顺时针，GPIO_PIN_SET = 逆时针
 */
void StepMotor_SetDir(StepMotorId motor, GPIO_PinState dir);

/**
 * @brief 设置电机 SLEEP 状态（电平控制）
 * @param state GPIO_PIN_RESET = 使能，GPIO_PIN_SET = 休眠
 */
void StepMotor_SetSleep(StepMotorId motor, GPIO_PinState state);

/**
 * @brief 启动指定电机的 PWM 输出
 */
void StepMotor_Start(StepMotorId motor);

/**
 * @brief 停止指定电机的 PWM 输出
 */
void StepMotor_Stop(StepMotorId motor);

/**
 * @brief 设置电机 PWM 占空比（单位：百分比）
 * @param percent 0~100，对应步进频率
 */
void StepMotor_SetDuty(StepMotorId motor, float percent);

void StepMotor_Turn(StepMotorId motor, float angle, float subdivide, uint8_t dir, float rpm);

#endif  // __STEP_MOTOR_H
