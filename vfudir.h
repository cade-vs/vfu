/****************************************************************************
 *
 * Copyright (c) 1996-2020 Vladi Belperchinov-Shabanski "Cade" 
 * http://cade.datamax.bg/  <cade@biscom.net> <cade@bis.bg> <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 ****************************************************************************/

#ifndef _VFUDIR_H_
#define _VFUDIR_H_

#include "vfu.h"

extern VArray size_cache;

/*###########################################################################*/

int vfu_get_dir_name( const char *prompt, VString &target, int should_exist = 1, int type = 'D' );
void vfu_chdir( const char *a_new_dir );
void vfu_chdir_history();

/*###########################################################################*/

void tree_view();
void tree_load();
void tree_save();
void tree_drop();
void tree_rebuild();
void tree_fix();

void tree_draw_item( int page, int index, int hilite = 0 );
void tree_draw_page( ScrollPos &scroll );
void tree_draw_pos( ScrollPos &scroll, int opos );

/*###########################################################################*/

int  tree_index( const char *s );
const char* tree_find( const char *s ); /* return full path by dirname */

/* return count of found dirnames and stores them to sc */
int tree_find( const char *s, VArray *va );

void size_cache_load();
void size_cache_save();
VString size_cache_compose_key( const char *s, fsize_t size );
int size_cache_index( const char *s );
fsize_t size_cache_get( const char *s );
void size_cache_set( const char *s, fsize_t size, int sort = 1 );
void size_cache_append( const char *s, fsize_t size );
void size_cache_clean( const char *s );
void size_cache_sort();

/*###########################################################################*/

#define DIR_SIZE_NORMAL         0
#define DIR_SIZE_FOLLOWSYMLINKS 2
#define DIR_SIZE_SAMEDEVONLY    4

fsize_t vfu_dir_size( const char *s, int sort = 1, int mode = DIR_SIZE_NORMAL );

#endif //_VFUDRI_H_

