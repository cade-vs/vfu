/*
 *
 * (c) Vladi Belperchinov-Shabanski "Cade" 1996-2000
 * http://www.biscom.net/~cade  <cade@biscom.net>  <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 */

#ifndef _VFUDIR_H_
#define _VFUDIR_H_

#include "vfu.h"

extern VArray size_cache;

/*###########################################################################*/

int vfu_get_dir_name( const char *prompt, String &target, int should_exist = 1 );
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
void tree_draw_page( TScrollPos &scroll );
void tree_draw_pos( TScrollPos &scroll, int opos );

/*###########################################################################*/

int  tree_index( const char *s );
const char* tree_find( const char *s ); /* return full path by dirname */

/* return count of found dirnames and stores them to sc */
int tree_find( const char *s, PSZCluster *sc ); 

int size_cache_index( const char *s );
fsize_t size_cache_get( const char *s );
void size_cache_set( const char *s, fsize_t size );

/*###########################################################################*/

fsize_t vfu_dir_size( const char *s );

#endif //_VFUDRI_H_

