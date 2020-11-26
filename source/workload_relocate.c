/*
    Copyright (C) HWPORT.COM
    All rights reserved.
    Author: JAEHYUK CHO <mailto:minzkn@minzkn.com>
*/

#if !defined(__def_hwport_workload_source_workload_relocate_c__)
# define __def_hwport_workload_source_workload_relocate_c__ "workdload_relocate.c"

#include "workload.h"

int workload_file_relocate(workload_t *s_workload);

int workload_file_relocate(workload_t *s_workload)
{
    workload_file_t *s_file;

    for(s_file = s_workload->m_file_head;s_file != NULL;s_file = s_file->m_next) {
        pthread_mutex_lock((pthread_mutex_t *)(&s_file->m_access_lock));
	if(s_file->m_reassign_disk != NULL) {
            (void)workload_assign_file_to_disk(s_workload, s_file, s_file->m_reassign_disk);
	    s_file->m_reassign_disk = NULL;
	}
        pthread_mutex_unlock((pthread_mutex_t *)(&s_file->m_access_lock));
    }
    
    return(0);
}

#endif

/* vim: set expandtab: */
/* End of source */
