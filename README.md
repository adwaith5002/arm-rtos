# ARM RTOS

A preemptive Real-Time Operating System kernel built from scratch for ARM Cortex-M in C and assembly.

## Features

- Preemptive round-robin scheduler with priority support
- Context switching via PendSV handler in ARM Thumb assembly
- Task management with READY, RUNNING, BLOCKED states
- `os_delay()` for tick-based task sleeping
- Mutex with ownership tracking
- Counting semaphore
- Message queue (circular buffer, blocking send/receive)
- Memory pool allocator (bump allocator, no fragmentation)
- Stack overflow detection via canary values
- Idle task for low-activity periods
- Dynamic task creation (up to 16 tasks)

## Architecture
- os.h      — API, structs, defines
- os.c      — kernel implementation
- os_asm.s  — PendSV context switch (ARM assembly)
- main.c    — demo application

## How Context Switching Works

1. SysTick fires every 1ms
2. SysTick runs the scheduler, picks next task, triggers PendSV
3. PendSV saves current task's R4-R11 onto its stack, saves PSP into TCB
4. PendSV loads next task's PSP from TCB, restores R4-R11
5. BX LR with EXC_RETURN 0xFFFFFFFD resumes next task in Thread mode

The hardware automatically saves R0-R3, R12, LR, PC, xPSR on exception entry.
PendSV runs at lowest priority so it never preempts other interrupts.

## Demo Application

The demo runs three tasks:
- **Task 1** — locks mutex, increments shared counter, sends to queue, posts semaphore, sleeps 500ms
- **Task 2** — waits on semaphore, receives from queue, locks mutex, increments counter, sleeps 100ms
- **Idle task** — runs when no other task is ready

## How to Build and Run

### Requirements
- Keil uVision 5
- QEMU (qemu-system-arm)
- ARM Compiler V6

### Build
Open `rtos_qemu.uvprojx` in Keil uVision and press F7.

### Run in QEMU
```bash
qemu-system-arm -machine mps2-an385 -cpu cortex-m3 -kernel Objects/rtos_qemu.axf -nographic
```

### Run in Keil Simulator
1. Open project in Keil
2. Debug → Start/Stop Debug Session (Ctrl+F5)
3. Press F5 to run
4. Watch `received_msg`, `shared_counter` in Watch window

## Target Hardware

Currently targeting ARM Cortex-M3 (QEMU mps2-an385).
Designed for STM32F401 (Cortex-M4) — retargeting requires:
- Change device to STM32F401CCUx in Keil
- Update `#include "ARMCM3.h"` to `#include "stm32f4xx.h"`
- Adjust `SysTick_Config` for STM32 clock speed

## Known Limitations

- No priority inheritance in mutex (priority inversion possible)
- Memory pool has no free() — bump allocator only
- No task deletion
- No timeout on mutex/semaphore wait
- Single core only
- No MPU support