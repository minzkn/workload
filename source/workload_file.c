/*
    Copyright (C) HWPORT.COM
    All rights reserved.
    Author: JAEHYUK CHO <mailto:minzkn@minzkn.com>
*/

#if !defined(__def_hwport_workload_source_workload_file_c__)
# define __def_hwport_workload_source_workload_file_c__ "workdload_file.c"

#if !defined(_ISOC99_SOURCE)
# define _ISOC99_SOURCE (1L)
#endif

#if !defined(_GNU_SOURCE)
# define _GNU_SOURCE (1L)
#endif

#include "workload.h"

#include <sys/stat.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <fcntl.h>

workload_file_t *workload_new_file(workload_t *s_workload, unsigned int s_id);
workload_file_t *workload_free_file(workload_file_t *s_file);

int workload_free_file_array(workload_t *s_workload);
int workload_update_file_array(workload_t *s_workload);

char *workload_file_pathname_alloc(workload_file_t *s_file, workload_disk_t *s_disk);

static int workload_deassign_file(workload_file_t *s_file);
static int __workload_move_assign_file(workload_t *s_workload, workload_file_t *s_file, workload_disk_t *s_disk);
int workload_assign_file_to_disk(workload_t *s_workload, workload_file_t *s_file, workload_disk_t *s_disk);

workload_file_t *workload_new_file(workload_t *s_workload, unsigned int s_id)
{
    workload_file_t *s_file;

    size_t s_filename_size;

    s_filename_size = strlen(workload_check_string(s_workload->m_cfg.m_prefix));
    s_filename_size += (size_t)10u;
    s_filename_size += strlen(workload_check_string(s_workload->m_cfg.m_suffix));

    s_file = (workload_file_t *)malloc(sizeof(workload_file_t) + s_filename_size + ((size_t)1u));
    if(s_file == NULL) { return(NULL); }

    s_file->m_prev = s_file->m_next = NULL;
    s_file->m_assign_prev = s_file->m_assign_next = NULL;

    s_file->m_assign_disk = s_file->m_reassign_disk = NULL;

    s_file->m_flags = def_workload_file_flag_none;

    s_file->m_index = (size_t)0u;

    s_file->m_id = s_id;
    (void)sprintf((char *)(&s_file[1]), "%s%u%s", s_workload->m_cfg.m_prefix, s_file->m_id, s_workload->m_cfg.m_suffix);
    s_file->m_filename = (const char *)(&s_file[1]);

    pthread_mutex_init((pthread_mutex_t *)(&s_file->m_access_lock), NULL);

    s_file->m_read_count = 0ull;
    s_file->m_write_count = 0ull;
    s_file->m_time_stamp = s_workload->m_time_stamp;

    return(s_file);
}

workload_file_t *workload_free_file(workload_file_t *s_file)
{
    if(s_file == NULL) { return(NULL); }

    /* deassign from disk */
    (void)workload_deassign_file(s_file);
    
    pthread_mutex_destroy((pthread_mutex_t *)(&s_file->m_access_lock));
    
    free((void *)s_file);

    return(NULL);
}

int workload_free_file_array(workload_t *s_workload)
{
    if(s_workload == NULL) { return(-1); }
    if(s_workload->m_file_ordered == ((workload_file_t **)0)) { return(0); }

    free((void *)s_workload->m_file_ordered);
    s_workload->m_file_even = (workload_file_t **)0;
    s_workload->m_file_sorted = (workload_file_t **)0;
    s_workload->m_file_ordered = (workload_file_t **)0;
    s_workload->m_file_count = (size_t)0u;

    return(0);
}

int workload_update_file_array(workload_t *s_workload)
{
    workload_file_t *s_file;
    size_t s_file_count;
    size_t s_index;

    for(s_file = s_workload->m_file_head, s_file_count = (size_t)0u;s_file != NULL;s_file = s_file->m_next, s_file_count++);
    if(s_file_count <= ((size_t)0u)) {
        if(workload_free_file_array(s_workload) == (-1)) { return(-1); }
        return(0);
    }
    if(s_workload->m_file_count != s_file_count) { /* reallocate array */
        if(workload_free_file_array(s_workload) == (-1)) { return(-1); }

        s_workload->m_file_ordered = (workload_file_t **)malloc(sizeof(workload_file_t *) * (s_file_count * ((size_t)3u)));
        if(s_workload->m_file_ordered == ((workload_file_t **)0)) { return(-1); }
        s_workload->m_file_sorted = (workload_file_t **)(&s_workload->m_file_ordered[s_file_count]);
        s_workload->m_file_even = (workload_file_t **)(&s_workload->m_file_sorted[s_file_count]);
        s_workload->m_file_count = s_file_count;
    }

    for(s_file = s_workload->m_file_head, s_index = (size_t)0u;s_file != NULL;s_file = s_file->m_next, s_index++) {
        s_workload->m_file_ordered[s_index] = s_file;
        s_workload->m_file_sorted[s_index] = s_file;
        s_workload->m_file_sorted[s_index]->m_index = s_index;
        s_workload->m_file_even[s_index] = s_file;
    }

    return(0);
}

