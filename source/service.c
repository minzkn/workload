/*
    Copyright (C) HWPORT.COM
    All rights reserved.
    Author: JAEHYUK CHO <mailto:minzkn@minzkn.com>
*/

#if !defined(__def_hwport_workload_source_service_c__)
# define __def_hwport_workload_source_service_c__ "service.c"

#include "workload.h"

#include <pthread.h>

#if !defined(workload_thread_ctx_t)
typedef struct workload_thread_ctx_ts {
    workload_thread_handler_t m_handler;
    void *m_argument;
}__workload_thread_ctx_t;
# define workload_thread_ctx_t __workload_thread_ctx_t
#endif

#define def_workload_service_ctrl_none (0x00000000u)
#define def_workload_service_ctrl_error (0x00000001u)
#define def_workload_service_ctrl_ack (0x00000002u)
#define def_workload_service_ctrl_nack (0x00000003u)
#define def_workload_service_ctrl_ready (0x00000004u)
#define def_workload_service_ctrl_start (0x00000005u)
#define def_workload_service_ctrl_stop (0x00000006u)

#define def_workload_service_timeout_msec (60000u)

#if !defined(workload_service_ctx_t)
typedef struct workload_service_ctx_ts {
    pthread_mutex_t m_lock;

    char *m_name;
    
    volatile unsigned int m_command;
    volatile unsigned int m_status;

    void *m_argument;
    size_t m_stack_size;
    workload_service_handler_t m_handler;
}__workload_service_ctx_t;
# define workload_service_ctx_t __workload_service_ctx_t
#endif

static void *workload_thread_handler(void *s_argument);

int workload_detached_thread_ex(workload_thread_handler_t s_thread_handler, void *s_argument, size_t s_stack_size);

static int workload_service_thread(void *s_argument);

int workload_check_service(void *s_service_handle);
int workload_break_service(void *s_service_handle);
void workload_ack_service(void *s_service_handle);
void workload_error_service(void *s_service_handle);

void *workload_open_service_ex(const char *s_name, workload_service_handler_t s_handler, void *s_argument, size_t s_stack_size);
void *workload_open_service(const char *s_name, workload_service_handler_t s_handler, void *s_argument);
void *workload_close_service_ex(void *s_service_handle, int s_timeout_msec);
void *workload_close_service(void *s_service_handle);

static void *workload_thread_handler(void *s_argument)
{
    workload_thread_ctx_t *s_thread_ctx = (workload_thread_ctx_t *)s_argument;

    if(s_thread_ctx == NULL) { return(NULL); }

    (void)s_thread_ctx->m_handler(s_thread_ctx->m_argument);
    free((void *)s_thread_ctx);

    return(NULL);
}

int workload_detached_thread_ex(workload_thread_handler_t s_thread_handler, void *s_argument, size_t s_stack_size)
{
    int s_result;

    workload_thread_ctx_t *s_thread_ctx;
    pthread_t s_thread_handle;
    pthread_attr_t s_thread_attr;
    size_t s_current_stack_size;

    if(s_thread_handler == ((workload_thread_handler_t)0)) { return(-1); }

    s_thread_ctx = (workload_thread_ctx_t *)malloc(sizeof(workload_thread_ctx_t));
    if(s_thread_ctx == NULL) { return(-1); }
    s_thread_ctx->m_handler = s_thread_handler;
    s_thread_ctx->m_argument = s_argument;

    if(pthread_attr_init((pthread_attr_t *)(&s_thread_attr)) != 0) {
        free((void *)s_thread_ctx);
        return(-1);
    }

    s_current_stack_size = (size_t)0u;
    if(pthread_attr_getstacksize((pthread_attr_t *)(&s_thread_attr), (size_t *)(&s_current_stack_size)) == 0) {
        if(s_stack_size <= ((size_t)0u)) {
            if(s_current_stack_size < ((size_t)(4u << 10))) { s_stack_size = (size_t)(4u << 10); }
        }
        else if(s_stack_size == ((size_t)s_current_stack_size)) { s_stack_size = (size_t)0u; }
        if(s_stack_size > ((size_t)0u)) { (void)pthread_attr_setstacksize((pthread_attr_t *)(&s_thread_attr), (size_t)s_stack_size); }
    }

    if(pthread_attr_setdetachstate((pthread_attr_t *)(&s_thread_attr), PTHREAD_CREATE_DETACHED) != 0) {
        free((void *)s_thread_ctx);
        s_result = (-1);
    }
    else if(pthread_create((pthread_t *)(&s_thread_handle), (pthread_attr_t *)(&s_thread_attr), workload_thread_handler, (void *)s_thread_ctx) != 0) {
        free((void *)s_thread_ctx);
        s_result = (-1);
    }
    else { /* ok! thread detached. */ s_result = 0; }

    (void)pthread_attr_destroy((pthread_attr_t *)(&s_thread_attr));

    return(s_result);
}

