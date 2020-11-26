/*
    Copyright (C) HWPORT.COM
    All rights reserved.
    Author: JAEHYUK CHO <mailto:minzkn@minzkn.com>
*/

#if !defined(__def_hwport_workload_source_string_c__)
# define __def_hwport_workload_source_string_c__ "string.c"

#if !defined(_ISOC99_SOURCE)
# define _ISOC99_SOURCE (1L)
#endif

#if !defined(_GNU_SOURCE)
# define _GNU_SOURCE (1L)
#endif

#include "workload.h"

#include <ctype.h>

char *workload_check_string_ex(const char *s_string, const char *s_default_string);
char *workload_check_string(const char *s_string);

int workload_is_digit_string(const char *s_string);

char *workload_get_word_sep_alloc_c(int s_skip_space, const char *s_sep, const char **s_sep_string);
int workload_check_pattern(const char *s_pattern, const char *s_string, int s_is_case);

workload_string_node_t *workload_free_string_node(workload_string_node_t *s_node);
static int workload_check_ignore_path_node(workload_string_node_t *s_current);
workload_string_node_t *workload_string_to_node_ex(const char *s_string, const char *s_sep, int s_is_ignore_tag);
workload_string_node_t *workload_string_to_node(const char *s_string, const char *s_sep);
char *workload_node_to_string(workload_string_node_t *s_node, const char *s_insert_string, int s_strip);
workload_string_node_t *workload_append_string_node_ex(workload_string_node_t *s_head, workload_string_node_t *s_node, int s_override, int s_is_ignore_tag);
workload_string_node_t *workload_append_string_node(workload_string_node_t *s_head, workload_string_node_t *s_node, int s_override);

workload_string_node_t *workload_free_path_node(workload_string_node_t *s_node);
workload_string_node_t *workload_path_to_node(const char *s_path);
char *workload_node_to_path(workload_string_node_t *s_node, int s_strip);
workload_string_node_t *workload_copy_path_node(workload_string_node_t *s_node);
workload_string_node_t *workload_append_path_node(workload_string_node_t *s_head, workload_string_node_t *s_node, int s_override);

char *workload_basename(char *s_pathname);

static char *workload_comma_string_private(char *s_buffer, const char *s_string, size_t s_string_size);
char *workload_comma_string(char *s_buffer, size_t s_buffer_size, const char *s_string);
char *workload_printable_comma_string(char *s_buffer, size_t s_buffer_size, unsigned long long s_value, unsigned long long s_div);

int workload_parse_boolean_string(const char *s_string, int s_default_value);

char *workload_check_string_ex(const char *s_string, const char *s_default_string)
{
    union { const char *m_const_string; char *m_string; }s_ptr = { (s_string == NULL) ? s_default_string : s_string };

    return(s_ptr.m_string);
}

char *workload_check_string(const char *s_string)
{
    static const char sg_empty_string[] = {'\0'};
    return(workload_check_string_ex(s_string, (const char *)(&sg_empty_string[0])));
}

int workload_is_digit_string(const char *s_string)
{
    while(isdigit(s_string[0]) != 0) { s_string = (const char *)(&s_string[1]); }

    return((s_string[0] == '\0') ? 1 : 0);
}

char *workload_get_word_sep_alloc_c(int s_skip_space, const char *s_sep, const char **s_sep_string)
{
    char *s_result;

    size_t s_token_size;
    const unsigned char *s_string;
    const unsigned char *s_left;
    const unsigned char *s_right;
    const unsigned char *s_sep_ptr;

    s_string = (const unsigned char *)(*(s_sep_string));

    if(s_skip_space != 0) {
        while(isspace((int)s_string[0]) != 0) { s_string = (const unsigned char *)(&s_string[1]); }
    }
    
    s_right = s_left = s_string;
    if(s_sep != ((const char *)0)) {
        while(s_string[0] != '\0') {
            s_sep_ptr = (const unsigned char *)s_sep;
            while((s_string[0] != s_sep_ptr[0]) && (s_sep_ptr[0] != '\0')) {
                s_sep_ptr = (const unsigned char *)(&s_sep_ptr[1]);
            }
            if(s_string[0] == s_sep_ptr[0]) {
                break;
            }
            s_string = (const unsigned char *)(&s_string[1]);
            s_right = s_string;
        }
    }
    
    s_token_size = (size_t)(s_right - s_left);
    s_result = (char *)malloc(s_token_size + ((size_t)1u));
    if(s_result != ((char *)0)) {
        if(s_token_size > ((size_t)0)) {
            (void)memcpy((void *)s_result, (const void *)s_left, s_token_size);
        }
        s_result[s_token_size] = '\0';
    }

    *(s_sep_string) = (const char *)s_string;

    return(s_result);
}

