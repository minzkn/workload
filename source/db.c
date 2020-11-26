/*
    Copyright (C) HWPORT.COM
    All rights reserved.
    Author: JAEHYUK CHO <mailto:minzkn@minzkn.com>
*/

#if !defined(__def_hwport_workload_source_db_c__)
# define __def_hwport_workload_source_db_c__ "db.c"

#if !defined(_ISOC99_SOURCE)
# define _ISOC99_SOURCE (1L)
#endif

#if !defined(_GNU_SOURCE)
# define _GNU_SOURCE (1L)
#endif

#include "workload.h"

#if !defined(def_workload_use_sqlite3)
# define def_workload_use_sqlite3 (1)
#endif

#if !defined(def_workload_use_mysql_connector_c)
# define def_workload_use_mysql_connector_c (0)
#endif

#if def_workload_use_sqlite3 != (0)
# include <unistd.h>
# include <sched.h>

# include "sqlite3.h"
#endif

#if def_workload_use_mysql_connector_c != (0)
# include <sys/socket.h>
# include "mysql.h"
#endif

#include <pthread.h>

#if def_workload_use_sqlite3 != (0)
typedef struct workload_sqlite3_node_ts {
    struct workload_sqlite3_node_ts *m_next;

    int m_argc;
    char **m_argv, **m_field_name;
}workload_sqlite3_node_t;

typedef struct {
    unsigned long long m_node_count;

    workload_sqlite3_node_t *m_head, *m_tail, *m_fetch;
}workload_sqlite3_result_t;
#endif

typedef struct {
    pthread_mutex_t m_lock;

    int m_db_type; /* 0=sqlite3, 1=MySQL */

    const char *m_hostname;
    int m_port;
    const char *m_username;
    const char *m_password;
    const char *m_database;

#if def_workload_use_sqlite3 != (0)
    sqlite3 *m_sqlite3;
#endif    
#if def_workload_use_mysql_connector_c != (0)
    MYSQL m_mysql_local;
    MYSQL *m_mysql;
#endif
}workload_db_t;

#if def_workload_use_sqlite3 != (0)
static workload_sqlite3_node_t *workload_new_sqlite3_node(int s_argc, char **s_argv, char **s_field_name);
static workload_sqlite3_node_t *workload_free_sqlite3_node(workload_sqlite3_node_t *s_node);

static workload_sqlite3_result_t *workload_new_sqlite3_result(void);
static workload_sqlite3_result_t *workload_free_sqlite3_result(workload_sqlite3_result_t *s_result);

static int workload_sqlite3_callback(void *s_user_context, int s_argc, char **s_argv, char **s_field_name);
static int workload_sqlite3_query(sqlite3 *s_sqlite3, const char *s_query, void *s_db_result, char **s_error_message);
#endif

void *workload_open_db_ex(int s_db_type, const char *s_hostname, int s_port, const char *s_username, const char *s_password, const char *s_database);
void *workload_open_db(int s_db_type, const char *s_database);
void *workload_close_db(void *s_handle);
int workload_db_query(void *s_handle, const char *s_query, void *s_db_result, char **s_error_message);
int workload_db_vquery(void *s_handle, const char *s_format, void *s_db_result, char **s_error_message, va_list s_var) __attribute__((__format__(__printf__,2,0)));
void *workload_db_query_get(void *s_handle, const char *s_format, ...) __attribute__((__format__(__printf__,2,3)));
int workload_db_query_set(void *s_handle, const char *s_format, ...) __attribute__((__format__(__printf__,2,3)));
void *workload_free_db_result(void *s_handle, void *s_result);

char **workload_db_fetch_row(void *s_handle, void *s_result);
unsigned long long workload_db_row_count(void *s_handle, void *s_result);
unsigned long long workload_db_field_count(void *s_handle, void *s_result);
int workload_db_field_index(void *s_handle, void *s_result, const char *s_name_hint, int s_is_case);
int workload_db_dump_result(void *s_handle, void *s_result, const char *s_title, long long s_max);

