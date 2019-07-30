#include "rtdef.h"
#include "rtconfig.h"
#include "rtservice.h"
#include "rtthread.h"
#include "rthw.h"

extern struct rt_thread *rt_current_thread;
extern rt_uint32_t rt_thread_ready_priority_group;
extern rt_uint8_t *rt_hw_stack_init(void       *tentry,     /* 线程入口 */
                             void       *parameter,  /* 线程形参 */
                             rt_uint8_t *stack_addr); /* 栈顶地址 */

rt_err_t rt_thread_init(struct rt_thread *thread,
						const char       *name,
                        void (*entry)(void *parameter),
                        void             *parameter,
                        void             *stack_start,
                        rt_uint32_t       stack_size,
                        rt_uint8_t        priority)
{
    /* init thread object */
    rt_object_init((rt_object_t)thread, RT_Object_Class_Thread, name);	

    /* init thread list */
    rt_list_init(&(thread->tlist));

    thread->entry = (void *)entry;
    thread->parameter = parameter;

    /* stack init */
    thread->stack_addr = stack_start;
    thread->stack_size = stack_size;

    thread->sp = (void *)rt_hw_stack_init(thread->entry, 
		                                  thread->parameter,
                                          (rt_uint8_t *)((char *)thread->stack_addr + thread->stack_size - sizeof(rt_ubase_t)));
	
    thread->init_priority    = priority;
    thread->current_priority = priority;
	thread->number_mask = 0;

    /* error and flags */
    thread->error = RT_EOK;
    thread->stat  = RT_THREAD_INIT;

	return RT_EOK;
} 


void rt_thread_delay(rt_tick_t tick)
{
#if 0
	struct rt_thread *thread;

	thread = rt_current_thread;
	thread->remaining_tick = tick;

	rt_schedule();
#else
	register rt_base_t temp;
	struct rt_thread *thread;

	/* 失能中断 */
	temp = rt_hw_interrupt_disable();

	thread = rt_current_thread;
	thread->remaining_tick = tick;

	/* 改变线程状态 */
	thread->stat = RT_THREAD_SUSPEND;
	rt_thread_ready_priority_group &= ~thread->number_mask;

	/* 使能中断 */
	rt_hw_interrupt_enable(temp);

	/* 进行系统调度 */
	rt_schedule();
#endif
}

rt_thread_t rt_thread_self(void)
{
	return rt_current_thread;
}


rt_err_t rt_thread_resume(rt_thread_t thread)
{
	register rt_base_t temp;
	
	if ((thread->stat & RT_THREAD_STAT_MASK) != RT_THREAD_SUSPEND)
	{
		return -RT_ERROR;
	}
	
    /* disable interrupt */
    temp = rt_hw_interrupt_disable();

    /* remove from suspend list */
    rt_list_remove(&(thread->tlist));

    /* enable interrupt */
    rt_hw_interrupt_enable(temp);

    /* insert to schedule ready list */
    rt_schedule_insert_thread(thread);
	
	return RT_EOK;
}



rt_err_t rt_thread_startup(rt_thread_t thread)
{
	thread->current_priority = thread->init_priority;
	thread->number_mask = 1L << thread->current_priority;

    /* change thread stat */
    thread->stat = RT_THREAD_SUSPEND;
    /* then resume it */
    rt_thread_resume(thread);
    if (rt_thread_self() != RT_NULL)
    {
        /* do a scheduling */
        rt_schedule();
    }

    return RT_EOK;	
}


