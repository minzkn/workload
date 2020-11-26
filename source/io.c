/*
    Copyright (C) HWPORT.COM
    All rights reserved.
    Author: JAEHYUK CHO <mailto:minzkn@minzkn.com>
*/

#if !defined(__def_hwport_workload_source_io_c__)
# define __def_hwport_workload_source_io_c__ "io.c"

#if !defined(_ISOC99_SOURCE)
# define _ISOC99_SOURCE (1L)
#endif

#if !defined(_GNU_SOURCE)
# define _GNU_SOURCE (1L)
#endif

#include "workload.h"

#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

int workload_is_readable_fd(int s_fd, int s_timeout_msec);
int workload_is_writable_fd(int s_fd, int s_timeout_msec);

ssize_t workload_read(int s_fd, void *s_data, size_t s_size, int s_msec);
ssize_t workload_write(int s_fd, const void *s_data, size_t s_size, int s_msec);
ssize_t workload_recvfrom(int s_fd, void *s_data, size_t s_size, int s_flags, struct sockaddr *s_sockaddr, socklen_t *s_addrlen, int s_msec);
ssize_t workload_sendto(int s_fd, const void *s_data, size_t s_size, int s_flags, const struct sockaddr *s_sockaddr, socklen_t s_addrlen, int s_msec);

int workload_delete(const char *s_pathname);

static int workload_mkdir_recursive(const char *s_pathname, mode_t s_mode, char **s_first_entry);
int workload_mkdir_ex(const char *s_pathname, mode_t s_mode, char **s_first_entry);
int workload_mkdir(const char *s_pathname, mode_t s_mode);

int workload_is_readable_fd(int s_fd, int s_timeout_msec)
{
    struct timeval s_timeval;
    fd_set s_rx;
    int s_check;

    if(s_timeout_msec < 0) {
        return(0);
    }

    s_timeval.tv_sec = s_timeout_msec / 1000;
    s_timeval.tv_usec = (s_timeout_msec % 1000) * 1000;

    FD_ZERO(&s_rx);
    FD_SET(s_fd, &s_rx);

    s_check = select(s_fd + 1, (fd_set *)(&s_rx), (fd_set *)0, (fd_set *)0, (struct timeval *)(&s_timeval));
    if(s_check == (-1)) { return(-1); }
    if(s_check == 0) { return(-2); }
    if(FD_ISSET(s_fd, &s_rx) == 0) { return(-1); }

    return(0);
}

int workload_is_writable_fd(int s_fd, int s_timeout_msec)
{
    struct timeval s_timeval;
    fd_set s_tx;
    int s_check;

    if(s_timeout_msec < 0) {
        return(0);
    }
    
    s_timeval.tv_sec = s_timeout_msec / 1000;
    s_timeval.tv_usec = (s_timeout_msec % 1000) * 1000;

    FD_ZERO(&s_tx);
    FD_SET(s_fd, &s_tx);

    s_check = select(s_fd + 1, (fd_set *)0, (fd_set *)(&s_tx), (fd_set *)0, (struct timeval *)(&s_timeval));
    if(s_check == (-1)) { return(-1); }
    if(s_check == 0) { return(-2); }
    if(FD_ISSET(s_fd, &s_tx) == 0) { return(-1); }

    return(0);
}

ssize_t workload_read(int s_fd, void *s_data, size_t s_size, int s_msec)
{
    int s_check;

    s_check = workload_is_readable_fd(s_fd, s_msec);
    if(s_check != 0) { return((ssize_t)s_check); }

    return(read(s_fd, s_data, s_size));
}

ssize_t workload_write(int s_fd, const void *s_data, size_t s_size, int s_msec)
{
    int s_check;

    s_check = workload_is_writable_fd(s_fd, s_msec);
    if(s_check != 0) {
        if(s_check == (-2)) { return((ssize_t)0); }
        return((ssize_t)s_check);
    }

    return(write(s_fd, s_data, s_size));
}

ssize_t workload_recvfrom(int s_fd, void *s_data, size_t s_size, int s_flags, struct sockaddr *s_sockaddr, socklen_t *s_addrlen, int s_msec)
{
    int s_check;

    s_check = workload_is_readable_fd(s_fd, s_msec);
    if(s_check != 0) { return((ssize_t)s_check); }

    return(recvfrom(s_fd, s_data, s_size, s_flags, s_sockaddr, s_addrlen));
}

ssize_t workload_sendto(int s_fd, const void *s_data, size_t s_size, int s_flags, const struct sockaddr *s_sockaddr, socklen_t s_addrlen, int s_msec)
{
    int s_check;

    s_check = workload_is_writable_fd(s_fd, s_msec);
    if(s_check != 0) {
        if(s_check == (-2)) { return((ssize_t)0); }
        return((ssize_t)s_check);
    }

    return(sendto(s_fd, s_data, s_size, s_flags, s_sockaddr, s_addrlen));
}