#if def_workload_use_sqlite3 != (0)
static workload_sqlite3_node_t *workload_new_sqlite3_node(int s_argc, char **s_argv, char **s_field_name)
{
    workload_sqlite3_node_t *s_node;

    int s_count;

    s_node = (workload_sqlite3_node_t *)malloc(sizeof(workload_sqlite3_node_t) + (sizeof(char *) * (((size_t)s_argc) << 1)));
    if(s_node == NULL) { return(NULL); }
    s_node->m_next = NULL;

    s_node->m_argv = (char **)(&s_node[1]);
    s_node->m_field_name = (char **)(void *)(((unsigned char *)&s_node[1]) + (sizeof(char *) * ((size_t)s_argc)));

    s_node->m_argc = s_argc;
    for(s_count = 0;s_count < s_argc;s_count++) {
        s_node->m_argv[s_count] = (s_argv[s_count] == NULL) ? NULL : strdup(s_argv[s_count]);
        s_node->m_field_name[s_count] = (s_field_name[s_count] == NULL) ? NULL : strdup(s_field_name[s_count]);
    }

    return(s_node);
}

static workload_sqlite3_node_t *workload_free_sqlite3_node(workload_sqlite3_node_t *s_node)
{
    workload_sqlite3_node_t *s_prev;

    int s_count;
    
    while(s_node != NULL) {
        s_prev = s_node;
        s_node = s_node->m_next;

        for(s_count = 0;s_count < s_prev->m_argc;s_count++) {
            if(s_prev->m_field_name[s_count] != NULL) {
                free((void *)s_prev->m_field_name[s_count]);
            }
            
            if(s_prev->m_argv[s_count] != NULL) {
                free((void *)s_prev->m_argv[s_count]);
            }
        }

        free(s_prev);
    }

    return(NULL);
}

static workload_sqlite3_result_t *workload_new_sqlite3_result(void)
{
    workload_sqlite3_result_t *s_result;

    s_result = (workload_sqlite3_result_t *)malloc(sizeof(workload_sqlite3_result_t));
    if(s_result == NULL) { return(NULL); }

    s_result->m_node_count = (unsigned long long)0u;
    s_result->m_head = NULL;
    s_result->m_tail = NULL;
    s_result->m_fetch = NULL;

    return(s_result);
}

static workload_sqlite3_result_t *workload_free_sqlite3_result(workload_sqlite3_result_t *s_result)
{
    if(s_result == NULL) { return(NULL); }

    (void)workload_free_sqlite3_node(s_result->m_head);

    free((void *)s_result);

    return(NULL);
}

static int workload_sqlite3_callback(void *s_user_context, int s_argc, char **s_argv, char **s_field_name)
{
    workload_sqlite3_result_t *s_result;
    workload_sqlite3_node_t *s_node;

    if(s_user_context == NULL) { /* bypass - no op callback */ return(SQLITE_OK); }
    s_result = (workload_sqlite3_result_t *)(*((void **)s_user_context));

    if((s_argc < 0) || (s_argv == ((char **)0)) || (s_field_name == ((char **)0))) { return(SQLITE_ERROR); }

    if(s_result == NULL) {
        s_result = workload_new_sqlite3_result();
        if(s_result == NULL) { return(SQLITE_NOMEM); }
    }

    s_node = workload_new_sqlite3_node(s_argc, s_argv, s_field_name);
    if(s_node == NULL) {
        (void)workload_free_sqlite3_result(s_result);
        return(SQLITE_NOMEM);
    }

    if(s_result->m_tail == NULL) {
        s_result->m_head = s_node;
        s_result->m_fetch = s_node;
    }
    else { s_result->m_tail->m_next = s_node; }
    s_result->m_tail = s_node;
    
    ++s_result->m_node_count;

    *((void **)s_user_context) = (void *)s_result;

    return(SQLITE_OK); 
}

