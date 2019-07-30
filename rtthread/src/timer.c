#include "rtthread.h"
#include "rtdef.h"
#include "rtservice.h"
#include "rthw.h"

/* 硬件定时器列表 */
static rt_list_t rt_timer_list[RT_TIMER_SKIP_LIST_LEVEL];

void rt_system_timer_init(void)
{
	int i;

	for (i = 0; i < sizeof(rt_timer_list) / sizeof(rt_timer_list[0]); i++)
	{
		rt_list_init(rt_timer_list + i);
	}
}

static void _rt_timer_init(rt_timer_t timer,
                           void (*timeout)(void *parameter),
                           void      *parameter,
                           rt_tick_t  time,
                           rt_uint8_t flag)
{
	int i;
	
	 /* set flag */
	 timer->parent.flag  = flag;
	
	 /* set deactivated */
	 timer->parent.flag &= ~RT_TIMER_FLAG_ACTIVATED;
	
	 timer->timeout_func = timeout;
	 timer->parameter	 = parameter;
	
	 timer->timeout_tick = 0;
	 timer->init_tick	 = time;
	
	 /* initialize timer list */
	 for (i = 0; i < RT_TIMER_SKIP_LIST_LEVEL; i++)
	 {
		 rt_list_init(&(timer->row[i]));
	 }
}


void rt_timer_init(rt_timer_t  timer,
				  const char *name,
				  void (*timeout)(void *parameter),
				  void		 *parameter,
				  rt_tick_t   time,
				  rt_uint8_t  flag)
{
	/* timer object initialization */
	rt_object_init((rt_object_t)timer, RT_Object_Class_Timer, name);
	_rt_timer_init(timer, timeout, parameter, time, flag);
}


rt_inline void _rt_timer_remove(rt_timer_t timer)
{
	int i;

	for (i = 0; i < RT_TIMER_SKIP_LIST_LEVEL; i++)
	{
		rt_list_remove(&timer->row[i]);
	}
}

rt_err_t rt_timer_stop(rt_timer_t timer)
{
    register rt_base_t level;


    if (!(timer->parent.flag & RT_TIMER_FLAG_ACTIVATED))
        return -RT_ERROR;

    /* disable interrupt */
    level = rt_hw_interrupt_disable();

    _rt_timer_remove(timer);

    /* enable interrupt */
    rt_hw_interrupt_enable(level);

    /* change stat */
    timer->parent.flag &= ~RT_TIMER_FLAG_ACTIVATED;

    return RT_EOK;
}


rt_err_t rt_timer_control(rt_timer_t timer, int cmd, void *arg)
{
    /* timer check */

    switch (cmd)
    {
    case RT_TIMER_CTRL_GET_TIME:
        *(rt_tick_t *)arg = timer->init_tick;
        break;

    case RT_TIMER_CTRL_SET_TIME:
        timer->init_tick = *(rt_tick_t *)arg;
        break;

    case RT_TIMER_CTRL_SET_ONESHOT:
        timer->parent.flag &= ~RT_TIMER_FLAG_PERIODIC;
        break;

    case RT_TIMER_CTRL_SET_PERIODIC:
        timer->parent.flag |= RT_TIMER_FLAG_PERIODIC;
        break;
    }

    return RT_EOK;
}