int workload_check_pattern(const char *s_pattern, const char *s_string, int s_is_case)
{
    size_t s_pattern_offset = (size_t)0u, s_string_offset = (size_t)0u;
    unsigned char s_pattern_byte, s_string_byte;

    for(;;) {
        s_pattern_byte = (unsigned char)s_pattern[s_pattern_offset];
        s_string_byte = (unsigned char)s_string[s_string_offset];

        if(s_pattern_byte == ((unsigned char)0u)) { break; }

        if(s_pattern_byte == ((unsigned char)'*')) {
            ++s_pattern_offset;
            s_pattern_byte = (unsigned char)s_pattern[s_pattern_offset];
            if(s_pattern_byte == ((unsigned char)'\\')) {
                ++s_pattern_offset;
                s_pattern_byte = (unsigned char)s_pattern[s_pattern_offset];
            }
            while(s_string_byte != ((unsigned char)0u)) {
	        if(s_is_case == 0) {
		    if(s_pattern_byte == s_string_byte)break;
		}
		else if(tolower((int)s_pattern_byte) == tolower((int)s_string_byte)){ break; }
                ++s_string_offset;
                s_string_byte = (unsigned char)s_string[s_string_offset];
            }
            if(s_pattern_byte == ((unsigned char)0u)) {
                break;
            }
        }
        else if(s_pattern_byte == ((unsigned char)'?')) {
            if(s_string_byte == ((unsigned char)0u)) {
                return(-1);
            }
        }
        else {
            if(s_pattern_byte == ((unsigned char)'\\')) {
                ++s_pattern_offset;
                s_pattern_byte = (unsigned char)s_pattern[s_pattern_offset];
                if(s_pattern_byte == ((unsigned char)0u)) {
                    break;
                }
            }
	    if(s_is_case == 0) {
	        if(s_pattern_byte != s_string_byte) { return(-1); }
	    }
	    else if(tolower((int)s_pattern_byte) != tolower((int)s_string_byte)) { return(-1); }
        }
        ++s_pattern_offset;
        if(s_string_byte != ((unsigned char)0u)) {
            ++s_string_offset;
        }
    }

    return((s_pattern_byte == s_string_byte) ? 0 : (-1));
}

workload_string_node_t *workload_free_string_node(workload_string_node_t *s_node)
{
    workload_string_node_t *s_prev;
    
    while(s_node != NULL) {
        s_prev = s_node;
        s_node = s_node->m_next;

        if(s_prev->m_string != NULL) { free((void *)s_prev->m_string); }

        free((void *)s_prev);
    }

    return(NULL);
}

static int workload_check_ignore_path_node(workload_string_node_t *s_current)
{
    workload_string_node_t *s_trace;
    size_t s_name_size = (s_current->m_string == NULL) ? ((size_t)0u) : strlen(s_current->m_string);

    if(s_name_size == ((size_t)0u)) {
        if(s_current->m_prev != NULL) {
            s_current->m_ignore = 1u;
        }
        
        return(0);
    }

    if(strcmp(s_current->m_string, "..") == 0) {
        s_current->m_ignore = 1u;
        
        for(s_trace = s_current->m_prev;s_trace != ((workload_string_node_t *)0);s_trace = s_trace->m_prev) {
            if(s_trace->m_ignore == 0u) {
                s_name_size = (s_trace->m_string == NULL) ? ((size_t)0u) : strlen(s_trace->m_string);

                if(s_name_size > ((size_t)0u)) {
                    s_trace->m_ignore = 1u;
                }
                break;
            }
        }

        return(0);
    }
    
    if(strcmp(s_current->m_string, ".") == 0) {
        s_current->m_ignore = 1u;
        return(0);
    }

    return(0);
}