static int workload_service_thread(void *s_argument)
{
    workload_service_ctx_t *s_service_ctx = (workload_service_ctx_t *)s_argument;

    pthread_mutex_lock((pthread_mutex_t *)(&s_service_ctx->m_lock));
    s_service_ctx->m_command = def_workload_service_ctrl_ack;
    pthread_mutex_unlock((pthread_mutex_t *)(&s_service_ctx->m_lock));

    (*s_service_ctx->m_handler)((void *)s_service_ctx, s_service_ctx->m_argument);

    pthread_mutex_lock((pthread_mutex_t *)(&s_service_ctx->m_lock));
    if(s_service_ctx->m_status != def_workload_service_ctrl_error) { s_service_ctx->m_status = def_workload_service_ctrl_stop; }
    pthread_mutex_unlock((pthread_mutex_t *)(&s_service_ctx->m_lock));

    return(0);
}

int workload_check_service(void *s_service_handle)
{
    int s_result;
    workload_service_ctx_t *s_service_ctx = (workload_service_ctx_t *)s_service_handle;

    if(s_service_ctx == NULL) { return(-1); }
        
    pthread_mutex_lock((pthread_mutex_t *)(&s_service_ctx->m_lock));
    if(s_service_ctx->m_status == def_workload_service_ctrl_start) { /* started service */ s_result = 1; }
    else if(s_service_ctx->m_status == def_workload_service_ctrl_error) { /* error service */ s_result = (-1); }
    else { /* stopped service */ s_result = 0; }
    pthread_mutex_unlock((pthread_mutex_t *)(&s_service_ctx->m_lock));

    return(s_result);
}

int workload_break_service(void *s_service_handle)
{
    int s_result;
    workload_service_ctx_t *s_service_ctx = (workload_service_ctx_t *)s_service_handle;

    if(s_service_ctx == NULL) { return(-1); }
        
    pthread_mutex_lock((pthread_mutex_t *)(&s_service_ctx->m_lock));
    if(s_service_ctx->m_command == def_workload_service_ctrl_stop) { /* stopped service */ s_result = 1; }
    else { /* running service */ s_result = 0; }
    pthread_mutex_unlock((pthread_mutex_t *)(&s_service_ctx->m_lock));

    return(s_result);
}

void workload_ack_service(void *s_service_handle)
{
    workload_service_ctx_t *s_service_ctx = (workload_service_ctx_t *)s_service_handle;

    if(s_service_ctx == NULL) { return; }
        
    pthread_mutex_lock((pthread_mutex_t *)(&s_service_ctx->m_lock));
    s_service_ctx->m_status = def_workload_service_ctrl_start;
    pthread_mutex_unlock((pthread_mutex_t *)(&s_service_ctx->m_lock));
}

void workload_error_service(void *s_service_handle)
{
    workload_service_ctx_t *s_service_ctx = (workload_service_ctx_t *)s_service_handle;

    if(s_service_ctx == NULL) { return; }
        
    pthread_mutex_lock((pthread_mutex_t *)(&s_service_ctx->m_lock));
    s_service_ctx->m_status = def_workload_service_ctrl_error;
    pthread_mutex_unlock((pthread_mutex_t *)(&s_service_ctx->m_lock));
}

