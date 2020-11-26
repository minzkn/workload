/*
    Copyright (C) HWPORT.COM
    All rights reserved.
    Author: JAEHYUK CHO <mailto:minzkn@minzkn.com>
*/

#if !defined(__def_hwport_workload_source_workload_c__)
# define __def_hwport_workload_source_workload_c__ "workdload.c"

#if !defined(_ISOC99_SOURCE)
# define _ISOC99_SOURCE (1L)
#endif

#if !defined(_GNU_SOURCE)
# define _GNU_SOURCE (1L)
#endif

#include "workload.h"

#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <getopt.h>
#include <netinet/in.h>

static int workload_init_option(workload_t *s_workload);
static int workload_parse_option(workload_t *s_workload);

workload_t *workload_init(workload_t *s_workload, volatile int *s_break_ptr, volatile int *s_reload_ptr, int s_argc, char **s_argv);
int workload_destroy(workload_t *s_workload);

unsigned long long workload_update_main_time_stamp(workload_t *s_workload);
unsigned long long workload_get_main_time_stamp(workload_t *s_workload);
unsigned long long workload_get_main_duration(workload_t *s_workload);

int workload_set_main_pause(workload_t *s_workload, int s_pause);
int workload_toggle_main_pause(workload_t *s_workload);
int workload_get_main_pause(workload_t *s_workload);

int workload_set_main_relocate(workload_t *s_workload, int s_relocate);
int workload_toggle_main_relocate(workload_t *s_workload);
int workload_get_main_relocate(workload_t *s_workload);

static int workload_cmd(workload_t *s_workload, const void *s_data, size_t s_size, char *s_res_data);
static int workload_key(workload_t *s_workload);
static int workload_udp(workload_t *s_workload);

size_t __workload_access_random_index(workload_t *s_workload);
static void __workload_access_service(void *s_service_handle, void *s_argument);

int workload_main(workload_t *s_workload);

static int workload_init_option(workload_t *s_workload)
{
    static const char cg_default_pidfile[] = { "/var/run/workload.pid" };

    (void)memset((void *)(&s_workload->m_opt), 0, sizeof(s_workload->m_opt));

    s_workload->m_opt.m_help = 0;
    s_workload->m_opt.m_debug = 0;
    s_workload->m_opt.m_verbose = 0;
    
    s_workload->m_opt.m_daemonize = 0;
    s_workload->m_opt.m_nochdir = 0;
    s_workload->m_opt.m_noclose = 0;
    s_workload->m_opt.m_pid_file = (const char *)(&cg_default_pidfile[0]);
    s_workload->m_opt.m_launcher = 0;

    s_workload->m_opt.m_config_file = NULL;

    s_workload->m_opt.m_refresh_screen_interval = 100;

    s_workload->m_opt.m_dump_disk = 0;
    s_workload->m_opt.m_dump_file = 0;
   
    s_workload->m_opt.m_left_ratio = 0u;
    s_workload->m_opt.m_right_ratio = 0u;

    s_workload->m_opt.m_duration = ~0ull;
    s_workload->m_opt.m_access_interval = ~0ull;

    s_workload->m_opt.m_files_assign_method = NULL;

    s_workload->m_opt.m_prefix = NULL;
    s_workload->m_opt.m_suffix = NULL;
 
    s_workload->m_opt.m_filecount = (size_t)0u;
    s_workload->m_opt.m_filesize = (off_t)(-1);

    s_workload->m_opt.m_pause = (-1);

    s_workload->m_opt.m_threads = (-1);

    return(0);
}

