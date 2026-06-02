#ifndef OS_H
#define OS_H

#include "ARMCM3.h"
#include <stdint.h>

// Configuration
#define MAX_TASKS       16
#define STACK_SIZE      512
#define POOL_SIZE       4096
#define QUEUE_SIZE      8
#define STACK_CANARY    0xDEADBEEF

// Task states
#define TASK_READY      0
#define TASK_RUNNING    1
#define TASK_BLOCKED    2
#define TASK_MUTEX_WAIT 3

// TCB
typedef struct {
    uint32_t *stack_ptr;
    uint32_t *stack_base;
    uint32_t  priority;
    uint32_t  task_id;
    uint32_t  state;
    uint32_t  wake_tick;
} TCB_t;

// Mutex
typedef struct {
    uint32_t locked;
    int      owner;
} mutex_t;

// Semaphore
typedef struct {
    int count;
} semaphore_t;

// Queue
typedef struct {
    uint32_t buffer[QUEUE_SIZE];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} queue_t;

// Globals
extern TCB_t     tasks[MAX_TASKS];
extern int       current_task;
extern int       next_task;
extern int       num_created_tasks;
extern uint32_t  os_tick;

// OS API
void      os_task_create(TCB_t *tcb, void (*task_func)(void), uint32_t *stack, uint32_t stack_size, uint32_t priority);
void      os_start(void);
void      os_delay(uint32_t ticks);
uint32_t* os_malloc(uint32_t size_words);
int       os_scheduler(void);

// Sync primitives
void      mutex_lock(mutex_t *m);
void      mutex_unlock(mutex_t *m);
void      sem_wait(semaphore_t *s);
void      sem_post(semaphore_t *s);
void      queue_send(queue_t *q, uint32_t msg);
uint32_t  queue_receive(queue_t *q);

#endif