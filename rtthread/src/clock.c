#include <rthw.h>
#include <rtthread.h>

static rt_tick_t rt_tick = 0; /* 系统时基计数器 */
extern rt_list_t rt_thread_priority_table[RT_THREAD_PRIORITY_MAX];
extern rt_uint32_t rt_thread_ready_priority_group;

void rt_tick_increase(void)
{
	rt_base_t i;
	struct rt_thread *thread;
	rt_tick++;

#if 0
	for(i=0; i<RT_THREAD_PRIORITY_MAX; i++)
	{
		thread = rt_list_entry(rt_thread_priority_table[i].next,
			                   struct rt_thread,
			                   tlist);
		if(thread->remaining_tick > 0)
		{
			thread->remaining_tick--;
		}
	}
#else
	for(i=0; i<RT_THREAD_PRIORITY_MAX; i++)
	{
		thread = rt_list_entry(rt_thread_priority_table[i].next,
			                   struct rt_thread,
			                   tlist);
		if(thread->remaining_tick > 0)
		{
			thread->remaining_tick--;
			if(thread->remaining_tick == 0)
			{
				rt_thread_ready_priority_group |= thread->number_mask;
			}
		}
	}	
#endif
	rt_schedule();
}


/* This is the timer interrupt service routine. */
void SysTick_Handler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    rt_tick_increase();

    /* leave interrupt */
    rt_interrupt_leave();
}



