#include "workbench.h"
#include "main.h"
#include "msg_queue.h"
#include "uart_dma.h"
#include "work_job.h"
#include <string.h>

static MsgQueue_t jobs;
static struct {
    uint32_t accepted, completed, rejected, invalid;
    uint32_t active_id, active_kind;
    uint8_t active;
} state;
static uint32_t next_id = 1U;
void Workbench_Init(void) { Queue_Init(&jobs); memset(&state, 0, sizeof(state)); }
static void help(void) {
    UART_Printf("\r\nSTM32 UART Workbench - commands:\r\n");
    UART_Printf("  stats <1..16 integers>   min / max / sum / mean\r\n"
                "  sort <1..16 integers>    ascending order\r\n");
    UART_Printf("  status                  worker, queue, job and UART counters\r\n"
                "  help                    show commands\r\n"
                "Input: 115200 8N1; CR or LF; enable terminal local echo.\r\n");
}
static void status(void) {
    uint32_t key = __get_PRIMASK(); __disable_irq();
    uint32_t accepted = state.accepted, completed = state.completed;
    uint32_t rejected = state.rejected, invalid = state.invalid;
    uint32_t id = state.active_id, kind = state.active_kind;
    uint8_t active = state.active;
    uint32_t pending = Queue_Count(&jobs);
    uint32_t tx = uart_dma_done_count, txerr = uart_dma_error_count;
    uint32_t drops = uart_rx_drop_count, rxerr = uart_rx_error_count;
    __set_PRIMASK(key);
    UART_Printf("STATUS worker=%s job=%lu type=%s queue=%lu/4\r\n",
        active ? "RUNNING" : "IDLE", (unsigned long)(active ? id : 0U),
        active ? Work_Name((WorkKind_t)kind) : "-", (unsigned long)pending);
    UART_Printf("COUNTERS accepted=%lu completed=%lu busy=%lu invalid=%lu dma_done=%lu tx_err=%lu rx_drop=%lu rx_err=%lu\r\n",
        (unsigned long)accepted, (unsigned long)completed, (unsigned long)rejected,
        (unsigned long)invalid, (unsigned long)tx, (unsigned long)txerr,
        (unsigned long)drops, (unsigned long)rxerr);
}
static void command(const char *line) {
    WorkJob_t job; char error[80];
    CommandType_t type = Work_Parse(line, &job, error, sizeof(error));
    if (type == COMMAND_HELP) { help(); return; }
    if (type == COMMAND_STATUS) { status(); return; }
    if (type == COMMAND_ERROR) {
        ++state.invalid; UART_Printf("ERR %s\r\n", error); return;
    }
    job.id = next_id;
    /* One producer: the consumer can only free slots while ACK is sent.
     * This reserves admission without blocking the console on a full queue,
     * and guarantees QUEUED is on the wire before the worker sees the job. */
    uint32_t key = __get_PRIMASK(); __disable_irq();
    uint8_t full = Queue_Count(&jobs) == MSG_QUEUE_CAPACITY;
    if (full) ++state.rejected;
    __set_PRIMASK(key);
    if (full) { UART_Printf("BUSY queue full; command not accepted; retry later\r\n"); return; }
    UART_Printf("QUEUED #%lu %s\r\n", (unsigned long)job.id, Work_Name(job.kind));
    key = __get_PRIMASK(); __disable_irq();
    if (Queue_TrySend(&jobs, &job) != QUEUE_OK) OS_Panic(30U);
    ++state.accepted; ++next_id;
    __set_PRIMASK(key);
}
void Console_Task(void) {
    char line[WORK_MAX_LINE]; size_t length = 0U;
    uint8_t discard = 0U, previous_cr = 0U;
    uint32_t fault_epoch = uart_rx_fault_epoch;
    help(); UART_Printf("READY - enter a command, then press Enter.\r\n");
    for (;;) {
        uint8_t byte = UART_ReadByte();
        if (fault_epoch != uart_rx_fault_epoch) {
            fault_epoch = uart_rx_fault_epoch; discard = 1U; length = 0U;
        }
        if (byte == '\n' && previous_cr) { previous_cr = 0U; continue; }
        previous_cr = byte == '\r';
        if (byte == '\r' || byte == '\n') {
            if (discard) { ++state.invalid; UART_Printf("ERR input too long or lost; resend the whole line\r\n"); }
            else if (length) { line[length] = '\0'; command(line); }
            length = 0U; discard = 0U;
        } else if (byte == '\b' || byte == 127U) {
            if (!discard && length) --length;
        } else if (!discard && byte >= 32U) {
            if (length + 1U < sizeof(line)) line[length++] = (char)byte;
            else { discard = 1U; length = 0U; }
        } else if (byte == 0U) { discard = 1U; }
    }
}
void Worker_Task(void) {
    WorkJob_t job; WorkResult_t result; char text[240];
    for (;;) {
        Queue_Receive(&jobs, &job);
        uint32_t key = __get_PRIMASK(); __disable_irq();
        state.active = 1U; state.active_id = job.id; state.active_kind = job.kind;
        __set_PRIMASK(key);
        uint32_t start = HAL_GetTick();
        Work_Execute(&job, &result);
        uint32_t elapsed = HAL_GetTick() - start;
        int length = Work_Format(&job, &result, text, sizeof(text));
        /* The TX mutex is acquired only after computation and queue operations. */
        if (length < 0 || length >= (int)sizeof(text)) UART_Printf("ERR #%lu result formatting\r\n", (unsigned long)job.id);
        else UART_Printf("RESULT #%lu %s %s elapsed_ms=%lu\r\n", (unsigned long)job.id,
            Work_Name(job.kind), text, (unsigned long)elapsed);
        key = __get_PRIMASK(); __disable_irq();
        ++state.completed; state.active = 0U;
        __set_PRIMASK(key);
    }
}