static int workload_parse_option(workload_t *s_workload)
{
    static struct option sg_workload_options[] = {
        {"help", 0, (int *)0, 'h'},
        {"debug", 0, (int *)0, 0},
        {"verbose", 0, (int *)0, 'v'},
        {"daemon", 0, (int *)0, 'd'},
        {"nochdir", 0, (int *)0, 0},
        {"noclose", 0, (int *)0, 0},
        {"pidfile", 0, (int *)0, 0},
        {"launcher", 0, (int *)0, 0},
        {"config", 1, (int *)0, 'c'},
        {"dump-disk", 0, (int *)0, 0},
        {"dump-file", 0, (int *)0, 0},
        {"left", 1, (int *)0, 0},
        {"right", 1, (int *)0, 0},
        {"duration", 1, (int *)0, 0},
        {"interval", 1, (int *)0, 0},
        {"assign-method", 1, (int *)0, 0},
        {"prefix", 1, (int *)0, 0},
        {"suffix", 1, (int *)0, 0},
        {"filecount", 1, (int *)0, 0},
        {"filesize", 1, (int *)0, 0},
        {"pause", 0, (int *)0, 0},
        {"relocate", 0, (int *)0, 0},
        {"threads", 1, (int *)0, 0},
        {(char *)0, 0, (int *)0, 0}
    };
    int s_option_check, s_option_index;
    
    for(s_option_index = 0;;) {
        s_option_check = getopt_long(s_workload->m_argc, s_workload->m_argv, "hvdc:", sg_workload_options, &s_option_index);
        if(s_option_check == (-1)) { break; }

        switch(s_option_check) {
            case 0:
                 if(sg_workload_options[s_option_index].flag != ((int *)0)) { break; }
                 if(strcmp(sg_workload_options[s_option_index].name, "nochdir") == 0) {
                     s_workload->m_opt.m_nochdir = 1;
                 }
                 else if(strcmp(sg_workload_options[s_option_index].name, "noclose") == 0) {
                     s_workload->m_opt.m_noclose = 1;
                 }
                 else if(strcmp(sg_workload_options[s_option_index].name, "pidfile") == 0) {
                     s_workload->m_opt.m_pid_file = (const char *)optarg;
                 }
                 else if(strcmp(sg_workload_options[s_option_index].name, "launcher") == 0) {
                     s_workload->m_opt.m_launcher = 1;
                 }
                 else if(strcmp(sg_workload_options[s_option_index].name, "debug") == 0) {
                     s_workload->m_opt.m_debug = 1;
                     s_workload->m_opt.m_verbose = 1;
                 }
                 else if(strcmp(sg_workload_options[s_option_index].name, "dump-disk") == 0) {
                     s_workload->m_opt.m_dump_disk = 1;
                 }
                 else if(strcmp(sg_workload_options[s_option_index].name, "dump-file") == 0) {
                     s_workload->m_opt.m_dump_file = 1;
                 }
                 else if(strcmp(sg_workload_options[s_option_index].name, "left") == 0) {
                     if(optarg != NULL) {
                         s_workload->m_opt.m_left_ratio = (unsigned int)atoi(optarg);
                     }
                 }
                 else if(strcmp(sg_workload_options[s_option_index].name, "right") == 0) {
                     if(optarg != NULL) {
                         s_workload->m_opt.m_right_ratio = (unsigned int)atoi(optarg);
                     }
                 }
                 else if(strcmp(sg_workload_options[s_option_index].name, "duration") == 0) {
                     if(optarg != NULL) {
                         s_workload->m_opt.m_duration = (unsigned long long)atoi(optarg);
                     }
                 }
                 else if(strcmp(sg_workload_options[s_option_index].name, "interval") == 0) {
                     if(optarg != NULL) {
                         s_workload->m_opt.m_access_interval = (unsigned long long)atoi(optarg);
                     }
                 }
                 else if(strcmp(sg_workload_options[s_option_index].name, "assign-method") == 0) {
                     s_workload->m_opt.m_files_assign_method = optarg;
                 }
                 else if(strcmp(sg_workload_options[s_option_index].name, "prefix") == 0) {
                     s_workload->m_opt.m_prefix = optarg;
                 }
                 else if(strcmp(sg_workload_options[s_option_index].name, "suffix") == 0) {
                     s_workload->m_opt.m_suffix = optarg;
                 }
                 else if(strcmp(sg_workload_options[s_option_index].name, "filecount") == 0) {
                     if(optarg != NULL) {
                         s_workload->m_opt.m_filecount = (size_t)atoi(optarg);
                     }
                 }
                 else if(strcmp(sg_workload_options[s_option_index].name, "filesize") == 0) {
                     if(optarg != NULL) {
                         s_workload->m_opt.m_filesize = (off_t)atoi(optarg);
                     }
                 }
                 else if(strcmp(sg_workload_options[s_option_index].name, "pause") == 0) {
                     s_workload->m_opt.m_pause = 1;
                 }
                 else if(strcmp(sg_workload_options[s_option_index].name, "relocate") == 0) {
                     workload_set_main_relocate(s_workload, 1);
                 }
                 else if(strcmp(sg_workload_options[s_option_index].name, "threads") == 0) {
                     if(optarg != NULL) {
                         s_workload->m_opt.m_threads = atoi(optarg);
                     }
                 }
                 
                 if(s_workload->m_opt.m_debug != 0) {
                     (void)workload_debug("option : \"%s\" = \"%s\"\n", 
                         sg_workload_options[s_option_index].name,
                         (optarg == ((char *)0)) ? "{NULL}" : optarg);
                 }
                 break;  
            case 'v': s_workload->m_opt.m_verbose = 1; break;
            case 'd': s_workload->m_opt.m_daemonize = 1; break;
            case 'c': s_workload->m_opt.m_config_file = optarg; break;
            case '?':
            case 'h':
            default: s_workload->m_opt.m_help = 1; break;
        }
    }

    return(0);
}

