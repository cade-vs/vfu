/*
 *
 * (c) Vladi Belperchinov-Shabanski "Cade" 1996-2002
 * http://soul.datamax.bg/~cade  <cade@biscom.net>  <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 * $Id: vfufiles.cpp,v 1.10 2003/01/19 17:32:43 cade Exp $
 *
 */

#include "vfu.h"
#include "vfufiles.h"
#include "vfuopt.h"
#include "vfuview.h"
#include "vfumenu.h"
#include "vfudir.h"

/*###########################################################################*/

static char __file_stat_type_buf[3];
const char* file_type_str( mode_t mode, int is_link )
{
  strcpy(__file_stat_type_buf, "--");
  if (S_ISDIR(mode) && is_link)
                      strcpy(__file_stat_type_buf, "<>"); else
  if (S_ISBLK(mode) ) strcpy(__file_stat_type_buf, "=="); else
  if (S_ISCHR(mode) ) strcpy(__file_stat_type_buf, "++"); else
  if (S_ISFIFO(mode)) strcpy(__file_stat_type_buf, "()"); else
  if (S_ISSOCK(mode)) strcpy(__file_stat_type_buf, "##"); else
  if (is_link       ) strcpy(__file_stat_type_buf, "->"); else
  if (S_ISDIR (mode)) strcpy(__file_stat_type_buf, "[]"); else
  if ((mode & S_IXOTH)||(mode & S_IXGRP)||(mode & S_IXUSR))
                      strcpy(__file_stat_type_buf, "**"); else
  ;
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

  /* save selection, remember which files are selected */
  VArray savea;
  if ( opt.keep_selection && sel_count > 0 )
    {
    for ( z = 0; z < files_count ; z++)
      if ( files_list[z]->sel )
        savea.push( files_list[z]->name() );
    }

  vfu_read_files( a_recursive );

  /* restore selection */
  if ( opt.keep_selection && savea.count() > 0 )
    {
    for ( z = 0; z < files_count ; z++ )
      for ( int i = 0; i < savea.count(); i++ )
        if ( strcmp( files_list[z]->name(), savea[i] ) == 0 )
          {
          savea[i] = "";
          files_list[z]->sel = 1;
          break;
          }
    update_status();
    }

  FLI = old_fli;
  FLP = old_flp;
  vfu_nav_update_pos();
};

/*---------------------------------------------------------------------------*/

void vfu_read_files( int a_recursive )
{
  say1( "Rescanning files..." );

  int z;

  /* clear files list -- delete all found entries */
  for ( z = 0; z < MAX_FILES ; z++) 
    if (files_list[z]) 
      { 
      delete files_list[z]; 
      files_list[z] = NULL; 
      }

  /* vfu_add_file() will need this */
  files_count = 0;

  /* FIXME: perhaps we could check work_mode here? ... anyway will ASSERT it */
  if ( archive_name != "" )
    {
    ASSERT( work_mode == WM_ARCHIVE );
    vfu_read_archive_files( a_recursive );
    } else
  if ( external_panelizer != "" )  
    {
    ASSERT( work_mode == WM_NORMAL );
    vfu_read_external_files();
    } else
  if ( list_panelizer.count()  )
    {
    ASSERT( work_mode == WM_NORMAL );
    vfu_read_pszlist_files();
    }
  else
    {
    ASSERT( work_mode == WM_NORMAL );
    vfu_read_local_files( a_recursive );  
    }

  /* update scroll parameters */
  file_list_index.min = 0;
  file_list_index.max = files_count - 1;
  file_list_index.page = 0;
  file_list_index.pagesize = con_max_y() - 7;

  update_status();
  vfu_nav_update_pos();
  vfu_sort_files();
  vfu_drop_all_views();
  FGO(0); /* this ignores the sort keep list position */


  say1( "" );
  say2( "" );
  do_draw = 2;
};

/*---------------------------------------------------------------------------*/

int vfu_add_file( const char* fname, const struct stat *st, int is_link )
{
  if ( files_count == MAX_FILES ) return 1;
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

  if ( !S_ISDIR( st->st_mode ) ) /* mask is not allowed for dirs */
    if ( vfu_fmask_match( ne ) ) return 0; /* doesn't match the mask */
  TF *fi = new TF( fname, st, is_link );
  files_list[files_count] = fi;
  files_count++;

  /* get dir sizes for directories, not symlinks */
  if ( work_mode == WM_NORMAL && fi->is_dir() && !fi->is_link() )
    fi->set_size( size_cache_get( fi->full_name( 1 ) ) );
  /* get dir sizes for directories, symlinks */
  if ( work_mode == WM_NORMAL && fi->is_dir() && fi->is_link() )
    {
    char t[MAX_PATH];
    expand_path( fi->full_name( 1 ), t );
    str_fix_path( t );
    fi->set_size( size_cache_get( t ) );
    }

  /* show progress ... */
  if ( files_count % 123 == 0 )
    {
    sprintf( ne, "Rescanning files... (%5d)  ", files_count);
    say1( ne );
    }
  return 0;
};

