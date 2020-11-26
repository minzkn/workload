/*
    Copyright (C) HWPORT.COM
    All rights reserved.
    Author: JAEHYUK CHO <mailto:minzkn@minzkn.com>
*/

/* this is ezxml's simple warpper DOM */

#if !defined(__def_hwport_workload_source_xml_c__)
# define __def_hwport_workload_source_xml_c__ "xml.c"

#if !defined(_ISOC99_SOURCE)
# define _ISOC99_SOURCE (1L)
#endif

#if !defined(_GNU_SOURCE)
# define _GNU_SOURCE (1L)
#endif

#include "workload.h"

#include <sys/stat.h>
#include <unistd.h>

#include "ezxml.h"

typedef struct workload_xml_t {
    size_t m_size;
    unsigned char *m_data;

    ezxml_t m_xml_root;
}workload_xml_t;

void *workload_open_xml(const void *s_data, size_t s_size);
void *workload_open_xml_from_file(const char *s_pathname);
void *workload_close_xml(void *s_handle);

void *workload_get_xml_node(void *s_handle, const char *s_path);

void *workload_xml_next_node_ex(void *s_node, const char *s_tag_name);
void *workload_xml_next_node(void *s_node);
void *workload_xml_sub_node_ex(void *s_node, const char *s_sub_tag_name);
void *workload_xml_sub_node(void *s_node);
char *workload_xml_tag_name(void *s_node);
char *workload_xml_tag_data(void *s_node);
char **workload_xml_tag_attr(void *s_node);
char *workload_xml_tag_attr_value(void *s_node, const char *s_attr_name);

char *workload_get_xml_data(void *s_handle, const char *s_path);
char *workload_get_xml_attr_value(void *s_handle, const char *s_path, const char *s_attr_name);

void *workload_open_xml(const void *s_data, size_t s_size)
{
    workload_xml_t *s_xml;

    if((s_data == NULL) || (s_size <= ((size_t)0u))) { return(NULL); }

    s_xml = (workload_xml_t *)malloc(sizeof(workload_xml_t) + s_size + (size_t)1u);
    if(s_xml == NULL) { return(NULL); }

    s_xml->m_size = s_size;
    s_xml->m_data = (unsigned char *)memcpy((void *)(&s_xml[1]), s_data, s_xml->m_size);
    s_xml->m_data[s_xml->m_size] = (unsigned char)'\0';

    s_xml->m_xml_root = ezxml_parse_str((void *)s_xml->m_data, s_xml->m_size);
    if(s_xml->m_xml_root == ((ezxml_t)0)) {
        free((void *)s_xml);
        return(NULL);
    }

    return((void *)s_xml);
}

void *workload_open_xml_from_file(const char *s_pathname)
{
    void *s_handle; 
    struct stat s_stat;
    int s_fd;

    unsigned char *s_buffer;
    size_t s_want_size;
    ssize_t s_read_bytes;
    off_t s_position;

    if(s_pathname == NULL) { return(NULL); }

    s_fd = open(s_pathname, O_RDONLY);
    if(s_fd == (-1)) { return(NULL); }

    if(fstat(s_fd, (struct stat *)(&s_stat)) == (-1)) {
        (void)close(s_fd);
	return(NULL);
    }

    if((s_stat.st_size <= ((off_t)0)) || (s_stat.st_size >= ((off_t)(4 << 20)))) {
        (void)close(s_fd);
	return(NULL);
    }

    s_buffer = (unsigned char *)malloc((size_t)s_stat.st_size);
    if(s_buffer == NULL) {
        (void)close(s_fd);
	return(NULL);
    }

    for(s_position = (off_t)0;;) {
        s_want_size = ((s_stat.st_size - s_position) > ((off_t)(64 << 10))) ? ((size_t)(64 << 10)) : ((size_t)(s_stat.st_size - s_position));
	s_read_bytes = workload_read(s_fd, (void *)(&s_buffer[s_position]), s_want_size, (-1) /* infinite */);
	if(s_read_bytes <= ((ssize_t)0)) { break; }
	s_position += (off_t)s_read_bytes;
    }

    s_handle = (s_position == s_stat.st_size) ? workload_open_xml(s_buffer, (size_t)s_position) : NULL;

    free((void *)s_buffer);
    (void)close(s_fd);

    return(s_handle);
}

