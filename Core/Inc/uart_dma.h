#ifndef UART_DMA_H
#define UART_DMA_H
#include "stm32f4xx_hal.h"
#define UART_RX_CAPACITY 512U
#define UART_TX_CAPACITY 320U
extern volatile uint32_t uart_dma_done_count, uart_dma_error_count;
extern volatile HAL_StatusTypeDef uart_dma_last_status;
extern volatile uint32_t uart_rx_drop_count, uart_rx_error_count, uart_rx_fault_epoch;
HAL_StatusTypeDef UART_ServiceInit(void);
HAL_StatusTypeDef UART_Printf(const char *format, ...) __attribute__((format(printf, 1, 2)));
uint8_t UART_ReadByte(void);
#endif