workload_t *workload_init(workload_t *s_workload, volatile int *s_break_ptr, volatile int *s_reload_ptr, int s_argc, char **s_argv)
{
    s_workload->m_break_ptr = s_break_ptr;
    s_workload->m_reload_ptr = s_reload_ptr;
    *s_workload->m_reload_ptr = 1;

    s_workload->m_argc = s_argc;
    s_workload->m_argv = s_argv;

    s_workload->m_exit_code = EXIT_SUCCESS;
    
    (void)workload_init_option(s_workload);
    (void)workload_init_config(s_workload);

    s_workload->m_socket = (-1);
    s_workload->m_vt = NULL;
    s_workload->m_xml = NULL;
    s_workload->m_db = NULL;

    pthread_mutex_init((pthread_mutex_t *)(&s_workload->m_time_stamp_lock), NULL);
    
    s_workload->m_time_stamp = workload_time_stamp_msec(NULL);
    s_workload->m_time_stamp_start = s_workload->m_time_stamp;

    pthread_mutex_init((pthread_mutex_t *)(&s_workload->m_control_lock), NULL);

    s_workload->m_flags = def_workload_flag_none;
    s_workload->m_pause = 0;
    s_workload->m_relocate = 0;

    pthread_mutex_init((pthread_mutex_t *)(&s_workload->m_reassign_lock), NULL);
    
    s_workload->m_disk_ready_count = 0;
    s_workload->m_disk_error_count = 0;

    s_workload->m_read_count = 0ull;
    s_workload->m_write_count = 0ull;
   
    s_workload->m_time_stamp_1sec = s_workload->m_time_stamp;
    s_workload->m_read_count_prev = 0ull;
    s_workload->m_write_count_prev = 0ull;
    s_workload->m_read_count_1sec = 0ull;
    s_workload->m_write_count_1sec = 0ull;

    s_workload->m_disk_head = NULL;
    s_workload->m_max_name_size = (size_t)0u;
    s_workload->m_disk_count = (size_t)0u;
    s_workload->m_disk = (workload_disk_t **)0;

    s_workload->m_file_head = NULL;
    s_workload->m_file_count = (size_t)0u;
    s_workload->m_file_ordered = (workload_file_t **)0;
    s_workload->m_file_sorted = (workload_file_t **)0;
    s_workload->m_file_even = (workload_file_t **)0;

    s_workload->m_next_file_index = NULL;

    /* ---- */

    if(workload_parse_option(s_workload) == (-1)) {
        (void)workload_destroy(s_workload);

        return(NULL);
    }
    
    if(s_workload->m_opt.m_help != 0) { /* help */
        (void)workload_printf(
            "workload v%s\n\n"
	    "usage: %s [<options>]\n"
            "options:\n"
            "\t-h, --help                       : help\n"
            "\t    --debug                      : debug mode\n"
            "\t-v, --verbose                    : verbose mode\n"
            "\t-d, --daemon                     : daemonize\n"
            "\t    --nochdir                    : no chdir (daemon)\n"
            "\t    --noclose                    : no close (daemon)\n"
            "\t    --pidfile=<pathname>         : pidfile (daemon)\n"
            "\t    --launcher                   : immortal process launcher\n"
            "\t-c, --config=<config file>       : config\n"
            "\t    --dump-disk                  : report disk info\n"
            "\t    --dump-file                  : report file info\n"
            "\t    --left=<value>               : left value\n"
            "\t    --right=<value>              : right value\n"
            "\t    --duration=<msec>            : duration\n"
            "\t    --interval=<msec>            : access interval\n"
            "\t    --assign-method=<method>     : files assign name (random|ordered|even)\n"
            "\t    --prefix=<string>            : file prefix name\n"
            "\t    --suffix=<string>            : file suffix name\n"
            "\t    --filecount=<count>          : file count\n"
            "\t    --filesize=<bytes>           : file size\n"
            "\t    --pause                      : pause starting mode\n"
            "\t    --relocate                   : relocate starting mode\n"
            "\t    --threads=<count>            : access threads\n"
            "\n"
            "Copyright (C) HWPORT.COM - All rights reserved.\n"
            "Author: JaeHyuk Cho <mailto:minzkn@minzkn.com>\n"
            "http://www.hwport.com/\n",
	    def_workload_version,
            basename(s_workload->m_argv[0])
        );
  
        (void)workload_destroy(s_workload);

        return(NULL);
    }
  
    if(s_workload->m_opt.m_daemonize != 0) { /* daemonize */
        FILE *s_pid_fp;

        if(s_workload->m_opt.m_verbose != 0) {
            (void)workload_printf(
                " * Daemonizing... (nochdir=%d, noclose=%d, pidfile=\"%s\")\n",
                s_workload->m_opt.m_nochdir,
                s_workload->m_opt.m_noclose,
                workload_check_string(s_workload->m_opt.m_pid_file)
            );
        }

        if(strlen(workload_check_string(s_workload->m_opt.m_pid_file)) <= ((size_t)0u)) {
            s_pid_fp = NULL;
        }
        else {
            s_pid_fp = fopen(s_workload->m_opt.m_pid_file, "w");
            if(s_pid_fp == NULL) {
                (void)workload_printf(" * ERROR: pidfile failed !\n");
                (void)workload_destroy(s_workload);
                return(NULL);
            }
        }

        if(workload_daemon(s_workload->m_opt.m_nochdir, s_workload->m_opt.m_noclose) != 0) {
            (void)workload_printf(" * ERROR: Daemonize failed !\n");
            (void)workload_destroy(s_workload);
            return(NULL);
        }

        if(s_pid_fp != NULL) {
            (void)fprintf(s_pid_fp, "%d\n", (int)getpid());
            (void)fclose(s_pid_fp);
        }
       
        (void)workload_set_log_mode(0 /* log */);
    }

    if(s_workload->m_opt.m_launcher != 0) { /* immortal process */
        (void)workload_launcher();
    }

    /* ---- */

    if(s_workload->m_opt.m_daemonize == 0) {
        if((s_workload->m_opt.m_debug == 0) && (s_workload->m_opt.m_verbose == 0)) {
            (void)workload_set_log_mode(0 /* log */);
        }
        else {
            (void)workload_set_log_mode(2 /* log */);
        }
    }
    else {
        (void)workload_set_log_mode(0 /* log */);
    }

    /* ---- */
   
    if(s_workload->m_opt.m_verbose != 0) {
        (void)workload_log(" * Initializing workload v%s...\n", def_workload_version);
    }
 
    /* initialize randomize */
    workload_srand((unsigned int)time((time_t *)0));

    s_workload->m_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(s_workload->m_socket == (-1)) {
        if(s_workload->m_opt.m_verbose != 0) {
            (void)workload_log(" * ERROR: can not open socket !\n");
        }
        (void)workload_destroy(s_workload);
        return(NULL);
    }
    else {
        uint16_t s_port;
        struct sockaddr_in s_sockaddr_in;

        s_port = (uint16_t)def_workload_config_control_port;

        (void)memset((void *)(&s_sockaddr_in), 0, sizeof(s_sockaddr_in));
        s_sockaddr_in.sin_family = AF_INET;
#if 0L
        s_sockaddr_in.sin_addr.s_addr = inet_addr("127.0.0.1");
#else
        s_sockaddr_in.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
        s_sockaddr_in.sin_port = htons(s_port);

        if(bind(s_workload->m_socket, (struct sockaddr *)(&s_sockaddr_in), (socklen_t)sizeof(s_sockaddr_in)) == (-1)) {
            (void)close(s_workload->m_socket);
            s_workload->m_socket = (-1);
        }
    }

    /* VT input */
    if(s_workload->m_opt.m_daemonize == 0) {
        s_workload->m_vt = workload_open_vt();
    }
    
    return(s_workload);
}

