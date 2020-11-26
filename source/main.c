/*
    Copyright (C) HWPORT.COM
    All rights reserved.
    Author: JAEHYUK CHO <mailto:minzkn@minzkn.com>
*/

#if !defined(_ISOC99_SOURCE)
# define _ISOC99_SOURCE (1L)
#endif

#if !defined(_GNU_SOURCE)
# define _GNU_SOURCE (1L)
#endif

#include "workload.h"

static void workload_signal_handler(int s_signal);
static void workload_install_signal(void);

int main(int s_argc, char **s_argv);

static volatile int g_workload_break = 0, g_workload_reload = 0;

static void workload_signal_handler(int s_signal)
{
    switch(s_signal) {
        case SIGSEGV:
        case SIGFPE:
        case SIGILL:
#if defined(SIGBUS)        
        case SIGBUS:
#endif        
#if defined(SIGSTKFLT)
        case SIGSTKFLT:
#endif
            /* critical signal */
            (void)fputs(workload_vt_reset_code(), stderr);
            (void)fprintf(stderr, "\nSignal %d !\n", s_signal);
            exit(EXIT_FAILURE);
            break;
        case SIGQUIT:
            /* force exit */
            exit(EXIT_SUCCESS);
            break;
        case SIGINT:
        case SIGTERM:
            /* break signal */
            g_workload_break = 1;
            (void)signal(s_signal, workload_signal_handler);
            break;
        case SIGHUP:
            /* reload signal */
            g_workload_break = g_workload_reload = 1;
            (void)signal(s_signal, workload_signal_handler);
            break;
#if defined(SIGWINCH)
        case SIGWINCH:
             (void)workload_set_terminal_changed_size();
            (void)signal(s_signal, workload_signal_handler);
             break;
#endif
        case SIGPIPE:
        default:
            /* ignore signal */
            (void)signal(s_signal, workload_signal_handler);
            break;
    }
}

static void workload_install_signal(void)
{
    static const int cg_signal_table[] = {
        SIGSEGV, SIGFPE, SIGILL, SIGINT, SIGTERM, SIGQUIT, SIGHUP, SIGPIPE,
#if defined(SIGBUS)
        SIGBUS,
#endif
#if defined(SIGSTKFLT)
        SIGSTKFLT,
#endif
#if defined(SIGWINCH)
        SIGWINCH,
#endif        
        (-1)
    };
    int s_index;

    for(s_index = 0;cg_signal_table[s_index] != (-1);s_index++) {
        (void)signal(
            cg_signal_table[s_index],
            workload_signal_handler
        );
    }
}

int main(int s_argc, char **s_argv)
{
    workload_t s_workload_local, *s_workload;

    if(s_argc >= 1) {
        const char *s_basename = basename(s_argv[0]);
        if((strcmp(s_basename, "workload_ctrl") == 0) || (strcmp(s_basename, "workload_control") == 0)) {
            return(workload_control_main(s_argc, s_argv));
        }
    }

    /* install signal */
    workload_install_signal();

    /* launch main */
    for(s_workload = workload_init(
            (workload_t *)memset((void *)(&s_workload_local), 0, sizeof(s_workload_local)),
            (volatile int *)(&g_workload_break),
            (volatile int *)(&g_workload_reload),
            s_argc, s_argv
        );
        (s_workload != NULL) &&
        (*s_workload->m_reload_ptr != 0) &&
        (workload_main(s_workload) != (-1));
        *s_workload->m_break_ptr = 0);

    return(workload_destroy(s_workload));
}

/* vim: set expandtab: */
/* End of source */
