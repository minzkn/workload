/*
    Copyright (C) HWPORT.COM
    All rights reserved.
    Author: JAEHYUK CHO <mailto:minzkn@minzkn.com>
*/

#if !defined(__def_hwport_workload_source_math_c__)
# define __def_hwport_workload_source_math_c__ "math.c"

#include "workload.h"

unsigned int workload_gcm_uint(unsigned int s_value1, unsigned int s_value2);
unsigned int workload_aspect_ratio_uint(unsigned int s_value1, unsigned int s_value2, unsigned int *s_ratio1_ptr, unsigned int *s_ratio2_ptr);

unsigned int workload_gcm_uint(unsigned int s_value1, unsigned int s_value2)
{
    unsigned int s_temp;

    if(s_value1 < s_value2) {
        s_temp = s_value1;
	s_value1 = s_value2;
	s_value2 = s_temp;
    }

    if(s_value2 == 0u) { /* invalid argument */ return(0u); }

    for(;;) {
        s_temp = s_value1 % s_value2;
	if(s_temp == 0u) { break; }
	s_value1 = s_value2;
	s_value2 = s_temp;
    }

    return(s_value2);
}

unsigned int workload_aspect_ratio_uint(unsigned int s_value1, unsigned int s_value2, unsigned int *s_ratio1_ptr, unsigned int *s_ratio2_ptr)
{
    unsigned int s_gcm;

    if((s_value1 == 0u) || (s_value2 == 0u)) {
        if(s_ratio1_ptr != NULL) { *s_ratio1_ptr = 1u; }
        if(s_ratio2_ptr != NULL) { *s_ratio2_ptr = 1u; }
	return(0u);
    }

    s_gcm = workload_gcm_uint(s_value1, s_value2);
    if(s_gcm == 0u) {
        if(s_ratio1_ptr != NULL) { *s_ratio1_ptr = 1u; }
        if(s_ratio2_ptr != NULL) { *s_ratio2_ptr = 1u; }
	return(0u);
    }
        
    if(s_ratio1_ptr != NULL) { *s_ratio1_ptr = s_value1 / s_gcm; }
    if(s_ratio2_ptr != NULL) { *s_ratio2_ptr = s_value2 / s_gcm; }

    return(s_gcm);
}

#endif

/* vim: set expandtab: */
/* End of source */
