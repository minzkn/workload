/*
    Copyright (C) HWPORT.COM
    All rights reserved.
    Author: JAEHYUK CHO <mailto:minzkn@minzkn.com>
*/

#if !defined(__def_hwport_workload_source_workload_control_c__)
# define __def_hwport_workload_source_workload_control_c__ "workdload_control.c"

#if !defined(_ISOC99_SOURCE)
# define _ISOC99_SOURCE (1L)
#endif

#if !defined(_GNU_SOURCE)
# define _GNU_SOURCE (1L)
#endif

#include "workload.h"

#include <unistd.h>
#include <getopt.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int workload_control_txrx(int s_noresponse, const void *s_data, size_t s_size);
static int workload_control_command(int s_noresponse, const char *s_format, ...) __attribute__((__format__(__printf__,2,3)));

int workload_control_main(int s_argc, char **s_argv);

static const char *g_workload_control_ip = "127.0.0.1";

static int workload_control_txrx(int s_noresponse, const void *s_data, size_t s_size)
{
    int s_socket;
    struct sockaddr_in s_sockaddr_in;

    s_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(s_socket == (-1)) { return(-1); }

    (void)memset((void *)(&s_sockaddr_in), 0, sizeof(s_sockaddr_in));
    s_sockaddr_in.sin_family = AF_INET;
    s_sockaddr_in.sin_addr.s_addr = htonl(INADDR_ANY);
    s_sockaddr_in.sin_port = htons(0);

    if(bind(s_socket, (struct sockaddr *)(&s_sockaddr_in), (socklen_t)sizeof(s_sockaddr_in)) == (-1)) {
        (void)close(s_socket);
        return(-1);
    }

    s_sockaddr_in.sin_addr.s_addr = inet_addr(g_workload_control_ip);
    s_sockaddr_in.sin_port = htons(def_workload_config_control_port);

    if(workload_sendto(
        s_socket,
        s_data,
        s_size,
        MSG_NOSIGNAL,
        (struct sockaddr *)(&s_sockaddr_in),
        (socklen_t)sizeof(s_sockaddr_in),
        16 * 1000) != ((ssize_t)s_size)) {
        (void)close(s_socket);
        return(-1);
    }

    if(s_noresponse == 0) {
        unsigned char s_res_buffer[ 4 << 10 ];
        ssize_t s_recv_bytes;

        s_recv_bytes = workload_recvfrom(
            s_socket,
            (void *)(&s_res_buffer[0]),
            sizeof(s_res_buffer),
            MSG_NOSIGNAL,
            NULL,
            NULL,
            16 * 1000
        );
        if(s_recv_bytes <= ((ssize_t)0)) {
            (void)close(s_socket);
            return(-1);
        }
        
        if(s_recv_bytes < ((ssize_t)sizeof(s_res_buffer))) { s_res_buffer[s_recv_bytes] = '\0'; }
        else { s_res_buffer[sizeof(s_res_buffer) - ((size_t)1u)] = '\0'; }

        (void)fputs((const char *)(&s_res_buffer[0]), stdout);
        (void)fflush(stdout);
    }

    (void)close(s_socket);

    return(0);
}

static int workload_control_command(int s_noresponse, const char *s_format, ...)
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

    if(s_string == NULL) { return(-1); }
  
    s_result = workload_control_txrx(s_noresponse, (const void *)s_string, strlen(s_string));

    free((void *)s_string);
        
    return(s_result);
}

