#include <rthw.h>
#include <rtthread.h>

static rt_tick_t rt_tick = 0; /* ϵͳʱ�������� */
extern rt_list_t rt_thread_priority_table[RT_THREAD_PRIORITY_MAX];
extern rt_uint32_t rt_thread_ready_priority_group;

rt_err_t rt_thread_yield(void)
{
	register rt_base_t level;
	struct rt_thread *thread;

	/* ���ж� */
	level = rt_hw_interrupt_disable();
	
	/* ��ȡ��ǰ�̵߳��߳̿��ƿ� */
	thread = rt_thread_self();

	/* ����߳��ھ���̬����ͬһ�����ȼ��²�ֹһ���߳� */
	if ((thread->stat & RT_THREAD_STAT_MASK) == RT_THREAD_READY &&
		thread->tlist.next != thread->tlist.prev)
	{
		/* ��ʱ��Ƭ������̴߳Ӿ����б��Ƴ� */
		rt_list_remove(&(thread->tlist));

		/* ���̲߳��뵽�����ȼ��µ������β�� */
		rt_list_insert_before(&(rt_thread_priority_table[thread->current_priority]),
		                      &(thread->tlist));
		/* ���ж� */
		rt_hw_interrupt_enable(level);

		/* ִ�е��� */
		rt_schedule();

		return RT_EOK;
	}
		
	/* ���ж� */
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

	/* ��ȡ��ǰ�߳��߳̿��ƿ� */
	thread = rt_thread_self();

	/* ʱ��Ƭ�ݼ� */
	-- thread->remaining_tick;

	/* ���ʱ��Ƭ���꣬������ʱ��Ƭ��Ȼ���ó������� */
	if (thread->remaining_tick == 0)
	{
		/* ����ʱ��Ƭ */
		thread->remaining_tick = thread->init_tick;

		/* �ó������� */
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


