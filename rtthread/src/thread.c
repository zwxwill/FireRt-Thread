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
                        rt_uint8_t        priority,
                        rt_uint32_t tick)
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

	/* 时间片相关 */
	thread->init_tick = tick;
	thread->remaining_tick = tick;

	/* 初始化线程定时器 */
	rt_timer_init(&(thread->thread_timer), /* 静态定时器对象 */
                  thread->name,
                  rt_thread_timeout,
                  thread,
                  0,
                  RT_TIMER_FLAG_ONE_SHOT);	

	return RT_EOK;
} 

rt_err_t rt_thread_suspend(rt_thread_t thread)
{
    register rt_base_t stat;
    register rt_base_t temp;


    if ((thread->stat & RT_THREAD_STAT_MASK) != RT_THREAD_READY)
    {
        return -RT_ERROR;
    }

    /* disable interrupt */
    temp = rt_hw_interrupt_disable();

    /* change thread stat */
	thread->stat = RT_THREAD_SUSPEND  ;
    rt_schedule_remove_thread(thread);
    

    /* stop thread timer anyway */
    rt_timer_stop(&(thread->thread_timer));

    /* enable interrupt */
    rt_hw_interrupt_enable(temp);

    return RT_EOK;
}						

rt_err_t rt_thread_sleep(rt_tick_t tick)
{
    register rt_base_t temp;
    struct rt_thread *thread;

    /* disable interrupt */
    temp = rt_hw_interrupt_disable();
	
    /* set to current thread */
    thread = rt_thread_self();

    /* suspend thread */
    rt_thread_suspend(thread);

    /* reset the timeout of thread timer and start it */
    rt_timer_control(&(thread->thread_timer), RT_TIMER_CTRL_SET_TIME, &tick);
    rt_timer_start(&(thread->thread_timer));

    /* enable interrupt */
    rt_hw_interrupt_enable(temp);

    rt_schedule();

//    /* clear error number of this thread to RT_EOK */
//    if (thread->error == -RT_ETIMEOUT)
//        thread->error = RT_EOK;

    return RT_EOK;
}						

#if 0
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
#else
rt_err_t rt_thread_delay(rt_tick_t tick)
{
	return rt_thread_sleep(tick);
}
#endif
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




void rt_thread_timeout(void *parameter)
{
    struct rt_thread *thread;

    thread = (struct rt_thread *)parameter;
	
    /* set error number */
    thread->error = -RT_ETIMEOUT;

    /* remove from suspend list */
    rt_list_remove(&(thread->tlist));

    /* insert to schedule ready list */
    rt_schedule_insert_thread(thread);

    /* do schedule */
    rt_schedule();
}


