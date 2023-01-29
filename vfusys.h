/****************************************************************************
 #
 # Copyright (c) 1996-2023 Vladi Belperchinov-Shabanski "Cade" 
 # https://cade.noxrun.com/  <cade@noxrun.com> <cade@bis.bg>
 # https://cade.noxrun.com/projects/vfu     https://github.com/cade-vs/vfu
 #
 # SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 #
 ****************************************************************************/

#ifndef _VFUSYS_H_
#define _VFUSYS_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/*
    VFU specific defines
*/

#define MODE_OFF      "----------"

#define MODE_STRING   "drwxrwxrwx"
#define MODE_MASK     "-?????????"
#define MODE_WRITE_ON "??w???????" /* allow WRITE mask */

typedef char mode_str_t[12];

/*
  these functions set/get file's attributes/mode from/to string
  with this format:
  for Linux/UNIX:  drwxrwxrwx
  for DOS:         DV----RHSA
  it is supposed that all attribs count is 10
*/

/* FIXME: these functions cannot handle symlink flag yet */
void file_get_mode_str( const mode_t tm, mode_str_t &mod_str );
int  file_get_mode_str( const char *filename, mode_str_t &mod_str );
int  file_set_mode_str( const char *filename, const mode_str_t mod_str );
int  vfu_edit_attr( mode_str_t mod_str, int allow_masking = 1 );

/* FIXME: dir_exist should check if directory really */
#define  dir_exist( d ) ( access( d, F_OK ) == 0)
#define file_exist( d ) ( access( d, F_OK ) == 0)

#endif /* _VFUSYS_H_ */

/* eof vfusys.h */