static int workload_sqlite3_query(sqlite3 *s_sqlite3, const char *s_query, void *s_db_result, char **s_error_message)
{
    int s_retry = 0;
    int s_retry_count = 0;
    int s_sqlite3_check;

    do {
        /* SQLITE_API int sqlite3_exec(sqlite3*, const char *sql, int (*callback)(void*,int,char**,char**), void *, char **errmsg); */
        s_sqlite3_check = sqlite3_exec(s_sqlite3, s_query, workload_sqlite3_callback, s_db_result, s_error_message);
        switch(s_sqlite3_check) {
            case SQLITE_OK:
                s_retry = 0;
                break;
            case SQLITE_BUSY:
            case SQLITE_LOCKED:
                s_retry = 1;
                ++s_retry_count;
                if(s_retry_count > 1000) {
                    workload_sleep_wait(0, 10);
                }
                else if(s_retry_count > 100) {
                    workload_sleep_wait(0, 1);
                }
                else if(s_retry_count > 10) {
                    sched_yield();
                }
                break;
            default:
                s_retry = 0;
                break;
        }
    }while(s_retry != 0);

    return(s_sqlite3_check);
}
#endif

void *workload_open_db_ex(int s_db_type, const char *s_hostname, int s_port, const char *s_username, const char *s_password, const char *s_database)
{
    workload_db_t *s_db;

    size_t s_hostname_size;
    size_t s_username_size;
    size_t s_password_size;
    size_t s_database_size;
    size_t s_size;
    unsigned char *s_shadow_ptr;

    s_hostname_size = strlen(workload_check_string(s_hostname));
    s_username_size = strlen(workload_check_string(s_username));
    s_password_size = strlen(workload_check_string(s_password));
    s_database_size = strlen(workload_check_string(s_database));

    s_size = (size_t)0u;
    s_size += s_hostname_size + ((size_t)1u);
    s_size += s_username_size + ((size_t)1u);
    s_size += s_password_size + ((size_t)1u);
    s_size += s_database_size + ((size_t)1u);
    s_db = (workload_db_t *)malloc(sizeof(workload_db_t) + s_size);
    if(s_db == NULL) { return(NULL); }
    (void)memset((void *)s_db, 0, sizeof(workload_db_t));

    /* ---- */

    pthread_mutex_init((pthread_mutex_t *)(&s_db->m_lock), NULL);

    s_db->m_db_type = s_db_type;

    s_size = (size_t)0u;
    s_shadow_ptr = (unsigned char *)(&s_db[1]);

    if(s_hostname != NULL) {
        s_db->m_hostname = strcpy((char *)(&s_shadow_ptr[s_size]), workload_check_string(s_hostname));
    }
    s_size += s_hostname_size + ((size_t)1u);

    s_db->m_port = s_port;
   
    if(s_username != NULL) {
        s_db->m_username = strcpy((char *)(&s_shadow_ptr[s_size]), workload_check_string(s_username));
    }
    s_size += s_username_size + ((size_t)1u);
   
    if(s_password != NULL) {
        s_db->m_password = strcpy((char *)(&s_shadow_ptr[s_size]), workload_check_string(s_password));
    }
    s_size += s_password_size + ((size_t)1u);
   
    if(s_database != NULL) {
        s_db->m_database = strcpy((char *)(&s_shadow_ptr[s_size]), workload_check_string(s_database));
    }
    s_size += s_database_size + ((size_t)1u);

#if def_workload_use_sqlite3 != (0)
    if(s_db_type == def_workload_db_type_sqlite3) {
        int s_sqlite3_check;

        if(strlen(workload_check_string(s_db->m_database)) <= ((size_t)0u)) { return(workload_close_db(s_db)); }

        /* SQLITE_API int sqlite3_open( const char *filename, sqlite3 **ppDb); */
        s_sqlite3_check = sqlite3_open(s_db->m_database, (sqlite3 **)(&s_db->m_sqlite3));
        if(s_sqlite3_check != SQLITE_OK) { return(workload_close_db(s_db)); }

        return((void *)s_db);
    }
#endif    

#if def_workload_use_mysql_connector_c != (0)
    if(s_db_type == def_workload_db_type_mysql) {
        int s_mysql_connect_timeout_sec = 48;
        int s_mysql_reconnect = 1;
        
        if((strlen(workload_check_string(s_db->m_database)) <= ((size_t)0u)) ||
            (s_port < 0) ||
            (s_port > 65535)) {
            return(workload_close_db(s_db));
        }

        /* MYSQL * STDCALL mysql_init(MYSQL *mysql); */
        s_db->m_mysql = mysql_init((MYSQL *)(&s_db->m_mysql_local));
        if(s_db->m_mysql == NULL) { return(workload_close_db(s_db)); }
 
        /* int STDCALL mysql_options(MYSQL *mysql,enum mysql_option option, const void *arg); */
        (void)mysql_options(s_db->m_mysql, MYSQL_OPT_CONNECT_TIMEOUT, (const void *)(&s_mysql_connect_timeout_sec));
        
        /* int STDCALL mysql_options(MYSQL *mysql,enum mysql_option option, const void *arg); */
        (void)mysql_options(s_db->m_mysql, MYSQL_OPT_RECONNECT, (const void *)(&s_mysql_reconnect));
    
        /* MYSQL * STDCALL mysql_real_connect(MYSQL *mysql, const char *host, const char *user, const char *passwd, const char *db, unsigned int port, const char *unix_socket, unsigned long clientflag); */
        if(mysql_real_connect(
            s_db->m_mysql,
            workload_check_string_ex(s_db->m_hostname, "localhost"),
            s_db->m_username,
            s_db->m_password,
            s_db->m_database,
            (unsigned int)s_db->m_port,
            (const char *)0,
            0ul) == ((MYSQL *)0)
        ) {
            /* errno = ... */
            return(workload_close_db(s_db));
        }

#if 1L
        do {
            struct linger s_linger;

            (void)memset((void *)(&s_linger), 0, sizeof(s_linger));
            s_linger.l_onoff = 1;
            s_linger.l_linger = 0;

            (void)setsockopt(s_db->m_mysql->net.fd, SOL_SOCKET, SO_LINGER, (const void *)(&s_linger), (socklen_t)sizeof(s_linger));
        }while(0);
#endif    
        
        return(s_db);
    }
#endif

    /* not supported OR failed */
    return(workload_close_db(s_db));
}

