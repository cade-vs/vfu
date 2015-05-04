/****************************************************************************
 *
 * (c) Vladi Belperchinov-Shabanski "Cade" 1996-2015
 * http://cade.datamax.bg/  <cade@biscom.net> <cade@bis.bg> <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 ****************************************************************************/

#ifndef _VFUCOPY_H_
#define _VFUCOPY_H_

#ifndef COPY_BUFFER_SIZE
#define COPY_BUFFER_SIZE  1024*1024 /* 1M */
#endif

/* copy modes **************************************************************/

#define CM_COPY  0 // copy
#define CM_MOVE  1 // move
#define CM_LINK  2 // symlink

/* clipboard modes *********************************************************/

#define CLIPBOARD_COPY      1
#define CLIPBOARD_MOVE      2
#define CLIPBOARD_SYMLINK   3

/* overwrite modes *********************************************************/

#define OM_ASK      0 /* ask before overwrite */
#define OM_ALWAYS   1 /* always overwrite */
#define OM_NEVER    2 /* never overwrite */
#define OM_IF_MTIME 3 /* if newer modify time*/
#define OM_ALWAYS_IF_MTIME 4 /* always if newer modify time*/

/* copy results ************************************************************/

#define CR_OK           0
#define CR_SKIP         200
#define CR_ABORT        255

/* run-time copy info structure ********************************************/

struct CopyInfo
{
  CopyInfo() { reset(); };
  void reset()
    {
    no_info = files_count = current_count = ok_count = skipped_count =
    no_free_check = over_mode = abort = 0;
    files_size = current_size = 0;
    };
  int no_info;

  fsize_t files_size;
  long    files_count; /* not used */
  fsize_t current_size;
  long    current_count; /* not used */

  long    ok_count; /* files copied ok */
  long    skipped_count; /* files skipped */

  int no_free_check; /* if 1 -- don't check for destination free space */
  int over_mode;     /* what to do if dest exist? see OM_XXX defines */
  int abort;         /* if != 0 -- abort and return */

  VString description;
};

/***************************************************************************
**
** utilities
**
****************************************************************************/

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

/***************************************************************************
**
** COPY/MOVE/SYMLINK
**
****************************************************************************/

/* copy/move ***************************************************************/

int __vfu_file_copy( const char* src, const char* dst, CopyInfo* copy_info );
int __vfu_file_move( const char* src, const char* dst, CopyInfo* copy_info );

int __vfu_dir_copy( const char* src, const char* dst, CopyInfo* copy_info );
int __vfu_dir_move( const char* src, const char* dst, CopyInfo* copy_info );

int __vfu_link_copy( const char* src, const char* dst, CopyInfo* copy_info );
int __vfu_link_move( const char* src, const char* dst, CopyInfo* copy_info );

/* erase *******************************************************************/

int __vfu_dir_erase( const char* target, fsize_t* bytes_freed = NULL );
int __vfu_file_erase( const char* target, fsize_t* bytes_freed = NULL );
int __vfu_link_erase( const char* target, fsize_t* bytes_freed = NULL );

/* shells, call __*_*_*() above ********************************************/

int __vfu_copy( const char* src, const char* dst, CopyInfo* copy_info );
int __vfu_move( const char* src, const char* dst, CopyInfo* copy_info );
int __vfu_symlink( const char* src, const char* dst, CopyInfo* copy_info );
int __vfu_erase( const char* target, fsize_t* bytes_freed = NULL );

/* high-level interface functions ******************************************/

void vfu_copy_files( int a_one, int a_mode );
void vfu_erase_files( int a_one );

/***************************************************************************
**
** CLIPBOARD
**
****************************************************************************/

void clipboard_add();
void clipboard_paste( int mode );
void clipboard_clear();
void clipboard_view();
void clipboard_menu( int act );

#endif //_VFUCOPY_H_

