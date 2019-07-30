#include "rtthread.h"
#include "rthw.h"
#include "rtdef.h"

volatile rt_uint8_t rt_interrupt_nest = 0;


void rt_interrupt_enter(void)
{
	rt_base_t level;

    level = rt_hw_interrupt_disable();
    rt_interrupt_nest ++;
	rt_hw_interrupt_enable(level);
}

void rt_interrupt_leave(void)
{
	rt_base_t level;

    level = rt_hw_interrupt_disable();
    rt_interrupt_nest --;
	rt_hw_interrupt_enable(level);
}



