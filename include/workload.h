/*
    Copyright (C) HWPORT.COM
    All rights reserved.
    Author: JAEHYUK CHO <mailto:minzkn@minzkn.com>
*/

#if !defined(__def_hwport_workload_header_workload_h__)
# define __def_hwport_workload_header_workload_h__ "workdload.h"

#define def_workload_version "0.0.9"

/* ---- */

#include <sys/types.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>

#include <pthread.h>

/* ---- */

#define def_workload_config_control_port (8702)

#define def_workload_config_default_files (1000u)
#define def_workload_config_max_files (1000000u)

#define def_workload_file_flag_none (0x00000000u)
#define def_workload_file_flag_created (0x00000001u)
#define def_workload_file_flag_inserted_db (0x00010000u)

#if !defined(workload_file_t)
struct workload_disk_ts;
# pragma pack(push,8)
typedef struct workload_file_ts {
    struct workload_file_ts *m_prev, *m_next;
    struct workload_file_ts *m_assign_prev, *m_assign_next;

    struct workload_disk_ts *m_assign_disk, *m_reassign_disk;

    unsigned int m_flags;

    size_t m_index; /* sorted index (workload_t *)->m_file_sorted[(workload_file_t *)->m_index] */

    unsigned int m_id; /* ordered index (workload_t *)->m_file_sorted[(workload_file_t *)->m_id - 1u] */
    const char *m_filename;

    pthread_mutex_t m_access_lock;

    unsigned long long m_read_count, m_write_count;
    unsigned long long m_time_stamp;
}__workload_file_t;
# pragma pack(pop)
# define workload_file_t __workload_file_t
#endif

#define def_workload_disk_flag_none (0x00000000u)
#define def_workload_disk_flag_create_path (0x00000001u)
#define def_workload_disk_flag_inserted_db (0x00010000u)

#if !defined(workload_disk_t)
# pragma pack(push,8)
struct workload_ts;
typedef struct workload_disk_ts {
    struct workload_disk_ts *m_prev, *m_next;

    unsigned int m_flags;

    unsigned int m_id;
    size_t m_name_size;
    const char *m_name;
    size_t m_path_size;
    const char *m_path;
    long long m_fakeoff_delay;
   
    /* - */
   
    void *m_service;
    struct workload_ts *m_workload;

    /* - */

    unsigned long long m_read_count, m_write_count;
    unsigned long long m_time_stamp;
    unsigned long long m_fakeoff_time_stamp;

    size_t m_assign_file_count;
    workload_file_t *m_assign_file;
}__workload_disk_t;
# pragma pack(pop)
# define workload_disk_t __workload_disk_t
#endif

#if !defined(workload_opt_t)
# pragma pack(push,8)
typedef struct workload_opt_ts {
    int m_help;
    int m_debug;
    int m_verbose;

    int m_daemonize;
    int m_nochdir;
    int m_noclose;
    const char *m_pid_file;
    int m_launcher;

    const char *m_config_file;

    int m_refresh_screen_interval;

    int m_dump_disk;
    int m_dump_file;

    unsigned int m_left_ratio, m_right_ratio;

    unsigned long long m_duration;
    unsigned long long m_access_interval;

    const char *m_files_assign_method;

    const char *m_prefix;
    const char *m_suffix;

    size_t m_filecount;
    off_t m_filesize;

    int m_pause;

    int m_threads;
}__workload_opt_t;
# pragma pack(pop)
# define workload_opt_t __workload_opt_t
#endif

#define def_workload_access_method_none (0x00000000u)
#define def_workload_access_method_read (0x00000001u)
#define def_workload_access_method_write (0x00000002u)
#define def_workload_access_method_rw (def_workload_access_method_read|def_workload_access_method_write)

#if !defined(workload_cfg_t)
# pragma pack(push,8)
typedef struct workload_cfg_ts {
    const char *m_config_file;

    unsigned int m_left_ratio, m_right_ratio;
    
    unsigned long long m_duration;
    unsigned long long m_access_interval;

    int m_files_remove;
    int m_files_assign_method; /* 0=random, 1=ordered, 2=even */

    const char *m_prefix;
    const char *m_suffix;

    size_t m_filecount;
    off_t m_filesize;

    unsigned int m_access_method;
    int m_access_sync;
    int m_access_sendfile;
    int m_access_threads;

}__workload_cfg_t;
# pragma pack(pop)
# define workload_cfg_t __workload_cfg_t
#endif

