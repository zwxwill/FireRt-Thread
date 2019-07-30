#include "rtdef.h"
#include "rtconfig.h"
#include "ARMCM3.h"
#include "rtservice.h"
#include "rthw.h"
#include "ARMCM3.h"
#include "rtthread.h"

extern rt_list_t rt_thread_priority_table[RT_THREAD_PRIORITY_MAX];
extern void rt_schedule(void);
extern void rt_system_scheduler_init(void);
extern void rt_system_scheduler_start(void);
extern void rt_thread_delay(rt_tick_t tick);
extern void rt_thread_idle_init(void);
						
						
#define uint32_t  int 
	
ALIGN(RT_ALIGN_SIZE)
/* task stack */
rt_uint8_t rt_flag1_thread_stack[512];
rt_uint8_t rt_flag2_thread_stack[512];
rt_uint8_t rt_flag3_thread_stack[512];


uint32_t flag1;
uint32_t flag2;
uint32_t flag3;


/* �����߳̿��ƿ� */
struct rt_thread rt_flag1_thread;
struct rt_thread rt_flag2_thread;
struct rt_thread rt_flag3_thread;



void delay(uint32_t count)
{
	for(; count!=0; count--);
}

void flag1_thread_entry( void *p_arg )
{
	for(;;)
	{
#if 0
		flag1 = 1;
		delay(100);
		flag1 = 0;
		delay(100);
#else 
		flag1 = 1;
		rt_thread_delay(3);
		flag1 = 0;
		rt_thread_delay(3);
#endif
		/* �߳��л����������ֶ��л� */
//		rt_schedule();
	}
}

void flag2_thread_entry( void *p_arg )
{
	for(;;)
	{
#if 0		
		flag2 = 1;
		delay(100);
		flag2 = 0;
		delay(100);	
#else
		flag2 = 1;
//		rt_thread_delay(2);
		delay( 100 );
		flag2 = 0;
//		rt_thread_delay(2);
		delay( 100 );

#endif
		/* �߳��л����������ֶ��л� */
//		rt_schedule();	
	}		
}

void flag3_thread_entry( void *p_arg )
{
	for(;;)
	{
		flag3 = 1;
//		rt_thread_delay(3);
		delay( 100 );

		flag3 = 0;
	//	rt_thread_delay(3);	
		delay( 100 );
	}
}

	

int main(void)
{
	/* ���ж� */
	rt_hw_interrupt_disable();

	SysTick_Config(SystemCoreClock / RT_TICK_PER_SECOND);
	
	/* ϵͳ��ʱ���б��ʼ�� */
	rt_system_timer_init();

	/* ��������ʼ�� */
	rt_system_scheduler_init();

	/* ��ʼ�������߳� */
	rt_thread_idle_init();

	/* ��ʼ���߳� */
	rt_thread_init( &rt_flag1_thread,
				    (const char *)"thread1",
			        flag1_thread_entry,
			        RT_NULL,
			        &rt_flag1_thread_stack[0],
			        sizeof(rt_flag1_thread_stack),
			        2,
			        4);
	/* ���̲߳��뵽�����б��� */
//	rt_list_insert_before( &(rt_thread_priority_table[0]),&(rt_flag1_thread.tlist) );				
	rt_thread_startup(&rt_flag1_thread);
	
	rt_thread_init( &rt_flag2_thread,
		            (const char *)"thread2",
			        flag2_thread_entry,
			        RT_NULL,
			        &rt_flag2_thread_stack[0],
			        sizeof(rt_flag2_thread_stack),
			        3,
			        2);		
	/* ���̲߳��뵽�����б��� */
//	rt_list_insert_before( &(rt_thread_priority_table[1]),&(rt_flag2_thread.tlist) );	
	rt_thread_startup(&rt_flag2_thread);

	rt_thread_init( &rt_flag3_thread,
		            (const char *)"thread3",
			        flag3_thread_entry,
			        RT_NULL,
			        &rt_flag3_thread_stack[0],
			        sizeof(rt_flag3_thread_stack),
			        3,
			        3);		
	rt_thread_startup(&rt_flag3_thread);


	/* ����ϵͳ������ */
	rt_system_scheduler_start();
}
void SysTick_Handler(void)
{
    /* �����ж� */
    rt_interrupt_enter();

    /* ����ʱ�� */
    rt_tick_increase();

    /* �뿪�ж� */
    rt_interrupt_leave();
}
