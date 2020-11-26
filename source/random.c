/*
    Copyright (C) HWPORT.COM
    All rights reserved.
    Author: JAEHYUK CHO <mailto:minzkn@minzkn.com>
*/

#if !defined(__def_hwport_workload_source_random_c__)
# define __def_hwport_workload_source_random_c__ "random.c"

#include "workload.h"

#include <sys/sysinfo.h>
#include <time.h>

#include <pthread.h>

#define def_workload_randomize_constant (0x015a4e35u)
#define workload_randomize_update(m_seed) (((m_seed)*def_workload_randomize_constant)+1u)

static void __workload_srand(unsigned int s_seed);
void workload_srand(unsigned int s_seed);
static int __workload_rand(void);
int workload_rand(void);

void *workload_fill_rand(void *s_data, size_t s_size);

/* ---- */

static pthread_mutex_t g_workload_rand_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_workload_rand_seed_init = 0;
static unsigned int g_workload_rand_seed = 0u;

/* ---- */

static void __workload_srand(unsigned int s_seed)
{
    struct sysinfo s_sysinfo;

    if(sysinfo((struct sysinfo *)(&s_sysinfo)) == 0) {
        g_workload_rand_seed += ((unsigned int)s_sysinfo.uptime);
    }
    
    g_workload_rand_seed += s_seed;
    g_workload_rand_seed = workload_randomize_update(g_workload_rand_seed);
    g_workload_rand_seed_init = 1;
}

void workload_srand(unsigned int s_seed)
{
    pthread_mutex_lock((pthread_mutex_t *)(&g_workload_rand_lock));
    __workload_srand(s_seed);
    pthread_mutex_unlock((pthread_mutex_t *)(&g_workload_rand_lock));
}

static int __workload_rand(void)
{
    int s_result;

    g_workload_rand_seed = workload_randomize_update(g_workload_rand_seed);
    s_result = (int)(((g_workload_rand_seed >> 16) | (g_workload_rand_seed << 16)) >> 1);

    return(s_result);
}

int workload_rand(void)
{
    int s_result;

    pthread_mutex_lock((pthread_mutex_t *)(&g_workload_rand_lock));
    if(g_workload_rand_seed_init == 0) { __workload_srand((unsigned int)time((time_t *)0)); }
    
    s_result = __workload_rand();
    pthread_mutex_unlock((pthread_mutex_t *)(&g_workload_rand_lock));

    return(s_result);
}

void *workload_fill_rand(void *s_data, size_t s_size)
{
    size_t s_offset, s_half_size;
    unsigned short *s_work_ptr;

    if((s_data == NULL) || (s_size <= ((size_t)0u))) { return(s_data); }

    pthread_mutex_lock((pthread_mutex_t *)(&g_workload_rand_lock));
    if(g_workload_rand_seed_init == 0) { __workload_srand((unsigned int)time((time_t *)0)); }

    for(s_offset = (size_t)0u, s_half_size = (s_size >> 1), s_work_ptr = (unsigned short *)s_data;s_offset < s_half_size;s_offset++) {
        s_work_ptr[s_offset] = (unsigned short)__workload_rand(); 
    }
    if((s_size & ((size_t)1u)) !=((size_t)0u)) {
        *(((unsigned char *)s_data) + (s_offset << 1)) = (unsigned char)__workload_rand();
    }
    pthread_mutex_unlock((pthread_mutex_t *)(&g_workload_rand_lock));

    return(s_data);
}

#endif

/* vim: set expandtab: */
/* End of source */
