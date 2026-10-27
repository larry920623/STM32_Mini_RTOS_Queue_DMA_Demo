#include "os.h"
#include "stm32f4xx.h"
#include <stddef.h>
TCB_t * volatile current_tcb = NULL;
volatile uint32_t os_panic_code = 0U;
void OS_Panic(uint32_t code) { __disable_irq(); os_panic_code = code; while (1) __NOP(); }
void OS_RequireTask(void) {
    if (!current_tcb || __get_IPSR() || __get_PRIMASK()) OS_Panic(1U);
}
void OS_Schedule(void) {
    if (current_tcb) { SCB->ICSR = SCB_ICSR_PENDSVSET_Msk; __DSB(); }
}
void OS_Delay(uint32_t ticks) {
    OS_RequireTask();
    uint32_t key = __get_PRIMASK(); __disable_irq();
    current_tcb->delay_ticks = ticks;
    OS_Schedule(); __set_PRIMASK(key); __DSB(); __ISB();
}
/* Called only by PendSV with interrupts masked; choose from fresh states. */
TCB_t *OS_SelectNext(void) {
    TCB_t *first = current_tcb == &tcb1 ? &tcb2 : &tcb1;
    TCB_t *second = first == &tcb1 ? &tcb2 : &tcb1;
    if (first->state == TASK_READY && !first->delay_ticks) current_tcb = first;
    else if (second->state == TASK_READY && !second->delay_ticks) current_tcb = second;
    else current_tcb = &tcb_idle;
    return current_tcb;
}
void OS_Tick(void) {
    if (!current_tcb) return;
    if (tcb1.delay_ticks) --tcb1.delay_ticks;
    if (tcb2.delay_ticks) --tcb2.delay_ticks;
    OS_Schedule();
}
static void task_returned(void) { OS_Panic(2U); }
void Task_Init(TCB_t *tcb, void (*entry)(void), uint32_t *stack, uint32_t size) {
    if (!tcb || !entry || !stack || size < 16U || ((uintptr_t)(stack + size) & 7U)) OS_Panic(3U);
    tcb->delay_ticks = 0U; tcb->state = TASK_READY;
    uint32_t *sp = stack + size - 16U;
    for (unsigned i = 0; i < 16U; ++i) sp[i] = 0U;
    sp[15] = 0x01000000U;
    sp[14] = (uint32_t)entry;
    sp[13] = (uint32_t)task_returned;
    tcb->psp = sp;
}
/* current_tcb remains NULL until IRQ masking and PSP setup begin here. */
__attribute__((naked, noreturn)) void OS_Start(TCB_t *first_task __attribute__((unused))) {
    __asm volatile(
        "CPSID i\n"
        "LDR R1, =current_tcb\n"
        "STR R0, [R1]\n"
        "LDR R0, [R0]\n"
        "LDMIA R0!, {R4-R11}\n"
        "MSR PSP, R0\n"
        "MOV R0, #2\n"
        "MSR CONTROL, R0\n"
        "ISB\n"
        "POP {R0-R3}\n"
        "POP {R12, LR}\n"
        "POP {R1, R2}\n"
        "CPSIE i\n"
        "BX R1\n"
    );
}
__attribute__((naked)) void PendSV_Handler(void) {
    __asm volatile(
        "CPSID i\n"
        "MRS R0, PSP\n"
        "STMDB R0!, {R4-R11}\n"
        "LDR R1, =current_tcb\n"
        "LDR R2, [R1]\n"
        "STR R0, [R2]\n"
        "PUSH {R3, LR}\n"
        "BL OS_SelectNext\n"
        "POP {R3, LR}\n"
        "LDR R0, [R0]\n"
        "LDMIA R0!, {R4-R11}\n"
        "MSR PSP, R0\n"
        "CPSIE i\n"
        "BX LR\n"
    );
}