void *workload_open_db(int s_db_type, const char *s_database)
{
    return(workload_open_db_ex(s_db_type, (const char *)0, (-1), (const char *)0, (const char *)0, s_database));
}

void *workload_close_db(void *s_handle)
{
    workload_db_t *s_db = (workload_db_t *)s_handle;

    if(s_db == NULL) { return(NULL); }

#if def_workload_use_mysql_connector_c != (0)
    if(s_db->m_mysql != NULL) {
        /* void STDCALL mysql_close(MYSQL *sock); */
        mysql_close(s_db->m_mysql);
    }
#endif

#if def_workload_use_sqlite3 != (0)
    if(s_db->m_sqlite3 != NULL) {
        /* SQLITE_API int sqlite3_close(sqlite3 *); */
        (void)sqlite3_close(s_db->m_sqlite3);
    }
#endif    
    
    pthread_mutex_destroy((pthread_mutex_t *)(&s_db->m_lock));

    free((void *)s_db);
  
    return(NULL);
}

int workload_db_query(void *s_handle, const char *s_query, void *s_db_result, char **s_error_message)
{
    workload_db_t *s_db = (workload_db_t *)s_handle;

    (void)s_query;
    (void)s_db_result;

    if(s_db == NULL) { return(-1); }

    if(s_error_message != ((char **)0)) { *s_error_message = (char *)0; }

#if def_workload_use_sqlite3 != (0)
    if(s_db->m_db_type == def_workload_db_type_sqlite3) {
        int s_check;
        void *s_db_result_local = NULL;

        if(s_db_result == NULL) { s_db_result = (void *)(&s_db_result_local); }

        pthread_mutex_lock((pthread_mutex_t *)(&s_db->m_lock));
        s_check = workload_sqlite3_query(s_db->m_sqlite3, s_query, s_db_result, s_error_message);
	pthread_mutex_unlock((pthread_mutex_t *)(&s_db->m_lock));
        
	if(s_db_result_local != NULL) { s_db_result_local = workload_free_db_result(s_handle, s_db_result_local); }

        return(s_check);
    }
#endif    

#if def_workload_use_mysql_connector_c != (0)
    if(s_db->m_db_type == def_workload_db_type_mysql) {
        int s_check;
        
	pthread_mutex_lock((pthread_mutex_t *)(&s_db->m_lock));

        /* int STDCALL mysql_query(MYSQL *mysql, const char *q); */
        s_check = mysql_query(s_db->m_mysql, s_query);
        if(s_check != 0) {
            /*const char * STDCALL mysql_error(MYSQL *mysql);*/
            if(s_error_message != ((char **)0)) { *s_error_message = workload_check_string(mysql_error(s_db->m_mysql)); }
	    
	    pthread_mutex_unlock((pthread_mutex_t *)(&s_db->m_lock));
            
	    return(-1); 
        }

        if(s_db_result == NULL) { /* set */
            /* my_ulonglong STDCALL mysql_affected_rows(MYSQL *mysql); */
            if(mysql_affected_rows(s_db->m_mysql) == ((my_ulonglong)(-1))) {
                /*const char * STDCALL mysql_error(MYSQL *mysql);*/
                if(s_error_message != ((char **)0)) { *s_error_message = workload_check_string(mysql_error(s_db->m_mysql)); }
	        
		pthread_mutex_unlock((pthread_mutex_t *)(&s_db->m_lock));
                
		return(-1);
            }
        }
        else { /* get */
            /* MYSQL_RES * STDCALL mysql_store_result(MYSQL *mysql); */
            *((void **)s_db_result) = (void *)mysql_store_result(s_db->m_mysql);
        }
	    
	pthread_mutex_unlock((pthread_mutex_t *)(&s_db->m_lock));

        return(s_check);
    }
#endif
        
    return(-1);
}

