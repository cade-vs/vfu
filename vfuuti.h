/*
 *
 * (c) Vladi Belperchinov-Shabanski "Cade" 1996-2003
 * http://soul.datamax.bg/~cade  <cade@biscom.net>  <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 * $Id: vfuuti.h,v 1.8 2005/06/05 22:00:10 cade Exp $
 *
 */

#ifndef _VFUUTI_H_
#define _VFUUTI_H_

#include "vfu.h"

/*###########################################################################*/

fsize_t file_st_size( struct stat* st );

/*###########################################################################*/

int vfu_update_shell_line( VString &a_line, VString &a_options );
int vfu_break_op(); /* return != 0 if ESC pressed, non blocking */
int vfu_ask( const char *prompt, const char *allowed ); /* blocking */
/* used before copy/move to calc estimated size */
fsize_t vfu_update_sel_size( int one ); 
VString& vfu_expand_mask( VString& mask );
char* time_str_compact( const time_t tim, char* buf );
void vfu_beep();

char* size_str_compact( const fsize_t siz, char* buf );

/*###########################################################################*/

void vfu_hist_add( int hist_id, const char* str );
const char* vfu_hist_get( int hist_id, int index = 0 );
char* vfu_hist_get( int hist_id, int index, char* str );
int vfu_hist_index( int hist_id, const char* value );
int vfu_hist_count( int hist_id );
void vfu_hist_remove( int hist_id, int index );
int vfu_hist_menu( int x, int y, const char* title, int hist_id );

void vfu_get_str_history( int key, VString &s, int &pos ); /* internal! */
int vfu_get_str( const char *prompt, VString& target, int hist_id, int x = -1, int y = -1 );

const char* vfu_temp();

/*###########################################################################*/

#endif//_VFUUTI_H_
