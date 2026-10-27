#include "os_sync.h"
#include "stm32f4xx.h"
#include <stddef.h>
void Sem_Init(Semaphore_t *s, uint32_t count) { s->count = count; s->blocked_task = NULL; }
void Sem_Wait(Semaphore_t *s) {
    OS_RequireTask();
    for (;;) {
        uint32_t key = __get_PRIMASK(); __disable_irq();
        if (s->count) { --s->count; __set_PRIMASK(key); return; }
        if (s->blocked_task && s->blocked_task != current_tcb) OS_Panic(10U);
        s->blocked_task = current_tcb; current_tcb->state = TASK_BLOCKED;
        OS_Schedule(); __set_PRIMASK(key); __DSB(); __ISB();
    }
}
static void sem_post(Semaphore_t *s) {
    uint32_t key = __get_PRIMASK(); __disable_irq();
    if (s->count == UINT32_MAX) OS_Panic(11U);
    ++s->count;
    if (s->blocked_task) {
        s->blocked_task->state = TASK_READY; s->blocked_task = NULL;
        OS_Schedule();
    }
    __set_PRIMASK(key);
}
void Sem_Signal(Semaphore_t *s) { sem_post(s); }
void Sem_SignalFromISR(Semaphore_t *s) { sem_post(s); }
void Mutex_Init(Mutex_t *m) { m->locked = 0U; m->owner = NULL; m->blocked_task = NULL; }
void Mutex_Lock(Mutex_t *m) {
    OS_RequireTask();
    uint32_t key = __get_PRIMASK(); __disable_irq();
    TCB_t *self = current_tcb;
    if (m->owner == self) OS_Panic(12U); /* non-recursive */
    if (!m->locked) { m->locked = 1U; m->owner = self; __set_PRIMASK(key); return; }
    if (m->blocked_task && m->blocked_task != self) OS_Panic(13U);
    m->blocked_task = self;
    while (m->owner != self) {
        self->state = TASK_BLOCKED; OS_Schedule();
        __set_PRIMASK(key); __DSB(); __ISB(); __disable_irq();
    }
    __set_PRIMASK(key);
}
void Mutex_Unlock(Mutex_t *m) {
    uint32_t key = __get_PRIMASK(); __disable_irq();
    if (!current_tcb || m->owner != current_tcb) OS_Panic(14U);
    if (m->blocked_task) {
        TCB_t *next = m->blocked_task;
        m->blocked_task = NULL; m->owner = next; m->locked = 1U;
        next->state = TASK_READY; OS_Schedule();
    } else { m->locked = 0U; m->owner = NULL; }
    __set_PRIMASK(key); __DSB(); __ISB();
}
