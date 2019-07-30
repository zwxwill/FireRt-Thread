#ifndef __RT_SERVICE_H__
#define __RT_SERVICE_H__

#include "rtdef.h"

rt_inline void rt_list_init(rt_list_t *l)
{
    l->next = l->prev = l;
}

/* ����ͷ l 
 * ������ n
 * ����nλ��l����
*/
rt_inline void rt_list_insert_after(rt_list_t *l, rt_list_t *n)
{
    l->next->prev = n;
    n->next = l->next;

    l->next = n;
    n->prev = l;
}

/* ����nλ��l֮ǰ */
rt_inline void rt_list_insert_before(rt_list_t *l, rt_list_t *n)
{
    l->prev->next = n;
    n->prev = l->prev;

    l->prev = n;
    n->next = l;
}

/*  */
rt_inline void rt_list_remove(rt_list_t *n)
{
    n->next->prev = n->prev;
    n->prev->next = n->next;

    n->next = n->prev = n;
}

rt_inline int rt_list_isempty(const rt_list_t *l)
{
    return l->next == l;
}

/* ��֪һ���ṹ���Ա�ĵ�ַ�����Ƴ��ýṹ���׵�ַ */
#define rt_container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))
    
#define rt_list_entry(node, type, member) \
    rt_container_of(node, type, member)



#endif

