/****************************************************************************
 *
 * Copyright (c) 1996-2022 Vladi Belperchinov-Shabanski "Cade" 
 * http://cade.noxrun.com/  <cade@noxrun.com> <cade@bis.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 ****************************************************************************/

#include <stdio.h>

#include "vfufiles.h"
#include "vfuopt.h"
#include "vfuview.h"
#include "vfumenu.h"
#include "vfudir.h"


/*###########################################################################*/

#define FILES_LIST_BUCKET_SIZE (32*1024)

TF**   files_list       = NULL;
int    files_list_cnt   = 0;
int    files_list_size  = 0;

/* index in the files list */
ScrollPos file_list_index;

void __files_list_resize( int new_size )
{

  while( new_size < files_list_cnt )
    {
    files_list_cnt--;
    delete files_list[ files_list_cnt ];
    files_list[ files_list_cnt ] = NULL;
    }
  int new_files_list_size = ( int( new_size / FILES_LIST_BUCKET_SIZE ) + 1 ) * FILES_LIST_BUCKET_SIZE;

  if( new_files_list_size == files_list_size ) return;

      
  TF** new_files_list = new TF*[ new_files_list_size ];
  memset( new_files_list,          0, sizeof(TF*) * ( new_files_list_size ) );
  memcpy( new_files_list, files_list, sizeof(TF*) * ( files_list_cnt      ) );
  delete [] files_list;

  files_list      = new_files_list;
  files_list_size = new_files_list_size;
  
  file_list_index.set_min_max( 0, files_list_cnt - 1 );
}

int  files_list_count()
{
  return files_list_cnt;
}

int  files_list_is_empty( int pos )
{
  ASSERT( pos >= 0 && pos < files_list_cnt );
  return files_list[pos] == NULL ? 1 : 0;
}

TF* files_list_get( int pos )
{
  ASSERT( pos >= 0 && pos < files_list_cnt );
  ASSERT( files_list[pos] ); // cannot get empty TF*
  return files_list[pos];
}

void files_list_set( int pos, TF* fp )
{
  ASSERT( pos >= 0 && pos < files_list_cnt );
  files_list[pos] = fp;
}

void files_list_add( TF* fp )
{
  __files_list_resize( files_list_cnt + 1 );
  files_list_cnt++;
  files_list_set( files_list_cnt - 1, fp );
}

void files_list_trim()
{
  if( files_list_cnt <= 0 ) return;
  __files_list_resize( files_list_cnt - 1 );
}

void files_list_del( int pos )
{
  ASSERT( pos >= 0 && pos < files_list_cnt );
  ASSERT( files_list[pos] );
  delete files_list[pos];
  files_list[pos] = NULL;
}

void files_list_clear()
{
  __files_list_resize( 0 );
}

void files_list_pack()
{
  int pos  = 0;
  int next = 0;

  while( pos < files_list_cnt )
    {
    if ( files_list[pos] == NULL )
      {
      next = pos + 1;
      while ( next < files_list_cnt && files_list[next] == NULL ) next++;
      if ( next < files_list_cnt && files_list[next] != NULL )
        {
        files_list[pos] = files_list[next];
        files_list[next] = NULL;
        }
      else
        break;
      }
    else
      pos++;
    }
  files_list_cnt = 0;
  while ( files_list_cnt < files_list_size && files_list[files_list_cnt] ) files_list_cnt++;

  /* update scroll parameters */
  file_list_index.set_min_max( 0, files_list_cnt - 1 );
  file_list_index.set_pagesize(  con_max_y() - 7 );

  update_status();
  vfu_nav_update_pos();
  do_draw = 2;
}

/*###########################################################################*/

static char __file_stat_type_buf[3];
const char* file_type_str( mode_t mode, int is_link )
{
  strcpy(__file_stat_type_buf, "--");
  if (S_ISDIR(mode) && is_link)
                      strcpy(__file_stat_type_buf, "<>"); else // box, but not exact
  if (S_ISBLK(mode) ) strcpy(__file_stat_type_buf, "=="); else // block, stacked
  if (S_ISCHR(mode) ) strcpy(__file_stat_type_buf, "++"); else // like dots, separates
  if (S_ISFIFO(mode)) strcpy(__file_stat_type_buf, "()"); else // () pipe mimic
  if (S_ISSOCK(mode)) strcpy(__file_stat_type_buf, "@@"); else // internet
  if (is_link       ) strcpy(__file_stat_type_buf, "->"); else // points, link
  if (S_ISDIR (mode)) strcpy(__file_stat_type_buf, "[]"); else // box
  if ((mode & S_IXOTH)||(mode & S_IXGRP)||(mode & S_IXUSR))
                      strcpy(__file_stat_type_buf, "**"); else // * marks executables
  {};
  return __file_stat_type_buf;
}

