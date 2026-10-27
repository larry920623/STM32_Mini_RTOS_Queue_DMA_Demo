#ifndef OS_H
#define OS_H
#include <stdint.h>
#define TASK_READY 0U
#define TASK_BLOCKED 1U
typedef struct { uint32_t *psp; volatile uint32_t delay_ticks; volatile uint8_t state; } TCB_t;
typedef struct { volatile uint8_t locked; TCB_t *owner; TCB_t *blocked_task; } Mutex_t;
typedef struct { volatile uint32_t count; TCB_t *blocked_task; } Semaphore_t;
extern TCB_t tcb1, tcb2, tcb_idle;
extern TCB_t * volatile current_tcb;
extern volatile uint32_t os_panic_code;
void Task_Init(TCB_t *, void (*)(void), uint32_t *, uint32_t);
void OS_Start(TCB_t *first_task) __attribute__((noreturn));
void OS_Delay(uint32_t ticks);
void OS_Schedule(void);
void OS_Tick(void);
void OS_Panic(uint32_t code) __attribute__((noreturn));
void OS_RequireTask(void);
#endif
