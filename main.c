#include "os.h"

// Shared resources
mutex_t      shared_mutex  = {0, 0xFF};
semaphore_t  data_ready    = {0};
queue_t      myqueue       = {0};

// Shared data
volatile uint32_t shared_counter = 0;
volatile uint32_t received_msg   = 0;
volatile uint32_t sensor_data    = 0;

// Task 1 - produces data into queue every 500ms
void task1(void) {
    uint32_t msg = 0;
    while(1) {
        mutex_lock(&shared_mutex);
        shared_counter++;
        mutex_unlock(&shared_mutex);

        queue_send(&myqueue, msg++);
        sem_post(&data_ready);

        os_delay(500);
    }
}

// Task 2 - consumes queue data and waits for semaphore signal
void task2(void) {
    while(1) {
        sem_wait(&data_ready);
        received_msg = queue_receive(&myqueue);

        mutex_lock(&shared_mutex);
        shared_counter++;
        mutex_unlock(&shared_mutex);

        os_delay(100);
    }
}

// Idle task - runs when no other task is ready
void idle_task(void) {
    while(1);
}

int main(void) {
    uint32_t *t1_stack   = os_malloc(STACK_SIZE);
    uint32_t *t2_stack   = os_malloc(STACK_SIZE);
    uint32_t *idle_stk   = os_malloc(STACK_SIZE);

    os_task_create(&tasks[0], task1,     t1_stack,  STACK_SIZE, 2);
    os_task_create(&tasks[1], task2,     t2_stack,  STACK_SIZE, 2);
    os_task_create(&tasks[2], idle_task, idle_stk,  STACK_SIZE, 0);

    os_start();
    while(1);
}