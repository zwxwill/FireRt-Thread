
#ifndef __RT_HW_H__
#define __RT_HW_H__

#include "rtdef.h"

void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to);
void rt_hw_context_switch_to(rt_ubase_t to);
void rt_hw_interrupt_enable(rt_base_t level);
rt_base_t rt_hw_interrupt_disable(void);

#endif