workload_string_node_t *workload_string_to_node_ex(const char *s_string, const char *s_sep, int s_is_ignore_tag)
{
    workload_string_node_t *s_head = NULL;
    workload_string_node_t *s_tail = NULL;
    workload_string_node_t *s_new;
    char *s_name;

    if(s_string == NULL) { return(NULL); }

    if(s_sep == ((const char *)0)) {
        s_new = (workload_string_node_t *)malloc(sizeof(workload_string_node_t));        
        if(s_new == NULL) { return(NULL); }

        s_new->m_prev = s_new->m_next = NULL;
        s_new->m_ignore = 0u;
        s_new->m_string = strdup(s_string);
        if(s_new->m_string == NULL) {
            return(workload_free_path_node(s_new));
        }

        return(s_new);
    }
     
    while(s_string[0] != '\0') {
        s_name = workload_get_word_sep_alloc_c(0, s_sep, (const char **)(&s_string));
        if(s_name == NULL) { break; }

        if(s_string[0] != '\0') { s_string = (const char *)(&s_string[1]); }

        s_new = (workload_string_node_t *)malloc(sizeof(workload_string_node_t));        
        if(s_new == NULL) {
            free((void *)s_name);
            return(workload_free_path_node(s_head));
        }

        s_new->m_prev = s_tail;
        s_new->m_next = NULL;
        s_new->m_ignore = 0u;
        s_new->m_string = s_name;

        if(s_tail == NULL) { s_head = s_new; }
        else { s_tail->m_next = s_new; }
        s_tail = s_new;

        if(s_is_ignore_tag != 0) { (void)workload_check_ignore_path_node(s_new); }
    }
     
    return(s_head);
}

workload_string_node_t *workload_string_to_node(const char *s_string, const char *s_sep)
{
    return(workload_string_to_node_ex(s_string, s_sep, 0));
}

char *workload_node_to_string(workload_string_node_t *s_node, const char *s_insert_string, int s_strip)
{
    char *s_result;
    workload_string_node_t *s_trace, *s_trace2;
    size_t s_alloc_size, s_name_size, s_insert_string_size;
    
    if(s_node == NULL) {
        return(NULL);
    }
    s_insert_string_size = strlen(workload_check_string(s_insert_string));

    for(s_alloc_size = (size_t)0u, s_trace = s_node;s_trace != ((workload_string_node_t *)0);) {
        if(s_strip != 0) {
            if(s_trace->m_ignore != 0u) {
                s_trace = s_trace->m_next;
                continue;
            }
        }

        s_trace2 = s_trace->m_next;
        while((s_strip != 0) && (s_trace2 != NULL)) {
            if(s_trace2->m_ignore == 0u) { break; }
            s_trace2 = s_trace2->m_next;
        }
        
        s_name_size = (s_trace->m_string == NULL) ? ((size_t)0u) : strlen(s_trace->m_string);
        if(s_trace2 == NULL) { s_alloc_size += ((s_name_size <= ((size_t)0u)) ? s_insert_string_size : s_name_size) + ((size_t)1u); }
        else { s_alloc_size += s_name_size + s_insert_string_size; }

        s_trace = s_trace->m_next;
    }
   
    s_result = (char *)malloc(s_alloc_size);
    if(s_result == NULL) { return((char *)0); }

    s_alloc_size = (size_t)0u;
    s_trace = s_node;
    while(s_trace != ((workload_string_node_t *)0)) {
        if(s_strip != 0) {
            if(s_trace->m_ignore != 0u) {
                s_trace = s_trace->m_next;
                continue;
            }
        }

        s_trace2 = s_trace->m_next;
        while((s_strip != 0) && (s_trace2 != ((workload_string_node_t *)0))) {
            if(s_trace2->m_ignore == 0u) {
                break;
            }
            s_trace2 = s_trace2->m_next;
        }
        
        s_name_size = (s_trace->m_string == ((char *)0)) ? ((size_t)0u) : strlen(s_trace->m_string);

        if(s_trace2 == ((workload_string_node_t *)0)) {
            if(s_name_size <= ((size_t)0u)) {
                s_alloc_size += (size_t)sprintf((char *)(&s_result[s_alloc_size]), "%s", workload_check_string(s_insert_string));
            }
            else {
                s_alloc_size += (size_t)sprintf((char *)(&s_result[s_alloc_size]), "%s", s_trace->m_string);
            }
        }
        else {
            s_alloc_size += (size_t)sprintf((char *)(&s_result[s_alloc_size]), "%s%s", s_trace->m_string, workload_check_string(s_insert_string));
        }

        s_trace = s_trace->m_next;
    }

    return(s_result);
}

