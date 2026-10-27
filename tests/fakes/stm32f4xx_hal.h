#ifndef TEST_HAL_H
#define TEST_HAL_H
#include "stm32f4xx.h"
#include <stdint.h>
typedef enum { HAL_OK=0, HAL_ERROR=1, HAL_BUSY=2, HAL_TIMEOUT=3 } HAL_StatusTypeDef;
typedef struct { uint32_t ErrorCode, RxState; } UART_HandleTypeDef;
#define HAL_UART_ERROR_DMA 16U
#define HAL_UART_STATE_READY 1U
HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef *, const uint8_t *, uint16_t);
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *, uint8_t *, uint16_t);
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *);
void HAL_UART_ErrorCallback(UART_HandleTypeDef *);
#endif
