#ifndef TEST_CMSIS_H
#define TEST_CMSIS_H
#include <stdint.h>
uint32_t __get_PRIMASK(void);
void __disable_irq(void);
void __set_PRIMASK(uint32_t key);
void __DSB(void);
void __ISB(void);
#endif
