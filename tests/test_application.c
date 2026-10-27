#include "os.h"
#include "uart_dma.h"
#include <assert.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
TCB_t tcb1,tcb2,tcb_idle;
TCB_t * volatile current_tcb;
volatile uint32_t os_panic_code;
volatile uint32_t uart_dma_done_count,uart_dma_error_count,uart_rx_drop_count,uart_rx_error_count,uart_rx_fault_epoch;
volatile HAL_StatusTypeDef uart_dma_last_status;
static uint32_t mask,ticks;
static char log_buffer[32768];static size_t log_used;
static const char *input;static size_t input_index,fault_at=(size_t)-1;
static jmp_buf stop;
uint32_t __get_PRIMASK(void){return mask;}
void __disable_irq(void){mask=1;}
void __set_PRIMASK(uint32_t key){mask=key;}
void __DSB(void){}void __ISB(void){}
void OS_RequireTask(void){assert(current_tcb && !mask);}
void OS_Schedule(void){if(current_tcb==&tcb2 && tcb2.state==TASK_BLOCKED)longjmp(stop,1);}
void OS_Panic(uint32_t code){fprintf(stderr,"panic %u\n",code);abort();}
uint32_t HAL_GetTick(void){return ticks++;}
HAL_StatusTypeDef UART_Printf(const char *format,...){
 va_list a;va_start(a,format);int n=vsnprintf(log_buffer+log_used,sizeof(log_buffer)-log_used,format,a);va_end(a);
 assert(n>=0 && (size_t)n<sizeof(log_buffer)-log_used);log_used+=(size_t)n;return HAL_OK;
}
uint8_t UART_ReadByte(void){
 if(!input[input_index])longjmp(stop,1);
 if(input_index==fault_at)uart_rx_fault_epoch++;
 return (uint8_t)input[input_index++];
}
#include "../Core/Src/workbench.c"
static void consume_all(void){current_tcb=&tcb2;tcb2.state=TASK_READY;if(!setjmp(stop))Worker_Task();mask=0;current_tcb=&tcb1;}
static void console_input(const char *s){input=s;input_index=0;current_tcb=&tcb1;if(!setjmp(stop))Console_Task();mask=0;}
int main(void){
 current_tcb=&tcb1;Workbench_Init();
 command("stats 10 20 30");command("sort 9 -3 4 4 0");command("stats -1 0 0");command("sort 3 2 1");
 assert(Queue_Count(&jobs)==4 && state.accepted==4);
 command("stats 1");assert(state.rejected==1 && Queue_Count(&jobs)==4 && strstr(log_buffer,"BUSY"));
 consume_all();assert(state.completed==4 && Queue_Count(&jobs)==0);
 assert(strstr(log_buffer,"n=3 min=10 max=30 sum=60 mean=20.000"));
 assert(strstr(log_buffer,"sort -3 0 4 4 9"));
 command("nonsense");assert(state.invalid==1);command("status");assert(strstr(log_buffer,"worker=IDLE"));
 Workbench_Init();console_input("stats 1 2\r\nsort 3 1\nstats 7\r");assert(state.accepted==3 && state.invalid==0);
 consume_all();assert(state.completed==3);
 char long_line[320];memset(long_line,'x',sizeof(long_line));long_line[300]='\n';strcpy(long_line+301,"stats 5\n");
 Workbench_Init();console_input(long_line);assert(state.invalid==1 && state.accepted==1);consume_all();
 Workbench_Init();fault_at=4;console_input("stats 123\nstats 7\n");fault_at=(size_t)-1;
 assert(state.invalid==1 && state.accepted==1);consume_all();
 puts("PASS: command-to-result pipeline, FIFO/busy accounting, CR/LF/CRLF, overlong lines and RX-loss recovery");
}
