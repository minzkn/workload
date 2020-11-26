/*
    Copyright (C) HWPORT.COM
    All rights reserved.
    Author: JAEHYUK CHO <mailto:minzkn@minzkn.com>
*/

#if !defined(__def_hwport_workload_source_workload_config_c__)
# define __def_hwport_workload_source_workload_config_c__ "workdload_config.c"

#if !defined(_ISOC99_SOURCE)
# define _ISOC99_SOURCE (1L)
#endif

#if !defined(_GNU_SOURCE)
# define _GNU_SOURCE (1L)
#endif

#include "workload.h"

#include <sys/stat.h>

int workload_init_config(workload_t *s_workload);
int workload_load_config(workload_t *s_workload);
int workload_unload_config(workload_t *s_workload);

int workload_init_config(workload_t *s_workload)
{
    if(s_workload == NULL) { return(-1); }

    (void)memset((void *)(&s_workload->m_cfg), 0, sizeof(s_workload->m_cfg));

    s_workload->m_cfg.m_config_file = NULL;

    s_workload->m_cfg.m_left_ratio = 1u;
    s_workload->m_cfg.m_right_ratio = 1u;

    s_workload->m_cfg.m_duration = 0ull;
    s_workload->m_cfg.m_access_interval = 0ull;

    s_workload->m_cfg.m_files_remove = 1;
    s_workload->m_cfg.m_files_assign_method = 0;

    s_workload->m_cfg.m_prefix = NULL;
    s_workload->m_cfg.m_suffix = NULL;
   
    s_workload->m_cfg.m_filecount = (size_t)0u;
    s_workload->m_cfg.m_filesize = (off_t)0;

    s_workload->m_cfg.m_access_method = def_workload_access_method_rw;
    s_workload->m_cfg.m_access_sync = 0;
    s_workload->m_cfg.m_access_sendfile = 0;
    s_workload->m_cfg.m_access_threads = 0;

    return(0);
}

