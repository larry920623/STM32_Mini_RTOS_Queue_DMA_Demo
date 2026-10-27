#include "uart_dma.h"
#include "os_sync.h"
#include <stdarg.h>
#include <stdio.h>
extern UART_HandleTypeDef huart1;
static Mutex_t tx_mutex;
static Semaphore_t tx_done, rx_ready;
static char tx_buffer[UART_TX_CAPACITY];
static uint8_t rx_byte, rx_buffer[UART_RX_CAPACITY];
static uint32_t rx_head, rx_tail, rx_count;
static volatile uint8_t tx_active;
static volatile HAL_StatusTypeDef tx_result;
volatile uint32_t uart_dma_done_count, uart_dma_error_count;
volatile HAL_StatusTypeDef uart_dma_last_status = HAL_OK;
volatile uint32_t uart_rx_drop_count, uart_rx_error_count, uart_rx_fault_epoch;

HAL_StatusTypeDef UART_ServiceInit(void) {
    Mutex_Init(&tx_mutex); Sem_Init(&tx_done, 0U); Sem_Init(&rx_ready, 0U);
    rx_head = rx_tail = rx_count = 0U; tx_active = 0U;
    return HAL_UART_Receive_IT(&huart1, &rx_byte, 1U);
}
HAL_StatusTypeDef UART_Printf(const char *format, ...) {
    OS_RequireTask();
    Mutex_Lock(&tx_mutex);
    va_list args; va_start(args, format);
    int length = vsnprintf(tx_buffer, sizeof(tx_buffer), format, args);
    va_end(args);
    HAL_StatusTypeDef status;
    if (length <= 0 || length >= (int)sizeof(tx_buffer)) status = HAL_ERROR;
    else {
        uint32_t key = __get_PRIMASK(); __disable_irq();
        tx_result = HAL_BUSY; tx_active = 1U;
        status = HAL_UART_Transmit_DMA(&huart1, (const uint8_t *)tx_buffer, (uint16_t)length);
        if (status != HAL_OK) tx_active = 0U;
        __set_PRIMASK(key);
        if (status == HAL_OK) {
            /* If completion precedes this wait, the semaphore retains its token. */
            Sem_Wait(&tx_done);
            status = tx_result;
        }
    }
    if (status != HAL_OK) ++uart_dma_error_count;
    uart_dma_last_status = status;
    /* Hold the resource until physical UART completion, not just DMA start. */
    Mutex_Unlock(&tx_mutex);
    return status;
}
uint8_t UART_ReadByte(void) {
    Sem_Wait(&rx_ready);
    uint32_t key = __get_PRIMASK(); __disable_irq();
    if (!rx_count) OS_Panic(20U);
    uint8_t byte = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1U) % UART_RX_CAPACITY; --rx_count;
    __set_PRIMASK(key);
    return byte;
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart != &huart1) return;
    uint32_t key = __get_PRIMASK(); __disable_irq();
    if (rx_count < UART_RX_CAPACITY) {
        rx_buffer[rx_head] = rx_byte;
        rx_head = (rx_head + 1U) % UART_RX_CAPACITY; ++rx_count;
        Sem_SignalFromISR(&rx_ready);
    } else { ++uart_rx_drop_count; ++uart_rx_fault_epoch; }
    __set_PRIMASK(key);
    if (HAL_UART_Receive_IT(&huart1, &rx_byte, 1U) != HAL_OK) {
        ++uart_rx_error_count; ++uart_rx_fault_epoch;
    }
}
static void tx_finish(HAL_StatusTypeDef result) {
    uint32_t key = __get_PRIMASK(); __disable_irq();
    if (tx_active) {
        tx_active = 0U; tx_result = result;
        if (result == HAL_OK) ++uart_dma_done_count;
        Sem_SignalFromISR(&tx_done);
    }
    __set_PRIMASK(key);
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == &huart1) tx_finish(HAL_OK);
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart != &huart1) return;
    if (huart->ErrorCode & HAL_UART_ERROR_DMA) tx_finish(HAL_ERROR);
    else { ++uart_rx_error_count; ++uart_rx_fault_epoch; }
    /* HAL ends RX on overrun; rearm only after it has returned to READY. */
    if (huart->RxState == HAL_UART_STATE_READY)
        if (HAL_UART_Receive_IT(huart, &rx_byte, 1U) != HAL_OK) {
            ++uart_rx_error_count; ++uart_rx_fault_epoch;
        }
}
