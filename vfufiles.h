/****************************************************************************
 *
 * (c) Vladi Belperchinov-Shabanski "Cade" 1996-2018
 * http://cade.datamax.bg/  <cade@biscom.net> <cade@bis.bg> <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 ****************************************************************************/

#ifndef _VFUFILES_H_
#define _VFUFILES_H_

#include "vfu.h"

int  files_list_count();
TF*  files_list_get( int pos );
void files_list_set( int pos, TF* fp );
void files_list_add( TF* fp );
void files_list_trim(); // remove last TF* item
void files_list_del( int pos );
void files_list_pack();
void files_list_clear();

/*###########################################################################*/

const char* file_type_str( mode_t mode, int is_link );

/*###########################################################################*/

void vfu_rescan_files( int a_recursive = 0 );

void vfu_read_files( int a_recursive = 0 );
int vfu_add_file( const char* fname, const struct stat *st, int is_link );

void vfu_read_archive_files( int a_recursive );
void vfu_read_local_files( int a_recursive );
void vfu_read_external_files();
void vfu_read_pszlist_files();

int vfu_fmask_match( const char* fname );

/*###########################################################################*/

void vfu_file_entry_move();

/*###########################################################################*/

int namenumcmp( const char* s1, const char* s2 );

int ficmp(int fn1, TF *f2);
void __vfu_sort(int l, int r);
void vfu_sort_files();
void vfu_arrange_files();


/*###########################################################################*/

#endif //_VFUFILES_H_

