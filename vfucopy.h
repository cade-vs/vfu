/*
 *
 * (c) Vladi Belperchinov-Shabanski "Cade" 1996-2003
 * http://soul.datamax.bg/~cade  <cade@biscom.net>  <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 * $Id: vfucopy.h,v 1.6 2003/01/26 21:48:42 cade Exp $
 *
 */

#ifndef _VFUCOPY_H_
#define _VFUCOPY_H_

#ifndef COPY_BUFFER_SIZE
#define COPY_BUFFER_SIZE  1024*1024 /* 1M */
#endif

#define CM_COPY  0 // copy
#define CM_MOVE  1 // move
#define CM_LINK  2 // symlink

#define OM_ASK      0 /* ask before overwrite */
#define OM_ALWAYS   1 /* always overwrite */
#define OM_NEVER    2 /* never overwrite */
#define OM_IF_MTIME 3 /* if newer modify time*/
#define OM_ALWAYS_IF_MTIME 4 /* always if newer modify time*/

/* copy results */
#define CR_OK           0
#define CR_SKIP         200
#define CR_ABORT        255

struct CopyInfo
{
  CopyInfo() { reset(); };
  void reset()
    {
    no_info = files_count = current_count =
    no_free_check = over_mode = abort = 0;
    files_size = current_size = 0;
    };
  int no_info;

  fsize_t files_size;
  long    files_count; /* not used */
  fsize_t current_size;
  long    current_count; /* not used */

  int no_free_check; /* if 1 -- don't check for destination free space */
  int over_mode;     /* what to do if dest exist? see OM_XXX defines */
  int abort;         /* if != 0 -- abort and return */

  VString description;
};

fsize_t device_free_space( const char *target ); /* user free space, NOT real! */

int file_is_same( const char *src, const char *dst );
int device_is_same( const char *src, const char *dst ); 
int fast_stat( const char* s, struct stat *st );
int over_if_exist( const char* src, const char *dst, 
                   CopyInfo* copy_info );

void show_copy_pos( fsize_t a_fc, /* file under copy current pos */
                    fsize_t a_fa, /* file under copy all size */
                    CopyInfo *copy_info ); /* totals info */

int vfu_copy_mode( const char* src, const char* dst );

int __vfu_file_copy( const char* src, const char* dst, CopyInfo* copy_info );
int __vfu_file_move( const char* src, const char* dst, CopyInfo* copy_info );
int __vfu_file_symlink( const char* src, const char* dst, CopyInfo* copy_info );

int __vfu_dir_copy( const char* src, const char* dst, CopyInfo* copy_info );
int __vfu_dir_move( const char* src, const char* dst, CopyInfo* copy_info );
#define __vfu_dir_symlink __vfu_file_symlink

int __vfu_link_copy( const char* src, const char* dst, CopyInfo* copy_info );
int __vfu_link_move( const char* src, const char* dst, CopyInfo* copy_info );
#define __vfu_link_symlink __vfu_file_symlink

int __vfu_dir_erase( const char* target, fsize_t* bytes_freed = NULL );
int __vfu_file_erase( const char* target, fsize_t* bytes_freed = NULL );
int __vfu_link_erase( const char* target );

void vfu_copy_files( int a_one, int a_mode );
void vfu_erase_files( int a_one );

/*
int __CopyFile( const char* src, const char* dst );
int __CopyLink( const char* src, const char* dst );
int CopyFile(const char* src, const char*dst, const char* name1, const char* name2, fsize_t fsize, int mode = CM_COPY );
int CopyDir (const char* src, const char*dst, const char* name1, const char* name2, fsize_t fsize, int mode = CM_COPY );
void CopyMoveFiles(int one, int mode = CM_COPY );
*/

#endif //_VFUCOPY_H_