static int __workload_parse_config(workload_t *s_workload)
{
    void *s_node, *s_sub_node;
    const char *s_temp_string;

    unsigned int s_id;
    unsigned int s_assign_disk_id;

    char *s_name;
    char *s_path;
    int s_is_create_path;
    long long s_fakeoff_delay;

    workload_disk_t *s_tail_disk, *s_new_disk, *s_trace_disk;

    workload_file_t *s_tail_file, *s_new_file;


    /* database */
    s_workload->m_db = workload_close_db(s_workload->m_db);
    s_node = workload_get_xml_node(s_workload->m_xml, "/workload/config/meta");
    if(s_node == NULL) {
        if(s_workload->m_opt.m_verbose != 0) {
            (void)workload_log(" * Disabled meta DB.\n");
        }
    }
    else {
        const char *s_db_type = workload_check_string(
            workload_xml_tag_attr_value(s_node, "type")
        );

        if(strcasecmp(s_db_type, "sqlite3") == 0) {
            static const char cg_default_pathname_name[] = {
                "./workload.db"
            };
            s_temp_string = workload_xml_tag_data(workload_xml_sub_node_ex(s_node, "pathname"));
            if(strlen(workload_check_string(s_temp_string)) <= ((size_t)0u)) { s_temp_string = workload_check_string((const char *)(&cg_default_pathname_name[0])); }
            s_workload->m_db = workload_open_db(def_workload_db_type_sqlite3, s_temp_string);
            if(s_workload->m_db == NULL) {
                if(s_workload->m_opt.m_verbose != 0) {
                    (void)workload_log(" * ERROR: %s DB failed !\n", s_db_type);
                }
                return(-1);
            }
            if(s_workload->m_opt.m_verbose != 0) {
                (void)workload_log(" * Meta DB is %s. (pathname=\"%s\")\n", s_db_type, s_temp_string);
            }
        }
        else if(strcasecmp(s_db_type, "mysql") == 0) {
            const char *s_hostname = workload_xml_tag_data(workload_xml_sub_node_ex(s_node, "hostname"));
            int s_port = atoi(workload_xml_tag_data(workload_xml_sub_node_ex(s_node, "port")));
            const char *s_username = workload_xml_tag_data(workload_xml_sub_node_ex(s_node, "username"));
            const char *s_password = workload_xml_tag_data(workload_xml_sub_node_ex(s_node, "password"));
            const char *s_database = workload_xml_tag_data(workload_xml_sub_node_ex(s_node, "database"));

            s_workload->m_db = workload_open_db_ex(
                def_workload_db_type_mysql,
                s_hostname,
                s_port,
                s_username,
                s_password,
                s_database
            );
            if(s_workload->m_db == NULL) {
                if(s_workload->m_opt.m_verbose != 0) {
                    (void)workload_log(" * ERROR: %s DB failed !\n", s_db_type);
                }
                return(-1);
            }
            if(s_workload->m_opt.m_verbose != 0) {
                (void)workload_log(
                    " * Meta DB is %s. (remote=\"[%s]:%d\", username=\"%s\", password=\"%s\", database=\"%s\")\n",
                    s_db_type,
                    workload_check_string(s_hostname),
                    s_port,
                    workload_check_string(s_username),
                    workload_check_string(s_password),
                    workload_check_string(s_database)
                );
            }
        }
        else {
            if(s_workload->m_opt.m_verbose != 0) {
                (void)workload_log(" * Not supported meta DB type ! (%s)\n", s_db_type);
            }
        }
    }
    if((s_workload->m_opt.m_dump_disk != 0) || (s_workload->m_opt.m_dump_file != 0)) {
        void *s_db_result;

        if(s_workload->m_opt.m_dump_disk != 0) {
            s_db_result = workload_db_query_get(
                s_workload->m_db,
                "SELECT * FROM %s "
                "ORDER BY "
                "%s %s,"
                "%s %s",
                def_workload_db_table_prefix "disk",
                def_workload_db_field_prefix "access", "DESC",
                def_workload_db_field_prefix "id", "ASC"
            );
            if(s_db_result == NULL) {
                (void)workload_printf("<EMPTY:DISK>\n");
            }
            else {
                (void)workload_db_dump_result(s_workload->m_db, s_db_result, "disk", (-1ll));
                (void)workload_free_db_result(s_workload->m_db, s_db_result); 
            }
        }
        
        if(s_workload->m_opt.m_dump_file != 0) {
            s_db_result = workload_db_query_get(
                s_workload->m_db,
                "SELECT * FROM %s "
                "ORDER BY "
                "%s %s,"
                "%s %s",
                def_workload_db_table_prefix "file",
                def_workload_db_field_prefix "access", "DESC",
                def_workload_db_field_prefix "disk_id", "ASC"
            );
            if(s_db_result == NULL) {
                (void)workload_printf("<EMPTY:FILE>\n");
            }
            else {
                (void)workload_db_dump_result(s_workload->m_db, s_db_result, "file", (-1ll));
                (void)workload_free_db_result(s_workload->m_db, s_db_result); 
            }
        }

        *s_workload->m_break_ptr = 1;
        return(0);
    }
    if(s_workload->m_db != NULL) {
        if(s_workload->m_opt.m_verbose != 0) {
            (void)workload_log(" * Resetting meta DB...\n");
        }
        if(workload_reset_db(s_workload) == (-1)) {
            if(s_workload->m_opt.m_verbose != 0) {
                (void)workload_log(" * ERROR: Reset meta DB failed !\n");

                return(-1);
            }

            s_workload->m_db = workload_close_db(s_workload->m_db);
        }
    }
        
    /* operation */
    if(s_workload->m_opt.m_pause != (-1)) {
        workload_set_main_pause(s_workload, s_workload->m_opt.m_pause);
    }
    else {
        s_node = workload_get_xml_node(s_workload->m_xml, "/workload/config/operation");
        if(s_node != NULL) {
            workload_set_main_pause(s_workload, workload_parse_boolean_string(workload_xml_tag_attr_value(s_node, "pause"), 0));
        }
    }
    
    /* operation/left */ /* operation/right */
    if(s_workload->m_opt.m_left_ratio != 0u) {
        s_workload->m_cfg.m_left_ratio = s_workload->m_opt.m_left_ratio;
    }
    else {
        s_workload->m_cfg.m_left_ratio = 1u;
        s_node = workload_get_xml_node(s_workload->m_xml, "/workload/config/operation/left");
        if(s_node != NULL) {
            s_temp_string = workload_xml_tag_data(s_node);
            if(strlen(s_temp_string) > ((size_t)0u)) {
                s_workload->m_cfg.m_left_ratio = (unsigned int)atoi(s_temp_string);
                if(s_workload->m_cfg.m_left_ratio <= 0u) { s_workload->m_cfg.m_left_ratio = 1u; }
            }
        }
    }
    if(s_workload->m_opt.m_right_ratio != 0u) {
        s_workload->m_cfg.m_right_ratio = s_workload->m_opt.m_right_ratio;
    }
    else {
        s_workload->m_cfg.m_right_ratio = 1u;
        s_node = workload_get_xml_node(s_workload->m_xml, "/workload/config/operation/right");
        if(s_node != NULL) {
            s_temp_string = workload_xml_tag_data(s_node);
            if(strlen(s_temp_string) > ((size_t)0u)) {
                s_workload->m_cfg.m_right_ratio = (unsigned int)atoi(s_temp_string);
                if(s_workload->m_cfg.m_right_ratio <= 0u) { s_workload->m_cfg.m_right_ratio = 1u; }
            }
        }
    }
    (void)workload_aspect_ratio_uint(
        s_workload->m_cfg.m_left_ratio,
        s_workload->m_cfg.m_right_ratio,
        (unsigned int *)(&s_workload->m_cfg.m_left_ratio),
        (unsigned int *)(&s_workload->m_cfg.m_right_ratio)
    );
    if(s_workload->m_opt.m_verbose != 0) {
        (void)workload_log(
            " * Access ratio : %u(left):%u(right)\n",
            s_workload->m_cfg.m_left_ratio,
            s_workload->m_cfg.m_right_ratio
        );
    }
    /* operation/interval */
    if(s_workload->m_opt.m_access_interval != (~0ull)) {
        s_workload->m_cfg.m_access_interval = s_workload->m_opt.m_access_interval;
    }
    else {
        s_workload->m_cfg.m_access_interval = 0ull;
        s_node = workload_get_xml_node(s_workload->m_xml, "/workload/config/operation/interval");
        if(s_node != NULL) {
            s_temp_string = workload_xml_tag_data(s_node);
            if(strlen(s_temp_string) > ((size_t)0u)) {
                s_workload->m_cfg.m_access_interval = (unsigned long long)atoll(s_temp_string);
            }
        }
    }
    if(s_workload->m_opt.m_verbose != 0) {
        (void)workload_log(
            " * Access interval : %llu msec\n",
            s_workload->m_cfg.m_access_interval
        );
    }
    /* operation/files */
    if(s_workload->m_opt.m_filecount != ((size_t)0u)) {
        s_workload->m_cfg.m_filecount = s_workload->m_opt.m_filecount;
    }
    else {
        s_workload->m_cfg.m_filecount = (size_t)0u;
        s_node = workload_get_xml_node(s_workload->m_xml, "/workload/config/operation/files");
        if(s_node != NULL) {
            s_temp_string = workload_xml_tag_data(s_node);
            if(strlen(s_temp_string) > ((size_t)0u)) {
                s_workload->m_cfg.m_filecount = (size_t)atoi(s_temp_string);
            }
        }
    }
    if(s_workload->m_cfg.m_filecount > ((size_t)def_workload_config_max_files)) {
        if(s_workload->m_opt.m_verbose != 0) {
            (void)workload_log(" * WARNING: Too many files configuration ! (%lu -> %lu)\n", (unsigned long)s_workload->m_cfg.m_filecount, (unsigned long)def_workload_config_max_files);
        }

        s_workload->m_cfg.m_filecount = ((size_t)def_workload_config_max_files);
    }
    if(s_workload->m_opt.m_verbose != 0) {
        (void)workload_log(" * Operation files : %lu\n", (unsigned long)s_workload->m_cfg.m_filecount);
    }
    s_workload->m_cfg.m_files_remove = workload_parse_boolean_string(workload_xml_tag_attr_value(s_node, "remove"), 1);
    s_workload->m_cfg.m_files_assign_method = 0;
    if(s_workload->m_opt.m_files_assign_method != NULL) {
        s_temp_string = s_workload->m_opt.m_files_assign_method;
    }
    else {
        s_temp_string = workload_xml_tag_attr_value(s_node, "assign");
    }
    if(s_temp_string != NULL) {
        if(strcasecmp(s_temp_string, "ordered") == 0) {
            s_workload->m_cfg.m_files_assign_method = 1;
        }
        else if(strcasecmp(s_temp_string, "even") == 0) {
            s_workload->m_cfg.m_files_assign_method = 2;
        }
    }
    /* operation/prefix */
    if(s_workload->m_opt.m_prefix != NULL) {
        s_workload->m_cfg.m_prefix = s_workload->m_opt.m_prefix;
    }
    else {
        s_workload->m_cfg.m_prefix = NULL;
        s_node = workload_get_xml_node(s_workload->m_xml, "/workload/config/operation/prefix");
        if(s_node != NULL) {
            s_workload->m_cfg.m_prefix = workload_xml_tag_data(s_node);
        }
    }
    if(s_workload->m_opt.m_verbose != 0) {
        (void)workload_log(
            " * File prefix : \"%s\"\n",
            workload_check_string(s_workload->m_cfg.m_prefix)
        );
    }
    /* operation/suffix */
    if(s_workload->m_opt.m_suffix != NULL) {
        s_workload->m_cfg.m_suffix = s_workload->m_opt.m_suffix;
    }
    else {
        s_workload->m_cfg.m_suffix = NULL;
        s_node = workload_get_xml_node(s_workload->m_xml, "/workload/config/operation/suffix");
        if(s_node != NULL) {
            s_workload->m_cfg.m_suffix = workload_xml_tag_data(s_node);
        }
    }
    if(s_workload->m_opt.m_verbose != 0) {
        (void)workload_log(
            " * File suffix : \"%s\"\n",
            workload_check_string(s_workload->m_cfg.m_suffix)
        );
    }
    /* operation/filesize */
    if(s_workload->m_opt.m_filesize != ((off_t)(-1))) {
        s_workload->m_cfg.m_filesize = s_workload->m_opt.m_filesize;
    }
    else {
        s_workload->m_cfg.m_filesize = (off_t)0;
        s_node = workload_get_xml_node(s_workload->m_xml, "/workload/config/operation/filesize");
        if(s_node != NULL) {
            s_temp_string = workload_xml_tag_data(s_node);
            if(strlen(s_temp_string) > ((size_t)0u)) {
                s_workload->m_cfg.m_filesize = (off_t)atoll(s_temp_string);
            }
        }
    }
    if(s_workload->m_cfg.m_filesize < ((off_t)0)) {
        s_workload->m_cfg.m_filesize = (off_t)0;
    }
    /* operation/duration */
    if(s_workload->m_opt.m_duration != (~0ull)) {
        s_workload->m_cfg.m_duration = s_workload->m_opt.m_duration;
    }
    else {
        s_workload->m_cfg.m_duration = 0ull;
        s_node = workload_get_xml_node(s_workload->m_xml, "/workload/config/operation/duration");
        if(s_node != NULL) {
            s_temp_string = workload_xml_tag_data(s_node);
            if(strlen(s_temp_string) > ((size_t)0u)) {
                s_workload->m_cfg.m_duration = (unsigned long long)atoll(s_temp_string);
            }
        }
    }
    if(s_workload->m_opt.m_verbose != 0) {
        (void)workload_log(
            " * Working duration : %llu msec\n",
            s_workload->m_cfg.m_duration
        );
    }
    /* operation/access */
    s_workload->m_cfg.m_access_method = def_workload_access_method_rw;
    s_node = workload_get_xml_node(s_workload->m_xml, "/workload/config/operation/access");
    if(s_node != NULL) {
        s_temp_string = workload_xml_tag_data(s_node);
        if(strlen(s_temp_string) > ((size_t)0u)) {
            if((strcasecmp(s_temp_string, "read") == 0) ||
               (strcasecmp(s_temp_string, "r") == 0) ||
               (strcasecmp(s_temp_string, "input") == 0) ||
               (strcasecmp(s_temp_string, "i") == 0) ||
               (strcasecmp(s_temp_string, "load") == 0)) {
                s_workload->m_cfg.m_access_method = def_workload_access_method_read;
            }
            else if((strcasecmp(s_temp_string, "write") == 0) ||
               (strcasecmp(s_temp_string, "w") == 0) ||
               (strcasecmp(s_temp_string, "ouput") == 0) ||
               (strcasecmp(s_temp_string, "o") == 0) ||
               (strcasecmp(s_temp_string, "save") == 0)) {
                s_workload->m_cfg.m_access_method = def_workload_access_method_write;
            }
            else {
                s_workload->m_cfg.m_access_method = def_workload_access_method_rw;
            }
        }
    }
    if(s_workload->m_opt.m_verbose != 0) {
        (void)workload_log(
            " * Access method : %s/%s\n",
            ((s_workload->m_cfg.m_access_method & def_workload_access_method_read) == def_workload_access_method_none) ? "NONE" : "READ",
            ((s_workload->m_cfg.m_access_method & def_workload_access_method_write) == def_workload_access_method_none) ? "NONE" : "WRITE"
        );
    }
    s_workload->m_cfg.m_access_sync = workload_parse_boolean_string(workload_xml_tag_attr_value(s_node, "sync"), 0);
    s_workload->m_cfg.m_access_sendfile = workload_parse_boolean_string(workload_xml_tag_attr_value(s_node, "sendfile"), 0);
    s_workload->m_cfg.m_access_threads = 0;
    if(s_workload->m_opt.m_threads > 0) {
        s_workload->m_cfg.m_access_threads = s_workload->m_opt.m_threads;
    }
    else {
        s_temp_string = workload_xml_tag_attr_value(s_node, "threads");
        if(s_temp_string != NULL) {
            s_workload->m_cfg.m_access_threads = atoi(s_temp_string);
        }
    }
    if(s_workload->m_opt.m_verbose != 0) {
        (void)workload_log(
            " * Access threads : %d\n",
	    s_workload->m_cfg.m_access_threads
        );
    }

    /* disk */
    (void)workload_free_disk_array(s_workload);
    s_workload->m_disk_head = workload_free_disk(s_workload->m_disk_head);
    for(s_node = workload_get_xml_node(s_workload->m_xml, "/workload/config/storage/disk"),
        s_tail_disk = s_workload->m_disk_head,
        s_id = 1u;
        s_node != NULL;
        s_node = workload_xml_next_node_ex(s_node, "disk")) {
        if(*s_workload->m_break_ptr != 0) {
            if(s_workload->m_opt.m_verbose != 0) {
                (void)workload_log(" * BREAK at adddisk\n");
            }
            return(-1);
        }
	s_fakeoff_delay = (-1ll);
        s_temp_string = workload_xml_tag_attr_value(s_node, "fakeoff");
	if(strlen(workload_check_string(s_temp_string)) > ((size_t)0u)) {
	    s_fakeoff_delay = atoll(s_temp_string);
	    if(s_fakeoff_delay != (-1ll)) {
	        s_workload->m_flags |= def_workload_flag_use_fakeoff;
	    }
	}

        s_sub_node = workload_xml_sub_node_ex(s_node, "path");
        s_path = workload_xml_tag_data(s_sub_node);
        if(strlen(s_path) <= ((size_t)0u)) {
            if(s_workload->m_opt.m_verbose != 0) {
                (void)workload_log(
                    " * WARNING: No path disk !\n"
                );
            }
            continue; 
        }
        s_is_create_path = workload_parse_boolean_string(workload_xml_tag_attr_value(s_sub_node, "create"), 0);
        
        s_sub_node = workload_xml_sub_node_ex(s_node, "name");
        s_name = workload_xml_tag_data(workload_xml_sub_node_ex(s_sub_node, "name"));
        if(strlen(s_name) <= ((size_t)0u)) { s_name = workload_basename(s_path); }
       
        /* duplicate check */
        for(s_trace_disk = s_workload->m_disk_head;s_trace_disk != NULL;s_trace_disk = s_trace_disk->m_next) {
            if((strcmp(s_path, s_trace_disk->m_path) == 0) && (strcmp(s_name, s_trace_disk->m_name) == 0)) { break; }
        }
        if(s_trace_disk != NULL) { /* duplicated disk */
            if(s_workload->m_opt.m_verbose != 0) {
                (void)workload_log(
                    " * WARNING: Duplicated disk \"%s\" ! (path=\"%s\", create=\"%s\")\n",
                    s_name,
                    s_path,
                    (s_is_create_path == 0) ? "no" : "yes"
                );
            }
            continue;
        }

        /* new disk */
        s_new_disk = workload_new_disk(s_workload, s_id, s_name, s_path, s_is_create_path);
        if(s_new_disk == NULL) {
            if(s_workload->m_opt.m_verbose != 0) {
                (void)workload_log(
                    " * WARNING: Failed add disk \"%s\" ! (path=\"%s\", create=\"%s\")\n",
                    s_name,
                    s_path,
                    (s_is_create_path == 0) ? "no" : "yes"
                );
            }
            continue;
        }
	s_new_disk->m_fakeoff_delay = s_fakeoff_delay;

        /* add disk */
        s_new_disk->m_prev = s_tail_disk;
        if(s_tail_disk == NULL) { s_workload->m_disk_head = s_new_disk; }
        else { s_tail_disk->m_next = s_new_disk; }
        s_tail_disk = s_new_disk;

        if(s_workload->m_opt.m_verbose != 0) {
	    if(s_fakeoff_delay == (-1ll)) {
                (void)workload_log(
                    " * Add storage \"%s\" (id=%d, path=\"%s\")\n",
                    s_new_disk->m_name,
                    s_new_disk->m_id,
                    s_new_disk->m_path
                );
	    }
	    else {
                (void)workload_log(
                    " * Add storage \"%s\" (id=%d, path=\"%s\", fakeoff=%lld msec)\n",
                    s_new_disk->m_name,
                    s_new_disk->m_id,
                    s_new_disk->m_path,
                    s_fakeoff_delay
                );
	    }
        }
       
        /* next id */
        ++s_id;
    }
    if(s_workload->m_opt.m_verbose != 0) {
        (void)workload_log(
            " * Updating disk array...\n"
        );
    }
    if(workload_update_disk_array(s_workload) == (-1)) {
        if(s_workload->m_opt.m_verbose != 0) {
            (void)workload_log(
                " * ERROR: Update disk array failed !\n"
            );
        }

        return(-1);
    }
    
    /* file */
    (void)workload_free_file_array(s_workload);
    s_workload->m_file_head = workload_free_file(s_workload->m_file_head);
    if(s_workload->m_cfg.m_filecount <= ((size_t)0u)) { s_workload->m_cfg.m_filecount = (size_t)def_workload_config_default_files; }
    for(s_id = 1u, s_tail_file = s_workload->m_file_head;((size_t)s_id) <= s_workload->m_cfg.m_filecount;s_id++) {
        if(*s_workload->m_break_ptr != 0) {
            if(s_workload->m_opt.m_verbose != 0) {
                (void)workload_log("\n * BREAK at addfile\n");
            }
            return(-1);
        }

        s_new_file = workload_new_file(s_workload, s_id);
        if(s_new_file == NULL) { break; }
        
        /* add file */
        s_new_file->m_prev = s_tail_file;
        if(s_tail_file == NULL) { s_workload->m_file_head = s_new_file; }
        else { s_tail_file->m_next = s_new_file; }
        s_tail_file = s_new_file;

        if(s_workload->m_opt.m_verbose != 0) {
            if(((s_id % 10u) == 0u) || (((size_t)s_id) == s_workload->m_cfg.m_filecount)) {
                unsigned int s_percent = (unsigned int)((((unsigned long long)s_new_file->m_id) * 10000ull) / ((unsigned long long)s_workload->m_cfg.m_filecount));
                (void)workload_log(
                    "\r * Add file (%u/%lu): %u.%02u%% (id=%u)    ",
                    s_new_file->m_id,
                    (unsigned long)s_workload->m_cfg.m_filecount,
                    s_percent / 100u,
                    s_percent % 100u,
                    s_new_file->m_id
                );
                (void)workload_puts(NULL);
            }
        }
    }
    if(s_workload->m_opt.m_verbose != 0) {
        (void)workload_log(
            "\n * Updating file array...\n"
        );
    }
    if(workload_update_file_array(s_workload) == (-1)) {
        if(s_workload->m_opt.m_verbose != 0) {
            (void)workload_log(
                " * ERROR: Update file array failed !\n"
            );
        }

        return(-1);
    }
    for(s_id = 1u;((size_t)s_id) <= s_workload->m_file_count;s_id++) {
        if(*s_workload->m_break_ptr != 0) {
            if(s_workload->m_opt.m_verbose != 0) {
                (void)workload_log("\n * BREAK at assignfile\n");
            }
            return(-1);
        }
        
        if(s_workload->m_cfg.m_files_assign_method == 1) { /* ordered assign */
            s_new_file = s_workload->m_file_ordered[s_id - 1u];
            s_assign_disk_id = (unsigned int)((((unsigned long long)(s_new_file->m_id - ((size_t)1u))) * ((unsigned long long)s_workload->m_disk_count)) / ((unsigned long long)s_workload->m_cfg.m_filecount));
        }
        else if(s_workload->m_cfg.m_files_assign_method == 2) { /* even random assign */
            size_t s_random_even = ((size_t)workload_rand()) % (s_workload->m_file_count - ((size_t)(s_id - 1u)));

            s_tail_file = s_workload->m_file_even[s_workload->m_file_count - ((size_t)s_id)]; 
            s_new_file = s_workload->m_file_even[s_random_even];

            s_workload->m_file_even[s_random_even] = s_tail_file;
            s_workload->m_file_even[s_workload->m_file_count - ((size_t)s_id)] = s_new_file; 
            
            s_assign_disk_id = (unsigned int)((((unsigned long long)(s_id - ((size_t)1u))) * ((unsigned long long)s_workload->m_disk_count)) / ((unsigned long long)s_workload->m_cfg.m_filecount));
        }
        else { /* default is random assign */
            s_new_file = s_workload->m_file_ordered[s_id - 1u];
            s_assign_disk_id = ((unsigned int)workload_rand()) % ((unsigned int)s_workload->m_disk_count);
        }

        if(workload_assign_file_to_disk(s_workload, s_new_file, s_workload->m_disk[s_assign_disk_id]) == (-1)) {
            if(s_workload->m_opt.m_verbose != 0) {
                (void)workload_log(
                    "\n * ERROR: Assign file to disk failed ! (%u/%lu)\n",
                    s_new_file->m_id + 1u,
                    (unsigned long)s_workload->m_cfg.m_filecount
                );
            }

            return(-1);
        }

        if(s_workload->m_opt.m_verbose != 0) {
            if(((s_id % 10u) == 0u) || (((size_t)s_id) == s_workload->m_file_count)) {
                unsigned int s_percent = (unsigned int)((((unsigned long long)s_id) * 10000ull) / ((unsigned long long)s_workload->m_cfg.m_filecount));
                (void)workload_log(
                    "\r * Assigning disk (%u/%lu): %u.%02u%% (file_id=%u to disk_id=%u)    ",
                    s_id,
                    (unsigned long)s_workload->m_cfg.m_filecount,
                    s_percent / 100u,
                    s_percent % 100u,
                    s_new_file->m_id,
                    s_assign_disk_id
                );
            }
        }
    }
    if(s_workload->m_next_file_index != NULL) { free((void *)s_workload->m_next_file_index); }
    s_workload->m_next_file_index = (size_t *)malloc(sizeof(size_t) * s_workload->m_disk_count);
    if(s_workload->m_next_file_index != NULL) {
        size_t s_disk_index, s_file_index;
        for(s_disk_index = (size_t)0u, s_file_index = (size_t)0u;s_disk_index < s_workload->m_disk_count;s_disk_index++) {
            s_file_index += s_workload->m_disk[s_disk_index]->m_assign_file_count;
            s_workload->m_next_file_index[s_disk_index] = s_file_index;
        }
    }

    if(s_workload->m_opt.m_verbose != 0) {
        (void)workload_log(
            "\n * Complete assign...\n"
        );
    }
  
    if(s_workload->m_opt.m_verbose != 0) {
        (void)workload_log(
            " * Starting disk service...\n"
        );
    }
    pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
    s_workload->m_disk_ready_count = 0;
    s_workload->m_disk_error_count = 0;
    pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
    for(s_id = 0u;s_id < s_workload->m_disk_count;s_id++) {
        if(s_workload->m_disk[s_id]->m_service != NULL) {
	    s_workload->m_disk[s_id]->m_service = workload_close_service(s_workload->m_disk[s_id]->m_service);
	}
        s_workload->m_disk[s_id]->m_service = workload_open_service("disk", workload_disk_service, (void *)s_workload->m_disk[s_id]); 
    }
    if(s_workload->m_opt.m_verbose != 0) {
        (void)workload_log(
            " * Waiting disk service...\n"
        );
    }
    for(;;) {
        size_t s_disk_ready_count;
	int s_disk_error_count;

        if(*s_workload->m_break_ptr != 0) {
            if(s_workload->m_opt.m_verbose != 0) {
                (void)workload_log("\n * BREAK at disk service\n");
            }
            return(-1);
        }
    
        pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_reassign_lock));
        s_disk_ready_count = s_workload->m_disk_ready_count;
        s_disk_error_count = s_workload->m_disk_error_count;
        pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_reassign_lock));

       if(s_disk_error_count > 0) {
            if(s_workload->m_opt.m_verbose != 0) {
                (void)workload_log("\n * Disk service error !\n");
            }
           return(-1);
       }

       if(s_disk_ready_count >= s_workload->m_disk_count) {
           break;
       }

       workload_sleep_wait(0, 10);
    }
    if(s_workload->m_opt.m_verbose != 0) {
        (void)workload_log(
            " * Started disk service...\n"
        );
    }

    /* wait */
    if((s_workload->m_opt.m_verbose != 0) && (s_workload->m_opt.m_daemonize == 0) && (workload_get_main_pause(s_workload) == 0)) {
        int s_startup_sec;

        for(s_startup_sec = 3;s_startup_sec > 0;s_startup_sec--) {
            (void)workload_log(
                "\r * Wait %d sec...",
                s_startup_sec
            );
            workload_sleep_wait(1, 0);
        }
        (void)workload_log("\n");
    }

    return(0);
}

