#ifndef OS_SYNC_H
#define OS_SYNC_H
#include "os.h"
void Sem_Init(Semaphore_t *s, uint32_t initial_count);
void Sem_Wait(Semaphore_t *s);
void Sem_Signal(Semaphore_t *s);
void Sem_SignalFromISR(Semaphore_t *s);
void Mutex_Init(Mutex_t *m);
void Mutex_Lock(Mutex_t *m);
void Mutex_Unlock(Mutex_t *m);
#endif