/*###########################################################################*/

/*
  actually this function is called only when 'R' key is pressed
  it calls vfu_read_files() and keeps selection and tag mark position

  update: now it is called and from vfu_shell when %r
*/

void vfu_rescan_files( int a_recursive )
{
  int z;
  int old_fli = FLI;
  int old_flp = FLP;

  VString keep = "1";

  /* save selection, remember which files are selected */
  VTrie savea;
  int savea_count = 0;
  if ( opt.keep_selection && sel_count > 0 )
    {
    for ( z = 0; z < files_list_cnt ; z++)
      if ( files_list_get(z)->sel )
        {
        savea[ files_list_get(z)->name() ] = keep;
        savea_count++;
        }
    }

  vfu_read_files( a_recursive );

  /* restore selection */
  if ( opt.keep_selection && savea_count > 0 )
    {
    for ( z = 0; z < files_list_count() ; z++ )
      if ( savea.exists( files_list_get(z)->name() ) )
        files_list_get(z)->sel = 1;
    update_status();
    }

  file_list_index.set_page( old_flp );
  file_list_index.set_pos( old_fli );
  vfu_nav_update_pos();


}

/*---------------------------------------------------------------------------*/

void vfu_read_files( int a_recursive )
{
  say1( "Rescanning files... press ESC to interrupt" );

  /* clear files list -- delete all found entries */
  files_list_clear();

  if ( archive_name != "" )
    {
    ASSERT( work_mode == WM_ARCHIVE );
    vfu_read_archive_files( a_recursive );
    } else
  if ( external_panelizer != "" )
    {
    ASSERT( work_mode == WM_NORMAL );
    vfu_read_external_files();
    opt.sort_order = L'U';
    } else
  if ( list_panelizer.count()  )
    {
    ASSERT( work_mode == WM_NORMAL );
    vfu_read_pszlist_files();
    opt.sort_order = L'U';
    }
  else
    {
    ASSERT( work_mode == WM_NORMAL );
    vfu_read_local_files( a_recursive );
    }

  /* update scroll parameters */
  file_list_index.set_min_max( 0, files_list_cnt - 1 );
  file_list_index.set_pagesize( con_max_y() - 7 );
  file_list_index.set_pagestep( OPT_SCROLL_PAGESTEP(opt.scroll_pagestep) );

  update_status();
  vfu_nav_update_pos();
  vfu_sort_files();
  vfu_drop_all_views();
  FGO(0); /* this ignores the sort keep list position */


  say1( "" );
  say2( "" );
  do_draw = 2;
}

/*---------------------------------------------------------------------------*/

int vfu_add_file( const char* fname, const struct stat *st, int is_link )
{

  VString ne = str_file_name_ext( fname );

  if ( ne == "."  || ne == ".." ) return 0;

  /* now try to hide `system/special' files */
  if ( !opt.show_hidden_files )
    {
    #ifdef _TARGET_GO32_
      mode_str_t mode_str;
      file_get_mode_str( st->st_mode, mode_str );
      if ( mode_str[7] == 'H' || mode_str[8] == 'S' ) return 0;
    #else
      if ( ne[0] == '.' ) return 0;
    #endif
    }

  int is_dir = S_ISDIR( st->st_mode );
  if ( ! is_dir ) /* mask is not allowed for dirs */
    if ( vfu_fmask_match( ne ) ) return 0; /* doesn't match the mask */
  TF *fi = new TF( fname, st, is_link );

  files_list_add( fi );

  /* get dir sizes for directories */
  if ( work_mode == WM_NORMAL && is_dir )
    {
    if ( is_link )
      {
      /* symlinks */
      fname_t t;
      expand_path( fi->full_name( 1 ), t );
      str_fix_path( t );
      fi->set_size( size_cache_get( t ) );
      }
    else
      {
      /* not symlinks */
      fi->set_size( size_cache_get( fi->full_name( 1 ) ) );
      }
    }

  /* show progress ... */
  if ( files_list_cnt % ( files_list_cnt > 1024 ? 373 : 73 ) == 0 )
    {
    VString files_list_cnt_str = files_list_cnt;
    str_comma( files_list_cnt_str );
    sprintf( ne, "Rescanning files... [%s] press ESC to interrupt", files_list_cnt_str.data() );
    say1( ne );
    }

  return 0;
}

/*---------------------------------------------------------------------------*/

int __vfu_ftw_add( const char* origin, const char* fname,
                   const struct stat *st, int is_link, int flag )
{
  if ( vfu_break_op() ) return 1;
  if ( flag == FTWALK_DX ) return 0; /* exit directory */

  VString str = fname;
  str_trim_left( str, str_len( origin ) );

  return vfu_add_file( str, st, is_link );
}

