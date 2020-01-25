/****************************************************************************
 *
 * (c) Vladi Belperchinov-Shabanski "Cade" 1996-2020
 * http://cade.datamax.bg/  <cade@biscom.net> <cade@bis.bg> <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 ****************************************************************************/

#ifndef _VFUARC_H_
#define _VFUARC_H_

#include "vfu.h"

  void vfu_read_archive_files();
  void vfu_browse_archive_file();
  void vfu_extract_files( int one );
  void vfu_user_external_archive_exec( VString &shell_line  );

#endif /* _VFUARC_H_ */

/* eof vfuarc.h */