workload_string_node_t *workload_append_string_node_ex(workload_string_node_t *s_head, workload_string_node_t *s_node, int s_override, int s_is_ignore_tag)
{
    workload_string_node_t *s_temp;

    if(s_override != 0) {
        if(s_node != NULL) {
            if(s_node->m_string != NULL) {
                if(strlen(s_node->m_string) <= ((size_t)0u)) {
                    s_head = workload_free_string_node(s_head); 
                }
            }
        }
    }

    if(s_head == NULL) {
        if(s_node != NULL) { s_node->m_prev = NULL; }
        s_head = s_node;
        s_node = NULL;
    }
        
    if(s_node != NULL) {
        s_temp = s_head;
        while(s_temp->m_next != NULL) { s_temp = s_temp->m_next; }
        s_node->m_prev = s_temp;
        s_temp->m_next = s_node;
    }
       
    if(s_is_ignore_tag != 0) {
        /* clear ignore */
        s_temp = s_head;
        while(s_temp != NULL) {
            s_temp->m_ignore = 0;
            s_temp = s_temp->m_next;
        }

        /* restrip */
        s_temp = s_head;
        while(s_temp != NULL) {
            (void)workload_check_ignore_path_node(s_temp);
            s_temp = s_temp->m_next;
        }
    }
        
    return(s_head);
}

workload_string_node_t *workload_append_string_node(workload_string_node_t *s_head, workload_string_node_t *s_node, int s_override)
{
    return(workload_append_string_node_ex(s_head, s_node, s_override, 0));
}

/* ---- */

workload_string_node_t *workload_free_path_node(workload_string_node_t *s_node)
{
    return(workload_free_string_node(s_node));
}

workload_string_node_t *workload_path_to_node(const char *s_path)
{
    return(workload_string_to_node_ex(s_path, "/\\", 1));
}

char *workload_node_to_path(workload_string_node_t *s_node, int s_strip)
{
    return(workload_node_to_string(s_node, "/", s_strip));
}

workload_string_node_t *workload_copy_path_node(workload_string_node_t *s_node)
{
    workload_string_node_t *s_result;
    char *s_path;

    s_path = workload_node_to_string(s_node, "/", 0);
    if(s_path == NULL) { return(NULL); }

    s_result = workload_string_to_node_ex(s_path, "/\\", 1);

    free((void *)s_path);

    return(s_result);
}

workload_string_node_t *workload_append_path_node(workload_string_node_t *s_head, workload_string_node_t *s_node, int s_override)
{
    return(workload_append_string_node_ex(s_head, s_node, s_override, 1));
}

char *workload_basename(char *s_pathname)
{
    static char sg_dot_string[] = {"."};

    char *s_result;
    size_t s_count;
    size_t s_offset;

    if(s_pathname == NULL) { return((char *)(&sg_dot_string[0])); }
    if(s_pathname[0] == '\0') { return(s_pathname); }

    s_count = (size_t)0u;
    s_offset = strlen(s_pathname);
    while(s_offset > ((size_t)0u)) {
        if((s_pathname[s_offset - ((size_t)1u)] == '/') ||
           (s_pathname[s_offset - ((size_t)1u)] == '\\')) {
            if(s_count > ((size_t)0u)) { break; }
            if(s_offset > ((size_t)1u)) { s_pathname[s_offset - ((size_t)1u)] = '\0'; }
        }
        else { ++s_count; }
        --s_offset;
    }
    
    s_result = (char *)(&s_pathname[s_offset]);
    if(strlen(s_result) <= ((size_t)0u)) { return((char *)(&sg_dot_string[0])); }
        
    return(s_result);
}

