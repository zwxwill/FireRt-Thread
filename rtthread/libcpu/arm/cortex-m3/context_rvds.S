
; �ⲿ����, IMPORT�൱��define
    IMPORT rt_thread_switch_interrupt_flag
    IMPORT rt_interrupt_from_thread
    IMPORT rt_interrupt_to_thread

; EQU �൱�ں궨�� define
SCB_VTOR		EQU 	0xE000ED08				 ; Vector Table Offset Register
NVIC_INT_CTRL	EQU 	0xE000ED04				 ; interrupt control state register
NVIC_SYSPRI2	EQU 	0xE000ED20				 ; system priority register (2)
NVIC_PENDSV_PRI EQU 	0x00FF0000				 ; PendSV priority value (lowest)
NVIC_PENDSVSET	EQU 	0x10000000				 ; value to trigger PendSV exception

;  AREA ���һ���µĴ���λ����ݶ�
    AREA |.text|, CODE, READONLY, ALIGN=2 ; ��ǰ�ļ�ָ�2^2����
    THUMB
    REQUIRE8  
    PRESERVE8

;/*
; * void rt_hw_context_switch_to(rt_uint32 to);
; * r0 --> to
; * this fucntion is used to perform the first thread switch
; */
rt_hw_context_switch_to    PROC    ; PROC���ڶ����ӳ�����ENDP�ɶ�ʹ�� 
	EXPORT rt_hw_context_switch_to ; ���� rt_hw_context_switch_to��ʹ�������C�ļ��б�����
	; set to thread
	LDR 	r1, =rt_interrupt_to_thread  ; r1 = &rt_interrupt_to_thread
	STR 	r0, [r1]  ;�Ĵ���R0 -> �洢�� [r1]

	; set from thread to 0
	LDR 	r1, =rt_interrupt_from_thread
	MOV 	r0, #0x0  ; r0 = 0
	STR 	r0, [r1]  ; [r1] = 0  => rt_interrupt_from_thread = 0

	; set interrupt flag to 1
	LDR 	r1, =rt_thread_switch_interrupt_flag
	MOV 	r0, #1
	STR 	r0, [r1]

	; set the PendSV exception priority
	LDR 	r0, =NVIC_SYSPRI2
	LDR 	r1, =NVIC_PENDSV_PRI
	LDR.W	r2, [r0,#0x00]		 ; read
	ORR 	r1,r1,r2			 ; modify ��λ��
	STR 	r1, [r0]			 ; write-back  NVIC_SYSPRI2 | NVIC_PENDSV_PRI

	; trigger the PendSV exception (causes context switch)
	LDR 	r0, =NVIC_INT_CTRL   
	LDR 	r1, =NVIC_PENDSVSET   
	STR 	r1, [r0]  ; NVIC_INT_CTRL = NVIC_PENDSVSE

	; enable interrupts at processor level
	CPSIE	F
	CPSIE	I

	; never reach here!
	ENDP


; r0 --> switch from thread stack
; r1 --> switch to thread stack
; psr, pc, lr, r12, r3, r2, r1, r0 are pushed into [from] stack
PendSV_Handler	 PROC
	EXPORT PendSV_Handler

	; disable interrupt to protect context switch
	MRS 	r2, PRIMASK  ; �������⹦�ܼĴ�����ֵ��ͨ�üĴ���
	CPSID	I

	; get rt_thread_switch_interrupt_flag
	LDR 	r0, =rt_thread_switch_interrupt_flag
	LDR 	r1, [r0]
	CBZ 	r1, pendsv_exit 		; pendsv already handled  ; �ж�r1�Ƿ�Ϊ0��Ϊ0����ת��pendsv_exit

	; clear rt_thread_switch_interrupt_flag to 0
	MOV 	r1, #0x00
	STR 	r1, [r0]

	LDR 	r0, =rt_interrupt_from_thread
	LDR 	r1, [r0]                ; ��һ���߳��л�ʱrt_interrupt_from_thread�϶�Ϊ0������ת��switch_to_thread
	CBZ 	r1, switch_to_thread	; skip register save at the first time  �ж�r1�Ƿ�Ϊ0��Ϊ0����ת��switch_to_thread


; ========================== ���ı��� ========================

	; ������PendSVC Handlerʱ����һ���߳����еĻ�������
	; xPSR��PC���߳���ڵ�ַ����R14��R12��R3��R2��R1��R0���̵߳��βΣ�
	; ��ЩCPU�Ĵ�����ֵ���Զ����浽�̵߳�ջ�У�ʣ�µ�r4~r11��Ҫ�ֶ�����
    ; ��ȡ�߳�ջָ�뵽r1
	MRS 	r1, psp 				; get from thread stack pointer

	;��CPU�Ĵ���r4~r11��ֵ�洢��r1ָ��ĵ�ַ(ÿ����һ�ε�ַ���ݼ�һ��)
	STMFD	r1!, {r4 - r11} 		; push r4 - r11 register

	; ����r0ָ��ֵ��r0����r0=rt_interrupt_from_thread
	LDR 	r0, [r0]
	STR 	r1, [r0]				; update from thread stack pointer

; ========================== �����л� ==========================
switch_to_thread
	; rt_interrupt_to_thread��һ��ȫ�ֱ��������������߳�ջָ��SP��ָ��
	LDR 	r1, =rt_interrupt_to_thread
	LDR 	r1, [r1]  ; ����rt_interrupt_to_thread��ֵ��r1����spָ��
	LDR 	r1, [r1]				; load thread stack pointer  ; ����rt_interrupt_to_thread��ֵ��r1����sp

    ;���߳�ջָ��r1(����֮ǰ�ȵݼ�)ָ������ݼ��ص�CPU�Ĵ���r4~r11
	LDMFD	r1!, {r4 - r11} 		; pop r4 - r11 register

	MSR 	psp, r1 				; update stack pointer  ;���߳�ջָ����µ�PSP

pendsv_exit
	; restore interrupt
	MSR 	PRIMASK, r2

	ORR 	lr, lr, #0x04  ; ȷ���쳣����ʹ�õ�ջָ����PSP����LR�Ĵ�����λ2ҪΪ1
	; �쳣���أ����ʱ��ջ�е�ʣ�����ݽ����Զ����ص�CPU�Ĵ�����
	; xPSR��PC���߳���ڵ�ַ����R14��R12��R3��R2��R1��R0���̵߳��βΣ�
	; ͬʱPSP��ֵҲ�����£���ָ���߳�ջ��ջ��
	BX		lr
	ENDP


rt_hw_context_switch	PROC
	EXPORT rt_hw_context_switch

	; set rt_thread_switch_interrupt_flag to 1
	LDR 	r2, =rt_thread_switch_interrupt_flag
	LDR 	r3, [r2]
	CMP 	r3, #1  ;r3��1�Ƚϣ������ִ��BEQָ�����ִ��
	BEQ 	_reswitch
	MOV 	r3, #1
	STR 	r3, [r2]

	LDR 	r2, =rt_interrupt_from_thread	; set rt_interrupt_from_thread
	STR 	r0, [r2]

_reswitch
	LDR 	r2, =rt_interrupt_to_thread 	; set rt_interrupt_to_thread
	STR 	r1, [r2]

	LDR 	r0, =NVIC_INT_CTRL				; trigger the PendSV exception (causes context switch)
	LDR 	r1, =NVIC_PENDSVSET
	STR 	r1, [r0]
	BX		LR
	ENDP

;/*
; * rt_base_t rt_hw_interrupt_disable();
; */
rt_hw_interrupt_disable    PROC
    EXPORT  rt_hw_interrupt_disable
    MRS     r0, PRIMASK
    CPSID   I
    BX      LR
    ENDP

;/*
; * void rt_hw_interrupt_enable(rt_base_t level);
; */
rt_hw_interrupt_enable    PROC
    EXPORT  rt_hw_interrupt_enable
    MSR     PRIMASK, r0
    BX      LR
    ENDP	
		
		ALIGN	4  ; ��ǰ�ļ�ָ�����Ҫ��4�ֽڶ���
		
		END  ;����ļ�������ÿ������ļ�����Ҫһ��END

