#include "os.h"
#include "os_sync.h"
#include "msg_queue.h"
#include "uart_dma.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
TCB_t tcb1, tcb2, tcb_idle;
TCB_t * volatile current_tcb;
volatile uint32_t os_panic_code;
UART_HandleTypeDef huart1;
static uint32_t mask, pending, schedules;
static void (*inject)(void);
static int injecting;
uint32_t __get_PRIMASK(void) { return mask; }
void __disable_irq(void) { mask=1; }
void __set_PRIMASK(uint32_t v) {
    mask=v;
    if (!v && pending && inject && !injecting) {
        void (*event)(void)=inject; inject=NULL; pending=0; injecting=1;
        event(); injecting=0;
    }
}
void __DSB(void) {}
void __ISB(void) {}
void OS_Schedule(void) { pending=1; schedules++; }
void OS_RequireTask(void) { assert(current_tcb && !mask); }
void OS_Panic(uint32_t code) { fprintf(stderr,"PANIC %u\n",code);abort(); }
static Semaphore_t sem;
static Mutex_t mutex;
static MsgQueue_t queue;
static uint8_t *rx_target;
static int tx_mode;
static char last_tx[UART_TX_CAPACITY];
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *h,uint8_t *p,uint16_t n) {
    (void)h;assert(n==1);rx_target=p;return HAL_OK;
}
HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef *h,const uint8_t *p,uint16_t n) {
    assert(n<sizeof(last_tx));memcpy(last_tx,p,n);last_tx[n]=0;
    if (tx_mode==2) return HAL_ERROR;
    if (tx_mode==0) HAL_UART_TxCpltCallback(h); /* completion before wait */
    return HAL_OK;
}
static void signal_sem(void) { Sem_SignalFromISR(&sem); }
static void handoff_mutex(void) {
    current_tcb=&tcb1;Mutex_Unlock(&mutex);current_tcb=&tcb2;
}
static void send_job(void) {
    assert(queue.blocked_receiver==&tcb1 && tcb1.state==TASK_BLOCKED);
    WorkJob_t j={.id=99};current_tcb=&tcb2;
    assert(Queue_TrySend(&queue,&j)==QUEUE_OK);current_tcb=&tcb1;
}
static void receive_job(void) {
    WorkJob_t j; current_tcb=&tcb2;
    assert(Queue_TryReceive(&queue,&j)==QUEUE_OK && j.id==1);
    current_tcb=&tcb1;
}
static void complete_tx(void) { assert(tcb1.state==TASK_BLOCKED);HAL_UART_TxCpltCallback(&huart1); }
static void feed(uint8_t b) { assert(rx_target);*rx_target=b;HAL_UART_RxCpltCallback(&huart1); }
static void receive_byte(void) { feed('Z'); }
int main(void) {
    current_tcb=&tcb1;Sem_Init(&sem,0);
    Sem_SignalFromISR(&sem);Sem_Wait(&sem);assert(sem.count==0);
    inject=signal_sem;Sem_Wait(&sem);assert(sem.count==0 && tcb1.state==TASK_READY && !sem.blocked_task);
    mask=1;Sem_SignalFromISR(&sem);assert(mask==1);mask=0;Sem_Wait(&sem);
    Mutex_Init(&mutex);Mutex_Lock(&mutex);assert(mutex.owner==&tcb1);
    current_tcb=&tcb2;inject=handoff_mutex;Mutex_Lock(&mutex);
    assert(mutex.owner==&tcb2 && mutex.locked && !mutex.blocked_task && tcb2.state==TASK_READY);
    Mutex_Unlock(&mutex);assert(!mutex.locked);
    current_tcb=&tcb1;Queue_Init(&queue);
    WorkJob_t j={0},out;
    assert(Queue_TryReceive(&queue,&out)==QUEUE_EMPTY);
    for (unsigned i=1;i<=4;i++) { j.id=i;assert(Queue_TrySend(&queue,&j)==QUEUE_OK); }
    assert(Queue_TrySend(&queue,&j)==QUEUE_FULL);
    j.id=5;inject=receive_job;Queue_Send(&queue,&j);
    for (unsigned i=2;i<=5;i++) { assert(Queue_TryReceive(&queue,&out)==QUEUE_OK && out.id==i); }
    inject=send_job;Queue_Receive(&queue,&out);assert(out.id==99 && tcb1.state==TASK_READY);
    assert(UART_ServiceInit()==HAL_OK);
    tx_mode=0;assert(UART_Printf("early %d",7)==HAL_OK && !strcmp(last_tx,"early 7"));
    HAL_UART_TxCpltCallback(&huart1); /* stale duplicate must not create another token */
    uint32_t before=schedules;tx_mode=1;pending=0;inject=complete_tx;
    assert(UART_Printf("late")==HAL_OK && schedules>before && !inject);
    tx_mode=2;assert(UART_Printf("failure")==HAL_ERROR);
    tx_mode=0;assert(UART_Printf("retry")==HAL_OK && uart_dma_error_count==1);
    for (unsigned i=0;i<UART_RX_CAPACITY;i++) feed((uint8_t)i);
    feed(42);assert(uart_rx_drop_count==1 && uart_rx_fault_epoch==1);
    for (unsigned i=0;i<UART_RX_CAPACITY;i++) assert(UART_ReadByte()==(uint8_t)i);
    pending=0;inject=receive_byte;assert(UART_ReadByte()=='Z');
    puts("PASS: semaphore early/late signals, mutex handoff, queue full/empty/wrap/wake, DMA early/late/error, RX wrap/overflow");
}
