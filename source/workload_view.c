/*
    Copyright (C) HWPORT.COM
    All rights reserved.
    Author: JAEHYUK CHO <mailto:minzkn@minzkn.com>
*/

#if !defined(__def_hwport_workload_source_workload_view_c__)
# define __def_hwport_workload_source_workload_view_c__ "workdload_view.c"

#include "workload.h"

int workload_update_screen(workload_t *s_workload);

int workload_update_screen(workload_t *s_workload)
{
    static const char cg_title[] = {
        " [ WORKLOAD v" def_workload_version " ] "
    };

    size_t s_width, s_height;
    int s_changed_terminal;

    size_t s_entry_x, s_entry_y, s_grid_x, s_grid_y;
    size_t s_x, s_y, s_ty, s_count;

    size_t s_disk_index;
    workload_disk_t *s_disk;

    char s_string_buffer[ 64 ];
    char *s_string;
    size_t s_size;

    unsigned long long s_read_count;
    unsigned long long s_write_count;
    unsigned int s_ratio;
    unsigned long long s_time_stamp;

    if(s_workload->m_opt.m_daemonize != 0) { return(-1); }

    pthread_mutex_lock((pthread_mutex_t *)(&s_workload->m_reassign_lock));

    (void)workload_get_terminal_size((size_t *)(&s_width), (size_t *)(&s_height), (int *)(&s_changed_terminal));
    s_entry_x = (s_width <= ((size_t)40u)) ? ((size_t)1u) : ((size_t)2u);
    s_entry_y = (s_height <= ((size_t)20u)) ? ((size_t)1u) : ((size_t)4u);
    
    s_grid_x = s_workload->m_max_name_size;
    if(s_grid_x < ((size_t)7u)) { s_grid_x = (size_t)7u; }
    s_grid_y = (size_t)8u;
    if((s_workload->m_flags & def_workload_flag_use_fakeoff) != def_workload_flag_none) {
        ++s_grid_y;
    }

    if(s_changed_terminal != 0) { /* redraw */
        (void)workload_vt_clear();

        /* draw title */
        if(s_entry_y >= ((size_t)4u)) {
            s_size = strlen((const char *)(&cg_title[0]));
            s_x = (s_width >> 1) - (s_size >> 1);
            (void)workload_vt_move(s_x, (size_t)2u);
            (void)workload_printf(workload_vt_code("42m") "%s" workload_vt_color_normal, (const char *)(&cg_title[0]));
        }

        /* draw frame */
        for(s_disk_index = (size_t)0u,
            s_x = s_entry_x,
            s_y = s_entry_y;
            s_disk_index < s_workload->m_disk_count;
            s_disk_index++) {
            s_disk = s_workload->m_disk[s_disk_index];
            if((s_y + s_grid_y + ((size_t)2u)) > s_height) {
                (void)workload_vt_puts(s_width - ((size_t)4u), s_height - ((size_t)1u), workload_vt_code("42m") "MORE" workload_vt_color_normal);
                break;    
            }

            if(s_x == s_entry_x) {
                s_ty = (size_t)s_y;
                (void)workload_vt_puts(s_x, s_ty++, "+=======+");
                (void)workload_vt_puts(s_x, s_ty++, "| " workload_vt_color_yellow "NAME" workload_vt_color_normal "  |");
                (void)workload_vt_puts(s_x, s_ty++, "| " workload_vt_color_yellow "FILES" workload_vt_color_normal " |");
                (void)workload_vt_puts(s_x, s_ty++, "| " workload_vt_color_yellow "USAGE" workload_vt_color_normal " |");
                (void)workload_vt_puts(s_x, s_ty++, "|       |");
                (void)workload_vt_puts(s_x, s_ty++, "| " workload_vt_color_yellow "RATIO" workload_vt_color_normal " |");
                (void)workload_vt_puts(s_x, s_ty++, "|       |");
                (void)workload_vt_puts(s_x, s_ty++, "| " workload_vt_color_yellow "IDLE" workload_vt_color_normal "  |");
                if((s_workload->m_flags & def_workload_flag_use_fakeoff) != def_workload_flag_none) {
                    (void)workload_vt_puts(s_x, s_ty++, "| " workload_vt_color_yellow "FAKE" workload_vt_color_normal "  |");
		}
                (void)workload_vt_puts(s_x, s_ty++, "+=======+");
                s_x += (size_t)8u; 
            }

            s_ty = (size_t)s_y;

            (void)workload_vt_move(s_x, s_ty);
            (void)workload_puts("+=");
            for(s_count = (size_t)0u;s_count < s_grid_x;s_count++) { (void)workload_puts("="); }
            (void)workload_puts("=+");
            ++s_ty;

            (void)workload_vt_move(s_x, s_ty);
            (void)workload_puts("| ");
            s_size = s_disk->m_name_size;
            s_size = (s_size < s_grid_x) ? (s_grid_x - s_size) : ((size_t)0u);
            for(s_count = (size_t)0u;s_count < s_size;s_count++) { (void)workload_puts(" "); }
            (void)workload_puts(s_disk->m_name);
            (void)workload_vt_move(s_x + s_grid_x + ((size_t)2u), s_ty);
            (void)workload_puts(" |");
            ++s_ty;
            
            (void)workload_vt_move(s_x, s_ty);
            (void)workload_puts("| ");
            (void)workload_vt_move(s_x + s_grid_x + ((size_t)2u), s_ty);
            (void)workload_puts(" |");
            ++s_ty;
            
            (void)workload_vt_move(s_x, s_ty);
            (void)workload_puts("| ");
            (void)workload_vt_move(s_x + s_grid_x + ((size_t)2u), s_ty);
            (void)workload_puts(" |");
            ++s_ty;
            
            (void)workload_vt_move(s_x, s_ty);
            (void)workload_puts("| ");
            (void)workload_vt_move(s_x + s_grid_x + ((size_t)2u), s_ty);
            (void)workload_puts(" |");
            ++s_ty;
            
            (void)workload_vt_move(s_x, s_ty);
            (void)workload_puts("| ");
            (void)workload_vt_move(s_x + s_grid_x + ((size_t)2u), s_ty);
            (void)workload_puts(" |");
            ++s_ty;
            
	    (void)workload_vt_move(s_x, s_ty);
            (void)workload_puts("| ");
            (void)workload_vt_move(s_x + s_grid_x + ((size_t)2u), s_ty);
            (void)workload_puts(" |");
            ++s_ty;
            
	    (void)workload_vt_move(s_x, s_ty);
            (void)workload_puts("| ");
            (void)workload_vt_move(s_x + s_grid_x + ((size_t)2u), s_ty);
            (void)workload_puts(" |");
            ++s_ty;
                
	    if((s_workload->m_flags & def_workload_flag_use_fakeoff) != def_workload_flag_none) {
	        (void)workload_vt_move(s_x, s_ty);
                (void)workload_puts("| ");
                (void)workload_vt_move(s_x + s_grid_x + ((size_t)2u), s_ty);
                (void)workload_puts(" |");
                ++s_ty;
	    }
            
            (void)workload_vt_move(s_x, s_ty);
            (void)workload_puts("+=");
            for(s_count = (size_t)0u;s_count < s_grid_x;s_count++) { (void)workload_puts("="); }
            (void)workload_puts("=+");
            ++s_ty;
             
            s_x += s_grid_x + ((size_t)3u);
            if((s_x + s_grid_x + ((size_t)3u)) >= s_width) {
                s_x = s_entry_x;
                s_y += s_grid_y;
            }
        }

        (void)workload_vt_move(s_entry_x, s_height - ((size_t)1u));
        (void)workload_printf(
            "[KEY] "
            "'" workload_vt_color_yellow "q'" workload_vt_color_normal ": quit, "
            "'" workload_vt_color_yellow "r'" workload_vt_color_normal ": relocate/keep, "
            "'" workload_vt_color_yellow "o'" workload_vt_color_normal ": oneshot relocate, "
            "'" workload_vt_color_yellow "p'" workload_vt_color_normal ": pause/resume"
        );
    }

    /* update */
    if(s_entry_y >= ((size_t)4u)) {
        (void)workload_vt_move(s_entry_x, (size_t)3u);
        
        /* duration */
        (void)workload_printf("[DURATION=%llus] [RADIO=%u:%u] ",
            (workload_get_main_duration(s_workload)) / 1000ull,
            s_workload->m_cfg.m_left_ratio,
            s_workload->m_cfg.m_right_ratio
        );

        /* status */
        if(workload_get_main_relocate(s_workload) == 0) {
            (void)workload_puts("[        ] ");
        }
        else {
            (void)workload_puts(workload_vt_code("41m") "[RELOCATE]" workload_vt_color_normal " ");
        }
	
	if((s_workload->m_flags & def_workload_flag_use_fakeoff) == def_workload_flag_none) {
            (void)workload_puts("[       ] ");
	}
	else {
            (void)workload_puts(workload_vt_code("41m") "[FAKEOFF]" workload_vt_color_normal " ");
	}
        
        if(workload_get_main_pause(s_workload) == 0) {
            (void)workload_puts("[ACCESS] ");
        }
        else {
            (void)workload_puts(workload_vt_code("41m") "[PAUSE ]" workload_vt_color_normal " ");
        }

        s_time_stamp = workload_get_main_time_stamp(s_workload);
	if((s_time_stamp - s_workload->m_time_stamp_1sec) >= 1000ull) {
            s_workload->m_read_count_1sec = s_workload->m_read_count - s_workload->m_read_count_prev;
            s_workload->m_write_count_1sec = s_workload->m_write_count - s_workload->m_write_count_prev;

	    s_workload->m_read_count_prev = s_workload->m_read_count;
	    s_workload->m_write_count_prev = s_workload->m_write_count;
	    
	    s_workload->m_time_stamp_1sec = s_time_stamp;
	}

	do { /* access count */
	    unsigned long long s_read_bytes;
	    unsigned long long s_write_bytes;
	    s_read_bytes = s_workload->m_read_count_1sec * ((unsigned long long)s_workload->m_cfg.m_filesize);
	    s_write_bytes = s_workload->m_write_count_1sec * ((unsigned long long)s_workload->m_cfg.m_filesize);
            (void)workload_printf(
	        "[R:%llu(" workload_vt_color_white "%llu.%03llu" workload_vt_color_normal "KB/s)] "
	        "[W:%llu(" workload_vt_color_white "%llu.%03llu" workload_vt_color_normal "KB/s)] ",
                s_workload->m_read_count,
		s_read_bytes / 1000ull,
		s_read_bytes % 1000ull,
                s_workload->m_write_count,
		s_write_bytes / 1000ull,
		s_write_bytes % 1000ull
            );
	}while(0);
    }
    for(s_disk_index = (size_t)0u,
        s_x = s_entry_x,
        s_y = s_entry_y;
        s_disk_index < s_workload->m_disk_count;
        s_disk_index++) {
        if((s_y + s_grid_y + ((size_t)2u)) > s_height) { break; }
            
        if(s_x == s_entry_x) { s_x += (size_t)8u; }

        s_ty = s_y + ((size_t)2u);
        
        s_disk = s_workload->m_disk[s_disk_index];

        /* files */
        s_string = workload_printable_comma_string(
            (char *)(&s_string_buffer[0]),
            s_grid_x + ((size_t)1u),
            (unsigned long long)s_disk->m_assign_file_count,
            1000ull
        );
        s_size = strlen((const char *)(&s_string_buffer[0]));
        s_size = (s_size < s_grid_x) ? (s_grid_x - s_size) : ((size_t)0u);
        (void)workload_vt_move(s_x + ((size_t)2u), s_ty);
        for(s_count = (size_t)0u;s_count < s_size;s_count++) { (void)workload_puts(" "); }
        (void)workload_puts(s_string);
        ++s_ty;

        /* usage */
        s_ratio = 0u;
        if((s_workload->m_read_count + s_workload->m_write_count) > 0ull) {
            s_ratio = (unsigned int)(((s_disk->m_read_count + s_disk->m_write_count) * 100000ull) / (s_workload->m_read_count + s_workload->m_write_count));
        }
        s_string = (char *)(&s_string_buffer[0]);
        (void)sprintf(s_string, "%u.%03u%%",
            s_ratio / 1000u,
            s_ratio % 1000u
        );
        s_size = strlen((const char *)(&s_string_buffer[0]));
        s_size = (s_size < s_grid_x) ? (s_grid_x - s_size) : ((size_t)0u);
        (void)workload_vt_move(s_x + ((size_t)2u), s_ty);
        for(s_count = (size_t)0u;s_count < s_size;s_count++) { (void)workload_puts(" "); }
        (void)workload_puts(s_string);
        ++s_ty;
        
        /* ratio count */
        s_string = workload_printable_comma_string(
            (char *)(&s_string_buffer[0]),
            (s_grid_x - ((size_t)1u)) + ((size_t)1u),
            s_disk->m_read_count + s_disk->m_write_count,
            1000ull
        );
        s_size = strlen((const char *)(&s_string_buffer[0]));
        s_size = (s_size < s_grid_x) ? (s_grid_x - s_size) : ((size_t)0u);
        (void)workload_vt_move(s_x + ((size_t)2u), s_ty);
        for(s_count = (size_t)0u;s_count < s_size;s_count++) { (void)workload_puts(" "); }
        (void)workload_puts(s_string);
	++s_ty;

        /* usage */
        s_ratio = 0u;
	s_read_count = 0ull;
	s_write_count = 0ull;
        if((s_workload->m_read_count + s_workload->m_write_count) > 0ull) {
            workload_file_t *s_file;
            
            for(s_file = s_disk->m_assign_file;
                s_file != NULL;s_file = s_file->m_assign_next) {
                s_read_count += s_file->m_read_count;
                s_write_count += s_file->m_write_count;
            }
            s_ratio = (unsigned int)(((s_read_count + s_write_count) * 100000ull) / (s_workload->m_read_count + s_workload->m_write_count));
        }
        s_string = (char *)(&s_string_buffer[0]);
        (void)sprintf(s_string, "%u.%03u%%",
            s_ratio / 1000u,
            s_ratio % 1000u
        );
        s_size = strlen((const char *)(&s_string_buffer[0]));
        s_size = (s_size < s_grid_x) ? (s_grid_x - s_size) : ((size_t)0u);
        (void)workload_vt_move(s_x + ((size_t)2u), s_ty);
        for(s_count = (size_t)0u;s_count < s_size;s_count++) { (void)workload_puts(" "); }
        (void)workload_puts(s_string);
        ++s_ty;
       
        /* usage count */
        s_string = workload_printable_comma_string(
            (char *)(&s_string_buffer[0]),
            (s_grid_x - ((size_t)1u)) + ((size_t)1u),
            s_read_count + s_write_count,
            1000ull
        );
        s_size = strlen((const char *)(&s_string_buffer[0]));
        s_size = (s_size < s_grid_x) ? (s_grid_x - s_size) : ((size_t)0u);
        (void)workload_vt_move(s_x + ((size_t)2u), s_ty);
        for(s_count = (size_t)0u;s_count < s_size;s_count++) { (void)workload_puts(" "); }
        (void)workload_puts(s_string);
	++s_ty;

        /* idle time */
        s_time_stamp = workload_get_main_time_stamp(s_workload) - s_disk->m_time_stamp;
	if(s_time_stamp >= 10000000ull) {
            s_string = workload_printable_comma_string(
                (char *)(&s_string_buffer[0]),
                (s_grid_x - ((size_t)1u)) + ((size_t)1u),
                s_time_stamp / 1000ull,
                1000ull
            );
            (void)strcat(s_string, "s");
	}
	else if(s_time_stamp >= 1000000ull) {
            s_string = (char *)(&s_string_buffer[0]);
            (void)sprintf(s_string, "%llu.%01llus",
                s_time_stamp / 1000ull,
                (s_time_stamp % 1000ull) / 100ull
            );
	}
	else if(s_time_stamp >= 100000ull) {
            s_string = (char *)(&s_string_buffer[0]);
            (void)sprintf(s_string, "%llu.%02llus",
                s_time_stamp / 1000ull,
                (s_time_stamp % 1000ull) / 10ull
            );
	}
	else {
            s_string = (char *)(&s_string_buffer[0]);
            (void)sprintf(s_string, "%llu.%03llus",
                s_time_stamp / 1000ull,
                s_time_stamp % 1000ull
            );
	}
	
        s_size = strlen((const char *)(&s_string_buffer[0]));
        s_size = (s_size < s_grid_x) ? (s_grid_x - s_size) : ((size_t)0u);
        (void)workload_vt_move(s_x + ((size_t)2u), s_ty);
        for(s_count = (size_t)0u;s_count < s_size;s_count++) { (void)workload_puts(" "); }
        (void)workload_puts(s_string);
        ++s_ty;
	
	/* fakeoff status */
	if((s_workload->m_flags & def_workload_flag_use_fakeoff) != def_workload_flag_none) {
            s_string = (char *)(&s_string_buffer[0]);
	    if((s_workload->m_relocate == 0) || (s_disk->m_fakeoff_delay == (-1ll))) {
                (void)sprintf(s_string, "%s",
		    "N/A"
                );
	    }
	    else if((workload_get_main_time_stamp(s_workload) - s_disk->m_fakeoff_time_stamp) >= ((unsigned long long)s_disk->m_fakeoff_delay)) {
                (void)sprintf(s_string, "%s",
		    "FAKE"
                );
	    }
	    else {
	        s_time_stamp = workload_get_main_time_stamp(s_workload) - s_disk->m_fakeoff_time_stamp;
	        if(s_time_stamp >= 10000000ull) {
                    s_string = workload_printable_comma_string(
                        (char *)(&s_string_buffer[0]),
                        (s_grid_x - ((size_t)1u)) + ((size_t)1u),
                        s_time_stamp / 1000ull,
                        1000ull
                    );
                    (void)strcat(s_string, "s");
        	}
        	else if(s_time_stamp >= 1000000ull) {
                    s_string = (char *)(&s_string_buffer[0]);
                    (void)sprintf(s_string, "%llu.%01llus",
                        s_time_stamp / 1000ull,
                        (s_time_stamp % 1000ull) / 100ull
                    );
        	}
        	else if(s_time_stamp >= 100000ull) {
                    s_string = (char *)(&s_string_buffer[0]);
                    (void)sprintf(s_string, "%llu.%02llus",
                        s_time_stamp / 1000ull,
                        (s_time_stamp % 1000ull) / 10ull
                    );
        	}
        	else {
                    s_string = (char *)(&s_string_buffer[0]);
                    (void)sprintf(s_string, "%llu.%03llus",
                        s_time_stamp / 1000ull,
                        s_time_stamp % 1000ull
                    );
        	}
	    }
            s_size = strlen((const char *)(&s_string_buffer[0]));
            s_size = (s_size < s_grid_x) ? (s_grid_x - s_size) : ((size_t)0u);
            (void)workload_vt_move(s_x + ((size_t)2u), s_ty);
            for(s_count = (size_t)0u;s_count < s_size;s_count++) { (void)workload_puts(" "); }
            (void)workload_puts(s_string);
            ++s_ty;
	}
	
        s_x += s_grid_x + ((size_t)3u);
        if((s_x + s_grid_x + ((size_t)3u)) >= s_width) {
            s_x = s_entry_x;
            s_y += s_grid_y;
        }
    }

    /* vt sync */
    (void)workload_vt_move(1, s_height);
    (void)workload_vt_sync();
    
    pthread_mutex_unlock((pthread_mutex_t *)(&s_workload->m_reassign_lock));

    return(0);
}

#endif

/* vim: set expandtab: */
/* End of source */
