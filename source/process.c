/*
    Copyright (C) HWPORT.COM
    All rights reserved.
    Author: JAEHYUK CHO <mailto:minzkn@minzkn.com>
*/

#if !defined(__def_hwport_workload_source_process_c__)
# define __def_hwport_workload_source_process_c__ "process.c"

#if !defined(_ISOC99_SOURCE)
# define _ISOC99_SOURCE (1L)
#endif

#if !defined(_GNU_SOURCE)
# define _GNU_SOURCE (1L)
#endif

#include "workload.h"

#include <sys/wait.h>
#include <unistd.h>

int workload_daemon(int s_nochdir, int s_noclose);
int workload_launcher(void);

int workload_daemon(int s_nochdir, int s_noclose)
{
    return(daemon(s_nochdir, s_noclose));
}

int workload_launcher(void)
{
    pid_t s_pid;
                
    for(;;) {
        s_pid = fork();
        if(s_pid == ((pid_t)(-1))) {
            return(-1);
        }
        else if(s_pid == ((pid_t)0)) {
            /* ok. immortal process start */
            
            /* need to default signal handler ! */
#if defined(SIGBUS)            
            (void)signal(SIGBUS, SIG_DFL);
#endif
#if defined(SIGSTKFLT)
            (void)signal(SIGSTKFLT, SIG_DFL);
#endif            
            (void)signal(SIGILL, SIG_DFL);
            (void)signal(SIGFPE, SIG_DFL);
            (void)signal(SIGSEGV, SIG_DFL);
            break;
        }
        else {
            pid_t s_waitpid_check;
            int s_status = 0;
            int s_options = WUNTRACED | WCONTINUED;
            int s_signum = (-1);
            
            (void)workload_log(
                " * Start monitoring by workload_launcher ! (pid=%u)\n",
                (unsigned int)s_pid
            );

            do {
                s_waitpid_check = waitpid(s_pid, (int *)(&s_status), s_options);
                if(s_waitpid_check == ((pid_t)(-1))) {
                    /* what happen ? */
             
                    (void)workload_log(
                        " * WARNING: Waitpid failed by workload_launcher ! (pid=%u)\n",
                        (unsigned int)s_pid
                    );

                    exit(EXIT_SUCCESS);
                } 

                if(WIFEXITED(s_status) != 0) {
                    /* normal exit */
                    
                    (void)workload_log(
                        " * Stop monitoring by workload_launcher ! (pid=%u)\n",
                        (unsigned int)s_pid
                    );

                    exit(EXIT_SUCCESS);
                }
                else if(WIFSIGNALED(s_status) != 0) {
                    s_signum = WTERMSIG(s_status);
                    if(
#if defined(SIGBUS)
                        (s_signum != SIGBUS) &&
#endif
#if defined(SIGSTKFLT)
                        (s_signum != SIGSTKFLT) &&
#endif
                        (s_signum != SIGILL) &&
                        (s_signum != SIGFPE) &&
                        (s_signum != SIGSEGV) &&
                        (s_signum != SIGPIPE)) {
                        /* normal exit */
                    
                        (void)workload_log(
                            " * WARNING: Stop monitoring by hwport_launcher ! (pid=%u, signum=%d)\n",
                            (unsigned int)s_pid,
                            s_signum
                        );

                        exit(EXIT_SUCCESS);
                    }
                }
            }while((WIFEXITED(s_status) == 0) && (WIFSIGNALED(s_status) == 0));

            (void)workload_log(
                " * Restarting by hwport_launcher ! (pid=%u, signum=%d)\n",
                (unsigned int)s_pid,
                s_signum
            );

            workload_sleep_wait(3, 0);

            /* retry launch loop */
        }
    }

    return(0);
}

#endif

/* vim: set expandtab: */
/* End of source */
