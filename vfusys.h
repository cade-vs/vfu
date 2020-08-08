/****************************************************************************
 *
 * Copyright (c) 1996-2020 Vladi Belperchinov-Shabanski "Cade" 
 * http://cade.datamax.bg/  <cade@biscom.net> <cade@bis.bg> <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 ****************************************************************************/

#ifndef _VFUSYS_H_
#define _VFUSYS_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/*
  following defines are taken from coreutils-5.2.1
  Copyright (C) 1989, 1991-2004 Free Software Foundation, Inc.
  to address the file size problem when file is larger than 2GB
*/

#ifndef CHAR_BIT
# define CHAR_BIT 8
#endif
#define TYPE_SIGNED(t) (! ((t) 0 < (t) -1))
#define TYPE_MINIMUM(t) ((t) (TYPE_SIGNED (t) ? ~ (t) 0 << (sizeof (t) * CHAR_BIT - 1) : (t) 0))
#define TYPE_MAXIMUM(t) ((t) (~ (t) 0 - TYPE_MINIMUM (t)))

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

#ifdef _TARGET_GO32_
int file_get_sfn( const char *in, char *out );
int file_get_lfn( const char *in, char *out );
#endif

#endif /* _VFUSYS_H_ */

/* eof vfusys.h */