char *workload_file_pathname_alloc(workload_file_t *s_file, workload_disk_t *s_disk)
{
    char *s_pathname = NULL;

    if(s_file == NULL) { return(NULL); }
    if(s_disk == NULL) {
        if(s_file->m_assign_disk == NULL) { return(NULL); }
        s_disk = s_file->m_assign_disk;
    }

    if(asprintf(
        (char **)(&s_pathname),
        "%s/%s",
        s_disk->m_path,
        s_file->m_filename
    ) == (-1)) {
        if(s_pathname != NULL) { free((void *)s_pathname); }
        return(NULL);
    }

    return(s_pathname);
}

static int workload_deassign_file(workload_file_t *s_file)
{
    workload_disk_t *s_assign_disk = s_file->m_assign_disk;

    if(s_assign_disk == NULL) { /* not assigned */ return(0); }
   
    if((s_file->m_flags & def_workload_file_flag_created) != def_workload_file_flag_none) {
        char *s_pathname = workload_file_pathname_alloc(s_file, NULL);
        if(s_pathname != NULL) {
            (void)workload_delete(s_pathname);
            s_file->m_flags &= (~def_workload_file_flag_created);
            free((void *)s_pathname);
	}
    }
   
    if(s_file->m_assign_next != NULL) {
        s_file->m_assign_next->m_assign_prev = s_file->m_assign_prev;
    }
    if(s_file->m_assign_prev == NULL) {
        s_assign_disk->m_assign_file = s_file->m_assign_next;
    }
    else {
        s_file->m_assign_prev->m_assign_next = s_file->m_assign_next;
    }

    s_assign_disk->m_assign_file_count -= (size_t)1u;
    
    s_file->m_assign_disk = NULL;
    s_file->m_assign_prev = s_file->m_assign_next = NULL;
    
    return(0);
}