/*---------------------------------------------------------------------------*/

int __vfu_ftw_add( const char* origin, const char* fname, 
                   const struct stat *st, int is_link, int flag )
{
  if ( vfu_break_op() ) return 1;
  if ( flag == FTWALK_DX ) return 0; /* exit directory */

  VString str = fname;
  str_trim_left( str, str_len( origin ) );
  
  return vfu_add_file( str, st, file_is_link( fname ) );
}

void vfu_read_local_files( int a_recursive )
{
  ftwalk( ".", __vfu_ftw_add, a_recursive ? -1 : 1 );
  
  if ( opt.auto_mount && files_count == 1 && 
       ( FNMATCH( files_list[0]->name(), "automount" ) == 0 ||
         FNMATCH( files_list[0]->name(), ".automount" ) == 0 ) )
   {
   VString tmp_file_name;
   tmp_file_name += tmp_path;
   tmp_file_name += "vfu_automount_error.";
   tmp_file_name += user_id_str;
   
   VString str = work_path;
   chdir( "/" );
   str = "mount " + str + " 2> " + tmp_file_name;
   say1( "AutoMount point detected, executing:" );
   say2( str );
   int err;
   if ( (err = system( str )) == 0)
     {
     //---------------
     delete files_list[0];
     files_list[0] = NULL;
     sel_count = 0;
     sel_size = 0;
     files_size = 0;
     files_count = 0;
     //---------------
     chdir( work_path );
     ftwalk( ".", __vfu_ftw_add, a_recursive ? -1 : 1 );
     }
   else
     {
     char t[128];
     FILE *f = fopen( tmp_file_name, "r" );
     t[0] = 0;
     fgets( t, 100, f );
     fclose(f);
     str_tr( t, "\n\r", "  " );
     say1( "AutoMount failed! ( press ESC ) reason:" );
     say2( t );
     con_beep();
     con_getch();
     }
   unlink( tmp_file_name );  
   }
};

/*---------------------------------------------------------------------------*/

void vfu_read_external_files()
{
  /* FIXME: this is not completely correct: lines read are in most
     cases far less in length that MAX_PATH which is about 2-4K so
     in general case this will work fine... */
  char tmp[MAX_PATH]; 
  char tmp1[MAX_PATH]; 

  if ( external_panelizer == "" ) return;
  say1( "Rescanning files...(external panelizer)" );
  FILE *f = popen( external_panelizer, "r" );
  while( fgets( tmp, MAX_PATH-1, f ) )
    {
    str_cut( tmp, " \t\n\r" );

    while( str_word( tmp, " \t:;", tmp1 ) )
      {
      if (access( tmp1, F_OK )) continue;

      struct stat st;
      stat( tmp1, &st );
      
      say2( tmp1 );
      
      if ( vfu_add_file( tmp1, &st, file_is_link( tmp1 ) ) ) 
        {
        pclose(f);
        external_panelizer = ""; /* reset -- there's no reload on this */
        return;
        }
      }
    }
  pclose( f );
  external_panelizer = ""; /* reset -- there's no reload on this */
};

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