void *workload_close_xml(void *s_handle)
{
    workload_xml_t *s_xml = (workload_xml_t *)s_handle;

    if(s_xml == NULL) { return(NULL); }

    if(s_xml->m_xml_root == ((ezxml_t)0)) { ezxml_free(s_xml->m_xml_root); }

    free((void *)s_xml);
 
    return(NULL);
}

void *workload_get_xml_node(void *s_handle, const char *s_path)
{
    workload_xml_t *s_xml = (workload_xml_t *)s_handle;
    ezxml_t s_result, s_xml_root, s_xml_this;
    workload_string_node_t *s_path_node, *s_trace_node;

    if(s_xml == NULL) { return(NULL); }

    s_path_node = workload_path_to_node(s_path);
    if(s_path_node == NULL) { return((void *)s_xml->m_xml_root); }

    for(s_result = (ezxml_t)0,
        s_xml_root = s_xml->m_xml_root,
        s_trace_node = s_path_node;
	(s_xml_root != ((ezxml_t)0)) &&
	(s_trace_node != ((workload_string_node_t *)0));
	s_trace_node = s_trace_node->m_next) {
        if(strlen(workload_check_string(s_trace_node->m_string)) <= ((size_t)0u)) { continue; }

        for(s_xml_this = s_xml_root;s_xml_this != ((ezxml_t)0);s_xml_this = s_xml_this->ordered) {
            if(strcmp(workload_check_string(ezxml_name(s_xml_this)), s_trace_node->m_string) == 0) {
	        if(s_trace_node->m_next == NULL) { s_result = s_xml_this; }
	        break;
	    }
	}
	if(s_xml_this == ((ezxml_t)0)) { break; }
	s_xml_root = s_xml_this->child;
    }
    (void)workload_free_path_node(s_path_node);

    return(s_result);
}

void *workload_xml_next_node_ex(void *s_node, const char *s_tag_name)
{
    ezxml_t s_xml_this = (ezxml_t)s_node;

    s_xml_this = (s_xml_this == NULL) ? NULL : s_xml_this->ordered;
    if(s_tag_name == NULL) { return((void *)s_xml_this); }

    for(;(s_xml_this != ((ezxml_t)0)) && (strcmp(s_tag_name, workload_check_string(ezxml_name(s_xml_this))) != 0);s_xml_this = s_xml_this->ordered);

    return(s_xml_this);
}

void *workload_xml_next_node(void *s_node)
{
    return(workload_xml_next_node_ex(s_node, NULL));
}

void *workload_xml_sub_node_ex(void *s_node, const char *s_sub_tag_name)
{
    ezxml_t s_xml_this = (ezxml_t)s_node;
 
    if(s_sub_tag_name == NULL) { return((s_xml_this == NULL) ? NULL : ((void *)s_xml_this->child)); }

    return(ezxml_child(s_xml_this, s_sub_tag_name));
}

void *workload_xml_sub_node(void *s_node)
{
    return(workload_xml_sub_node_ex(s_node, NULL));
}

char *workload_xml_tag_name(void *s_node)
{
    return(ezxml_name(((ezxml_t)s_node)));
}

char *workload_xml_tag_data(void *s_node)
{
    return(workload_check_string(ezxml_txt(((ezxml_t)s_node))));
}

char **workload_xml_tag_attr(void *s_node)
{
    ezxml_t s_xml_this = (ezxml_t)s_node;

    return((s_xml_this == ((ezxml_t)0)) ? ((char **)0) : s_xml_this->attr);
}

char *workload_xml_tag_attr_value(void *s_node, const char *s_attr_name)
{
    ezxml_t s_xml_this = (ezxml_t)s_node;
    char *s_attr_value;
    int s_attr_index;

    if(s_xml_this == NULL) { return(NULL); }
    if(s_xml_this->attr == ((char **)0)) { return(NULL); }

    for(s_attr_index = 0, s_attr_value = NULL;s_xml_this->attr[s_attr_index] != NULL;s_attr_index += 2) {
	if(strcmp(s_xml_this->attr[s_attr_index], s_attr_name) == 0) {
	    s_attr_value = s_xml_this->attr[s_attr_index + 1];
	    break;
	}
    }

    return(s_attr_value);
}

char *workload_get_xml_data(void *s_handle, const char *s_path)
{
    return(workload_xml_tag_data(workload_get_xml_node(s_handle, s_path)));
}

char *workload_get_xml_attr_value(void *s_handle, const char *s_path, const char *s_attr_name)
{
    return(workload_xml_tag_attr_value(workload_get_xml_node(s_handle, s_path), s_attr_name));
}

#endif

/* vim: set expandtab: */
/* End of source */
