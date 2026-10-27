#include "main.h"
#include "msg_queue.h"

void Queue_Init(MsgQueue_t *q)
{
    q->head = 0U;
    q->tail = 0U;
    q->count = 0U;

    q->blocked_receiver = NULL;
    q->blocked_sender = NULL;

    q->receive_wait_count = 0U;
    q->send_wait_count = 0U;
}

QueueStatus_t Queue_TrySend(
    MsgQueue_t *q,
    const Message_t *msg)
{
    uint32_t saved_primask = __get_PRIMASK();

    __disable_irq();

    if (q->count == MSG_QUEUE_CAPACITY) {
        __set_PRIMASK(saved_primask);
        return QUEUE_FULL;
    }

    q->buffer[q->tail] = *msg;
    q->tail = (q->tail + 1U) % MSG_QUEUE_CAPACITY;
    q->count++;

    if (q->blocked_receiver != NULL) {
        TCB_t *receiver = q->blocked_receiver;

        q->blocked_receiver = NULL;
        receiver->state = TASK_READY;

        OS_Schedule();
    }

    __set_PRIMASK(saved_primask);

    return QUEUE_OK;
}

QueueStatus_t Queue_TryReceive(
    MsgQueue_t *q,
    Message_t *msg)
{
    uint32_t saved_primask = __get_PRIMASK();

    __disable_irq();

    if (q->count == 0U) {
        __set_PRIMASK(saved_primask);
        return QUEUE_EMPTY;
    }

    *msg = q->buffer[q->head];

    q->head = (q->head + 1U) % MSG_QUEUE_CAPACITY;
    q->count--;

    if (q->blocked_sender != NULL) {
        TCB_t *sender = q->blocked_sender;

        q->blocked_sender = NULL;
        sender->state = TASK_READY;

        OS_Schedule();
    }

    __set_PRIMASK(saved_primask);

    return QUEUE_OK;
}
void Queue_Receive(MsgQueue_t *q, Message_t *msg)
{
    OS_RequireTask();
    while (1) {
        uint32_t saved_primask = __get_PRIMASK();

        __disable_irq();

        if (Queue_TryReceive(q, msg) == QUEUE_OK) {
            __set_PRIMASK(saved_primask);
            return;
        }

        q->blocked_receiver = current_tcb;
        q->receive_wait_count++;

        current_tcb->state = TASK_BLOCKED;

        OS_Schedule();

        __set_PRIMASK(saved_primask);

        __DSB();
        __ISB();

    }
}

void Queue_Send(
    MsgQueue_t *q,
    const Message_t *msg)
{
    OS_RequireTask();
    while (1) {
        uint32_t saved_primask = __get_PRIMASK();

        __disable_irq();

        if (Queue_TrySend(q, msg) == QUEUE_OK) {
            __set_PRIMASK(saved_primask);
            return;
        }

        q->blocked_sender = current_tcb;
        q->send_wait_count++;

        current_tcb->state = TASK_BLOCKED;

        OS_Schedule();

        __set_PRIMASK(saved_primask);

        __DSB();
        __ISB();

    }
}

uint32_t Queue_Count(MsgQueue_t *q) {
    uint32_t key = __get_PRIMASK(); __disable_irq();
    uint32_t count = q->count;
    __set_PRIMASK(key); return count;
}