int workload_destroy(workload_t *s_workload)
{
    if(s_workload == ((workload_t *)0)) { return(EXIT_FAILURE); }
    
    if(s_workload->m_opt.m_verbose != 0) {
        if(s_workload->m_opt.m_help == 0) {
            (void)workload_log(" * Stopping workload...\n");
        }
    }
 
    (void)workload_unload_config(s_workload);
    
    pthread_mutex_destroy((pthread_mutex_t *)(&s_workload->m_reassign_lock));
    pthread_mutex_destroy((pthread_mutex_t *)(&s_workload->m_control_lock));
    pthread_mutex_destroy((pthread_mutex_t *)(&s_workload->m_time_stamp_lock));
    
    (void)workload_close_vt(s_workload->m_vt);
    if(s_workload->m_socket != (-1)) { (void)close(s_workload->m_socket); }

    return(s_workload->m_exit_code);
}

unsigned long long workload_update_main_time_stamp(workload_t *s_workload)
{
    unsigned long long s_time_stamp;

    pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_time_stamp_lock));
    s_workload->m_time_stamp = workload_time_stamp_msec(NULL);
    s_time_stamp = s_workload->m_time_stamp;
    pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_time_stamp_lock));

    return(s_time_stamp);
}

unsigned long long workload_get_main_time_stamp(workload_t *s_workload)
{
    unsigned long long s_time_stamp;

    pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_time_stamp_lock));
    s_time_stamp = s_workload->m_time_stamp;
    pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_time_stamp_lock));

    return(s_time_stamp);
}

unsigned long long workload_get_main_duration(workload_t *s_workload)
{
    unsigned long long s_duration;

    pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_time_stamp_lock));
    s_duration = s_workload->m_time_stamp - s_workload->m_time_stamp_start;
    pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_time_stamp_lock));

    return(s_duration);
}

