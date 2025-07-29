#ifndef INC_KEY_H_
#define INC_KEY_H_

#include "main.h"

// 硬件读取宏
#define KEY HAL_GPIO_ReadPin(KEY_motor_GPIO_Port, KEY_motor_Pin)
// #define KEY1 HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13)
// #define KEY2 HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_14)


// 轮询检测函数
uint8_t click_N_Double (uint8_t time);
uint8_t click(void);
uint8_t Long_Press(void);

// 初始化函数（启用 EXTI）
void Key_Init(void);

// // 单击检测
// uint8_t click(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
// // 单/双击检测
// uint8_t click_N_Double(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint16_t time);
// // 长按检测
// uint8_t Long_Press(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint16_t press_time);

#endif /* INC_KEY_H_ */
