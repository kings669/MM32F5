#ifndef __SCHEDULER_H_
#define __SCHEDULER_H_
#include "hal_common.h"

#define TICK_PER_SECOND	1000
#ifdef __cplusplus
 extern "C" {
#endif 

typedef struct
{
void(*task_func)(void);
uint16_t rate_hz;
uint16_t interval_ticks;
uint32_t last_run;
}sched_task_t;

void Scheduler_Setup(void);
void Scheduler_Run(void);


#ifdef __cplusplus
}
#endif


#endif