int workload_load_config(workload_t *s_workload)
{
    static const char *cg_conf_table[] = {
        (const char *)0,
        "./workload.xml",
        "/etc/workload.xml",
        (const char *)0
    };
    int s_conf_index;
    
    if(s_workload->m_xml != NULL) { return(-1); }
    
    /* load config */
    s_conf_index = 0;
    cg_conf_table[s_conf_index] = s_workload->m_opt.m_config_file;
    if(cg_conf_table[s_conf_index] == NULL) { ++s_conf_index; }

    for(s_workload->m_cfg.m_config_file = cg_conf_table[s_conf_index];
        s_workload->m_cfg.m_config_file != NULL;
        s_workload->m_cfg.m_config_file = cg_conf_table[++s_conf_index]) {
        
        s_workload->m_xml = workload_open_xml_from_file(s_workload->m_cfg.m_config_file);
        if(s_workload->m_opt.m_verbose != 0) {
            (void)workload_log(
                " * Checking config [%s] : %s\n",
                s_workload->m_cfg.m_config_file,
                (s_workload->m_xml == NULL) ? "NOT FOUND" : "OK"
            );
        }
        if(s_workload->m_xml != NULL) { break; }
        if(s_conf_index == 0) { break; }
    }
    if(s_workload->m_xml == NULL) { 
        if(s_workload->m_opt.m_verbose != 0) {
            (void)workload_log(
                " * ERROR: Need config !\n"
            );
        }

        return(-1); 
    }

    return(__workload_parse_config(s_workload));
}

