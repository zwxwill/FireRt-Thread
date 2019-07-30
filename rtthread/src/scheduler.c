#include "rtdef.h"
#include "rtconfig.h"
#include "rtservice.h"
#include "rthw.h"
#include "rtthread.h"

/* 线程就绪列表 */
rt_list_t rt_thread_priority_table[RT_THREAD_PRIORITY_MAX];

struct rt_thread *rt_current_thread;  /* 当前正在运行的线程控制块 */

extern struct rt_thread rt_flag1_thread;
extern struct rt_thread rt_flag2_thread;

extern struct rt_thread idle;

/* 线程休眠列表 */
rt_list_t rt_thread_defunct;

/* 线程就绪优先级组 */
rt_uint32_t rt_thread_ready_priority_group;

/* 当前优先级 */
rt_uint8_t rt_current_priority;


void rt_system_scheduler_init(void)
{
	/* register 关键字暗示编译程序相应的变量将会被频繁的使用，如果可能的话，
	   应将其保存在 CPU 的寄存器中，以加快其存取速度 */
	register rt_base_t offset;
	
    for (offset = 0; offset < RT_THREAD_PRIORITY_MAX; offset++)
    {
        rt_list_init(&rt_thread_priority_table[offset]);
    }

	/* 初始化当前优先级 */
	rt_current_priority = RT_THREAD_PRIORITY_MAX - 1;

	/* 初始化当前线程控制块指针 */
	rt_current_thread = RT_NULL;
	
	rt_thread_ready_priority_group = 0;
} 


void rt_system_scheduler_start(void)
{
#if 0
	register struct rt_thread *to_thread;

	to_thread = rt_list_entry(rt_thread_priority_table[0].next,
		                      struct rt_thread,
		                      tlist);
	rt_current_thread = to_thread;
	/* 切换到第一个线程，该函数在context_rvds.S中实现
	   在rthw.h声明，用于实现第一次线程切换，
	   当一个汇编函数在C文件中调用的时候，如果有形参，
	   则执行的时候会将形参传入CPU寄存器0 */
	rt_hw_context_switch_to((rt_uint32_t)&to_thread->sp);
#else
	register struct rt_thread *to_thread;
	register rt_ubase_t highest_ready_priority;

	/* 获取就绪的最高优先级*/
	highest_ready_priority = __rt_ffs(rt_thread_ready_priority_group) - 1;
	
	/* 获取将要运行线程的线程控制块 */		
	to_thread = rt_list_entry(rt_thread_priority_table[highest_ready_priority].next,
                              struct rt_thread,
                              tlist);
	
	rt_current_thread = to_thread;
	
	/* 切换到新的线程 */
	rt_hw_context_switch_to((rt_uint32_t)&to_thread->sp);
	
	/* 永远不会返回 */
#endif
}

void rt_schedule_insert_thread(struct rt_thread *thread)
{
	register rt_base_t level;

    /* disable interrupt */
    level = rt_hw_interrupt_disable();
	
    /* 改变线程状态 */
    thread->stat = RT_THREAD_READY;	

    /* insert thread to ready list */
    rt_list_insert_before(&(rt_thread_priority_table[thread->current_priority]),
                          &(thread->tlist));	

    rt_thread_ready_priority_group |= thread->number_mask;						  

    /* enable interrupt */
    rt_hw_interrupt_enable(level);	
}



void rt_schedule_remove_thread(struct rt_thread *thread)
{
	register rt_base_t level;

    /* disable interrupt */
    level = rt_hw_interrupt_disable();

    /* remove thread from ready list */
    rt_list_remove(&(thread->tlist));

	if (rt_list_isempty(&(rt_thread_priority_table[thread->current_priority])))
	{
		rt_thread_ready_priority_group &= ~thread->number_mask;
	}

    /* enable interrupt */
    rt_hw_interrupt_enable(level);		
}


void rt_schedule(void)
{
	struct rt_thread *to_thread;
	struct rt_thread *from_thread;

#if 0
	if ( rt_current_thread == rt_list_entry( rt_thread_priority_table[0].next,
		                                     struct rt_thread,
		                                     tlist) )
	{
		from_thread = rt_current_thread;
		to_thread = rt_list_entry( rt_thread_priority_table[1].next,
                                   struct rt_thread,
                                   tlist);
		rt_current_thread = to_thread;
	}
	else
	{
		from_thread = rt_current_thread;
		to_thread = rt_list_entry( rt_thread_priority_table[0].next,
                                   struct rt_thread,
                                   tlist);
		rt_current_thread = to_thread;

	}
	/* 产生上下文切换 */
	rt_hw_context_switch((rt_uint32_t)&from_thread->sp,(rt_uint32_t)&to_thread->sp);	
#endif

#if 0
	if(rt_current_thread == &idle)
	{
		if(rt_flag1_thread.remaining_tick == 0)
		{
			from_thread = rt_current_thread;
			to_thread   = &rt_flag1_thread;
			rt_current_thread = to_thread;
		}
		else if(rt_flag2_thread.remaining_tick == 0)
		{
			from_thread = rt_current_thread;
			to_thread = &rt_flag2_thread;
			rt_current_thread = to_thread;
		}
		else
		{
			return;
		}
	}
	else
	{
		if(rt_current_thread == &rt_flag1_thread)
		{
			if(rt_flag2_thread.remaining_tick == 0)
			{
				from_thread = rt_current_thread;
				to_thread = &rt_flag2_thread;
				rt_current_thread = to_thread;
			}
			else if(rt_current_thread->remaining_tick != 0)
			{
				from_thread = rt_current_thread;
				to_thread = &idle;
				rt_current_thread = to_thread;
			}
			else
			{
				return;
			}
		}
		else if(rt_current_thread == &rt_flag2_thread)
		{
			if(rt_flag1_thread.remaining_tick == 0)
			{
				from_thread = rt_current_thread;
				to_thread = &rt_flag1_thread;
				rt_current_thread = to_thread;
			}
			else if(rt_current_thread->remaining_tick != 0)
			{
				from_thread = rt_current_thread;
				to_thread = &idle;
				rt_current_thread = to_thread;
			}
			else
			{
				return;
			}
		}
	}
	/* 产生上下文切换 */
	rt_hw_context_switch((rt_uint32_t)&from_thread->sp,(rt_uint32_t)&to_thread->sp);
#else
	rt_base_t level;
	register rt_base_t highest_ready_priority;

	/* 关中断 */
	level = rt_hw_interrupt_disable();
	
    /* 获取就绪的最高优先级 */
    highest_ready_priority = __rt_ffs(rt_thread_ready_priority_group) - 1;
	
	/* 获取就绪的最高优先级对应的线程控制块 */
	to_thread = rt_list_entry(rt_thread_priority_table[highest_ready_priority].next,
	                          struct rt_thread,
	                          tlist);
	
	/* 如果目标线程不是当前线程，则要进行线程切换 */
	if (to_thread != rt_current_thread)
	{
		rt_current_priority = (rt_uint8_t)highest_ready_priority;
		from_thread = rt_current_thread;
		rt_current_thread = to_thread;

		rt_hw_context_switch((rt_uint32_t)&from_thread->sp,
			                 (rt_uint32_t)&to_thread->sp);
		/* 开中断 */
		rt_hw_interrupt_enable(level);					 
	}
	else
	{
		/* 开中断 */
		rt_hw_interrupt_enable(level);			
	}
#endif
}

















