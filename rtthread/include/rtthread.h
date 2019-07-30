#ifndef __RT_THREAD_H__
#define __RT_THREAD_H__

#include <rtconfig.h>
#include "rtdef.h"
#include "rtservice.h"
#include "rthw.h"
#include "rtthread.h"


extern char *rt_strncpy(char *dst, const char *src, rt_ubase_t n);

extern void rt_object_init(struct rt_object         *object,
                    enum rt_object_class_type type,
                    const char               *name);
					
					
extern void rt_interrupt_enter(void);
extern void rt_interrupt_leave(void);		

extern void rt_schedule(void);		

extern rt_err_t rt_thread_init(struct rt_thread *thread,
						const char       *name,
                        void (*entry)(void *parameter),
                        void             *parameter,
                        void             *stack_start,
                        rt_uint32_t       stack_size,
						rt_uint8_t        priority,
						rt_uint32_t tick);	
						
extern void rt_schedule_insert_thread(struct rt_thread *thread);						
extern void rt_schedule_remove_thread(struct rt_thread *thread);
extern rt_err_t rt_thread_startup(rt_thread_t thread);
						
extern int __rt_ffs(int value);						

extern rt_tick_t rt_tick_get(void);

extern void rt_thread_timeout(void *parameter);
extern void rt_timer_check(void);
extern void rt_timer_init(rt_timer_t  timer,
				  const char *name,
				  void (*timeout)(void *parameter),
				  void		 *parameter,
				  rt_tick_t   time,
				  rt_uint8_t  flag);						
extern rt_err_t rt_timer_stop(rt_timer_t timer);
extern rt_thread_t rt_thread_self(void);
extern rt_err_t rt_timer_control(rt_timer_t timer, int cmd, void *arg);
extern rt_err_t rt_timer_start(rt_timer_t timer);
extern void rt_system_timer_init(void);
						
#endif

