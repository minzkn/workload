/*
    Copyright (C) HWPORT.COM
    All rights reserved.
    Author: JAEHYUK CHO <mailto:minzkn@minzkn.com>
*/

#if !defined(__def_hwport_workload_source_workload_storage_c__)
# define __def_hwport_workload_source_workload_storage_c__ "workdload_storage.c"

#include "workload.h"

#include <sys/stat.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <fcntl.h>

void workload_disk_service(void *s_service_handle, void *s_argument);

workload_disk_t *workload_new_disk(workload_t *s_workload, unsigned int s_id, const char *s_name, const char *s_path, int s_is_create_path);
workload_disk_t *workload_free_disk(workload_disk_t *s_disk);

int workload_free_disk_array(workload_t *s_workload);
int workload_update_disk_array(workload_t *s_workload);

void workload_disk_service(void *s_service_handle, void *s_argument)
{
    workload_disk_t *s_disk = (workload_disk_t *)s_argument;
    workload_t *s_workload;

    char *s_created_path = NULL;
    
    workload_file_t *s_file;

    int s_to_fd;
    char *s_to_pathname;
    off_t s_offset;
    size_t s_want_size;
    ssize_t s_write_bytes;
    unsigned char s_buffer[4 << 10];

    s_workload = s_disk->m_workload;

    workload_ack_service(s_service_handle);
  
    if((s_disk->m_flags & def_workload_disk_flag_create_path) == def_workload_disk_flag_none) {
        struct stat s_sys_stat;
        if((stat(s_disk->m_path, (struct stat *)(&s_sys_stat)) == (-1)) ||
           (S_ISDIR(s_sys_stat.st_mode) == 0)) {
            if(s_workload->m_opt.m_verbose != 0) {
                (void)workload_log(
                    " * WARNING: No entry path \"%s\" ! (path=\"%s\", create=\"%s\")\n",
                    s_disk->m_name,
                    s_disk->m_path,
                    "no"
                );
            }
            /* failed */
            pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
            ++s_workload->m_disk_error_count;
            pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
	    return;
        }
    }
    else {
        if(workload_mkdir_ex(s_disk->m_path, 0755, (char **)(&s_created_path)) == (-1)) {
            if(s_workload->m_opt.m_verbose != 0) {
                (void)workload_log(
                    " * WARNING: Failed mkdir \"%s\" ! (path=\"%s\", create=\"%s\")\n",
                    s_disk->m_name,
                    s_disk->m_path,
                    "yes"
                );
            }
            /* failed */
            pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
            ++s_workload->m_disk_error_count;
            pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
	    return;
        }
    }

    for(s_file = s_disk->m_assign_file;(s_file != NULL) && (workload_break_service(s_service_handle) == 0);s_file = s_file->m_assign_next) {
        s_to_pathname = workload_file_pathname_alloc(s_file, s_disk);
        if(s_to_pathname == NULL) {
            pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
            ++s_workload->m_disk_error_count;
            pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
	    break;
	}
       
        pthread_mutex_lock((pthread_mutex_t *)(&s_file->m_access_lock));
	s_to_fd = open(s_to_pathname, O_CREAT | O_EXCL | O_WRONLY | O_LARGEFILE /* | ((s_workload->m_access_sync == 0) ? 0 : O_SYNC) */ /* | O_DIRECT */, 0644);
        if(s_to_fd == (-1)) {
            s_to_fd = open(s_to_pathname, O_CREAT | O_WRONLY | O_TRUNC | O_LARGEFILE /* | ((s_workload->m_access_sync == 0) ? 0 : O_SYNC) */ /* | O_DIRECT */, 0644);
        }
        free((void *)s_to_pathname);

	if(s_to_fd == (-1)) {
            pthread_mutex_unlock((pthread_mutex_t *)(&s_file->m_access_lock));
            pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
            ++s_workload->m_disk_error_count;
            pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
	    break;
	}
        
	for(s_offset = (off_t)0;(*s_workload->m_break_ptr == 0) && (s_offset < s_workload->m_cfg.m_filesize);) {
            s_want_size = ((s_workload->m_cfg.m_filesize - s_offset) >= ((off_t)sizeof(s_buffer))) ? sizeof(s_buffer) : ((size_t)(s_workload->m_cfg.m_filesize - s_offset));
            (void)workload_fill_rand((void *)(&s_buffer[0]), s_want_size);
            s_write_bytes = workload_write(s_to_fd, (const void *)(&s_buffer[0]), s_want_size, (-1));
            if(s_write_bytes <= ((ssize_t)0)) { break; }
            s_offset += (off_t)s_write_bytes;
        }

        close(s_to_fd);
        pthread_mutex_unlock((pthread_mutex_t *)(&s_file->m_access_lock));
	
	/* - */
	
	pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
        
	s_file->m_write_count += 1ull;
        s_file->m_time_stamp = workload_get_main_time_stamp(s_workload);

        s_disk->m_write_count += 1ull;
        s_disk->m_time_stamp = workload_get_main_time_stamp(s_workload);

        s_workload->m_write_count += 1ull;

	(void)workload_update_db(s_workload, s_file, s_disk);
        
	pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
    }

    pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
    ++s_workload->m_disk_ready_count;
    pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_reassign_lock));

    while(workload_break_service(s_service_handle) == 0) {
        workload_sleep_wait(0, 100);
    }
        
    if(s_created_path != NULL) {
        (void)workload_delete(s_created_path);
        free((void *)s_created_path); 
    }
}