#define def_workload_flag_none (0x00000000u)
#define def_workload_flag_use_fakeoff (0x00000001u)

#if !defined(workload_t)
# pragma pack(push,8)
typedef struct workload_ts {
    volatile int *m_break_ptr;
    volatile int *m_reload_ptr;

    int m_argc;
    char **m_argv;

    int m_exit_code;

    /* ---- config */
   
    workload_opt_t m_opt;
    workload_cfg_t m_cfg;
    
    /* ---- */

    int m_socket; /* udp listen socket */
    void *m_vt;
    void *m_xml;
    void *m_db;
  
    /* ---- */

    pthread_mutex_t m_time_stamp_lock;
    unsigned long long m_time_stamp;
    unsigned long long m_time_stamp_start;
    
    /* ---- */

    pthread_mutex_t m_control_lock;
    unsigned int m_flags;
    int m_pause;
    int m_relocate;
    
    /* ---- */
    
    pthread_mutex_t m_reassign_lock;

    size_t m_disk_ready_count;
    int m_disk_error_count;

    unsigned long long m_read_count;
    unsigned long long m_write_count;

    unsigned long long m_time_stamp_1sec; /* by view only */
    unsigned long long m_read_count_prev;
    unsigned long long m_write_count_prev;
    unsigned long long m_read_count_1sec;
    unsigned long long m_write_count_1sec;

    workload_disk_t *m_disk_head;
    size_t m_max_name_size;
    size_t m_disk_count;
    workload_disk_t **m_disk; /* this pointer is reindexing to array */

    workload_file_t *m_file_head;
    size_t m_file_count;
    workload_file_t **m_file_ordered; /* this pointer is reindexing to array (ordered) */
    workload_file_t **m_file_sorted; /* this pointer is reindexing to array (sorted by access count) */
    workload_file_t **m_file_even; /* this pointer is reindexing to array (even random buffer) */

    size_t *m_next_file_index;
}__workload_t;
# pragma pack(pop)
# define workload_t __workload_t
#endif

#if !defined(__def_hwport_workload_source_workload_c__)
extern workload_t *workload_init(workload_t *s_workload, volatile int *s_break_ptr, volatile int *s_reload_ptr, int s_argc, char **s_argv);
extern int workload_destroy(workload_t *s_workload);

extern unsigned long long workload_update_main_time_stamp(workload_t *s_workload);
extern unsigned long long workload_get_main_time_stamp(workload_t *s_workload);
extern unsigned long long workload_get_main_duration(workload_t *s_workload);

extern int workload_set_main_pause(workload_t *s_workload, int s_pause);
extern int workload_toggle_main_pause(workload_t *s_workload);
extern int workload_get_main_pause(workload_t *s_workload);

extern int workload_set_main_relocate(workload_t *s_workload, int s_relocate);
extern int workload_toggle_main_relocate(workload_t *s_workload);
extern int workload_get_main_relocate(workload_t *s_workload);

extern int workload_main(workload_t *s_workload);
#endif

#if !defined(__def_hwport_workload_source_workload_control_c__)
extern int workload_control_main(int s_argc, char **s_argv);
#endif

#if !defined(__def_hwport_workload_source_workload_config_c__)
extern int workload_init_config(workload_t *s_workload);
extern int workload_load_config(workload_t *s_workload);
extern int workload_unload_config(workload_t *s_workload);
#endif

#if !defined(__def_hwport_workload_source_workload_view_c__)
extern int workload_update_screen(workload_t *s_workload);
#endif

#define def_workload_db_table_prefix "wl_"
#define def_workload_db_field_prefix "wl_"

#if !defined(__def_hwport_workload_source_workload_db_c__)
extern int workload_reset_db(workload_t *s_workload);
extern int workload_update_db(workload_t *s_workload, workload_file_t *s_file, workload_disk_t *s_disk);
#endif

#if !defined(__def_hwport_workload_source_workload_storage_c__)
extern void workload_disk_service(void *s_service_handle, void *s_argument);

extern workload_disk_t *workload_new_disk(workload_t *s_workload, unsigned int s_id, const char *s_name, const char *s_path, int s_is_create_path);
extern workload_disk_t *workload_free_disk(workload_disk_t *s_disk);

extern int workload_free_disk_array(workload_t *s_workload);
extern int workload_update_disk_array(workload_t *s_workload);
#endif