int workload_db_vquery(void *s_handle, const char *s_format, void *s_db_result, char **s_error_message, va_list s_var)
{
    int s_result;
    char *s_query = NULL;

    if(vasprintf((char **)(&s_query), s_format, s_var) == (-1)) {
        if(s_query != NULL) { free((void *)s_query); }
        return(-1);
    }

    s_result = workload_db_query(s_handle, s_query, s_db_result, s_error_message);
 
    free((void *)s_query);

    return(s_result);
}

void *workload_db_query_get(void *s_handle, const char *s_format, ...)
{
    void *s_db_result = NULL;

    va_list s_var;

    va_start(s_var, s_format);

    if(workload_db_vquery(s_handle, s_format, (void *)(&s_db_result), (char **)0, s_var) == (-1)) {
        s_db_result = NULL;
    }

    va_end(s_var);

    return(s_db_result);
}

int workload_db_query_set(void *s_handle, const char *s_format, ...)
{
    int s_result;

    va_list s_var;

    va_start(s_var, s_format);

    s_result = workload_db_vquery(s_handle, s_format, NULL, (char **)0, s_var);

    va_end(s_var);

    return(s_result);
}

void *workload_free_db_result(void *s_handle, void *s_result)
{
    workload_db_t *s_db = (workload_db_t *)s_handle;
    
    (void)s_result;

    if(s_db == NULL) { return(NULL); }

#if def_workload_use_mysql_connector_c != (0)
    if(s_db->m_db_type == def_workload_db_type_mysql) {
        /* void STDCALL mysql_free_result(MYSQL_RES *result); */
        mysql_free_result((MYSQL_RES *)s_result);

        return(NULL);
    } 
#endif

#if def_workload_use_sqlite3 != (0)
    if(s_db->m_db_type == def_workload_db_type_sqlite3) {
        return((void *)workload_free_sqlite3_result((workload_sqlite3_result_t *)s_result));
    }
#endif

    return(NULL);
}

