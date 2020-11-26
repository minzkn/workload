/*
    Copyright (C) HWPORT.COM
    All rights reserved.
    Author: JAEHYUK CHO <mailto:minzkn@minzkn.com>
*/

#if !defined(__def_hwport_workload_source_vt_c__)
# define __def_hwport_workload_source_vt_c__ "vt.c"

#if !defined(_ISOC99_SOURCE)
# define _ISOC99_SOURCE (1L)
#endif

#if !defined(_GNU_SOURCE)
# define _GNU_SOURCE (1L)
#endif

#include "workload.h"

#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>

#include <pthread.h>

#if defined(STDIN_FILENO)
# define def_workload_vt_fileno (STDIN_FILENO)
#else
# define def_workload_vt_fileno (0)
#endif

#if defined(STDOUT_FILENO)
# define def_workload_stdout_fileno (STDOUT_FILENO)
#else
# define def_workload_stdout_fileno (1)
#endif

typedef struct {
    int m_fd;
    int m_repair;

    struct termios m_prev_termios;
    struct termios m_new_termios;

    unsigned char m_buffer[ 128 ];
}workload_vt_t;

void *workload_open_vt(void);
void *workload_close_vt(void *s_handle);
void *workload_get_vt(void *s_handle, size_t *s_size_ptr, int s_timeout);

int workload_dump_vt(void *s_data, size_t s_size);

int workload_set_terminal_changed_size(void);
int workload_get_terminal_size(size_t *s_width_ptr, size_t *s_height_ptr, int *s_changed_ptr);

static volatile int g_workload_terminal_changed_size = 1;

void *workload_open_vt(void)
{
    workload_vt_t *s_vt = (workload_vt_t *)malloc(sizeof(workload_vt_t));

    if(s_vt == NULL) { return(NULL); }

    s_vt->m_fd = def_workload_vt_fileno;
    s_vt->m_repair = 0;
    (void)memset((void *)(&s_vt->m_buffer[0]), 0, sizeof(s_vt->m_buffer));

    /* ---- */

    if((isatty(s_vt->m_fd) == 0) ||
        (tcgetpgrp(s_vt->m_fd) != getpgrp()) ||
	(tcgetattr(s_vt->m_fd, (struct termios *)(&s_vt->m_prev_termios)) != 0)) {
        return(workload_close_vt((void *)s_vt));
    }
    (void)memcpy((void *)(&s_vt->m_new_termios), (const void *)(&s_vt->m_prev_termios), sizeof(struct termios));
    s_vt->m_new_termios.c_lflag |= (ISIG);
    s_vt->m_new_termios.c_lflag &= ~((unsigned int)(ECHO | ICANON));
    s_vt->m_new_termios.c_iflag |= (BRKINT);
    s_vt->m_new_termios.c_iflag &= ~((unsigned int)(IGNBRK));
    s_vt->m_new_termios.c_cc[VMIN] = 1;
    s_vt->m_new_termios.c_cc[VTIME] = 0;

    if(tcsetattr(s_vt->m_fd, TCSAFLUSH, (struct termios *)(&s_vt->m_new_termios)) != 0) {
        return(workload_close_vt((void *)s_vt));
    }
    s_vt->m_repair = 1;

    return((void *)s_vt);
}

void *workload_close_vt(void *s_handle)
{
    workload_vt_t *s_vt = (workload_vt_t *)s_handle;
    
    if(s_vt == NULL) { return(NULL); }
 
    if(s_vt->m_repair != 0) { (void)tcsetattr(s_vt->m_fd, TCSAFLUSH, (struct termios *)(&s_vt->m_prev_termios)); }

    free((void *)s_vt);
    
    return(NULL);
}

void *workload_get_vt(void *s_handle, size_t *s_size_ptr, int s_timeout)
{
    workload_vt_t *s_vt = (workload_vt_t *)s_handle;
    ssize_t s_read_bytes;
    
    if(s_size_ptr != ((size_t *)0)) { *(s_size_ptr) = (size_t)0u; }
    if(s_vt == NULL) { return(NULL); }

    s_read_bytes = workload_read(s_vt->m_fd, (void *)(&s_vt->m_buffer[0]), sizeof(s_vt->m_buffer) - ((size_t)1u), s_timeout);
    if(s_read_bytes <= ((ssize_t)0)) { return(NULL); }
    s_vt->m_buffer[s_read_bytes] = '\0';
    if(s_size_ptr != ((size_t *)0)) { *(s_size_ptr) = (size_t)s_read_bytes; }

    return((void *)(&s_vt->m_buffer[0]));
}

int workload_dump_vt(void *s_data, size_t s_size)
{
    unsigned char *s_code = (unsigned char *)s_data;
    size_t s_index;

    if((s_code == NULL) || (s_size <= ((size_t)0u))) { return(-1); }

    (void)workload_puts("VT CODE=\"");
    for(s_index = (size_t)0u;s_index < s_size;s_index++) {
        if(s_code[s_index] == ((unsigned char)0x1bu)) {
            (void)workload_puts("ESC");
        }
        else if((s_code[s_index] >= ((unsigned char)0x20u)) && (s_code[s_index] <= ((unsigned char)0x7e))) {
            (void)workload_printf("%c", (int)s_code[s_index]);
        }
        else {
            (void)workload_printf("(%02XH)", (int)s_code[s_index]);
        }
    }
    
    (void)workload_puts("\"\n");
    (void)workload_puts((const char *)0);

    return((int)s_size);
}

int workload_set_terminal_changed_size(void)
{
    g_workload_terminal_changed_size = 1;

    return(0);
}

int workload_get_terminal_size(size_t *s_width_ptr, size_t *s_height_ptr, int *s_changed_ptr)
{
    static pthread_mutex_t sg_lock = PTHREAD_MUTEX_INITIALIZER;
    static size_t sg_width = (size_t)0u;
    static size_t sg_height = (size_t)0u;

    pthread_mutex_lock((pthread_mutex_t *)(&sg_lock));

    if(s_changed_ptr != NULL) { *s_changed_ptr = g_workload_terminal_changed_size; }

    if(g_workload_terminal_changed_size != 0) {
        g_workload_terminal_changed_size = 0;

        sg_width = (size_t)atoi(workload_check_string_ex(getenv("COLUMNS"), "0"));
        sg_height = (size_t)atoi(workload_check_string_ex(getenv("LINES"), "0"));

        if((sg_width <= ((size_t)0u)) || (sg_height <= ((size_t)0u))) {
#if defined(TIOCGWINSZ) && defined(__GNUC__)
            if(isatty(def_workload_stdout_fileno) != 0) {
                struct winsize s_winsize;
                int s_check;

                do {
                    s_check = ioctl(def_workload_stdout_fileno, TIOCGWINSZ, &s_winsize);
                }while((s_check == (-1)) && (errno == EINTR));
                if(s_check == 0) {
                    sg_width = (size_t)s_winsize.ws_col;
                    sg_height = (size_t)s_winsize.ws_row;
                }
            }
#endif

            if(sg_width <= ((size_t)0u)) { sg_width = (size_t)80; }
            if(sg_height <= ((size_t)0u)) { sg_height = (size_t)24; }
        }
    }

    if(s_width_ptr != NULL) { *(s_width_ptr) = sg_width; }
    if(s_height_ptr != NULL) { *(s_height_ptr) = sg_height; }
    
    pthread_mutex_unlock((pthread_mutex_t *)(&sg_lock));

    return(0);
}

#endif

/* vim: set expandtab: */
/* End of source */