rt_err_t rt_timer_start(rt_timer_t timer)
{
    unsigned int row_lvl;
    rt_list_t *timer_list;
    register rt_base_t level;
    rt_list_t *row_head[RT_TIMER_SKIP_LIST_LEVEL];
    unsigned int tst_nr;
    static unsigned int random_nr;

    /* stop timer firstly */
    level = rt_hw_interrupt_disable();
    /* remove timer from list */
    _rt_timer_remove(timer);
    /* change status of timer */
    timer->parent.flag &= ~RT_TIMER_FLAG_ACTIVATED;
    rt_hw_interrupt_enable(level);

    /*
     * get timeout tick,
     * the max timeout tick shall not great than RT_TICK_MAX/2
     */
    timer->timeout_tick = rt_tick_get() + timer->init_tick;

    /* disable interrupt */
    level = rt_hw_interrupt_disable();

    /* insert timer to system timer list */
    timer_list = rt_timer_list;

    row_head[0]  = &timer_list[0];
    for (row_lvl = 0; row_lvl < RT_TIMER_SKIP_LIST_LEVEL; row_lvl++)
    {
        for (; row_head[row_lvl] != timer_list[row_lvl].prev;
             row_head[row_lvl]  = row_head[row_lvl]->next)
        {
            struct rt_timer *t;
            rt_list_t *p = row_head[row_lvl]->next;

            /* fix up the entry pointer */
            t = rt_list_entry(p, struct rt_timer, row[row_lvl]);

            /* If we have two timers that timeout at the same time, it's
             * preferred that the timer inserted early get called early.
             * So insert the new timer to the end the the some-timeout timer
             * list.
             */
            if ((t->timeout_tick - timer->timeout_tick) == 0)
            {
                continue;
            }
            else if ((t->timeout_tick - timer->timeout_tick) < RT_TICK_MAX / 2)
            {
                break;
            }
        }
        if (row_lvl != RT_TIMER_SKIP_LIST_LEVEL - 1)
            row_head[row_lvl + 1] = row_head[row_lvl] + 1;
    }

    /* Interestingly, this super simple timer insert counter works very very
     * well on distributing the list height uniformly. By means of "very very
     * well", I mean it beats the randomness of timer->timeout_tick very easily
     * (actually, the timeout_tick is not random and easy to be attacked). */
    random_nr++;
    tst_nr = random_nr;

    rt_list_insert_after(row_head[RT_TIMER_SKIP_LIST_LEVEL - 1],
                         &(timer->row[RT_TIMER_SKIP_LIST_LEVEL - 1]));
	
    for (row_lvl = 2; row_lvl <= RT_TIMER_SKIP_LIST_LEVEL; row_lvl++)
    {
        if (!(tst_nr & RT_TIMER_SKIP_LIST_MASK))
            rt_list_insert_after(row_head[RT_TIMER_SKIP_LIST_LEVEL - row_lvl],
                                 &(timer->row[RT_TIMER_SKIP_LIST_LEVEL - row_lvl]));
        else
            break;
        /* Shift over the bits we have tested. Works well with 1 bit and 2
         * bits. */
        tst_nr >>= (RT_TIMER_SKIP_LIST_MASK + 1) >> 1;
    }

    timer->parent.flag |= RT_TIMER_FLAG_ACTIVATED;

    /* enable interrupt */
    rt_hw_interrupt_enable(level);

    return -RT_EOK;
}


void rt_timer_check(void)
{
    struct rt_timer *t;
    rt_tick_t current_tick;
    register rt_base_t level;

    current_tick = rt_tick_get();

    /* disable interrupt */
    level = rt_hw_interrupt_disable();

    while (!rt_list_isempty(&rt_timer_list[RT_TIMER_SKIP_LIST_LEVEL - 1]))
    {
        t = rt_list_entry(rt_timer_list[RT_TIMER_SKIP_LIST_LEVEL - 1].next,
                          struct rt_timer, row[RT_TIMER_SKIP_LIST_LEVEL - 1]);

        /*
         * It supposes that the new tick shall less than the half duration of
         * tick max.
         */
        if ((current_tick - t->timeout_tick) < RT_TICK_MAX / 2)
        {
            /* remove timer from timer list firstly */
            _rt_timer_remove(t);

            /* call timeout function */
            t->timeout_func(t->parameter);

            /* re-get tick */
            current_tick = rt_tick_get();

            if ((t->parent.flag & RT_TIMER_FLAG_PERIODIC) &&
                (t->parent.flag & RT_TIMER_FLAG_ACTIVATED))
            {
                /* start it */
                t->parent.flag &= ~RT_TIMER_FLAG_ACTIVATED;
                rt_timer_start(t);
            }
            else
            {
                /* stop timer */
                t->parent.flag &= ~RT_TIMER_FLAG_ACTIVATED;
            }
        }
        else
            break;
    }

    /* enable interrupt */
    rt_hw_interrupt_enable(level);
}




