/*
 *
 * (c) Vladi Belperchinov-Shabanski "Cade" 1996-2002
 * http://soul.datamax.bg/~cade  <cade@biscom.net>  <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 * $Id: vfuarc.h,v 1.5 2003/01/19 17:32:43 cade Exp $
 *
 */

#ifndef _VFUARC_H_
#define _VFUARC_H_

#include "vfu.h"

  void vfu_read_archive_files();
  void vfu_browse_archive_file();
  void vfu_extract_files( int one );
  void vfu_user_external_archive_exec( VString &shell_line  );

#endif /* _VFUARC_H_ */

/* eof vfuarc.h */
