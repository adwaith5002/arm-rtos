    PRESERVE8
	THUMB
	AREA SVC_Code, CODE, READONLY
	EXPORT PendSV_Handler
	IMPORT current_task
	IMPORT next_task
	IMPORT tasks

PendSV_Handler
	MRS     R0, PSP
	STMDB   R0!, {R4-R11}

	LDR     R1, =current_task
	LDR     R2, [R1]
	LDR     R3, =tasks
	MOV     R12, #24
	MUL     R12, R2, R12        ; offset = index * 16 (TCB size)
	STR     R0, [R3, R12]       ; save current SP into TCB

	LDR     R1, =next_task
	LDR     R2, [R1]
	LDR     R1, =current_task
	STR     R2, [R1]

	MOV     R12, #24
	MUL     R12, R2, R12        ; offset = next index * 16
	LDR     R0, [R3, R12]       ; load next SP from TCB

	LDMIA   R0!, {R4-R11}
	MSR     PSP, R0
	MOV     LR, #0xFFFFFFFD
	BX      LR

	END