int workload_unload_config(workload_t *s_workload)
{
    if(s_workload->m_next_file_index != NULL) {
        free((void *)s_workload->m_next_file_index);
	s_workload->m_next_file_index = NULL;
    }

    (void)workload_free_file_array(s_workload);
    s_workload->m_file_head = workload_free_file(s_workload->m_file_head);

    (void)workload_free_disk_array(s_workload);
    s_workload->m_disk_head = workload_free_disk(s_workload->m_disk_head);

    s_workload->m_write_count = 0ull;
    s_workload->m_read_count = 0ull;
    
    s_workload->m_disk_error_count = 0;
    s_workload->m_disk_ready_count = 0;

    s_workload->m_db = workload_close_db(s_workload->m_db);
    s_workload->m_xml = workload_close_xml(s_workload->m_xml);

    s_workload->m_cfg.m_access_sendfile = 0;
    s_workload->m_cfg.m_access_sync = 0;
    s_workload->m_cfg.m_access_method = def_workload_access_method_rw;

    s_workload->m_cfg.m_filesize = (off_t)0;
    s_workload->m_cfg.m_filecount = (size_t)0;

    s_workload->m_cfg.m_suffix = NULL;
    s_workload->m_cfg.m_prefix = NULL;

    s_workload->m_cfg.m_files_assign_method = 0;
    s_workload->m_cfg.m_files_remove = 1;

    s_workload->m_cfg.m_access_interval = 0ull;
    s_workload->m_cfg.m_duration = 0ull;

    s_workload->m_cfg.m_right_ratio = 1u;
    s_workload->m_cfg.m_left_ratio = 1u;
    
    s_workload->m_cfg.m_config_file = NULL;
    
    return(0);
}

#endif

/* vim: set expandtab: */
/* End of source */