#if !defined(__def_hwport_workload_source_workload_file_c__)
extern workload_file_t *workload_new_file(workload_t *s_workload, unsigned int s_id);
extern workload_file_t *workload_free_file(workload_file_t *s_file);

extern int workload_free_file_array(workload_t *s_workload);
extern int workload_update_file_array(workload_t *s_workload);

char *workload_file_pathname_alloc(workload_file_t *s_file, workload_disk_t *s_disk);

extern int workload_assign_file_to_disk(workload_t *s_workload, workload_file_t *s_file, workload_disk_t *s_disk);
#endif

#if !defined(__def_hwport_workload_source_workload_access_c__)
extern int workload_file_access(workload_t *s_workload, workload_file_t *s_file);
extern int workload_reset_file_access_count(workload_t *s_workload);
#endif

#if !defined(__def_hwport_workload_source_workload_relocate_c__)
extern int workload_file_relocate(workload_t *s_workload);
#endif

/* ---- */

#if !defined(__def_hwport_workload_source_log_c__)
extern int workload_debug(const char *s_format, ...) __attribute__((__format__(__printf__,1,2)));

extern int workload_set_log_mode(int s_mode);
extern int workload_log(const char *s_format, ...) __attribute__((__format__(__printf__,1,2)));

extern int workload_puts(const char *s_string);
extern int workload_vprintf(const char *s_format, va_list s_var);
extern int workload_printf(const char *s_format, ...) __attribute__((__format__(__printf__,1,2)));
#endif

/* ---- */

#if !defined(__def_hwport_workload_source_math_c__)
extern unsigned int workload_gcm_uint(unsigned int s_value1, unsigned int s_value2);
extern unsigned int workload_aspect_ratio_uint(unsigned int s_value1, unsigned int s_value2, unsigned int *s_ratio1_ptr, unsigned int *s_ratio2_ptr);
#endif

/* ---- */

#if !defined(__def_hwport_workload_source_random_c__)
extern void workload_srand(unsigned int s_seed);
extern int workload_rand(void);

extern void *workload_fill_rand(void *s_data, size_t s_size);
#endif

/* ---- */

#if !defined(workload_string_node_t)
# pragma pack(push,8)
typedef struct workload_string_node_ts {
    struct workload_string_node_ts *m_prev, *m_next;
    unsigned int m_ignore;
    char *m_string;
}__workload_string_node_t;
# pragma pack(pop)
# define workload_string_node_t __workload_string_node_t
#endif

#if !defined(__def_hwport_workload_source_string_c__)
extern char *workload_check_string_ex(const char *s_string, const char *s_default_string);
extern char *workload_check_string(const char *s_string);

extern int workload_is_digit_string(const char *s_string);

extern char *workload_get_word_sep_alloc_c(int s_skip_space, const char *s_sep, const char **s_sep_string);
extern int workload_check_pattern(const char *s_pattern, const char *s_string, int s_is_case);

extern workload_string_node_t *workload_free_string_node(workload_string_node_t *s_node);
extern workload_string_node_t *workload_string_to_node_ex(const char *s_string, const char *s_sep, int s_is_ignore_tag);
extern workload_string_node_t *workload_string_to_node(const char *s_string, const char *s_sep);
extern char *workload_node_to_string(workload_string_node_t *s_node, const char *s_insert_string, int s_strip);
extern workload_string_node_t *workload_append_string_node_ex(workload_string_node_t *s_head, workload_string_node_t *s_node, int s_override, int s_is_ignore_tag);
extern workload_string_node_t *workload_append_string_node(workload_string_node_t *s_head, workload_string_node_t *s_node, int s_override);

extern workload_string_node_t *workload_free_path_node(workload_string_node_t *s_node);
extern workload_string_node_t *workload_path_to_node(const char *s_path);
extern char *workload_node_to_path(workload_string_node_t *s_node, int s_strip);
extern workload_string_node_t *workload_copy_path_node(workload_string_node_t *s_node);
extern workload_string_node_t *workload_append_path_node(workload_string_node_t *s_head, workload_string_node_t *s_node, int s_override);

extern char *workload_basename(char *s_pathname);

extern char *workload_comma_string(char *s_buffer, size_t s_buffer_size, const char *s_string);
extern char *workload_printable_comma_string(char *s_buffer, size_t s_buffer_size, unsigned long long s_value, unsigned long long s_div);