static char *workload_comma_string_private(char *s_buffer, const char *s_string, size_t s_string_size)
{
    size_t s_from_offset;
    size_t s_to_offset;
    
    size_t s_remain;
    
    if(s_buffer == NULL) { return(NULL); }

    /* skip no-digit */
    for(s_to_offset = (size_t)0u, s_from_offset = (size_t)0u;(s_from_offset < s_string_size) && (isdigit((int)s_string[s_from_offset]) == 0);) {
        s_buffer[s_to_offset] = s_string[s_from_offset];
        ++s_to_offset;
        ++s_from_offset;
    }
    
    while(s_from_offset < s_string_size) {
        s_buffer[s_to_offset] = s_string[s_from_offset];
        ++s_to_offset;
        ++s_from_offset;
        
        s_remain = s_string_size - s_from_offset;
        if(s_remain <= ((size_t)0u)) {
            continue;
        }

        if((s_remain % ((size_t)3u)) == ((size_t)0u)) {
            s_buffer[s_to_offset] = ',';
            ++s_to_offset;
        }
    }
    s_buffer[s_to_offset] = '\0';

    return(s_buffer);
}

char *workload_comma_string(char *s_buffer, size_t s_buffer_size, const char *s_string)
{
    size_t s_size;
    size_t s_require_buffer_size;
    
    if(s_string == NULL) { return(NULL); }

    s_size = strlen(s_string);
    s_require_buffer_size = s_size + ((s_size > ((size_t)0u)) ? ((s_size - ((size_t)1u)) / ((size_t)3u)) : ((size_t)0u)) + ((size_t)1u);

    if(s_buffer_size < s_require_buffer_size) { /* not enough buffer size */ return(NULL); }

    return(workload_comma_string_private(s_buffer, s_string, s_size));
}

char *workload_printable_comma_string(char *s_buffer, size_t s_buffer_size, unsigned long long s_value, unsigned long long s_div)
{
    static const int cg_unit_table[] = {
        '\0', 'K', 'M', 'G', 'T', 'P', (-1)
    };
    int s_unit_index = 0;
    char s_temp_string[ 64 ];
    char *s_string;
    size_t s_string_size;

    do {
        (void)sprintf((char *)(&s_temp_string[0]), "%llu", s_value);
        s_string = workload_comma_string(
	    s_buffer,
	    s_buffer_size - ((cg_unit_table[s_unit_index] == '\0') ? ((size_t)0u) : ((size_t)1u)),
	    (char *)(&s_temp_string[0]));
	if(s_string != NULL) {
	    s_string_size = strlen(s_string);
	    if(cg_unit_table[s_unit_index] != '\0') {
	        s_string[s_string_size++] = (char)cg_unit_table[s_unit_index];
	        s_string[s_string_size] = '\0';
	    }
	    return(s_string);
	}
	
	++s_unit_index;
	if(s_div == 0ull) { break; }
	s_value /= s_div;
    }while((s_string == NULL) && (cg_unit_table[s_unit_index] != (-1)));

    return(NULL);
}


int workload_parse_boolean_string(const char *s_string, int s_default_value)
{
    static struct {
        int m_value;
	const char *m_string;
    }sg_boolean_string_table[] = {
        {0, "false"},
        {0, "off"},
        {0, "disable"},
        {0, "no"},
        {1, "true"},
        {1, "on"},
        {1, "enable"},
        {1, "yes"},
	{(-1), NULL}
    };
    int s_index;

    if(s_string == NULL) { return(s_default_value); }

    for(s_index = 0;sg_boolean_string_table[s_index].m_string != NULL;s_index++) {
        if(strcasecmp(s_string, sg_boolean_string_table[s_index].m_string) == 0) {
	    return(sg_boolean_string_table[s_index].m_value);
	}
    }

    return(s_default_value);
}

#endif

/* vim: set expandtab: */
/* End of source */