char **workload_db_fetch_row(void *s_handle, void *s_result)
{
    workload_db_t *s_db = (workload_db_t *)s_handle;
    
    (void)s_result;

    if(s_db == NULL) { return((char **)0); }
    if(s_result == NULL) { return((char **)0); }

#if def_workload_use_sqlite3 != (0)
    if(s_db->m_db_type == def_workload_db_type_sqlite3) {
        char **s_fetch_row;
        workload_sqlite3_result_t *s_db_result = (workload_sqlite3_result_t *)s_result;

        if(s_db_result->m_fetch == NULL) { return((char **)0); }

        s_fetch_row = s_db_result->m_fetch->m_argv;
        s_db_result->m_fetch = s_db_result->m_fetch->m_next;

        return(s_fetch_row);
    }
#endif

#if def_workload_use_mysql_connector_c != (0)
    if(s_db->m_db_type == def_workload_db_type_mysql) {
        /* MYSQL_ROW STDCALL mysql_fetch_row(MYSQL_RES *result); */
        return((char **)mysql_fetch_row((MYSQL_RES *)s_result));
    } 
#endif

    return((char **)0);
}

unsigned long long workload_db_row_count(void *s_handle, void *s_result)
{
    workload_db_t *s_db = (workload_db_t *)s_handle;
    
    (void)s_result;

    if(s_db == NULL) { return(0ull); }
    if(s_result == NULL) { return(0ull); }

#if def_workload_use_sqlite3 != (0)
    if(s_db->m_db_type == def_workload_db_type_sqlite3) {
        workload_sqlite3_result_t *s_db_result = (workload_sqlite3_result_t *)s_result;

        return(s_db_result->m_node_count);
    }
#endif

#if def_workload_use_mysql_connector_c != (0)
    if(s_db->m_db_type == def_workload_db_type_mysql) {
        /* my_ulonglong STDCALL mysql_num_rows(MYSQL_RES *res); */
        return((unsigned long long)mysql_num_rows((MYSQL_RES *)s_result));
    } 
#endif

    return((unsigned long long)0ull);
}

unsigned long long workload_db_field_count(void *s_handle, void *s_result)
{
    workload_db_t *s_db = (workload_db_t *)s_handle;
    
    (void)s_result;

    if(s_db == NULL) { return(0ull); }
    if(s_result == NULL) { return(0ull); }

#if def_workload_use_sqlite3 != (0)
    if(s_db->m_db_type == def_workload_db_type_sqlite3) {
        workload_sqlite3_result_t *s_db_result = (workload_sqlite3_result_t *)s_result;

        if(s_db_result->m_head == NULL) { return((unsigned long long)0ull); }

        return((unsigned long long)s_db_result->m_head->m_argc);
    }
#endif

#if def_workload_use_mysql_connector_c != (0)
    if(s_db->m_db_type == def_workload_db_type_mysql) {
        /* unsigned int STDCALL mysql_num_fields(MYSQL_RES *res); */
        return((unsigned long long)mysql_num_fields((MYSQL_RES *)s_result));
    } 
#endif

    return((unsigned long long)0ull);
}

int workload_db_field_index(void *s_handle, void *s_result, const char *s_name_hint, int s_is_case)
{
    workload_db_t *s_db = (workload_db_t *)s_handle;
    
    (void)s_result;
    (void)s_name_hint;
    (void)s_is_case;

    if(s_db == NULL) { return(-1); }
    if((s_result == NULL) || (s_name_hint == NULL)) { return(-1); }

#if def_workload_use_sqlite3 != (0)
    if(s_db->m_db_type == def_workload_db_type_sqlite3) {
        int s_field_index;
        char *s_pattern;
        workload_sqlite3_result_t *s_db_result = (workload_sqlite3_result_t *)s_result;

        if(s_db_result->m_head == NULL) { return(-1); }

        while(s_name_hint[0] != '\0') {
            s_pattern = workload_get_word_sep_alloc_c(0, "|", (const char **)(&s_name_hint));
            if(s_pattern == ((char *)0)) { break; }
            for(s_field_index = 0;s_field_index < s_db_result->m_head->m_argc;s_field_index++) {
                if(workload_check_pattern(s_pattern, workload_check_string(s_db_result->m_head->m_field_name[s_field_index]), s_is_case) == 0) { break; }
            }
            free((void *)s_pattern);
            if(s_field_index != s_db_result->m_head->m_argc) { return(s_field_index); }
            if(s_name_hint[0] != '\0') { s_name_hint = (const char *)(&s_name_hint[1]); }
        }
    }
#endif

#if def_workload_use_mysql_connector_c != (0)
    if(s_db->m_db_type == def_workload_db_type_mysql) {
        unsigned int s_field_count;
        unsigned int s_field_index;
        char *s_pattern;
        MYSQL_FIELD *s_field;

        /* unsigned int STDCALL mysql_num_fields(MYSQL_RES *res); */
        s_field_count = mysql_num_fields((MYSQL_RES *)s_result);
        while(s_name_hint[0] != '\0') {
            s_pattern = workload_get_word_sep_alloc_c(0, "|", (const char **)(&s_name_hint));
            if(s_pattern == ((char *)0)) { break; }
            for(s_field_index = 0;s_field_index < s_field_count;s_field_index++) {
                /* MYSQL_FIELD *STDCALL mysql_fetch_field_direct(MYSQL_RES *res, unsigned int fieldnr); */
                s_field = mysql_fetch_field_direct((MYSQL_RES *)s_result, s_field_index);
                if(s_field == NULL) { continue; }
                if(workload_check_pattern(s_pattern, workload_check_string(s_field->name), s_is_case) == 0) { break; }
            }
            free((void *)s_pattern);
            if(s_field_index != s_field_count) { return((int)s_field_index); }
            if(s_name_hint[0] != '\0') { s_name_hint = (const char *)(&s_name_hint[1]); }
        }
    } 
#endif

    return(-1);
}

