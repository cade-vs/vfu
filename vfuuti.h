/****************************************************************************
 *
 * Copyright (c) 1996-2022 Vladi Belperchinov-Shabanski "Cade" 
 * http://cade.noxrun.com/  <cade@noxrun.com> <cade@bis.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 ****************************************************************************/

#ifndef _VFUUTI_H_
#define _VFUUTI_H_

#include "vfu.h"

/*###########################################################################*/

fsize_t file_st_size( struct stat* st );

VString vfu_readlink( const char* fname );

/*###########################################################################*/

int vfu_update_shell_line( VString &a_line, VString &a_options );
int vfu_break_op(); /* return != 0 if ESC pressed, non blocking */
wchar_t vfu_ask( const wchar_t *prompt, const wchar_t *allowed ); /* blocking */
/* used before copy/move to calc estimated size */
fsize_t vfu_update_sel_size( int one );
VString& vfu_expand_mask( VString& mask );
char* time_str_compact( const time_t tim, char* buf );
void vfu_beep();

VString size_str_compact( const fsize_t siz );

char* vfu_str_comma( char* target );
VString& vfu_str_comma( VString& target );
VString vfu_str_comma( fsize_t size );

/*###########################################################################*/

void vfu_hist_add( int hist_id, const char* str );
const char* vfu_hist_get( int hist_id, int index = 0 );
char* vfu_hist_get( int hist_id, int index, char* str );
int vfu_hist_index( int hist_id, const char* value );
int vfu_hist_count( int hist_id );
void vfu_hist_remove( int hist_id, int index );
int vfu_hist_menu( int x, int y, const wchar_t* title, int hist_id );

void vfu_get_str_history( int key, VString &s, int &pos ); /* internal! */
int vfu_get_str( const char *prompt, VString& target, int hist_id, int x = -1, int y = -1 );

const char* vfu_temp();

/*###########################################################################*/

void vfu_con_out( int x, int y, const char    *s );
void vfu_con_out( int x, int y, const char    *s, int attr );
void vfu_con_out( int x, int y, const wchar_t *s );
void vfu_con_out( int x, int y, const wchar_t *s, int attr );

/*###########################################################################*/

#endif//_VFUUTI_H_
