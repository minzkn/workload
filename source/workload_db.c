/*
    Copyright (C) HWPORT.COM
    All rights reserved.
    Author: JAEHYUK CHO <mailto:minzkn@minzkn.com>
*/

#if !defined(__def_hwport_workload_source_workload_db_c__)
# define __def_hwport_workload_source_workload_db_c__ "workdload_db.c"

#include "workload.h"

int workload_reset_db(workload_t *s_workload);
static int workload_update_db_file(workload_t *s_workload, workload_file_t *s_file);
static int workload_update_db_disk(workload_t *s_workload, workload_disk_t *s_disk);
int workload_update_db(workload_t *s_workload, workload_file_t *s_file, workload_disk_t *s_disk);

int workload_reset_db(workload_t *s_workload)
{
    static const char *cg_drop_table[] = {
        def_workload_db_table_prefix "file", def_workload_db_table_prefix "disk", NULL
    };
    int s_drop_table_index;

    if(s_workload == NULL) { return(-1); }
    if(s_workload->m_db == NULL) { return(-1); }

    for(s_drop_table_index = 0;cg_drop_table[s_drop_table_index] != NULL;s_drop_table_index++) {
        if(workload_db_query_set(
            s_workload->m_db,
            "DROP TABLE IF EXISTS %s",
            cg_drop_table[s_drop_table_index]) == (-1)) {
            return(-1);
        }
    }

    /* create disk table */
    if(workload_db_query_set(
        s_workload->m_db,
        "CREATE TABLE IF NOT EXISTS %s ("
        "%s INT UNSIGNED NOT NULL DEFAULT '0' PRIMARY KEY," /* id */
        "%s INT UNSIGNED NOT NULL DEFAULT '0'," /* files */
        "%s BIGINT UNSIGNED NOT NULL DEFAULT '0'," /* read */
        "%s BIGINT UNSIGNED NOT NULL DEFAULT '0'," /* write */
        "%s BIGINT UNSIGNED NOT NULL DEFAULT '0'," /* access */
        "%s BLOB NOT NULL DEFAULT ''," /* name */
        "%s BLOB NOT NULL DEFAULT ''" /* pathname */
        ")" /* " engine=myisam default charset=utf8" */,
        def_workload_db_table_prefix "disk",
        def_workload_db_field_prefix "id",
        def_workload_db_field_prefix "files",
        def_workload_db_field_prefix "read",
        def_workload_db_field_prefix "write",
        def_workload_db_field_prefix "access",
        def_workload_db_field_prefix "name",
        def_workload_db_field_prefix "pathname"
    ) == (-1)) {
        return(-1);
    }
    
    /* create file table */
    if(workload_db_query_set(
        s_workload->m_db,
        "CREATE TABLE IF NOT EXISTS %s ("
        "%s INT UNSIGNED NOT NULL DEFAULT '0' PRIMARY KEY," /* id */
        "%s INT UNSIGNED NOT NULL DEFAULT '0'," /* disk id */
        "%s BIGINT UNSIGNED NOT NULL DEFAULT '0'," /* read */
        "%s BIGINT UNSIGNED NOT NULL DEFAULT '0'," /* write */
        "%s BIGINT UNSIGNED NOT NULL DEFAULT '0'," /* access */
        "%s BLOB NOT NULL DEFAULT ''" /* pathname */
        ")" /* " engine=myisam default charset=utf8" */,
        def_workload_db_table_prefix "file",
        def_workload_db_field_prefix "id",
        def_workload_db_field_prefix "disk_id",
        def_workload_db_field_prefix "read",
        def_workload_db_field_prefix "write",
        def_workload_db_field_prefix "access",
        def_workload_db_field_prefix "pathname"
    ) == (-1)) {
        return(-1);
    }

    return(0);
}

