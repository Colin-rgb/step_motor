#include "usart_user.h"
extern UART_HandleTypeDef huart2;

uint8_t send_buf[TX_BUF_SIZE];

// 通过修改huart2，指定接口
void usart_printf(const char* format, ...)
{
    va_list args;
    uint32_t length;
    va_start(args, format);
    length = vsnprintf((char*)send_buf, TX_BUF_SIZE, (const char*)format, args);
    va_end(args);

    HAL_UART_Transmit_DMA(&huart2, (uint8_t*)send_buf, length);
}
