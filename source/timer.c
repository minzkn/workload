/*
    Copyright (C) HWPORT.COM
    All rights reserved.
    Author: JAEHYUK CHO <mailto:minzkn@minzkn.com>
*/

#if !defined(__def_hwport_workload_source_timer_c__)
# define __def_hwport_workload_source_timer_c__ "timer.c"

#include "workload.h"

#include <sys/times.h>
#include <sys/select.h>
#include <unistd.h>

#include <pthread.h>

void workload_sleep_wait(int s_sec, int s_usec);

static unsigned long long workload_time_stamp_msec_private(workload_timer_t *s_timer);
unsigned long long workload_time_stamp_msec(workload_timer_t *s_timer);

void workload_init_timer(workload_timer_t *s_timer, unsigned long long s_timeout);
void workload_set_timer(workload_timer_t *s_timer, unsigned long long s_timeout);
void workload_update_timer(workload_timer_t *s_timer, unsigned long long s_timeout);
int workload_check_timer_ex(workload_timer_t *s_timer, unsigned long long *s_remain);
int workload_check_timer(workload_timer_t *s_timer);

void workload_sleep_wait(int s_sec, int s_usec)
{
#if (defined(__GNUC__) || defined(__UCLIBC__)) && (!defined(__STDC__))
    s_sec += s_usec / 1000000;
    s_usec = s_usec % 1000000;
    
    if(s_sec > 0)(void)sleep((unsigned int)s_sec); /* POSIX v1.0 */
    if(s_usec > 0)(void)usleep((unsigned int)s_usec); /* BSD v4.3 */
#elif defined(EINTR)
    int s_check;
    struct timeval s_timeval;
    
    s_timeval.tv_sec = (time_t)(s_sec + (s_usec / 1000000));
    s_timeval.tv_usec = (suseconds_t)(s_usec % 1000000);

    do {
        s_check = select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, (struct timeval *)(&s_timeval));
    }while((s_check == (-1)) && (errno == EINTR) && ((s_timeval.tv_sec > ((time_t)0l)) || (s_timeval.tv_usec > ((suseconds_t)0l))));
#else
    struct timeval s_timeval;
    
    s_timeval.tv_sec = (long)(s_sec + (s_usec / 1000000));
    s_timeval.tv_usec = (long)(s_usec % 1000000);
    
    (void)select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, (struct timeval *)(&s_timeval));
#endif
}

static unsigned long long workload_time_stamp_msec_private(workload_timer_t *s_timer)
{
    clock_t s_clock;

#if defined(__linux__)
    /* get linux kernel's jiffes (tick counter) */
    s_clock = times((struct tms *)0);
#else
    do { struct tms s_tms; s_clock = times((struct tms *)(&s_tms)); }while(0);
#endif    
    
    if(s_clock == ((clock_t)(-1))) { /* overflow clock timing */ return(s_timer->m_time_stamp); }

    if(s_timer->m_clock_tick <= 0l) {
        /* get ticks per second */ 
        s_timer->m_clock_tick = sysconf(_SC_CLK_TCK);
        if(s_timer->m_clock_tick <= 0l) { /* invalid clock tick */ return(s_timer->m_time_stamp); }
        s_timer->m_prev_clock = s_clock;
    }

    /* update time stamp (clock to upscale) */
    s_timer->m_time_stamp += (((unsigned long long)(s_clock - s_timer->m_prev_clock)) * 1000ull) / ((unsigned long long)s_timer->m_clock_tick);
    s_timer->m_prev_clock = s_clock;

    return(s_timer->m_time_stamp);
}

unsigned long long workload_time_stamp_msec(workload_timer_t *s_timer)
{
    static pthread_mutex_t sg_lock = PTHREAD_MUTEX_INITIALIZER;
    static workload_timer_t sg_timer = def_workload_init_timer;
    
    unsigned long long s_result;
    
    if(s_timer == NULL) { /* global timer */
        pthread_mutex_lock((pthread_mutex_t *)(&sg_lock));
        s_result = workload_time_stamp_msec_private((workload_timer_t *)(&sg_timer));
        pthread_mutex_unlock((pthread_mutex_t *)(&sg_lock));

        return(s_result);
    }

    return(workload_time_stamp_msec_private(s_timer));
}

void workload_init_timer(workload_timer_t *s_timer, unsigned long long s_timeout)
{
    static workload_timer_t sg_init_timer = def_workload_init_timer;

    /* timer stat part */
    workload_set_timer((workload_timer_t *)memcpy((void *)s_timer, (const void *)(&sg_init_timer), sizeof(sg_init_timer)), s_timeout); 
}

void workload_set_timer(workload_timer_t *s_timer, unsigned long long s_timeout)
{
    s_timer->m_start_time_stamp = workload_time_stamp_msec_private(s_timer);
    s_timer->m_timeout = s_timeout;
    s_timer->m_duration = 0ull;
}

void workload_update_timer(workload_timer_t *s_timer, unsigned long long s_timeout)
{
    /* increment timeout */
    s_timer->m_timeout += s_timeout;
}

int workload_check_timer_ex(workload_timer_t *s_timer, unsigned long long *s_remain)
{
    s_timer->m_duration = workload_time_stamp_msec_private(s_timer) - s_timer->m_start_time_stamp; 

    if(s_timer->m_duration >= s_timer->m_timeout) {
        if(s_remain != ((unsigned long long *)0)) { *s_remain = (unsigned long long)0u; }
        return(1);
    }

    if(s_remain != ((unsigned long long *)0)) { *s_remain = s_timer->m_timeout - s_timer->m_duration; }
    
    return(0);
}

int workload_check_timer(workload_timer_t *s_timer)
{
    return(workload_check_timer_ex(s_timer, (unsigned long long *)0));
}

#endif

/* vim: set expandtab: */
/* End of source */
