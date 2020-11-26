/*
    Copyright (C) HWPORT.COM
    All rights reserved.
    Author: JAEHYUK CHO <mailto:minzkn@minzkn.com>
*/

#if !defined(__def_hwport_workload_source_workload_access_c__)
# define __def_hwport_workload_source_workload_access_c__ "workdload_access.c"

#if !defined(_ISOC99_SOURCE)
# define _ISOC99_SOURCE (1L)
#endif

#if !defined(_GNU_SOURCE)
# define _GNU_SOURCE (1L)
#endif

#include "workload.h"

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int workload_file_access(workload_t *s_workload, workload_file_t *s_file);
int workload_reset_file_access_count(workload_t *s_workload);

int workload_file_access(workload_t *s_workload, workload_file_t *s_file)
{
    char *s_pathname;
  
    int s_access_mode; /* 0=read, 1=write */
    int s_fd;
    int s_delta_read = 0;
    int s_delta_write = 0;
   
    unsigned char s_buffer[ 4 << 10 ];

    unsigned long long s_access_count;

    if(s_workload == NULL) { return(-1); }
    if(s_file == NULL) { return(-1); }
    
    if(s_file->m_assign_disk == NULL) { /* not assigned */ return(0); }

    s_pathname = workload_file_pathname_alloc(s_file, NULL);
    if(s_pathname == NULL) {
        return(-1);
    }

    if(s_workload->m_cfg.m_access_method == def_workload_access_method_read) {
        s_access_mode = 0;
    }
    else if(s_workload->m_cfg.m_access_method == def_workload_access_method_write) {
        s_access_mode = 1;
    }
    else { s_access_mode = workload_rand() % 2; }

    if((s_workload->m_relocate != 0) &&
        (s_file->m_assign_disk->m_fakeoff_delay != (-1ll)) && 
        ((workload_get_main_time_stamp(s_workload) - s_file->m_assign_disk->m_fakeoff_time_stamp) >= ((unsigned long long)s_file->m_assign_disk->m_fakeoff_delay))
    ) { /* fake access */
#if 0L /* with access count */
        if(s_access_mode == 0) { s_delta_read = 1; }
	else { s_delta_write = 1; }
#endif	
    }
    else if(s_access_mode == 0) { /* read */
        s_fd = open(s_pathname, O_RDONLY | O_LARGEFILE /* | ((s_workload->m_cfg.m_access_sync == 0) ? 0 : O_DIRECT) */);
        if(s_fd != (-1)) {
            if(s_workload->m_cfg.m_filesize <= ((off_t)0)) { s_delta_read = 1; }
            while((*s_workload->m_break_ptr == 0) && (workload_read(s_fd, (void *)(&s_buffer[0]), sizeof(s_buffer), (-1)) > ((ssize_t)0)))s_delta_read = 1;
            close(s_fd);
        }
    }
    else { /* write */
        s_fd = open(s_pathname, O_WRONLY | O_TRUNC | O_LARGEFILE /* | ((s_workload->m_cfg.m_access_sync == 0) ? 0 : O_SYNC) */ /* | ((s_workload->m_cfg.m_access_sync == 0) ? 0 : O_DIRECT) */);
        if(s_fd != (-1)) {
            off_t s_offset;
            size_t s_want_size;
            ssize_t s_write_bytes;

            if(s_workload->m_cfg.m_filesize <= ((off_t)0)) { s_delta_write = 1; }

            for(s_offset = (off_t)0;(*s_workload->m_break_ptr == 0) && (s_offset < s_workload->m_cfg.m_filesize);) {
                s_want_size = ((s_workload->m_cfg.m_filesize - s_offset) >= ((off_t)sizeof(s_buffer))) ? sizeof(s_buffer) : ((size_t)(s_workload->m_cfg.m_filesize - s_offset));
                (void)workload_fill_rand((void *)(&s_buffer[0]), s_want_size);
                s_write_bytes = workload_write(s_fd, (const void *)(&s_buffer[0]), s_want_size, (-1));
                if(s_write_bytes <= ((ssize_t)0)) { break; }
                s_delta_write = 1;
                s_offset += (off_t)s_write_bytes;
            }

            if(s_workload->m_cfg.m_access_sync != 0) { (void)fsync(s_fd); }

            close(s_fd);
        }
    }
    
    free((void *)s_pathname);

    if((s_delta_read + s_delta_write) > 0) {
        size_t s_disk_index, s_file_index;
        
	pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_reassign_lock));

        s_file->m_read_count += (unsigned long long)s_delta_read;
        s_file->m_write_count += (unsigned long long)s_delta_write;
        s_access_count = s_file->m_read_count + s_file->m_write_count;
        s_file->m_time_stamp = workload_get_main_time_stamp(s_workload);

        s_file->m_assign_disk->m_read_count += (unsigned long long)s_delta_read;
        s_file->m_assign_disk->m_write_count += (unsigned long long)s_delta_write;
        s_file->m_assign_disk->m_time_stamp = workload_get_main_time_stamp(s_workload);

        s_workload->m_read_count += (unsigned long long)s_delta_read;
        s_workload->m_write_count += (unsigned long long)s_delta_write;

        /* sort index */
        s_file_index = s_file->m_index;
        while(s_file_index > ((size_t)0u)) {
            if((s_workload->m_file_sorted[s_file_index - ((size_t)1u)]->m_read_count + s_workload->m_file_sorted[s_file_index - ((size_t)1u)]->m_write_count) >= s_access_count) { break; }
        
            s_workload->m_file_sorted[s_file_index] = s_workload->m_file_sorted[s_file_index - ((size_t)1u)];
            s_workload->m_file_sorted[s_file_index]->m_index = s_file_index;
            --s_file_index;
            s_workload->m_file_sorted[s_file_index] = s_file;
            s_workload->m_file_sorted[s_file_index]->m_index = s_file_index;
        }
    
        if(s_workload->m_next_file_index != NULL) {
            for(s_disk_index = (size_t)0u,
                s_file_index = (size_t)0u;
                s_disk_index < s_workload->m_disk_count;
                s_disk_index++) {
                for(;s_file_index < s_workload->m_next_file_index[s_disk_index];s_file_index++) {
                    s_workload->m_file_sorted[s_file_index]->m_reassign_disk = (s_workload->m_file_sorted[s_file_index]->m_assign_disk == s_workload->m_disk[s_disk_index]) ? NULL : s_workload->m_disk[s_disk_index];
                }
            }
	}
        
	(void)workload_update_db(s_workload, s_file, s_file->m_assign_disk);
        
	pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
    }
    
    return(s_delta_read + s_delta_write);
}

int workload_reset_file_access_count(workload_t *s_workload)
{
    workload_disk_t *s_disk;
    workload_file_t *s_file;
	
    pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
    
    for(s_file = s_workload->m_file_head;s_file != NULL;s_file = s_file->m_next) {
        s_file->m_read_count = s_file->m_write_count = 0ull;
        s_file->m_time_stamp = s_workload->m_time_stamp;

        (void)workload_update_db(s_workload, s_file, NULL);
    }
    for(s_disk = s_workload->m_disk_head;s_disk != NULL;s_disk = s_disk->m_next) {
        s_disk->m_read_count = s_disk->m_write_count = 0ull;
        s_disk->m_time_stamp = s_workload->m_time_stamp;
        (void)workload_update_db(s_workload, NULL, s_disk);
    }
    s_workload->m_read_count = s_workload->m_write_count = 0ull;
    
    pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
    
    return(0);
}

#endif

/* vim: set expandtab: */
/* End of source */