int workload_db_dump_result(void *s_handle, void *s_result, const char *s_title, long long s_max)
{
    workload_db_t *s_db = (workload_db_t *)s_handle;
    
    (void)s_result;
    (void)s_title;
    (void)s_max;

    if(s_db == NULL) { return(-1); }
    if(s_result == NULL) { return(-1); }
        
    if(s_title != ((const char *)0)) {
        (void)workload_printf("*** DB QUERY: \"%s\"\n", s_title);
    }
    
#if def_workload_use_sqlite3 != (0)
    if(s_db->m_db_type == def_workload_db_type_sqlite3) {
        workload_sqlite3_result_t *s_db_result = (workload_sqlite3_result_t *)s_result;
                        
        int s_count;
        unsigned long long s_node_index;
        workload_sqlite3_node_t *s_node;

        void *s_temp_buffer;
        size_t *s_max_width;
        int *s_is_digit;
        size_t s_size;

        if(s_db_result->m_head == NULL) { return(0); }

        s_temp_buffer = (size_t *)malloc((sizeof(size_t) * ((size_t)s_db_result->m_head->m_argc)) + (sizeof(int) * ((size_t)s_db_result->m_head->m_argc)));
        if(s_temp_buffer == NULL) { return(-1); }
        s_max_width = (size_t *)s_temp_buffer;
        s_is_digit = (int *)(&s_max_width[s_db_result->m_head->m_argc]);
        for(s_count = 0;s_count < s_db_result->m_head->m_argc;s_count++) {
            s_max_width[s_count] = (size_t)strlen(s_db_result->m_head->m_field_name[s_count]);
            s_is_digit[s_count] = 1;
            for(s_node = s_db_result->m_head;s_node != NULL;s_node = s_node->m_next) {
                s_size = strlen(workload_check_string(s_node->m_argv[s_count]));
                if(s_size > s_max_width[s_count]) { s_max_width[s_count] = s_size; }
                if(s_is_digit[s_count] != 0) { s_is_digit[s_count] = workload_is_digit_string(s_node->m_argv[s_count]); }
            }
        }
                        
        (void)workload_printf(
            "%4s/%llu",
            "n",
            s_db_result->m_node_count
        );

        for(s_count = 0;s_count < s_db_result->m_head->m_argc;s_count++) {
            (void)workload_printf(
                " \"%-*s\"",
                (int)s_max_width[s_count],
                workload_check_string(s_db_result->m_head->m_field_name[s_count])
            );
        }
        (void)workload_puts("\n");
    
        for(s_node_index = (unsigned long long)0ull, s_node = s_db_result->m_head;
            ((s_max == ((long long)(-1ll))) ||
                (s_node_index < ((unsigned long long)s_max))) &&
            (s_node != NULL);
            s_node_index++,
            s_node = s_node->m_next) {
            (void)workload_printf(
                "%4llu/%llu",
                (s_node_index + ((unsigned long long)1ull)),
                s_db_result->m_node_count
            );
            for(s_count = 0;s_count < s_node->m_argc;s_count++) {
                (void)workload_printf(
                    (s_is_digit[s_count] == 0) ? " \"%-*s\"" : " \"%*s\"",
                    (int)s_max_width[s_count],
                    workload_check_string(s_node->m_argv[s_count]));
            }
            (void)workload_puts("\n");
        }

        free((void *)s_temp_buffer);

        return(0);
    }
#endif

#if def_workload_use_mysql_connector_c != (0)
    if(s_db->m_db_type == def_workload_db_type_mysql) {
        my_ulonglong s_row_count;
        my_ulonglong s_row_index;

        unsigned int s_field_count;
        unsigned int s_field_index;

        MYSQL_ROW s_row;
        MYSQL_FIELD *s_field;

        size_t s_max_width;
        int s_is_digit;

        /* my_ulonglong STDCALL mysql_num_rows(MYSQL_RES *res); */
        s_row_count = mysql_num_rows((MYSQL_RES *)s_result);
        
        /* unsigned int STDCALL mysql_num_fields(MYSQL_RES *res); */
        s_field_count = mysql_num_fields((MYSQL_RES *)s_result);

        (void)workload_printf(
            "%4s/%llu",
            "n",
            (unsigned long long)s_row_count
        );
            
        for(s_field_index = 0u;s_field_index < s_field_count;s_field_index++) {
            /* MYSQL_FIELD *STDCALL mysql_fetch_field_direct(MYSQL_RES *res, unsigned int fieldnr); */
            s_field = mysql_fetch_field_direct((MYSQL_RES *)s_result, s_field_index);
            s_max_width = (s_field == NULL) ? ((size_t)0u) : ((((size_t)s_field->max_length) > ((size_t)s_field->name_length)) ? ((size_t)s_field->max_length) : ((size_t)s_field->name_length));
            (void)workload_printf(
                " \"%-*s\"",
                (int)s_max_width,
                (s_field == NULL) ? "" : workload_check_string(s_field->name)
            );
        }
        (void)workload_puts("\n");
       
        for(s_row_index = (my_ulonglong)0u;s_row_index < s_row_count;s_row_index++) {
            if((s_max != ((long long)(-1ll))) && (s_row_index >= ((my_ulonglong)s_max))) { break; }

            /* MYSQL_ROW	STDCALL mysql_fetch_row(MYSQL_RES *result); */
            s_row = mysql_fetch_row((MYSQL_RES *)s_result);
            (void)workload_printf(
                "%4llu/%llu",
                (unsigned long long)(s_row_index + ((my_ulonglong)1u)), (unsigned long long)s_row_count);
            for(s_field_index = 0u;s_field_index < s_field_count;s_field_index++) {
                /* MYSQL_FIELD *STDCALL mysql_fetch_field_direct(MYSQL_RES *res, unsigned int fieldnr); */
                s_field = mysql_fetch_field_direct((MYSQL_RES *)s_result, s_field_index);
#if 0L                
                s_max_width = (s_field == NULL) ? ((size_t)0u) : ((((size_t)s_field->max_length) > ((size_t)s_field->name_length)) ? ((size_t)s_field->max_length) : ((size_t)s_field->name_length));
#else
                s_max_width = (s_field == NULL) ? ((size_t)0u) : ((size_t)s_field->max_length);
#endif
                s_is_digit = (s_field == NULL) ? 0 : ((IS_NUM_FIELD(s_field) == 0) ? 0 : 1);

                (void)workload_printf(
                    (s_is_digit == 0) ? " \"%-*s\"" : " \"%*s\"",
                    (int)s_max_width,
                    workload_check_string(s_row[s_field_index]));
            }
            (void)workload_puts("\n");
        }
    }
#endif    

    return(-1);
}

#endif

/* vim: set expandtab: */
/* End of source */
