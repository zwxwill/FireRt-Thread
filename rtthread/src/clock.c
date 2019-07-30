#include <rthw.h>
#include <rtthread.h>

static rt_tick_t rt_tick = 0; /* 系统时基计数器 */
extern rt_list_t rt_thread_priority_table[RT_THREAD_PRIORITY_MAX];
extern rt_uint32_t rt_thread_ready_priority_group;

rt_err_t rt_thread_yield(void)
{
	register rt_base_t level;
	struct rt_thread *thread;

	/* 关中断 */
	level = rt_hw_interrupt_disable();
	
	/* 获取当前线程的线程控制块 */
	thread = rt_thread_self();

	/* 如果线程在就绪态，且同一个优先级下不止一个线程 */
	if ((thread->stat & RT_THREAD_STAT_MASK) == RT_THREAD_READY &&
		thread->tlist.next != thread->tlist.prev)
	{
		/* 将时间片耗完的线程从就绪列表移除 */
		rt_list_remove(&(thread->tlist));

		/* 将线程插入到该优先级下的链表的尾部 */
		rt_list_insert_before(&(rt_thread_priority_table[thread->current_priority]),
		                      &(thread->tlist));
		/* 开中断 */
		rt_hw_interrupt_enable(level);

		/* 执行调度 */
		rt_schedule();

		return RT_EOK;
	}
		
	/* 开中断 */
	rt_hw_interrupt_enable(level);

}




#if 0
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
#else
void rt_tick_increase(void)
{
	struct rt_thread *thread;
	
	rt_tick++;

	/* 获取当前线程线程控制块 */
	thread = rt_thread_self();

	/* 时间片递减 */
	-- thread->remaining_tick;

	/* 如果时间片用完，则重置时间片，然后让出处理器 */
	if (thread->remaining_tick == 0)
	{
		/* 重置时间片 */
		thread->remaining_tick = thread->init_tick;

		/* 让出处理器 */
		rt_thread_yield();
	}
	
	rt_timer_check();
}
#endif



rt_tick_t rt_tick_get(void)
{
    /* return the global tick */
    return rt_tick;
}