int workload_set_main_pause(workload_t *s_workload, int s_pause)
{
    pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_control_lock));
    s_workload->m_pause = s_pause;
    pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_control_lock));

    return(s_pause);
}

int workload_toggle_main_pause(workload_t *s_workload)
{
    int s_pause;

    pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_control_lock));
    s_workload->m_pause = (s_workload->m_pause == 0) ? 1 : 0;
    s_pause = s_workload->m_pause;
    pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_control_lock));

    return(s_pause);
}

int workload_get_main_pause(workload_t *s_workload)
{
    int s_pause;

    pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_control_lock));
    s_pause = s_workload->m_pause;
    pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_control_lock));

    return(s_pause);
}

int workload_set_main_relocate(workload_t *s_workload, int s_relocate)
{
    workload_disk_t *s_disk;

    pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_control_lock));
    s_workload->m_relocate = s_relocate;
    pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_control_lock));
    
    pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
    for(s_disk = s_workload->m_disk_head;s_disk != NULL;s_disk = s_disk->m_next) {
        s_disk->m_read_count = s_disk->m_write_count = 0ull;
        s_disk->m_time_stamp = s_workload->m_time_stamp;
        s_disk->m_fakeoff_time_stamp = s_workload->m_time_stamp;
        (void)workload_update_db(s_workload, NULL, s_disk);
    }
    pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_reassign_lock));

    return(s_relocate);
}

int workload_toggle_main_relocate(workload_t *s_workload)
{
    int s_relocate;
    workload_disk_t *s_disk;

    pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_control_lock));
    s_workload->m_relocate = (s_workload->m_relocate == 0) ? 1 : 0;
    s_relocate = s_workload->m_relocate;
    pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_control_lock));
    
    pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
    for(s_disk = s_workload->m_disk_head;s_disk != NULL;s_disk = s_disk->m_next) {
        s_disk->m_read_count = s_disk->m_write_count = 0ull;
        s_disk->m_time_stamp = s_workload->m_time_stamp;
        s_disk->m_fakeoff_time_stamp = s_workload->m_time_stamp;
        (void)workload_update_db(s_workload, NULL, s_disk);
    }
    pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_reassign_lock));

    return(s_relocate);
}

int workload_get_main_relocate(workload_t *s_workload)
{
    int s_relocate;

    pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_control_lock));
    s_relocate = s_workload->m_relocate;
    pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_control_lock));

    return(s_relocate);
}