int workload_control_main(int s_argc, char **s_argv)
{
    int s_exit_code = EXIT_SUCCESS;
    int s_option_check, s_option_index;
    
    int s_do_help = 0;
    
    char s_cmd_buffer[ 4 << 10 ];
    size_t s_cmd_offset;

    (void)workload_set_log_mode(1 /* puts */);
    
    s_cmd_offset = (size_t)0u;
    s_cmd_buffer[s_cmd_offset] = '\0';

    for(s_option_index = 0;;) {
        static struct option sg_workload_options[] = {
            {"help", 0, (int *)0, 'h'},
            {"ip", 0, (int *)0, 0},
            {"cmd", 0, (int *)0, 'c'},
            {"quit", 0, (int *)0, 'q'},
            {"restart", 0, (int *)0, 'Q'},
            {"pause", 0, (int *)0, 'p'},
            {"resume", 0, (int *)0, 'r'},
            {"oneshot", 0, (int *)0, 'o'},
            {"start", 0, (int *)0, 's'},
            {"stop", 0, (int *)0, 'k'},
            {"toggle", 0, (int *)0, 0},
            {"status", 0, (int *)0, 0},
            {"left", 1, (int *)0, 0},
            {"right", 1, (int *)0, 0},
            {"reset", 0, (int *)0, 0},
            {(char *)0, 0, (int *)0, 0}
        };

        s_option_check = getopt_long(s_argc, s_argv, "hc:qQprosk", sg_workload_options, &s_option_index);
        if(s_option_check == (-1)) { break; }

        switch(s_option_check) {
            case 0:
                if(sg_workload_options[s_option_index].flag != ((int *)0)) { break; }
                if(strcmp(sg_workload_options[s_option_index].name, "ip") == 0) {
                    if(optarg != NULL) { g_workload_control_ip = optarg; }
                }
                else if(strcmp(sg_workload_options[s_option_index].name, "status") == 0) {
                    s_cmd_offset += (size_t)snprintf((char *)(&s_cmd_buffer[s_cmd_offset]), sizeof(s_cmd_buffer) - s_cmd_offset, "?\n");
                }
                else if(strcmp(sg_workload_options[s_option_index].name, "left") == 0) {
                    if(optarg != NULL) {
                        s_cmd_offset += (size_t)snprintf((char *)(&s_cmd_buffer[s_cmd_offset]), sizeof(s_cmd_buffer) - s_cmd_offset, "l %s\n", optarg);
                    }
                }
                else if(strcmp(sg_workload_options[s_option_index].name, "right") == 0) {
                    if(optarg != NULL) {
                        s_cmd_offset += (size_t)snprintf((char *)(&s_cmd_buffer[s_cmd_offset]), sizeof(s_cmd_buffer) - s_cmd_offset, "r %s\n", optarg);
                    }
                }
                else if(strcmp(sg_workload_options[s_option_index].name, "reset") == 0) {
                    s_cmd_offset += (size_t)snprintf((char *)(&s_cmd_buffer[s_cmd_offset]), sizeof(s_cmd_buffer) - s_cmd_offset, "!\n");
                }
                else if(strcmp(sg_workload_options[s_option_index].name, "toggle") == 0) {
                    s_cmd_offset += (size_t)snprintf((char *)(&s_cmd_buffer[s_cmd_offset]), sizeof(s_cmd_buffer) - s_cmd_offset, "r\n");
		}
                break;  
            case 'c':
                if(optarg != NULL) {
                    s_cmd_offset += (size_t)snprintf((char *)(&s_cmd_buffer[s_cmd_offset]), sizeof(s_cmd_buffer) - s_cmd_offset, "%s\n", optarg);
                }
                break;
            case 'q':
                s_cmd_offset += (size_t)snprintf((char *)(&s_cmd_buffer[s_cmd_offset]), sizeof(s_cmd_buffer) - s_cmd_offset, "q\n");
                break;
            case 'Q':
                s_cmd_offset += (size_t)snprintf((char *)(&s_cmd_buffer[s_cmd_offset]), sizeof(s_cmd_buffer) - s_cmd_offset, "Q\n");
                break;
            case 'p':
                s_cmd_offset += (size_t)snprintf((char *)(&s_cmd_buffer[s_cmd_offset]), sizeof(s_cmd_buffer) - s_cmd_offset, "P\n");
                break;
            case 'r':
                s_cmd_offset += (size_t)snprintf((char *)(&s_cmd_buffer[s_cmd_offset]), sizeof(s_cmd_buffer) - s_cmd_offset, "R\n");
                break;
            case 'o':
                s_cmd_offset += (size_t)snprintf((char *)(&s_cmd_buffer[s_cmd_offset]), sizeof(s_cmd_buffer) - s_cmd_offset, "o\n");
                break;
            case 's':
                s_cmd_offset += (size_t)snprintf((char *)(&s_cmd_buffer[s_cmd_offset]), sizeof(s_cmd_buffer) - s_cmd_offset, "S\n");
                break;
            case 'k':
                s_cmd_offset += (size_t)snprintf((char *)(&s_cmd_buffer[s_cmd_offset]), sizeof(s_cmd_buffer) - s_cmd_offset, "K\n");
                break;
            case '?':
            case 'h':
            default:
                s_do_help = 1;
                break;
        }
    }

    if((s_do_help != 0) || (strlen((const char *)(&s_cmd_buffer[0])) <= ((size_t)0u))) {
        (void)workload_printf(
            "workload v%s\n\n"
	    "usage: %s [<options>]\n"
            "options:\n"
            "\t-h, --help                       : help\n"
            "\t    --ip=<server ip>             : server ip\n"
            "\t-c, --cmd=<custum command>       : custum command\n"
            "\t-q, --quit                       : quit command\n"
            "\t-Q, --restart                    : restart command\n"
            "\t-p, --pause                      : pause command\n"
            "\t-r, --resume                     : resume command\n"
            "\t-o, --oneshot                    : oneshot relocate command\n"
            "\t-s, --start                      : start relocate command\n"
            "\t-k, --stop                       : stop relocate command\n"
            "\t    --toggle                     : toggle relocate command\n"
            "\t    --status                     : status command\n"
            "\t    --left=<ratio>               : left command\n"
            "\t    --right=<ratio>              : right command\n"
            "\t    --reset                      : reset command\n"
            "\n"
            "Copyright (C) HWPORT.COM - All rights reserved.\n"
            "Author: JaeHyuk Cho <mailto:minzkn@minzkn.com>\n"
            "http://www.hwport.com/\n",
	    def_workload_version,
            basename(s_argv[0])
        );
    }
    else {
        if(workload_control_command(0, "workload\n%s\n", (const char *)(&s_cmd_buffer[0])) == (-1)) { s_exit_code = EXIT_FAILURE; }
    }

    return(s_exit_code);
}
        
#endif

/* vim: set expandtab: */
/* End of source */