void vfu_pack_files_list()
{
  int pos  = 0;
  int next = 0;

  while( pos < files_count )
    {
    if ( files_list[pos] == NULL )
      {
      next = pos + 1;
      while ( next < files_count && files_list[next] == NULL ) next++;
      if ( next < files_count && files_list[next] != NULL )
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
  files_count = 0;
  while ( files_count < MAX_FILES && files_list[files_count] ) files_count++;

  /* update scroll parameters */
  file_list_index.max = files_count - 1;
  file_list_index.pagesize = con_max_y() - 7;

  update_status();
  vfu_nav_update_pos();
  do_draw = 2;
}

/*###########################################################################*/

/* this compares Name20 and Name3 and returns second as smaller :) (or so) */
int namenumcmp( const char* s1, const char* s2 )
{
  VRegexp re1( "^(.*)([0123456789]+)(\\.(.*))?$" );
  VRegexp re2( "^(.*)([0123456789]+)(\\.(.*))?$" );
  if ( re1.m(s1) && re2.m(s2) )
    {
    VString ss1;
    VString ss2;
    sprintf( ss1, "%020d", atoi(re1[2]) );
    sprintf( ss2, "%020d", atoi(re1[2]) );
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
if ( f1->is_dir() && !f2->is_dir()) return -1;
if (!f1->is_dir() &&  f2->is_dir()) return 1;

z = 0;
if (opt.sort_order == 'U') return 0;
switch (opt.sort_order)
 {
 case 'N' : z = pathcmp(f1->name(), f2->name());
            break;

 case 'M' : z = namenumcmp(f1->name(), f2->name());
            break;

 case 'E' : z = pathcmp(f1->ext(), f2->ext());
            break;

 case 'S' : z = (f2->size()  < f1->size()) - (f2->size()  > f1->size());
            break;

 case 'T' : z = f1->st()->st_mtime - f2->st()->st_mtime;
            break;

 case 'H' : z = f1->st()->st_ctime - f2->st()->st_ctime;
            break;

 case 'C' : z = f1->st()->st_atime - f2->st()->st_atime;
            break;

 case 'A' : z = strcmp( f1->mode_str(), f2->mode_str() );
            break;

 case 'O' : z = (f2->st()->st_uid  > f1->st()->st_uid)  - 
                (f2->st()->st_uid  < f1->st()->st_uid);
            if ( z == 0 )
            z = (f2->st()->st_gid  > f1->st()->st_gid)  - 
                (f2->st()->st_gid  < f1->st()->st_gid);
            break;

 case 'G' : z = (f2->st()->st_gid  > f1->st()->st_gid)  - 
                (f2->st()->st_gid  < f1->st()->st_gid);
            if ( z == 0 )
            z = (f2->st()->st_uid  > f1->st()->st_uid)  - 
                (f2->st()->st_uid  < f1->st()->st_uid);
            break;

 case 'Y' : z = strcmp( f1->type_str(), f2->type_str() );
            break;
 default  : ASSERT( !"Non valid sort order (opt.sort_order)" ); break;
 }

if (z == 0) z = pathcmp(f1->name(), f2->name());

ASSERT( opt.sort_direction == 'A' || opt.sort_direction == 'D' );
if (z)
  {
  z = (z > 0) - (z < 0);
  if (opt.sort_direction == 'D') return -z;
  }
return z;
}

/*---------------------------------------------------------------------------*/

void __vfu_sort(int l, int r)
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
    while (ficmp(j,midf) == 1) j--;
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
  if (!files_count) return;
  VString str = files_list[FLI]->name();
  __vfu_sort( 0, files_count - 1 );
  do_draw = 1;
  if ( str != "" )
    {
    int z = 0;
    for (z = 0; z < files_count; z++)
      if ( str == files_list[z]->name() )
        {
        FGO(z);
        break;
        }
    };
}

/*---------------------------------------------------------------------------*/

void vfu_arrange_files()
{
  int _ord;
  int _rev;
  mb.undef();
  mb.push( "N Name" );
  mb.push( "M Name### (RTFM)" );
  mb.push( "E Extension" );
  mb.push( "S Size" );
  mb.push( "T Modify Time" );
  mb.push( "H Change Time" );
  mb.push( "C Access Time" );
  mb.push( "A Attr/mode" );
  mb.push( "O Owner" );
  mb.push( "G Group" );
  mb.push( "Y Type (TP)" );
  mb.push( "U Unsorted" );
  mb.push( "---" );
  mb.push( "R Randomize" );
  mb.push( "V Move Entry" );
  mb.push( "---" );
  mb.push( "D Modify Time (compat)" );
  if ( vfu_menu_box( 50, 5, "Arrange" ) == -1 ) return;
  _ord = menu_box_info.ec;
  if ( _ord == 'D' ) _ord = 'T';

  if (_ord == 'V' )
    {
    vfu_file_entry_move();
    return;
    }

  if (_ord == 'R')
    {
    /* Fisher-Yates shuffle */
    int i = files_count - 1;
    while( i >= 0 )
      {
      int j = rand() % ( i + 1 );
      TF *tmp;
      tmp = files_list[i];
      files_list[i] = files_list[j];
      files_list[j] = tmp;
      i--;
      }
    do_draw = 2;  
    return;
    }
  
  mb.undef();
  mb.push( "A Ascending");
  mb.push( "D Descending" );
  if ( vfu_menu_box( 50, 5, "Order" ) == -1 ) return;
  _rev = menu_box_info.ec;

  opt.sort_order = _ord;
  opt.sort_direction = _rev;

  say1("Sorting...");
  vfu_sort_files();
  say1("");
}

/*###########################################################################*/

void vfu_file_entry_move()
{
  char t[128];
  sprintf( t, "MOVE/REORDER File entry: %s", files_list[FLI]->name() );
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
};

/*###########################################################################*/

/* eof vfufiles.cpp */