void vfu_read_local_files( int a_recursive )
{
  ftwalk( ".", __vfu_ftw_add, a_recursive ? -1 : 1 );
}

/*---------------------------------------------------------------------------*/

void vfu_read_external_files()
{
  fname_t fn_line;

  if ( external_panelizer == "" ) return;
  say1( "Rescanning files...(external panelizer)" );
  FILE *f = popen( external_panelizer, "r" );
  while( fgets( fn_line, MAX_PATH - 1, f ) )
    {
    str_cut( fn_line, " \t\n\r" );
    if ( access( fn_line, F_OK ) ) continue;

    struct stat st;
    stat( fn_line, &st );

    say2( fn_line );

    vfu_add_file( fn_line, &st, file_is_link( fn_line ) );
    }
  pclose( f );
  external_panelizer = ""; /* reset -- there's no reload on this */
}

/*---------------------------------------------------------------------------*/

void vfu_read_pszlist_files()
{
  int z;
  for ( z = 0; z < list_panelizer.count(); z++ )
    {
    const char* pc = list_panelizer[z];
    struct stat st;
    stat( pc, &st );
    vfu_add_file( pc, &st, file_is_link( pc )  );
    }
  list_panelizer.undef(); /* reset -- there's no reload on this */
}

/*---------------------------------------------------------------------------*/

int vfu_fmask_match( const char* fname )
{
  int z;
  for(z = 0; z < files_mask_array.count(); z++)
    if ( FNMATCH(files_mask_array[z],fname) == 0) return 0;
  return 1;
}

/*###########################################################################*/

/* this compares Name20 and Name3 and returns second as smaller :) (or so) */
int namenumcmp( const char* s1, const char* s2 )
{
  VRegexp re1( "^(.*?)([0123456789]+)(\\.(.*))?$" );
  VRegexp re2( "^(.*?)([0123456789]+)(\\.(.*))?$" );
  if ( re1.m(s1) && re2.m(s2) )
    {
    VString ss1;
    VString ss2;
    sprintf( ss1, "%020d", atoi(re1[2]) );
    sprintf( ss2, "%020d", atoi(re2[2]) );
    ss1 = re1[1] + ss1 + re1[3];
    ss2 = re2[1] + ss2 + re2[3];
    return pathcmp( ss1, ss2 );
    }
  else
    {
    return pathcmp( s1, s2 );
    }
}

/*---------------------------------------------------------------------------*/

int ficmp(int nf1, TF *f2)
{
TF *f1 = files_list[nf1];

int z = 0;

/* keep dirs on top */
if( opt.sort_top_dirs )
  {
  if ( f1->is_dir() && !f2->is_dir()) return -1;
  if (!f1->is_dir() &&  f2->is_dir()) return  1;
  }

if (opt.sort_order == L'U') return 0;
switch (opt.sort_order)
 {
 case L'N' : z = pathcmp(f1->name(), f2->name());
             break;

 case L'M' : z = namenumcmp(f1->name(), f2->name());
             break;

 case L'E' : z = pathcmp(f1->ext(), f2->ext());
             break;

 case L'S' : z = (f2->size()  < f1->size()) - (f2->size()  > f1->size());
             break;

 case L'T' : z = f1->st()->st_mtime - f2->st()->st_mtime;
             break;

 case L'H' : z = f1->st()->st_ctime - f2->st()->st_ctime;
             break;

 case L'C' : z = f1->st()->st_atime - f2->st()->st_atime;
             break;

 case L'A' : z = strcmp( f1->mode_str(), f2->mode_str() );
             break;

 case L'O' : z =   (f2->st()->st_uid  > f1->st()->st_uid)  -
                   (f2->st()->st_uid  < f1->st()->st_uid);
             if ( z == 0 )
               z = (f2->st()->st_gid  > f1->st()->st_gid)  -
                   (f2->st()->st_gid  < f1->st()->st_gid);
             break;

 case L'G' : z =   (f2->st()->st_gid  > f1->st()->st_gid)  -
                   (f2->st()->st_gid  < f1->st()->st_gid);
             if ( z == 0 )
               z = (f2->st()->st_uid  > f1->st()->st_uid)  -
                   (f2->st()->st_uid  < f1->st()->st_uid);
             break;

 case L'Y' : z = strcmp( f1->type_str(), f2->type_str() );
             break;
 default  : ASSERT( ! "Non valid sort order (opt.sort_order)" ); break;
 }

if ( z == 0 ) z = pathcmp( f1->name(), f2->name() );

ASSERT( opt.sort_direction == L'A' || opt.sort_direction == L'D' );
if (z)
  {
  z = (z > 0) - (z < 0);
  if (opt.sort_direction == L'D') return -z;
  }
return z;
}

