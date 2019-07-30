
; 外部变量, IMPORT相当于define
    IMPORT rt_thread_switch_interrupt_flag
    IMPORT rt_interrupt_from_thread
    IMPORT rt_interrupt_to_thread

; EQU 相当于宏定义 define
SCB_VTOR		EQU 	0xE000ED08				 ; Vector Table Offset Register
NVIC_INT_CTRL	EQU 	0xE000ED04				 ; interrupt control state register
NVIC_SYSPRI2	EQU 	0xE000ED20				 ; system priority register (2)
NVIC_PENDSV_PRI EQU 	0x00FF0000				 ; PendSV priority value (lowest)
NVIC_PENDSVSET	EQU 	0x10000000				 ; value to trigger PendSV exception

;  AREA 汇编一个新的代码段或数据段
    AREA |.text|, CODE, READONLY, ALIGN=2 ; 当前文件指令按2^2对齐
    THUMB
    REQUIRE8  
    PRESERVE8

;/*
; * void rt_hw_context_switch_to(rt_uint32 to);
; * r0 --> to
; * this fucntion is used to perform the first thread switch
; */
rt_hw_context_switch_to    PROC    ; PROC用于定义子程序，与ENDP成对使用 
	EXPORT rt_hw_context_switch_to ; 导出 rt_hw_context_switch_to，使其可以在C文件中被调用
	; set to thread
	LDR 	r1, =rt_interrupt_to_thread  ; r1 = &rt_interrupt_to_thread
	STR 	r0, [r1]  ;寄存器R0 -> 存储器 [r1]

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
	ORR 	r1,r1,r2			 ; modify 按位或
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
	MRS 	r2, PRIMASK  ; 加载特殊功能寄存器的值到通用寄存器
	CPSID	I

	; get rt_thread_switch_interrupt_flag
	LDR 	r0, =rt_thread_switch_interrupt_flag
	LDR 	r1, [r0]
	CBZ 	r1, pendsv_exit 		; pendsv already handled  ; 判断r1是否为0，为0则跳转到pendsv_exit

	; clear rt_thread_switch_interrupt_flag to 0
	MOV 	r1, #0x00
	STR 	r1, [r0]

	LDR 	r0, =rt_interrupt_from_thread
	LDR 	r1, [r0]                ; 第一次线程切换时rt_interrupt_from_thread肯定为0，则跳转到switch_to_thread
	CBZ 	r1, switch_to_thread	; skip register save at the first time  判断r1是否为0，为0则跳转到switch_to_thread


; ========================== 上文保存 ========================

	; 当进入PendSVC Handler时，上一个线程运行的环境即：
	; xPSR，PC（线程入口地址），R14，R12，R3，R2，R1，R0（线程的形参）
	; 这些CPU寄存器的值会自动保存到线程的栈中，剩下的r4~r11需要手动保存
    ; 获取线程栈指针到r1
	MRS 	r1, psp 				; get from thread stack pointer

	;将CPU寄存器r4~r11的值存储到r1指向的地址(每操作一次地址将递减一次)
	STMFD	r1!, {r4 - r11} 		; push r4 - r11 register

	; 加载r0指向值到r0，即r0=rt_interrupt_from_thread
	LDR 	r0, [r0]
	STR 	r1, [r0]				; update from thread stack pointer

; ========================== 下文切换 ==========================
switch_to_thread
	; rt_interrupt_to_thread是一个全局变量，里面存的是线程栈指针SP的指针
	LDR 	r1, =rt_interrupt_to_thread
	LDR 	r1, [r1]  ; 加载rt_interrupt_to_thread的值到r1，即sp指针
	LDR 	r1, [r1]				; load thread stack pointer  ; 加载rt_interrupt_to_thread的值到r1，即sp

    ;将线程栈指针r1(操作之前先递减)指向的内容加载到CPU寄存器r4~r11
	LDMFD	r1!, {r4 - r11} 		; pop r4 - r11 register

	MSR 	psp, r1 				; update stack pointer  ;将线程栈指针更新到PSP

pendsv_exit
	; restore interrupt
	MSR 	PRIMASK, r2

	ORR 	lr, lr, #0x04  ; 确保异常返回使用的栈指针是PSP，即LR寄存器的位2要为1
	; 异常返回，这个时候栈中的剩下内容将会自动加载到CPU寄存器：
	; xPSR，PC（线程入口地址），R14，R12，R3，R2，R1，R0（线程的形参）
	; 同时PSP的值也将更新，即指向线程栈的栈顶
	BX		lr
	ENDP


rt_hw_context_switch	PROC
	EXPORT rt_hw_context_switch

	; set rt_thread_switch_interrupt_flag to 1
	LDR 	r2, =rt_thread_switch_interrupt_flag
	LDR 	r3, [r2]
	CMP 	r3, #1  ;r3与1比较，相等则执行BEQ指令，否则不执行
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
		
		ALIGN	4  ; 当前文件指令代码要求4字节对齐
		
		END  ;汇编文件结束，每个汇编文件都需要一个END


