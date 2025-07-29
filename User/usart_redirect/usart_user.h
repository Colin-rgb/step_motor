#ifndef __USART_USER_H
#define __USART_USER_H

#include "main.h"
#include <stdarg.h>
#include <stdio.h>

#define TX_BUF_SIZE 512

void usart_printf(const char* format, ...);

#endif