static int __workload_move_assign_file(workload_t *s_workload, workload_file_t *s_file, workload_disk_t *s_disk)
{
    char *s_to_pathname;
    int s_to_fd, s_from_fd, s_created;
    off_t s_offset;
    size_t s_want_size;
    ssize_t s_write_bytes;
    unsigned char s_buffer[4 << 10];

    /* open file */
    s_from_fd = (-1);
    if(s_file->m_assign_disk != NULL) {
        char *s_from_pathname = workload_file_pathname_alloc(s_file, NULL);
        if(s_from_pathname == NULL) { return(-1); }

        s_from_fd = open(s_from_pathname, O_RDONLY | O_LARGEFILE | ((s_workload->m_cfg.m_access_sync == 0) ? 0 : O_DIRECT));
        free((void *)s_from_pathname);
        if(s_from_fd == (-1)) {
	    return(-1);
	}
    }
    else {
        s_file->m_flags &= (~def_workload_file_flag_created);
	return(0);
    }
    
    s_to_pathname = workload_file_pathname_alloc(s_file, s_disk);
    if(s_to_pathname == NULL) {
        if(s_from_fd != (-1)) { close(s_from_fd); }
        return(-1);
    }
    s_to_fd = open(s_to_pathname, O_CREAT | O_EXCL | O_WRONLY | O_LARGEFILE /* | ((s_workload->m_access_sync == 0) ? 0 : O_SYNC) */ /* | O_DIRECT */, 0644);
    if(s_to_fd == (-1)) {
        s_created = 0;
        s_to_fd = open(s_to_pathname, O_CREAT | O_WRONLY | O_TRUNC | O_LARGEFILE /* | ((s_workload->m_access_sync == 0) ? 0 : O_SYNC) */ /* | O_DIRECT */, 0644);
    }
    else { s_created = 1; }
    free((void *)s_to_pathname);
    if(s_to_fd == (-1)) {
        if(s_from_fd != (-1)) { close(s_from_fd); }
        return(-1);
    }

    if(s_from_fd == (-1)) { /* new file */
        for(s_offset = (off_t)0;(*s_workload->m_break_ptr == 0) && (s_offset < s_workload->m_cfg.m_filesize);) {
            s_want_size = ((s_workload->m_cfg.m_filesize - s_offset) >= ((off_t)sizeof(s_buffer))) ? sizeof(s_buffer) : ((size_t)(s_workload->m_cfg.m_filesize - s_offset));
            (void)workload_fill_rand((void *)(&s_buffer[0]), s_want_size);
            s_write_bytes = workload_write(s_to_fd, (const void *)(&s_buffer[0]), s_want_size, (-1));
            if(s_write_bytes <= ((ssize_t)0)) { break; }
            s_offset += (off_t)s_write_bytes;
        }
    }
    else { /* move file */
        if(s_workload->m_cfg.m_access_sendfile == 0) {
            ssize_t s_read_bytes;

            while(*s_workload->m_break_ptr == 0) {
                s_read_bytes = workload_read(s_from_fd, (void *)(&s_buffer[0]), sizeof(s_buffer), (-1));
                if(s_read_bytes <= ((ssize_t)0)) { break; }
                s_write_bytes = workload_write(s_to_fd, (const void *)(&s_buffer[0]), (size_t)s_read_bytes, (-1));
                if(s_write_bytes != s_read_bytes) { break; }
            }
	}
	else { /* fast copy */
	    ssize_t s_copy_bytes;

	    s_copy_bytes = sendfile(s_to_fd, s_from_fd, NULL, (size_t)s_workload->m_cfg.m_filesize);
    
            (void)s_copy_bytes;
	}

        /* already assigned => do reassign */
        if(workload_deassign_file(s_file) == (-1)) {
            close(s_to_fd);
            close(s_from_fd);
            return(-1);
        }
    }
            
#if 0L
    if(s_workload->m_access_sync != 0) { (void)fsync(s_to_fd); }
#endif    
    
    close(s_to_fd);
    if(s_from_fd != (-1)) { close(s_from_fd); }
    
    if(s_created == 0) { s_file->m_flags &= (~def_workload_file_flag_created); }
    else { s_file->m_flags |= def_workload_file_flag_created; }

    return(0);
}

int workload_assign_file_to_disk(workload_t *s_workload, workload_file_t *s_file, workload_disk_t *s_disk)
{
    workload_disk_t *s_old_disk;
    int s_result;

    if((s_workload == NULL) || (s_file == NULL) || (s_disk == NULL)) { return(-1); }
    
    s_old_disk = s_file->m_assign_disk;
    if((s_old_disk != NULL) && (s_old_disk == s_disk)) {
        /* already (re)assigned */
        return(0);
    }

    s_result = __workload_move_assign_file(s_workload, s_file, s_disk);
    if(s_result == 0) {
        /* assign */
        s_file->m_assign_prev = NULL;
        s_file->m_assign_next = s_disk->m_assign_file;
        if(s_disk->m_assign_file != NULL) { s_disk->m_assign_file->m_assign_prev = s_file; }
        s_disk->m_assign_file = s_file;

        s_file->m_assign_disk = s_disk;
        s_file->m_assign_disk->m_assign_file_count += (size_t)1u;
        
        pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
	
	if(s_old_disk != NULL) {
#if 0L /* do not counting ! */	
            ++s_file->m_read_count;
	    ++s_old_disk->m_read_count;
            s_old_disk->m_time_stamp = workload_get_main_time_stamp(s_workload);
	    ++s_workload->m_read_count;
#else
            s_old_disk->m_time_stamp = workload_get_main_time_stamp(s_workload);
#endif
        }
#if 0L /* do not counting ! */	
        ++s_file->m_write_count;
	++s_disk->m_write_count;
	++s_workload->m_write_count;
        s_file->m_time_stamp = workload_get_main_time_stamp(s_workload);
        s_disk->m_time_stamp = workload_get_main_time_stamp(s_workload);
#else
        s_file->m_time_stamp = workload_get_main_time_stamp(s_workload);
        s_disk->m_time_stamp = workload_get_main_time_stamp(s_workload);
#endif

        (void)workload_update_db(s_workload, s_file, NULL);
        pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
    }
    
    return(s_result);
}

#endif

/* vim: set expandtab: */
/* End of source */
