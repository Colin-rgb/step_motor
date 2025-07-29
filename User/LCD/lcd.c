/* lcd_hal.c - HAL-based driver for ST7735R using STM32F103RCT6 */
#include "lcd.h"
#include "main.h"
#include "stm32f1xx_hal.h"
#include "Font.h"
#include <stdio.h>
#include <string.h>
// 模拟SPI发送一个字节
void LCD_SPI_Write(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++) {
        if (data & 0x80)
            LCD_SDA_H();
        else
            LCD_SDA_L();

        LCD_SCL_L();
        LCD_SCL_H();
        data <<= 1;
    }
}

void LCD_WriteCommand(uint8_t cmd)
{
    LCD_CS_L();
    LCD_DC_L();
    LCD_SPI_Write(cmd);
    LCD_CS_H();
}

void LCD_WriteData(uint8_t data)
{
    LCD_CS_L();
    LCD_DC_H();
    LCD_SPI_Write(data);
    LCD_CS_H();
}

void LCD_WriteData16(uint16_t data)
{
    LCD_CS_L();
    LCD_DC_H();
    LCD_SPI_Write(data >> 8);
    LCD_SPI_Write(data & 0xFF);
    LCD_CS_H();
}

void LCD_Reset(void)
{
    LCD_RES_L();
    HAL_Delay(100);
    LCD_RES_H();
    HAL_Delay(50);
}

void LCD_SetRegion(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end)
{
    LCD_WriteCommand(0x2A);
    LCD_WriteData(0x00);
    LCD_WriteData(x_start + 2);
    LCD_WriteData(0x00);
    LCD_WriteData(x_end + 2);

    LCD_WriteCommand(0x2B);
    LCD_WriteData(0x00);
    LCD_WriteData(y_start + 3);
    LCD_WriteData(0x00);
    LCD_WriteData(y_end + 3);

    LCD_WriteCommand(0x2C);
}

void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
{
    LCD_SetRegion(x, y, x, y);
    LCD_WriteData16(color);
}

void LCD_Clear(uint16_t color)
{
    LCD_SetRegion(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    for (uint32_t i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        LCD_WriteData16(color);
    }
}

void LCD_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 |
                          GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void LCD_Init(void)
{
    LCD_GPIO_Init();
    LCD_Reset();

    LCD_WriteCommand(0x11);
    HAL_Delay(120);

    LCD_WriteCommand(0xB1);
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);

    LCD_WriteCommand(0xB2);
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);

    LCD_WriteCommand(0xB3);
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);

    LCD_WriteCommand(0xB4);
    LCD_WriteData(0x07);

    LCD_WriteCommand(0xC0);
    LCD_WriteData(0xA2);
    LCD_WriteData(0x02);
    LCD_WriteData(0x84);

    LCD_WriteCommand(0xC1);
    LCD_WriteData(0xC5);

    LCD_WriteCommand(0xC2);
    LCD_WriteData(0x0A);
    LCD_WriteData(0x00);

    LCD_WriteCommand(0xC3);
    LCD_WriteData(0x8A);
    LCD_WriteData(0x2A);
    LCD_WriteCommand(0xC4);
    LCD_WriteData(0x8A);
    LCD_WriteData(0xEE);

    LCD_WriteCommand(0xC5);
    LCD_WriteData(0x0E);

    LCD_WriteCommand(0x36);
    LCD_WriteData(0xC8);

    LCD_WriteCommand(0x3A);
    LCD_WriteData(0x05);

    LCD_WriteCommand(0x29); // display ON
    LCD_BLK_H();
}

// 显示16x16中英文字符
void LCD_DrawFont_GBK16(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *s)
{
    uint8_t i, j;
    uint16_t k, x0 = x;

    while (*s)
    {
        if (*s < 128)
        {
            k = *s;
            if (k == 13) { x = x0; y += 16; }
            else {
                k = (k > 32) ? (k - 32) : 0;
                for (i = 0; i < 16; i++)
                    for (j = 0; j < 8; j++)
                        LCD_DrawPoint(x + j, y + i, (asc16[k * 16 + i] & (0x80 >> j)) ? fc : bc);
                x += 8;
            }
            s++;
        }
        else
        {
            for (k = 0; k < hz16_num; k++)
            {
                // if (hz16[k].Index[0] == *(s) && hz16[k].Index[1] == *(s + 1))
                // {
                //     for (i = 0; i < 16; i++)
                //     {
                //         for (j = 0; j < 8; j++)
                //         {
                //             LCD_DrawPoint(x + j, y + i, (hz16[k].Msk[i * 2] & (0x80 >> j)) ? fc : bc);
                //             LCD_DrawPoint(x + j + 8, y + i, (hz16[k].Msk[i * 2 + 1] & (0x80 >> j)) ? fc : bc);
                //         }
                //     }
                // }
            }
            s += 2;
            x += 16;
        }
    }
}



void LCD_DisplayLabelFloat(uint16_t x, uint16_t y, const char *label, float value, uint8_t decimals, uint16_t fc, uint16_t bc)
{
    char buffer[20];
    LCD_DrawFont_GBK16(x, y, fc, bc, (uint8_t *)label); // 显示标签（中文或英文）

    snprintf(buffer, sizeof(buffer), "%.*f", decimals, value); // 格式化浮点数
    LCD_DrawFont_GBK16(x + 16 * strlen(label), y, fc, bc, (uint8_t *)buffer); // 显示值
}

void LCD_DisplayLabelInt(uint16_t x, uint16_t y, const char *label, int value, uint16_t fc, uint16_t bc)
{
    char buffer[20];
    LCD_DrawFont_GBK16(x, y, fc, bc, (uint8_t *)label);

    snprintf(buffer, sizeof(buffer), "%d", value);
    LCD_DrawFont_GBK16(x + 16 * strlen(label), y, fc, bc, (uint8_t *)buffer);
}

void LCD_ShowParamLine(uint16_t x, uint16_t y, const char *label, float value, uint8_t decimals, uint16_t fc, uint16_t bg)
{
    char valStr[20];
    snprintf(valStr, sizeof(valStr), "%.*f", decimals, value);

    LCD_DrawFont_GBK16(x, y, fc, bg, (uint8_t *)label);
    LCD_DrawFont_GBK16(x + strlen(label) * 16, y, fc, bg, (uint8_t *)valStr);
}

void LCD_ShowParamPage(float voltage, float temperature, float rpm)
{
    LCD_Clear(BLACK);

    LCD_DrawFont_GBK16(32, 10, YELLOW, BLACK, (uint8_t *)"系统参数");

    // 分隔线
    for (int i = 0; i < LCD_WIDTH; i++) {
        LCD_DrawPoint(i, 30, BLUE);
    }

    LCD_ShowParamLine(10, 40, "vol:", voltage, 2, WHITE, BLACK);
    LCD_ShowParamLine(10, 60, "temp:", temperature, 1, WHITE, BLACK);
    LCD_ShowParamLine(10, 80, "rpm:", rpm, 0, WHITE, BLACK);
}