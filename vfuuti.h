/*
 *
 * (c) Vladi Belperchinov-Shabanski "Cade" 1996-2000
 * http://www.biscom.net/~cade  <cade@biscom.net>  <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 */

#ifndef _VFUUTI_H_
#define _VFUUTI_H_

#include "vfu.h"

/*###########################################################################*/

int vfu_update_shell_line( String &a_line, String &a_options );
int vfu_break_op(); /* return != 0 if ESC pressed, non blocking */
int vfu_ask( const char *prompt, const char *allowed ); /* blocking */
/* used before copy/move to calc estimated size */
fsize_t vfu_update_sel_size( int one ); 
String& vfu_expand_mask( String& mask );
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

void vfu_get_str_history( int key, String &s, int &pos ); /* internal! */
int vfu_get_str( const char *prompt, String& target, int hist_id, int x = -1, int y = -1 );

/*###########################################################################*/

#endif//_VFUUTI_H_