extern int workload_parse_boolean_string(const char *s_string, int s_default_value);
#endif

/* ---- */

#if !defined(workload_timer_t)
# pragma pack(push,8)
typedef struct workload_timer_ts {
    /* need shadow */
    long m_clock_tick;
    clock_t m_prev_clock;
    unsigned long long m_time_stamp;

    /* timer stat */
    unsigned long long m_start_time_stamp;
    unsigned long long m_timeout;
    unsigned long long m_duration;
}__workload_timer_t;
# pragma pack(pop)
# define workload_timer_t __workload_timer_t
# define def_workload_init_timer {0l,(clock_t)0,0ull,1000ull,0ull,0ull}
#endif

#if !defined(__def_hwport_workload_source_timer_c__)
extern void workload_sleep_wait(int s_sec, int s_usec);

extern unsigned long long workload_time_stamp_msec(workload_timer_t *s_timer);
extern void workload_init_timer(workload_timer_t *s_timer, unsigned long long s_timeout);

extern void workload_set_timer(workload_timer_t *s_timer, unsigned long long s_timeout);
extern void workload_update_timer(workload_timer_t *s_timer, unsigned long long s_timeout);
extern int workload_check_timer_ex(workload_timer_t *s_timer, unsigned long long *s_remain);
extern int workload_check_timer(workload_timer_t *s_timer);
#endif

/* ---- */

#if !defined(__def_hwport_workload_source_io_c__)
extern int workload_is_readable_fd(int s_fd, int s_timeout_msec);
extern int workload_is_writable_fd(int s_fd, int s_timeout_msec);

extern ssize_t workload_read(int s_fd, void *s_data, size_t s_size, int s_msec);
extern ssize_t workload_write(int s_fd, const void *s_data, size_t s_size, int s_msec);
extern ssize_t workload_recvfrom(int s_fd, void *s_data, size_t s_size, int s_flags, struct sockaddr *s_sockaddr, socklen_t *s_addrlen, int s_msec);
extern ssize_t workload_sendto(int s_fd, const void *s_data, size_t s_size, int s_flags, const struct sockaddr *s_sockaddr, socklen_t s_addrlen, int s_msec);

extern int workload_delete(const char *s_pathname);

extern int workload_mkdir_ex(const char *s_pathname, mode_t s_mode, char **s_first_entry);
extern int workload_mkdir(const char *s_pathname, mode_t s_mode);
#endif

/* ---- */

#if !defined(__def_hwport_workload_source_process_c__)
extern int workload_daemon(int s_nochdir, int s_noclose);
extern int workload_launcher(void);
#endif

/* ---- */

#define workload_vt_code(m_code)               "\x1b[" m_code
#define workload_vt_reset_code()                 workload_vt_code("c") workload_vt_code("(K") workload_vt_code("[J") workload_vt_code("[0m") workload_vt_code("[?25h")
#define workload_vt_sync()                       workload_puts(NULL)
#define workload_vt_clear()                      workload_puts(workload_vt_code("H") workload_vt_code("J"))
#define workload_vt_reset()                      workload_puts(workload_vt_reset_code())
#define workload_vt_move(m_x,m_y)                workload_printf(workload_vt_code("%d;%dH"),(int)(m_y),(int)(m_x))
#define workload_vt_move_x(m_x)                  workload_printf(workload_vt_code(";%dH"),(int)(m_x))
#define workload_vt_move_y(m_y)                  workload_printf(workload_vt_code("%dd"),(int)(m_y))
#define workload_vt_puts(m_x,m_y,m_message)      workload_printf(workload_vt_code("%d;%dH") "%s",(int)(m_y),(int)(m_x),m_message)

#define workload_vt_color_normal                 workload_vt_code("0m")
#define workload_vt_color_black                  workload_vt_code("1;30m")
#define workload_vt_color_red                    workload_vt_code("1;31m")
#define workload_vt_color_green                  workload_vt_code("1;32m")
#define workload_vt_color_yellow                 workload_vt_code("1;33m")
#define workload_vt_color_blue                   workload_vt_code("1;34m")
#define workload_vt_color_magenta                workload_vt_code("1;35m")
#define workload_vt_color_cyan                   workload_vt_code("1;36m")
#define workload_vt_color_white                  workload_vt_code("1;37m")
#define workload_vt_color_brown                  workload_vt_code("0;31m")