static int workload_cmd(workload_t *s_workload, const void *s_data, size_t s_size, char *s_res_data)
{
    const unsigned char *s_code = (const unsigned char *)s_data;

    if(s_res_data != NULL) { s_res_data[0] = '\0'; }

    if(s_size == ((size_t)1u)) {
        if((s_code[0] == ((unsigned char)0x1bu)) || (s_code[0] == ((unsigned char)'q'))) { /* quit */
            *s_workload->m_break_ptr = 1;
            if(s_res_data != NULL) {
                (void)sprintf(s_res_data,
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                    "<control>\n"
                    "  <cmd>quit</cmd>\n"
                    "  <res>ack</res>\n"
                    "  <pid>%u</pid>\n"
                    "</control>\n",
                    (unsigned int)getpid()
                );
            }
            return(0);
        }

        if(s_code[0] == 'Q') { /* restart */
            *s_workload->m_reload_ptr = 1;
            *s_workload->m_break_ptr = 1;
            if(s_res_data != NULL) {
                (void)sprintf(s_res_data,
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                    "<control>\n"
                    "  <cmd>restart</cmd>\n"
                    "  <res>ack</res>\n"
                    "</control>\n"
                );
            }
            return(0);
        }
        
        if(s_code[0] == '?') { /* toggle relocate/keep */
            if(s_res_data != NULL) {
                pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
                (void)sprintf(s_res_data,
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                    "<control>\n"
                    "  <cmd>status</cmd>\n"
                    "  <relocate>%s</relocate>\n"
                    "  <pause>%s</pause>\n"
                    "  <left>%u</left>\n"
                    "  <right>%u</right>\n"
                    "  <interval>%llu</interval>\n"
                    "  <read>%llu</read>\n"
                    "  <write>%llu</write>\n"
                    "  <duration>%llu</duration>\n"
                    "  <res>ack</res>\n"
                    "</control>\n",
                    (workload_get_main_relocate(s_workload) == 0) ? "no" : "yes",
                    (workload_get_main_pause(s_workload) == 0) ? "no" : "yes",
                    s_workload->m_cfg.m_left_ratio,
                    s_workload->m_cfg.m_right_ratio,
                    s_workload->m_cfg.m_access_interval,
                    s_workload->m_read_count,
                    s_workload->m_write_count,
                    s_workload->m_time_stamp - s_workload->m_time_stamp_start
                );
                pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
            }
        }
        else if(s_code[0] == 'r') { /* toggle relocate/keep */
            if(workload_toggle_main_relocate(s_workload) != 0) {
                (void)workload_file_relocate(s_workload);
            }
            if(s_res_data != NULL) {
                (void)sprintf(s_res_data,
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                    "<control>\n"
                    "  <cmd>relocate</cmd>\n"
                    "  <status>%s</status>\n"
                    "  <res>ack</res>\n"
                    "</control>\n",
                    (workload_get_main_relocate(s_workload) == 0) ? "no" : "yes"
                );
            }
        }
        else if(s_code[0] == 'o') { /* oneshot relocate */
            (void)workload_file_relocate(s_workload);
            if(s_res_data != NULL) {
                (void)sprintf(s_res_data,
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                    "<control>\n"
                    "  <cmd>relocate</cmd>\n"
                    "  <status>%s</status>\n"
                    "  <res>ack</res>\n"
                    "</control>\n",
                    (workload_get_main_relocate(s_workload) == 0) ? "no" : "yes"
                );
            }
        }
        else if(s_code[0] == 'p') { /* toggle pause/resume */
	    workload_toggle_main_pause(s_workload);
            if(s_res_data != NULL) {
                (void)sprintf(s_res_data,
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                    "<control>\n"
                    "  <cmd>pause</cmd>\n"
                    "  <status>%s</status>\n"
                    "  <res>ack</res>\n"
                    "</control>\n",
                    (workload_get_main_pause(s_workload) == 0) ? "no" : "yes"
                );
            }
        }
        else if(s_code[0] == 'S') { /* start relocate */
            workload_set_main_relocate(s_workload, 1);
            (void)workload_file_relocate(s_workload);
            if(s_res_data != NULL) {
                (void)sprintf(s_res_data,
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                    "<control>\n"
                    "  <cmd>relocate</cmd>\n"
                    "  <status>%s</status>\n"
                    "  <res>ack</res>\n"
                    "</control>\n",
                    "yes"
                );
            }
        }
        else if(s_code[0] == 'K') { /* stop relocate(keep) */
            workload_set_main_relocate(s_workload, 0);
            if(s_res_data != NULL) {
                (void)sprintf(s_res_data,
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                    "<control>\n"
                    "  <cmd>relocate</cmd>\n"
                    "  <status>%s</status>\n"
                    "  <res>ack</res>\n"
                    "</control>\n",
                    "no"
                );
            }
        }
        else if(s_code[0] == 'P') { /* pause */
            workload_set_main_pause(s_workload, 1);
            if(s_res_data != NULL) {
                (void)sprintf(s_res_data,
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                    "<control>\n"
                    "  <cmd>pause</cmd>\n"
                    "  <status>%s</status>\n"
                    "  <res>ack</res>\n"
                    "</control>\n",
                    "yes"
                );
            }
        }
        else if(s_code[0] == 'R') { /* resume */
            workload_set_main_pause(s_workload, 0);
            if(s_res_data != NULL) {
                (void)sprintf(s_res_data,
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                    "<control>\n"
                    "  <cmd>pause</cmd>\n"
                    "  <status>%s</status>\n"
                    "  <res>ack</res>\n"
                    "</control>\n",
                    "no"
                );
            }
        }
        else if(s_code[0] == '!') { /* reset */
            (void)workload_reset_file_access_count(s_workload);

            if(s_res_data != NULL) {
                (void)sprintf(s_res_data,
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                    "<control>\n"
                    "  <cmd>reset</cmd>\n"
                    "  <res>ack</res>\n"
                    "</control>\n"
                );
            }
        }
    }
    else if((s_size >= ((size_t)2u)) && (isspace((int)s_code[1]) != 0)) {
        int s_value;

        /* argument command */
        if(s_code[0] == 'l') { /* left */
            s_value = atoi((const char *)(&s_code[2]));
            if(s_value > 0) {
                s_workload->m_cfg.m_left_ratio = (unsigned int)s_value;
                if(s_res_data != NULL) {
                    (void)sprintf(s_res_data,
                        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                        "<control>\n"
                        "  <cmd>left</cmd>\n"
                        "  <status>%u</status>\n"
                        "  <res>ack</res>\n"
                        "</control>\n",
                        s_workload->m_cfg.m_left_ratio
                    );
                }
            }
            else {
                if(s_res_data != NULL) {
                    (void)sprintf(s_res_data,
                        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                        "<control>\n"
                        "  <cmd>left</cmd>\n"
                        "  <status>%u</status>\n"
                        "  <res>nack</res>\n"
                        "</control>\n",
                        s_workload->m_cfg.m_left_ratio
                    );
                }
            }
        }
        else if(s_code[0] == 'r') { /* right */
            s_value = atoi((const char *)(&s_code[2]));
            if(s_value > 0) {
                s_workload->m_cfg.m_right_ratio = (unsigned int)s_value;
                if(s_res_data != NULL) {
                    (void)sprintf(s_res_data,
                        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                        "<control>\n"
                        "  <cmd>right</cmd>\n"
                        "  <status>%u</status>\n"
                        "  <res>ack</res>\n"
                        "</control>\n",
                        s_workload->m_cfg.m_right_ratio
                    );
                }
            }
            else {
                if(s_res_data != NULL) {
                    (void)sprintf(s_res_data,
                        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                        "<control>\n"
                        "  <cmd>right</cmd>\n"
                        "  <status>%u</status>\n"
                        "  <res>nack</res>\n"
                        "</control>\n",
                        s_workload->m_cfg.m_right_ratio
                    );
                }
            }
        }
    }

    return(0);
}

