#ifndef __LCD_HAL_H
#define __LCD_HAL_H

#include "stm32f1xx_hal.h"

// 分辨率
#define LCD_WIDTH  128
#define LCD_HEIGHT 128

// 颜色定义
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define WHITE   0xFFFF
#define BLACK   0x0000
#define YELLOW  0xFFE0

// GPIO控制宏
#define LCD_SCL_H() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET)
#define LCD_SCL_L() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET)

#define LCD_SDA_H() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET)
#define LCD_SDA_L() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET)

#define LCD_RES_H() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET)
#define LCD_RES_L() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET)

#define LCD_DC_H()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET)
#define LCD_DC_L()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET)

#define LCD_CS_H()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET)
#define LCD_CS_L()  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET)

#define LCD_BLK_H() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET)
#define LCD_BLK_L() HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET)

// 函数声明
void LCD_Init(void);
void LCD_GPIO_Init(void);
void LCD_Reset(void);

void LCD_WriteCommand(uint8_t cmd);
void LCD_WriteData(uint8_t data);
void LCD_WriteData16(uint16_t data);
void LCD_SPI_Write(uint8_t data);

void LCD_SetRegion(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end);
void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color);
void LCD_Clear(uint16_t color);
void LCD_DrawFont_GBK16(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *s);

void LCD_DisplayLabelFloat(uint16_t x, uint16_t y, const char *label, float value, uint8_t decimals, uint16_t fc, uint16_t bc);
void LCD_DisplayLabelInt(uint16_t x, uint16_t y, const char *label, int value, uint16_t fc, uint16_t bc);

void LCD_ShowParamLine(uint16_t x, uint16_t y, const char *label, float value, uint8_t decimals, uint16_t fc, uint16_t bg);
void LCD_ShowParamPage(float voltage, float temperature, float rpm);
#endif