void *workload_open_service_ex(const char *s_name, workload_service_handler_t s_handler, void *s_argument, size_t s_stack_size)
{
    workload_service_ctx_t *s_service_ctx;
    size_t s_name_size;

    int s_is_break, s_is_error, s_tick;
#if 0L /* DEBUG */	
    workload_timer_t s_timer;
#endif    

    if(s_handler == ((workload_service_handler_t)0)) { return((void *)0); }
    s_name_size = (s_name == ((const char *)0)) ? ((size_t)0u) : (strlen(s_name) + ((size_t)1u));
    s_service_ctx = (workload_service_ctx_t *)malloc(sizeof(workload_service_ctx_t) + s_name_size);
    if(s_service_ctx == NULL) { return((void *)0); }

    (void)pthread_mutex_init((pthread_mutex_t *)(&s_service_ctx->m_lock), NULL);

    if(s_name_size <= ((size_t)0u)) {
        static char cg_default_service_name[] = {"no service name"};

        s_service_ctx->m_name = (char *)(&cg_default_service_name[0]);
    }
    else {
        s_service_ctx->m_name = (char *)strcpy((char *)(&s_service_ctx[1]), (const char *)s_name);
    }

    s_service_ctx->m_command = def_workload_service_ctrl_start;
    s_service_ctx->m_status = def_workload_service_ctrl_ready;

    s_service_ctx->m_argument = s_argument;
    s_service_ctx->m_stack_size = s_stack_size;
    
    s_service_ctx->m_handler = s_handler;

    /* ---- */

    if(workload_detached_thread_ex(workload_service_thread, (void *)s_service_ctx, s_service_ctx->m_stack_size) == (-1)) {
        return(workload_close_service((void *)s_service_ctx));
    }

    s_is_break = s_is_error = s_tick = 0;

#if 0L /* DEBUG */	
    workload_init_timer((workload_timer_t *)(&s_timer), (unsigned long long)(def_workload_service_timeout_msec));
#endif    

    for(;;) {
        pthread_mutex_lock((pthread_mutex_t *)(&s_service_ctx->m_lock));
        if(s_service_ctx->m_command == def_workload_service_ctrl_ack) {
            if(s_service_ctx->m_status == def_workload_service_ctrl_start) { s_is_break = 1; }
            else if(s_service_ctx->m_status != def_workload_service_ctrl_ready) { s_is_break = s_is_error = 1; }
        }
        pthread_mutex_unlock((pthread_mutex_t *)(&s_service_ctx->m_lock));

        if(s_is_break != 0) { break; }

        if(s_tick != 0) { /* no sleep for first loop */ workload_sleep_wait(0, 10); }
        ++s_tick;

#if 0L /* DEBUG */	
        if(workload_check_timer((workload_timer_t *)(&s_timer)) != 0) {
            /* WARNING: too long time wait ! */
            (void)fprintf(stderr,
                "\n"
                "-=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=-\n"
                "iec: [WARNING] wait for start service (name=\"%s\", %lu.%03lu sec)\n"
                "-=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=-\n",
                (char *)s_service_ctx->m_name,
                (unsigned long)(s_timer.m_duration / 1000ull),
                (unsigned long)(s_timer.m_duration % 1000ull)
            );
            workload_init_timer((workload_timer_t *)(&s_timer), 1000ull);
        }
#endif	    
    }

    if(s_is_error == 0) { return((void *)s_service_ctx); }

    return(workload_close_service((void *)s_service_ctx));
}

void *workload_open_service(const char *s_name, workload_service_handler_t s_handler, void *s_argument)
{
    return(workload_open_service_ex(s_name, s_handler, s_argument, (size_t)0u));
}

void *workload_close_service_ex(void *s_service_handle, int s_timeout_msec)
{
    workload_service_ctx_t *s_service_ctx = (workload_service_ctx_t *)s_service_handle;

    int s_is_break = 0;
    workload_timer_t s_timer;

    if(s_service_ctx == NULL) { return(NULL); }
 
    if(s_timeout_msec == (-1)) {
        workload_init_timer((workload_timer_t *)(&s_timer), (unsigned long long)(def_workload_service_timeout_msec));
    }
    else {
        workload_init_timer((workload_timer_t *)(&s_timer), (unsigned long long)s_timeout_msec);
    }

    for(;;) {
        pthread_mutex_lock((pthread_mutex_t *)(&s_service_ctx->m_lock));
        if(s_service_ctx->m_status == def_workload_service_ctrl_start) {
            s_service_ctx->m_command = def_workload_service_ctrl_stop;
        }
        else { s_is_break = 1; }
        pthread_mutex_unlock((pthread_mutex_t *)(&s_service_ctx->m_lock));

        if(s_is_break != 0) { /* stopped */
            (void)pthread_mutex_destroy((pthread_mutex_t *)(&s_service_ctx->m_lock));
            free(s_service_handle);
	    s_service_handle = NULL;
            break;
        }
        
        if(workload_check_timer((workload_timer_t *)(&s_timer)) != 0) {
            if(s_timeout_msec != (-1)) { /* pending */ break; }

#if 0L /* DEBUG */
            /* WARNING: too long time wait ! */
            (void)fprintf(stderr,
                "\n"
                "-=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=-\n"
                "iec: [WARNING] wait for stop service (name=\"%s\", %lu.%03lu sec)\n"
                "-=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=- -=*=-\n",
                (char *)s_service_ctx->m_name,
                (unsigned long)(s_timer.m_duration / 1000ull),
                (unsigned long)(s_timer.m_duration % 1000ull)
            );
#endif	    
            workload_init_timer((workload_timer_t *)(&s_timer), 1000ull);
        }
        
        workload_sleep_wait(0, 10);
    }


    return(s_service_handle);
}

void *workload_close_service(void *s_service_handle)
{
    return(workload_close_service_ex(s_service_handle, (-1) /* suspend */));
}

#endif

/* vim: set expandtab: */
/* End of source */