static int workload_key(workload_t *s_workload)
{
    unsigned char *s_code;
    size_t s_code_size;

    if(s_workload->m_vt == NULL) { return(0); }

    s_code = (unsigned char *)workload_get_vt(s_workload->m_vt, (size_t *)(&s_code_size), 0);
    if(s_code_size <= ((size_t)0u)) {
        return(0);
    }

#if 0L /* DEBUG */       
    if(s_workload->m_opt.m_verbose != 0) { (void)workload_dump_vt((void *)s_code, s_code_size); }
#endif            

    return(workload_cmd(s_workload, (const void *)s_code, s_code_size, NULL));
}

static int workload_udp(workload_t *s_workload)
{
    unsigned char s_buffer[ 4 << 10 ];
    unsigned char s_res_buffer[ 4 << 10 ];
    ssize_t s_recv_bytes;
    struct sockaddr_storage s_sockaddr_storage;
    socklen_t s_socklen;

    size_t s_begin, s_end, s_line_size, s_lines;
    char *s_line;

    int s_valid_header;

    if(s_workload->m_socket == (-1)) { return(0); }

    for(;;) {
        s_socklen = (socklen_t)sizeof(s_sockaddr_storage);
        s_recv_bytes = workload_recvfrom(
            s_workload->m_socket,
            (void *)(&s_buffer[0]),
            sizeof(s_buffer),
            MSG_NOSIGNAL,
            (struct sockaddr *)(&s_sockaddr_storage),
            (socklen_t *)(&s_socklen),
            10
        );
        if(s_recv_bytes <= ((ssize_t)0)) { return(0); }

#if 0L /* DEBUG */
        (void)workload_log(" * UDP packet : %ld bytes\n", (long)s_recv_bytes);
#endif        
    
        for(s_end = s_lines = (size_t)0u, s_valid_header = 0;s_end < ((size_t)s_recv_bytes);s_lines++) {
            s_begin = s_end;
            for(;(s_begin < ((size_t)s_recv_bytes)) && ((s_buffer[s_begin] == '\n') || (s_buffer[s_begin] == '\r'));s_begin++);
            for(s_end = s_begin;
                (s_end < ((size_t)s_recv_bytes)) &&
                (s_buffer[s_end] != '\n') &&
                (s_buffer[s_end] != '\r');
                s_end++
            );

            s_line_size = s_end - s_begin;
            if(s_line_size <= ((size_t)0u)) { continue; }

            s_line = malloc(s_line_size + ((size_t)1u));
            if(s_line == NULL) { continue; }
            (void)strncpy(s_line, (const char *)(&s_buffer[s_begin]), s_line_size);
            s_line[s_line_size] = '\0';
   
            if(s_lines == ((size_t)0u)) {
                if(strcmp(s_line, "workload") == 0) { s_valid_header = 1; }
            }
            else {
                size_t s_res_size;

                (void)workload_cmd(s_workload, (const void *)s_line, s_line_size, (char *)(&s_res_buffer[0]));

                s_res_size = strlen((const char *)(&s_res_buffer[0]));
                if(s_res_size > ((size_t)0u)) {
                    (void)workload_sendto(
                        s_workload->m_socket,
                        (const void *)(&s_res_buffer[0]),
                        s_res_size,
                        MSG_NOSIGNAL,
                        (struct sockaddr *)(&s_sockaddr_storage),
                        s_socklen,
                        16 * 1000
                    );
                }
            }

            free((void *)s_line);

            if(s_valid_header == 0) { /* invalid header ! drop packet */
                break;
            }
        }
    }

    return(0);
}