static int workload_update_db_file(workload_t *s_workload, workload_file_t *s_file)
{
    if((s_file->m_flags & def_workload_file_flag_inserted_db) == def_workload_file_flag_none) {
        /* insert */
        if(workload_db_query_set(
            s_workload->m_db,
            "INSERT INTO %s ("
            "%s"
            ") VALUES ("
            "'%u'"
            ")",
            def_workload_db_table_prefix "file",
            def_workload_db_field_prefix "id",
            s_file->m_id
        ) == (-1)) {
            return(-1);
        }

        s_file->m_flags |= def_workload_file_flag_inserted_db;
    }

    if(workload_db_query_set(
        s_workload->m_db,
        "UPDATE %s SET "
        "%s='%u'," /* disk_id */
        "%s='%llu'," /* read */
        "%s='%llu'," /* write */
        "%s='%llu'," /* access */
        "%s='%s%s%s'" /* pathname */
        "WHERE %s='%u'",
        def_workload_db_table_prefix "file",
        def_workload_db_field_prefix "disk_id", (s_file->m_assign_disk == NULL) ? 0u : s_file->m_assign_disk->m_id,
        def_workload_db_field_prefix "read", s_file->m_read_count,
        def_workload_db_field_prefix "write", s_file->m_write_count,
        def_workload_db_field_prefix "access", s_file->m_read_count + s_file->m_write_count,
        def_workload_db_field_prefix "pathname", (s_file->m_assign_disk == NULL) ? "" : workload_check_string(s_file->m_assign_disk->m_path),
                    (s_file->m_assign_disk == NULL) ? "" : "/",
                    s_file->m_filename,
        def_workload_db_field_prefix "id", s_file->m_id
    ) == (-1)) {
        return(-1);
    }

    return(0);
}

static int workload_update_db_disk(workload_t *s_workload, workload_disk_t *s_disk)
{
    if((s_disk->m_flags & def_workload_disk_flag_inserted_db) == def_workload_disk_flag_none) {
        /* insert */
        if(workload_db_query_set(
            s_workload->m_db,
            "INSERT INTO %s ("
            "%s,%s,%s"
            ") VALUES ("
            "'%u','%s','%s'"
            ")",
            def_workload_db_table_prefix "disk",
            def_workload_db_field_prefix "id",
            def_workload_db_field_prefix "name",
            def_workload_db_field_prefix "pathname",
            s_disk->m_id,
            s_disk->m_name,
            s_disk->m_path
        ) == (-1)) {
            return(-1);
        }

        s_disk->m_flags |= def_workload_disk_flag_inserted_db;
    }
   
    if(workload_db_query_set(
        s_workload->m_db,
        "UPDATE %s SET "
        "%s='%lu'," /* files */
        "%s='%llu'," /* read */
        "%s='%llu'," /* write */
        "%s='%llu'" /* access */
        "WHERE %s='%u'",
        def_workload_db_table_prefix "disk",
        def_workload_db_field_prefix "files", (unsigned long)s_disk->m_assign_file_count,
        def_workload_db_field_prefix "read", s_disk->m_read_count,
        def_workload_db_field_prefix "write", s_disk->m_write_count,
        def_workload_db_field_prefix "access", s_disk->m_read_count + s_disk->m_write_count,
        def_workload_db_field_prefix "id", s_disk->m_id
    ) == (-1)) {
        return(-1);
    }

    return(0);
}

int workload_update_db(workload_t *s_workload, workload_file_t *s_file, workload_disk_t *s_disk)
{
    if(s_workload == NULL) { return(-1); }
    if(s_workload->m_db == NULL) { return(-1); }

    if(s_file != NULL) {
        if(workload_update_db_file(s_workload, s_file) == (-1)) { return(-1); }

        if(s_file->m_assign_disk != NULL) {
            if(s_file->m_assign_disk == s_disk) {
                return(workload_update_db_disk(s_workload, s_file->m_assign_disk));
            }
            if(workload_update_db_disk(s_workload, s_file->m_assign_disk) == (-1)) { return(-1); }
        }
    }

    if(s_disk == NULL) { return(0); }
    
    return(workload_update_db_disk(s_workload, s_disk));
}

#endif

/* vim: set expandtab: */
/* End of source */
