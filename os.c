#include "os.h"

// Global OS state
TCB_t    tasks[MAX_TASKS];
int      current_task      = 0;
int      next_task         = 0;
int      num_created_tasks = 0;
uint32_t os_tick           = 0;

// Memory pool
static uint32_t memory_pool[POOL_SIZE];
static uint32_t pool_index = 0;

uint32_t* os_malloc(uint32_t size_words) {
    if(pool_index + size_words > POOL_SIZE) {
        return 0;
    }
    uint32_t *ptr = &memory_pool[pool_index];
    pool_index += size_words;
    return ptr;
}

void os_task_create(TCB_t *tcb, void (*task_func)(void), uint32_t *stack, uint32_t stack_size, uint32_t priority) {
    uint32_t *stack_top = stack + stack_size - 16;

    stack_top[15] = (1 << 24);
    stack_top[14] = (uint32_t)task_func;
    stack_top[13] = 0xFFFFFFFD;
    stack_top[12] = 0x12;
    stack_top[11] = 0x3;
    stack_top[10] = 0x2;
    stack_top[9]  = 0x1;
    stack_top[8]  = 0x0;
    stack_top[7]  = 0x11;
    stack_top[6]  = 0x10;
    stack_top[5]  = 0x9;
    stack_top[4]  = 0x8;
    stack_top[3]  = 0x7;
    stack_top[2]  = 0x6;
    stack_top[1]  = 0x5;
    stack_top[0]  = 0x4;

    stack[0]        = STACK_CANARY;
    tcb->stack_ptr  = stack_top;
    tcb->stack_base = stack;
    tcb->priority   = priority;
    tcb->state      = TASK_READY;
    tcb->task_id    = (tcb - tasks);
    tcb->wake_tick  = 0;

    num_created_tasks++;
}

int os_scheduler(void) {
    if(tasks[current_task].state == TASK_RUNNING) {
        tasks[current_task].state = TASK_READY;
    }
    for(int i = 1; i <= num_created_tasks; i++) {
        int next = (current_task + i) % num_created_tasks;
        if(tasks[next].state == TASK_READY) {
            tasks[next].state = TASK_RUNNING;
            return next;
        }
    }
    tasks[current_task].state = TASK_RUNNING;
    return current_task;
}

void os_start(void) {
    NVIC_SetPriority(PendSV_IRQn, 0xFF);
    SysTick_Config(16000);
    tasks[0].state = TASK_RUNNING;
    __set_PSP((uint32_t)tasks[0].stack_ptr);
    __set_CONTROL(0x02);
    __ISB();
    // jump to first task
    void (*first_task)(void) = (void(*)(void))(tasks[0].stack_base[STACK_SIZE - 2]);
    first_task();
}

void os_delay(uint32_t ticks) {
    tasks[current_task].wake_tick = os_tick + ticks;
    tasks[current_task].state     = TASK_BLOCKED;
    next_task                     = os_scheduler();
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

void SysTick_Handler(void) {
    os_tick++;

    // stack overflow check
    for(int i = 0; i < num_created_tasks; i++) {
        if(tasks[i].stack_base != 0) {
            if(tasks[i].stack_base[0] != STACK_CANARY) {
                while(1); // stack overflow detected - halt
            }
        }
    }

    // unblock sleeping tasks
    for(int i = 0; i < num_created_tasks; i++) {
        if(tasks[i].state == TASK_BLOCKED && os_tick >= tasks[i].wake_tick) {
            tasks[i].state = TASK_READY;
        }
    }

    next_task = os_scheduler();
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

void HardFault_Handler(void) {
    while(1);
}

// Mutex
void mutex_lock(mutex_t *m) {
    while(m->locked && m->owner != current_task) {
        tasks[current_task].state = TASK_MUTEX_WAIT;
        next_task                 = os_scheduler();
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    }
    m->locked = 1;
    m->owner  = current_task;
}

void mutex_unlock(mutex_t *m) {
    if(m->owner == current_task) {
        m->locked = 0;
        m->owner  = 0xFF;
        for(int i = 0; i < num_created_tasks; i++) {
            if(tasks[i].state == TASK_MUTEX_WAIT) {
                tasks[i].state = TASK_READY;
            }
        }
    }
}

// Semaphore
void sem_wait(semaphore_t *s) {
    while(s->count <= 0) {
        tasks[current_task].state = TASK_BLOCKED;
        next_task                 = os_scheduler();
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    }
    s->count--;
}

void sem_post(semaphore_t *s) {
    s->count++;
    for(int i = 0; i < num_created_tasks; i++) {
        if(tasks[i].state == TASK_BLOCKED) {
            tasks[i].state = TASK_READY;
            break;
        }
    }
}

// Queue
void queue_send(queue_t *q, uint32_t msg) {
    while(q->count >= QUEUE_SIZE) {
        tasks[current_task].state = TASK_BLOCKED;
        next_task                 = os_scheduler();
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    }
    q->buffer[q->tail] = msg;
    q->tail            = (q->tail + 1) % QUEUE_SIZE;
    q->count++;
}

uint32_t queue_receive(queue_t *q) {
    while(q->count == 0) {
        tasks[current_task].state = TASK_BLOCKED;
        next_task                 = os_scheduler();
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    }
    uint32_t msg = q->buffer[q->head];
    q->head      = (q->head + 1) % QUEUE_SIZE;
    q->count--;
    return msg;
}