size_t __workload_access_random_index(workload_t *s_workload)
{
    unsigned int s_sum, s_value;
    size_t s_entry, s_count, s_temp;
        
    s_sum = s_workload->m_cfg.m_left_ratio + s_workload->m_cfg.m_right_ratio;
    for(s_entry = (size_t)0u, s_count = s_workload->m_file_count;s_count > ((size_t)1u);) {
        s_value = ((unsigned int)workload_rand()) % s_sum;
        s_temp = s_count >> 1;
        if(s_value < s_workload->m_cfg.m_right_ratio) { s_entry += s_temp; }
        s_count -= s_temp;
    }

    return(s_entry);
}

static void __workload_access_service(void *s_service_handle, void *s_argument)
{
    workload_t *s_workload = (workload_t *)s_argument;
    unsigned long long s_time_stamp, s_time_stamp_access;
    workload_file_t *s_file;

    s_time_stamp_access = workload_get_main_time_stamp(s_workload);

    workload_ack_service(s_service_handle);

    while((*s_workload->m_break_ptr == 0) && (workload_break_service(s_service_handle) == 0)) {
        s_time_stamp = workload_get_main_time_stamp(s_workload);
        if((workload_get_main_pause(s_workload) != 0) ||
	   ((s_time_stamp - s_time_stamp_access) < ((unsigned long long)s_workload->m_cfg.m_access_interval))) { /* access interval */
	    workload_sleep_wait(0, 10);

	    continue;
	}
        s_time_stamp_access = s_time_stamp;

        pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
	s_file = s_workload->m_file_ordered[__workload_access_random_index(s_workload)];
        pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_reassign_lock));

        pthread_mutex_lock((pthread_mutex_t *)(&s_file->m_access_lock));
        (void)workload_file_access(s_workload, s_file);
        pthread_mutex_unlock((pthread_mutex_t *)(&s_file->m_access_lock));
        
	if(workload_get_main_relocate(s_workload) != 0) { (void)workload_file_relocate(s_workload); }
    }
}

int workload_main(workload_t *s_workload)
{
    unsigned long long s_time_stamp, s_time_stamp_refresh_screen, s_time_stamp_access;
    int s_thread_index, s_threads;
    void **s_service;
 
    if(s_workload == ((workload_t *)0)) { return(-1); }
    if(s_workload->m_opt.m_verbose != 0) {
        (void)workload_log(" * Launch workload...\n");
    }
    *s_workload->m_reload_ptr = 0;

    if(workload_load_config(s_workload) == (-1)) { /* not found configuration */ return(-1); }
    if(*s_workload->m_break_ptr != 0) { return(0); }
    
    s_time_stamp = s_time_stamp_refresh_screen = s_time_stamp_access = workload_update_main_time_stamp(s_workload);
    
    s_threads = s_workload->m_cfg.m_access_threads;
    if(s_threads <= 0) { s_threads = (int)s_workload->m_disk_count; }
    else if(s_threads > 1000) { s_threads = 1000; }
    s_service = (void **)malloc(sizeof(void *) * ((size_t)s_threads));
    if(s_service != ((void **)0)) {
        for(s_thread_index = 0;s_thread_index < s_threads;s_thread_index++) {
            s_service[s_thread_index] = workload_open_service("access service", __workload_access_service, (void *)s_workload);
        }
    }
    
    pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_time_stamp_lock));
    s_workload->m_time_stamp_start = s_time_stamp;
    pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_time_stamp_lock));
    
    /* first draw */
    (void)workload_set_terminal_changed_size();
    (void)workload_update_screen(s_workload);

    while(*s_workload->m_break_ptr == 0) {
        s_time_stamp = workload_update_main_time_stamp(s_workload);
        if((s_workload->m_cfg.m_duration > 0ull) &&
            (workload_get_main_duration(s_workload) >= s_workload->m_cfg.m_duration)) {
            if(s_workload->m_opt.m_verbose != 0) {
                (void)workload_log(" * Expire duration time : %llu >= %llu\n",
                    workload_get_main_duration(s_workload),
                    s_workload->m_cfg.m_duration
                );
            }
            *s_workload->m_break_ptr = 1;
            continue;
        }

        if((s_time_stamp - s_time_stamp_refresh_screen) >= ((unsigned long long)s_workload->m_opt.m_refresh_screen_interval)) { /* refresh interval */
            s_time_stamp_refresh_screen = s_time_stamp;
            (void)workload_update_screen(s_workload);
        }

        (void)workload_udp(s_workload);
        (void)workload_key(s_workload);
    }

    /* final draw */
    (void)workload_update_screen(s_workload);

    if(s_service != ((void **)0)) {
        for(s_thread_index = 0;s_thread_index < s_threads;s_thread_index++) {
	    if(s_service[s_thread_index] != NULL) {
	        s_service[s_thread_index] = workload_close_service(s_service[s_thread_index]);
	    }
        }

        free((void *)s_service);
    }
   
    (void)workload_unload_config(s_workload);

    return(0);
}

#endif

/* vim: set expandtab: */
/* End of source */
