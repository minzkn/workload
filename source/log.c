/*
    Copyright (C) HWPORT.COM
    All rights reserved.
    Author: JAEHYUK CHO <mailto:minzkn@minzkn.com>
*/

#if !defined(__def_hwport_workload_source_log_c__)
# define __def_hwport_workload_source_log_c__ "log.c"

#if !defined(_ISOC99_SOURCE)
# define _ISOC99_SOURCE (1L)
#endif

#if !defined(_GNU_SOURCE)
# define _GNU_SOURCE (1L)
#endif

#include "workload.h"

#include <stdarg.h>
#include <syslog.h>

int workload_debug(const char *s_format, ...) __attribute__((__format__(__printf__,1,2)));

int workload_set_log_mode(int s_mode);
static int __workload_log(const char *s_string);
int workload_log(const char *s_format, ...) __attribute__((__format__(__printf__,1,2)));

int workload_puts(const char *s_string);
int workload_vprintf(const char *s_format, va_list s_var);
int workload_printf(const char *s_format, ...) __attribute__((__format__(__printf__,1,2)));

static int g_workload_log_mode = 0; /* (-1)=disable, 0=log, 1=puts, 2=log&puts */

int workload_debug(const char *s_format, ...)
{
#if 1L /* DEBUG: output to stdout */
    int s_result;
    va_list s_var;

    va_start(s_var, s_format);
    s_result = workload_vprintf(s_format, s_var);
    va_end(s_var);

    return(s_result);
#else /* disable */
    return(0);
#endif
}

int workload_set_log_mode(int s_mode)
{
    g_workload_log_mode = s_mode;
    return(s_mode);
}

static int __workload_log(const char *s_string)
{
    int s_result = (int)strlen(s_string);
    size_t s_begin, s_end, s_line_size;
    char *s_line;

    openlog("workload", LOG_PID | LOG_CONS, LOG_USER);
    for(s_end = (size_t)0u;;) {
        s_begin = s_end;
        for(;(s_string[s_begin] == '\n') || (s_string[s_begin] == '\r');s_begin++);
        if(s_string[s_begin] == '\0') { break; }
        for(s_end = s_begin;(s_string[s_end] != '\0') && (s_string[s_end] != '\n') && (s_string[s_end] != '\r');s_end++);

        s_line_size = s_end - s_begin;
        if(s_line_size <= ((size_t)0u)) { continue; }

        s_line = malloc(s_line_size + ((size_t)1u));
        if(s_line == NULL) { continue; }
        (void)strncpy(s_line, (const char *)(&s_string[s_begin]), s_line_size);
        s_line[s_line_size] = '\0';

        syslog(LOG_INFO, "%s", s_line);

        free((void *)s_line);
    }
    closelog();
        
    return(s_result);
}

int workload_log(const char *s_format, ...)
{
    int s_result;
    char *s_string;

    va_list s_var;

    va_start(s_var, s_format);
    s_string = NULL;
    if(vasprintf((char **)(&s_string), s_format, s_var) == (-1)) {
        if(s_string != NULL) { free((void *)s_string); s_string = NULL; }
    }
    va_end(s_var);

    if(s_string == NULL) { return(0); }

    switch(g_workload_log_mode) {
        case 0:
            s_result = __workload_log(s_string);
            break;
        case 1:
            s_result = workload_puts(s_string);
            (void)workload_puts(NULL);
            break;
        case 2:
            s_result = workload_puts(s_string);
            (void)workload_puts(NULL);
            (void)__workload_log(s_string);
            break;
        default:
            s_result = 0;
            break;
    }

    free((void *)s_string);
        
    return(s_result);
}

int workload_puts(const char *s_string)
{
    if(s_string == NULL) {
        (void)fflush(stdout);
	return(0);
    }

    return(fputs(s_string, stdout));
}

int workload_vprintf(const char *s_format, va_list s_var)
{
    int s_result;
    char *s_string;

    if(s_format == NULL) { return(workload_puts(NULL)); }

    s_string = NULL;
    if(vasprintf((char **)(&s_string), s_format, s_var) == (-1)) {
        if(s_string != NULL) { free((void *)s_string); }
        return(0);
    }

    s_result = workload_puts(s_string);

    free((void *)s_string);

    return(s_result);
}

int workload_printf(const char *s_format, ...)
{
    int s_result;
    va_list s_var;

    va_start(s_var, s_format);
    s_result = workload_vprintf(s_format, s_var);
    va_end(s_var);

    return(s_result);
}

#endif

/* vim: set expandtab: */
/* End of source */