#if !defined(__def_hwport_workload_source_vt_c__)
extern void *workload_open_vt(void);
extern void *workload_close_vt(void *s_handle);
extern void *workload_get_vt(void *s_handle, size_t *s_size_ptr, int s_timeout);

extern int workload_dump_vt(void *s_data, size_t s_size);

extern int workload_set_terminal_changed_size(void);
extern int workload_get_terminal_size(size_t *s_width_ptr, size_t *s_height_ptr, int *s_changed_ptr);
#endif

/* ---- */

#if !defined(__def_hwport_workload_source_xml_c__)
extern void *workload_open_xml(const void *s_data, size_t s_size);
extern void *workload_open_xml_from_file(const char *s_pathname);
extern void *workload_close_xml(void *s_handle);

extern void *workload_get_xml_node(void *s_handle, const char *s_path);

extern void *workload_xml_next_node_ex(void *s_node, const char *s_tag_name);
extern void *workload_xml_next_node(void *s_node);
extern void *workload_xml_sub_node_ex(void *s_node, const char *s_sub_tag_name);
extern void *workload_xml_sub_node(void *s_node);
extern char *workload_xml_tag_name(void *s_node);
extern char *workload_xml_tag_data(void *s_node);
extern char **workload_xml_tag_attr(void *s_node);
extern char *workload_xml_tag_attr_value(void *s_node, const char *s_attr_name);

extern char *workload_get_xml_data(void *s_handle, const char *s_path);
extern char *workload_get_xml_attr_value(void *s_handle, const char *s_path, const char *s_attr_name);
#endif

/* ---- */

#define def_workload_db_type_sqlite3 (0)
#define def_workload_db_type_mysql (1)

#if !defined(__def_hwport_workload_source_db_c__)
extern void *workload_open_db_ex(int s_db_type, const char *s_hostname, int s_port, const char *s_username, const char *s_password, const char *s_database);
extern void *workload_open_db(int s_db_type, const char *s_database);
extern void *workload_close_db(void *s_handle);

extern int workload_db_query(void *s_handle, const char *s_query, void *s_db_result, char **s_error_message);
extern int workload_db_vquery(void *s_handle, const char *s_format, void *s_db_result, char **s_error_message, va_list s_var) __attribute__((__format__(__printf__,2,0)));
extern void *workload_db_query_get(void *s_handle, const char *s_format, ...) __attribute__((__format__(__printf__,2,3)));
extern int workload_db_query_set(void *s_handle, const char *s_format, ...) __attribute__((__format__(__printf__,2,3)));
extern void *workload_free_db_result(void *s_handle, void *s_result);

extern char **workload_db_fetch_row(void *s_handle, void *s_result);
extern unsigned long long workload_db_row_count(void *s_handle, void *s_result);
extern unsigned long long workload_db_field_count(void *s_handle, void *s_result);
extern int workload_db_field_index(void *s_handle, void *s_result, const char *s_name_hint, int s_is_case);
extern int workload_db_dump_result(void *s_handle, void *s_result, const char *s_title, long long s_max);
#endif

/* ---- */

#if !defined(workload_thread_handler_t)
typedef int (* __workload_thread_handler_t)(void * /* s_argument */);
# define workload_thread_handler_t __workload_thread_handler_t
#endif

#if !defined(workload_service_handler_t)
typedef void (* __workload_service_handler_t)(void * /* s_service_handle */, void * /* s_argument */);
# define workload_service_handler_t __workload_service_handler_t
#endif

#if !defined(__def_hwport_workload_source_service_c__)
extern int workload_detached_thread_ex(workload_thread_handler_t s_thread_handler, void *s_argument, size_t s_stack_size);

extern int workload_check_service(void *s_service_handle);
extern int workload_break_service(void *s_service_handle);
extern void workload_ack_service(void *s_service_handle);
extern void workload_error_service(void *s_service_handle);

extern void *workload_open_service_ex(const char *s_name, workload_service_handler_t s_handler, void *s_argument, size_t s_stack_size);
extern void *workload_open_service(const char *s_name, workload_service_handler_t s_handler, void *s_argument);
extern void *workload_close_service_ex(void *s_service_handle, int s_timeout_msec);
extern void *workload_close_service(void *s_service_handle);
#endif

/* ---- */

#endif

/* vim: set expandtab: */
/* End of source */