int workload_delete(const char *s_pathname)
{
    struct stat s_stat;

    DIR *s_dir;
    struct dirent *s_dirent;
    char *s_sub_path;

    if(lstat((const char *)s_pathname, (struct stat *)(&s_stat)) == (-1)) { return(0); }

    if(S_ISDIR(s_stat.st_mode) == 0) { return(remove(s_pathname)); }

    s_dir = opendir(s_pathname);
    if(s_dir == NULL) { return(remove(s_pathname)); }

    for(;;) {
        s_dirent = readdir(s_dir);
        if(s_dirent == NULL) { break; }
        if((strcmp(s_dirent->d_name, ".") == 0) || (strcmp(s_dirent->d_name, "..") == 0)) { continue; }

        s_sub_path = (char *)0;
	if(asprintf((char **)(&s_sub_path), "%s%s%s", s_pathname, "/", s_dirent->d_name) == (-1)) {
	    if(s_sub_path != NULL) { free((void *)s_sub_path); }
	    continue;
	}
        if(s_sub_path == NULL) { continue; }

        (void)workload_delete(s_sub_path);

        free((void *)s_sub_path);
    }
    (void)closedir(s_dir);

    return(remove(s_pathname));
}

static int workload_mkdir_recursive(const char *s_pathname, mode_t s_mode, char **s_first_entry)
{
    int s_result;
    struct stat s_sys_stat;
    mode_t s_mask;
    size_t s_tail_offset;
    char *s_parent_pathname;
    int s_check;
    int s_errno;

    if(s_pathname == NULL) { return(-1); }

    s_check = stat(s_pathname, (struct stat *)(&s_sys_stat));
    s_errno = errno;

    if(s_check == 0) {
        if(S_ISDIR(s_sys_stat.st_mode) != 0) { return(0); }

        return(-1);
    }
    if(s_check != (-1)) { return(-1); }
 
    if(s_errno == ENOENT) {
        s_mask = umask((mode_t)0);
        (void)umask(s_mask);

        s_tail_offset = strlen(s_pathname);
        if(s_tail_offset <= ((size_t)0u)) { return(-1); }
        --s_tail_offset;

        while(s_tail_offset > ((size_t)0u)) {
            if((s_pathname[s_tail_offset] != '/') &&
	       (s_pathname[s_tail_offset] != '\\')) {
	        break;
	    }

            --s_tail_offset;
        }

        for(s_result = 0;s_tail_offset > ((size_t)0u);) {
            if((s_pathname[s_tail_offset] == '/') ||
	       (s_pathname[s_tail_offset] == '\\')) {
                s_parent_pathname = strndup(s_pathname, s_tail_offset);
                if(s_parent_pathname != NULL) {
                    s_result = workload_mkdir_recursive(
		        s_parent_pathname,
			(((mode_t)(S_IRWXU | S_IRWXG | S_IRWXO)) & (~s_mask)) | ((mode_t)(S_IWUSR | S_IXUSR)),
			s_first_entry);
                    free((void *)s_parent_pathname);
                }
                break;
            }
            --s_tail_offset;
        }
        if(s_result != 0) { return(s_result); }

        s_check = mkdir(s_pathname, s_mode);
	if((s_check == (-1)) && (errno == EEXIST)) { return(0); }
        if(s_check == 0) {
            if(s_first_entry != ((char **)0)) {
                if((*s_first_entry) == NULL) {
                    *s_first_entry = strdup(s_pathname);
                }
            }
            return(0);
        }

        return(-1);
    }
 
    return(-1);
}

int workload_mkdir_ex(const char *s_pathname, mode_t s_mode, char **s_first_entry)
{
    int s_check;
    char *s_entry;

    s_entry = (char *)0;

    s_check = workload_mkdir_recursive(s_pathname, s_mode, (char **)(&s_entry));
    if(s_check == (-1)) {
        if(s_entry != NULL) {
            (void)workload_delete(s_entry);
            free((void *)s_entry);
	    s_entry = (char *)0;
        }
    }
    
    if(s_first_entry == ((char **)0)) {
        if(s_entry != NULL) { free((void *)s_entry); }
    }
    else { *s_first_entry = s_entry; }

    return(s_check);
}

int workload_mkdir(const char *s_pathname, mode_t s_mode)
{
    return(workload_mkdir_ex(s_pathname, s_mode, (char **)0));
}

#endif

/* vim: set expandtab: */
/* End of source */