workload_disk_t *workload_new_disk(workload_t *s_workload, unsigned int s_id, const char *s_name, const char *s_path, int s_is_create_path)
{
    workload_disk_t *s_disk;
    size_t s_path_size;
    size_t s_name_size;
    char *s_name_entry;
    char *s_path_entry;

    if(s_path == NULL) { return(NULL); }
    s_name_size = strlen(workload_check_string(s_name));
    s_path_size = strlen(s_path);
    s_disk = (workload_disk_t *)malloc(
        sizeof(workload_disk_t) + s_name_size + ((size_t)1u) + s_path_size + ((size_t)1u)
    );
    if(s_disk == NULL) { return(NULL); }
    s_name_entry = (char *)(&s_disk[1]);
    s_path_entry = (char *)(&s_name_entry[s_name_size + ((size_t)1u)]);

    s_disk->m_prev = s_disk->m_next = NULL;

    s_disk->m_flags = def_workload_disk_flag_none;

    s_disk->m_id = s_id;
    s_disk->m_path = (const char *)strcpy(s_path_entry, s_path);
    s_disk->m_name = (s_name_size <= ((size_t)0u)) ? ((const char *)workload_basename(s_path_entry)) : ((const char *)strcpy(s_name_entry, s_name));

    s_disk->m_name_size = strlen(s_disk->m_name);
    s_disk->m_path_size = strlen(s_disk->m_path);

    s_disk->m_fakeoff_delay = (-1ll);
  
    s_disk->m_service = NULL;
    s_disk->m_workload = s_workload;

    s_disk->m_read_count = 0ull;
    s_disk->m_write_count = 0ull;
    s_disk->m_time_stamp = s_workload->m_time_stamp;
    s_disk->m_fakeoff_time_stamp = s_workload->m_time_stamp;
    
    s_disk->m_assign_file_count = (size_t)0u;
    s_disk->m_assign_file = NULL;

    /* ---- */

    if(s_is_create_path != 0) {
        s_disk->m_flags |= def_workload_disk_flag_create_path;
    }

#if 0L
    s_disk->m_service = workload_open_service("disk", workload_disk_service, (void *)s_disk);
    if(s_disk->m_service == NULL) {
        return(workload_free_disk(s_disk));
    }
#endif    

    return(s_disk);
}

workload_disk_t *workload_free_disk(workload_disk_t *s_disk)
{
    workload_disk_t *s_prev;

    while(s_disk != NULL) {
        s_prev = s_disk;
        s_disk = s_disk->m_next;

	if(s_prev->m_service != NULL) {
	    (void)workload_close_service(s_prev->m_service);
	}
    
        free((void *)s_prev);
    }

    return(NULL);
}

int workload_free_disk_array(workload_t *s_workload)
{
    if(s_workload == NULL) { return(-1); }

    if(s_workload->m_disk == ((workload_disk_t **)0)) {
        return(0);
    }

    free((void *)s_workload->m_disk);
    s_workload->m_disk = (workload_disk_t **)0;
    s_workload->m_disk_count = (size_t)0u;
    s_workload->m_max_name_size = (size_t)0u;

    return(0);
}

int workload_update_disk_array(workload_t *s_workload)
{
    workload_disk_t *s_disk;
    size_t s_index;

    if(workload_free_disk_array(s_workload) == (-1)) { return(-1); }

    for(s_disk = s_workload->m_disk_head;s_disk != NULL;s_disk = s_disk->m_next, s_workload->m_disk_count++);
    if(s_workload->m_disk_count <= ((size_t)0u)) { return(0); }

    s_workload->m_disk = (workload_disk_t **)malloc(sizeof(workload_disk_t *) * s_workload->m_disk_count);
    if(s_workload->m_disk == ((workload_disk_t **)0)) {
        s_workload->m_disk_count = (size_t)0u;
        return(-1);
    }
    for(s_disk = s_workload->m_disk_head, s_index = (size_t)0u;s_disk != NULL;s_disk = s_disk->m_next, s_index++) {
        s_workload->m_disk[s_index] = s_disk;
        if(s_disk->m_name_size > s_workload->m_max_name_size) { s_workload->m_max_name_size = s_disk->m_name_size; }
    }

    return(0);
}

#endif

/* vim: set expandtab: */
/* End of source */