/*---------------------------------------------------------------------------*/

void __vfu_sort( int l, int r )
{
  int i;
  int j;
  int mid;
  TF *fi;
  TF *midf;
  i = l;
  j = r;
  mid = ((l+r) / 2);
  midf = files_list[mid];

  do
    {
    while (ficmp(i,midf) == -1) i++;
    while (ficmp(j,midf) ==  1) j--;
    if (i <= j)
      {
      fi = files_list[i];
      files_list[i] = files_list[j];
      files_list[j] = fi;
      i++;
      j--;
      }
    }
  while (i <= j);
  if (l < j) __vfu_sort(l, j);
  if (i < r) __vfu_sort(i, r);
}

/*---------------------------------------------------------------------------*/

void vfu_sort_files()
{
  if ( ! files_list_cnt ) return;
  if ( opt.sort_order == L'U' ) return;
  VString str = files_list[FLI]->name();

  VString ss = "Sorting... [";
  str_add_ch( ss, opt.sort_order );
  str_add_ch( ss, opt.sort_direction );
  ss += "]";
  say1( ss );
  __vfu_sort( 0, files_list_cnt - 1 );

  do_draw = 1;
  if ( str != "" )
    {
    int z = 0;
    for (z = 0; z < files_list_cnt; z++)
      if ( str == files_list[z]->name() )
        {
        FGO(z);
        break;
        }
    }
}

/*---------------------------------------------------------------------------*/

void vfu_arrange_files()
{
  int _ord;
  int _rev;
  mb.undef();
  mb.push( L"N Name" );
  mb.push( L"M Name### (RTFM)" );
  mb.push( L"E Extension" );
  mb.push( L"S Size" );
  mb.push( L"T Modify Time" );
  mb.push( L"H Change Time" );
  mb.push( L"C Access Time" );
  mb.push( L"D Attr/mode" );
  mb.push( L"O Owner" );
  mb.push( L"G Group" );
  mb.push( L"Y Type (TP)" );
  mb.push( L"U Unsorted" );
  mb.push( L"---" );
  mb.push( L"R Randomize" );
  mb.push( L"V Move Entry" );
  mb.push( L"---" );
  mb.push( L"A Quick swap NAME/CHANGE" );
  if ( vfu_menu_box( 50, 5, L"Arrange" ) == -1 ) return;
  _ord = menu_box_info.ec;

  if ( _ord == L'A' )
    {
    opt.sort_direction = L'A';
    opt.sort_order = opt.sort_order == L'N' ? L'H' : L'N';
    vfu_sort_files();
    say1( VString( "File list is now arranged by " ) + ( opt.sort_order == 'N' ? "NAME (ASCENDING)" : "CHANGE TIME (ASCENDING)" ) );
    return;
    }

  if (_ord == L'V' )
    {
    vfu_file_entry_move();
    return;
    }

  if (_ord == L'R')
    {
    /* Fisher-Yates shuffle */
    int i = files_list_count() - 1;
    while( i >= 0 )
      {
      int j = rand() % ( i + 1 );
      TF *tmp = files_list_get( i );
      files_list_set( i, files_list_get(j) );
      files_list_set( j, tmp );
      i--;
      }
    do_draw = 2;
    return;
    }

  opt.sort_order = _ord;
  
  if( opt.sort_order == L'U' ) 
    {
    say1( "Next directory rescan will be unsorted." );
    return;
    }

  mb.undef();
  mb.push( L"A Ascending");
  mb.push( L"D Descending" );
  if ( vfu_menu_box( 50, 5, L"Order" ) == -1 ) return;
  _rev = menu_box_info.ec;

  opt.sort_direction = _rev;

  vfu_sort_files();
  say1("");
}

/*###########################################################################*/

void vfu_file_entry_move()
{
  VString t;
  t = t + "MOVE/REORDER File entry: " + files_list[FLI]->name();
  say1( t );
  say2( "Use Up/Down Arrows to reorder, ESC,ENTER when done." );
  int key = 0;
  while( key != 13 && key != 27 ) // enter or esc
    {
    int old = FLI;
    switch(key)
      {
      case KEY_UP    : vfu_nav_up(); break;
      case KEY_DOWN  : vfu_nav_down(); break;
      }

    if ( old != FLI )
      {
      TF* fi = files_list[old];
      files_list[old] = files_list[FLI];
      files_list[FLI] = fi;
      vfu_redraw();
      }
    key = con_getch();
    }
  say1( " " );
  say2( " " );
}

/*###########################################################################*/

/* eof vfufiles.cpp */
