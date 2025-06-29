/****************************************************************************
 #
 # Copyright (c) 1996-2023 Vladi Belperchinov-Shabanski "Cade" 
 # https://cade.noxrun.com/  <cade@noxrun.com> <cade@bis.bg>
 # https://cade.noxrun.com/projects/vfu     https://github.com/cade-vs/vfu
 #
 # SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 #
 ****************************************************************************/

#include "vfu.h"
#include "vfudir.h"
#include "vfumenu.h"
#include "vfufiles.h"
#include "vfuview.h"
#include "vfusys.h"
#include "vfucopy.h"
#include "vfuuti.h"
#include <errno.h>

/****************************************************************************
**
** globals
**
****************************************************************************/

#define COPY_PROG_FIELD_WIDTH 24

const char *CM_DESC[] = { "COPY", "MOVE", "SYMLINK" };
char *copy_buff = NULL;

int ignore_copy_errors = 0; /* actually it is used for copy/move/erase */

/* clipboard ***************************************************************/

VTrie clipboard;
CopyInfo clipboard_copy_info;

/****************************************************************************
**
** utilities
**
****************************************************************************/

fsize_t device_free_space( const char *target )
{
  struct statfs stafs;
  int res = statfs( str_file_path( target ), &stafs );
  if( res == 0 ) return ((fsize_t)(stafs.f_bsize)) * stafs.f_bfree;
  return 0;
}

fsize_t device_avail_space( const char *target )
{
  struct statfs stafs;
  int res = statfs( str_file_path( target ), &stafs );
  if( res == 0 ) return ((fsize_t)(stafs.f_bsize)) * stafs.f_bavail;
  return 0;
}

/*
 *  return 0 if src and dst are actually the same file
 */
int file_is_same( const char *src, const char *dst )
{
  struct stat st1;
  struct stat st2;
  if(stat( src, &st1 )) return 1;
  if(stat( dst, &st2 )) return 1;
  return !( st1.st_dev == st2.st_dev && /* same device */
            st1.st_ino == st2.st_ino ); /* same inode */
}

/*
  return 0 if src and dst are on the same device
*/
int device_is_same( const char *src, const char *dst )
{
  char *ch;
  struct stat st1;
  struct stat st2;
  fname_t _f1;
  fname_t _f2;
  strcpy( _f1, src );
  ch = strrchr( _f1, '/' );
  if (ch == NULL) _f1[0] = 0; else ch[1] = 0;
  strcat( _f1, "." );
  strcpy( _f2, dst );
  ch = strrchr( _f2, '/' );
  if (ch == NULL) _f2[0] = 0; else ch[1] = 0;
  strcat( _f2, "." );
  if(stat( _f1, &st1 )) return 1;
  if(stat( _f2, &st2 )) return 1;
  return !( st1.st_dev == st2.st_dev );
}


/*###########################################################################*/

void show_copy_pos( fsize_t a_fc, /* file under copy, current pos */
                    fsize_t a_fa, /* file under copy, file size   */
                    long    a_et, /* elapsed time for current file copy */
                    CopyInfo *copy_info ) /* totals info */
{
  char t[128];

  fsize_t c1 = a_fc;
  fsize_t a1 = a_fa;
  fsize_t c2 = copy_info->current_size;
  fsize_t a2 = copy_info->files_size;

  long t1  = a_et;
  long t2  = copy_info->elapsed_time;
  long eta = ( ( t1 + t2 ) * a2 ) / ( c1 + c2 ) - ( t1 + t2 );

  int eta_h = eta / ( 60*60 );
  int eta_m = eta / 60;
  
  int  eta_v;
  char eta_c;
  
  if( eta_h > 0 )
    {
    eta_v = eta_h;
    eta_c = 'h';
    }
  else if( eta > 99 && eta_m > 0 )
    {
    eta_v = eta_m;
    eta_c = 'm';
    }
  else
    {
    eta_v = eta;
    eta_c = 's';
    }  
  
  int speed = -1;
  // if( t1 > 0 ) speed = ( c1 / ( 1024 * 1024 ) ) / t1; // current MiB/s
  if( t1 + t2 > 0 ) speed = ( ( c1 + c2 ) / ( 1024 * 1024 ) ) / ( t1 + t2 ); // total MiB/s

  if ( a1 < 1 ) return; // current file has zero size
  if ( a2 < 1 ) 
    {
    // no info about total files' size or size is zero, 
    // only current file progress and speed are available
    sprintf( t, "%5.1f%%  n/a @%3dM      ", (100.0*c1)/a1, speed );
    }
  else if ( c1 == a1 ) /* hack, every single 100% each is not meaningfull really */
    sprintf( t, "     %%%5.1f @%3dM/s%3d%c ", (100.0*(c1+c2))/a2, speed, eta_v, eta_c );
  else
    sprintf( t, "%5.1f%%%5.1f @%3dM/s%3d%c ", (100.0*c1)/a1, (100.0*(c1+c2))/a2, speed, eta_v, eta_c );
  vfu_con_out( con_max_x() - COPY_PROG_FIELD_WIDTH, con_max_y(), t, cSTATUS2 );
}

/*###########################################################################*/

/*
  this will return 1 if copy should proceed and 0 if should not
  if destination exists it asks for interaction or proceed by
  the last answer...
  if dest. doesn't exist -- always return 1
*/
int over_if_exist( const char* src, const char *dst, CopyInfo* copy_info )
{
  if ( access( dst, F_OK) ) return 1; /* doesn't exist -- copy... */
  if ( copy_info->over_mode == OM_NEVER  ) return 0; /* skip it */
  if ( copy_info->over_mode == OM_ALWAYS ) return 1; /* overwrite! */

  struct stat stat_src;
  struct stat stat_dst;
  stat( src, &stat_src );
  stat( dst, &stat_dst );

  if ( copy_info->over_mode == OM_ALWAYS_IF_MTIME &&
       stat_src.st_mtime > stat_dst.st_mtime ) return 1; /* newer mtime, do it! */

  int wch = 0;
  while(4)
    {
    vfu_redraw();
    vfu_redraw_status();
    VString str;
    char sttime[32];

    char s_t = (stat_src.st_mtime == stat_dst.st_mtime)?'*':' '; // same time
    char s_s = (stat_src.st_size  == stat_dst.st_size)?'*':' '; // same size

    fname_t t;

    time_str_compact( stat_src.st_mtime, sttime);
    str = stat_src.st_size;
    vfu_str_comma(str);
    str_pad( str, 15 );
    snprintf( t, sizeof(t), "SRC: %s%c %s%c %s", sttime, s_t, str.data(), s_s, src );
    say1( t );

    time_str_compact(stat_dst.st_mtime, sttime);
    str = stat_dst.st_size;
    vfu_str_comma(str);
    str_pad( str, 15 );
    snprintf( t, sizeof(t), "DST: %s%c %s%c %s", sttime, s_t, str.data(), s_s, dst );
    say2( t );

    vfu_beep();
    vfu_menu_box( L"Overwrite", L"Y Yes,N No,A Always overwrite,V Never overwrite,I If newer (MODIFY),W Always if newer (MODIFY),D View differences,  Abort (ESC)", -1 );
    wch = menu_box_info.ec;
    if( wch == L'D' )
      {
      VString diff = vfu_temp();
      VString cmd;
      cmd = shell_diff + " " + shell_escape( dst ) + " " + shell_escape( src ) + " > " + diff;
      if(system( cmd ) != 0)
        {
        say1( "Cannot execute command: " + cmd );
        say2errno();
        }
      else
        {  
        vfu_browse( diff );
        unlink( diff );
        }
      continue;
      }
    break;
    }
  say1( "" );
  say2( "" );

  switch (wch)
    {
    case L'Y' : return 1;
    case L'N' : return 0;
    case L'A' : copy_info->over_mode = OM_ALWAYS; return 1;
    case L'V' : copy_info->over_mode = OM_NEVER; return 0;
    case L'I' : return ( stat_src.st_mtime > stat_dst.st_mtime ); break;
    case L'W' : copy_info->over_mode = OM_ALWAYS_IF_MTIME; return 0;
    default  : copy_info->abort = 1; return 0;
    }
  return 1;
}

/*###########################################################################*/

int vfu_copy_mode( const char* src, const char* dst )
{
  struct stat st;
  if (stat( src, &st )) return 1; /* FIXME: or silent? */

  /* copy mode */
  mode_str_t mode_str;
  file_get_mode_str( st.st_mode, mode_str );
  file_set_mode_str( dst, mode_str );

  /* copy access/modify time */
  utimbuf utb = { 0, 0 };
  utb.actime  = st.st_atime;
  utb.modtime = st.st_mtime;
  utime( dst, &utb );

  /* copy owner/group */
  if (opt.copy_keep_mode)
    if (chown( dst, st.st_uid, st.st_gid ))
      say1( "Cannot change file mode" );

  return 0;
}

/***************************************************************************
**
** COPY/MOVE/SYMLINK
**
****************************************************************************/

/* copy/move ***************************************************************/


//  return 0 for ok

int __vfu_file_copy( const char* src, const char* dst, CopyInfo* copy_info )
{
  errno = 0; /* clear error status */

  fsize_t size = file_size( src );
  if ( size == -1 ) return 1;

  if ( ! over_if_exist( src, dst, copy_info ) )
    {
    copy_info->current_size += size; /* consider it ok */
    copy_info->skipped_count++;
    return copy_info->abort ? CR_ABORT : CR_SKIP; /* ok */
    }

  if ( file_exist( dst ) )
    { /* destination file exists */
    if ( file_is_same( src, dst ) == 0 )
      {
      copy_info->skipped_count++;
      return CR_SKIP; /* dst is src actually */
      }
    __vfu_file_erase( dst ); /* overwrite! */
    }

  /* progress report */
  VString str = dst;
  str = str_dot_reduce( str, con_max_x() - 10 );
  str = "COPY TO: " + str;
  say1( str );
  str = copy_info->description;
  str += "  Entries: ";
  str += copy_info->current_count;
  str += " of ";
  str += copy_info->files_count;
  
  str_pad( str, - con_max_x() + COPY_PROG_FIELD_WIDTH + 1 ); // :)
  vfu_con_out( 1, con_max_y(), str, cMESSAGE );


  if ( ! copy_info->no_free_check && ! copy_info->no_info )
    {
    while(4)
      {
      fsize_t dev_free = device_avail_space( dst );
      if (size <= dev_free ) break;

      vfu_beep();
      say1( VString() + "Insufficient disk space! Free: " + vfu_str_comma( dev_free ) + ", Required: " + vfu_str_comma( size ) );
      say2( dst );

      vfu_menu_box( L"Error prompt", L"C Continue anyway,P Space check,S Skip file,N No free space check,  Abort (ESC)", -1 );

      if ( menu_box_info.ec == L'C' ) break; 
      if ( menu_box_info.ec == L'P' ) continue; 
      if ( menu_box_info.ec == L'S' ) { copy_info->skipped_count++; return CR_SKIP; }
      if ( menu_box_info.ec == L'N' ) { copy_info->no_free_check = 1; break; }

      copy_info->abort = 1;
      return CR_ABORT;
      }  
    }

  ASSERT( copy_buff );
  if ( copy_buff == NULL ) return 1;

  FILE *f1 = NULL;
  FILE *f2 = NULL;

  f1 = fopen( src, "rb" );
  if (!f1) return 1;

  f2 = fopen( dst, "wb" );
  if (!f2)
    {
    fclose(f1);
    return 1;
    }

  long z = 0;
  fsize_t cp = 0; /* current position in file */

  int aborted = 0;

  
  time_t timer_start   = time(NULL);
  long   elapsed_time  = 0;
  long   elapsed_break = 0;
  
  do
    {
    time_t timer_break = time(NULL);
    if ( vfu_break_op() )
      {
      aborted = 1;
      break;
      }
    elapsed_break += time(NULL) - timer_break;
      
    z = fread( copy_buff, 1, COPY_BUFFER_SIZE, f1 );
    if (z > 0) z = fwrite( copy_buff, 1, z, f2 );
    if (z == -1)
      {
      fclose(f1);
      fclose(f2);
      unlink( dst ); /* remove dst if partially copied */
      return 1;
      }
    cp += z;
    //ASSERT( cp <= size );
    elapsed_time = time(NULL) - timer_start - elapsed_break;
    show_copy_pos( cp, size, elapsed_time, copy_info );
    }
  while ( z == COPY_BUFFER_SIZE );

  fclose(f1);
  fclose(f2);

  if ( cp < size )
    {
    unlink( dst ); /* remove dst if partially copied */
    if ( aborted ) return CR_ABORT;
    return 1;
    }
  //ASSERT( cp == size );

  if ( vfu_copy_mode( src, dst ) ) return 1;

  copy_info->current_size += size;
  copy_info->elapsed_time += elapsed_time;
  show_copy_pos( 1, 1, 0, copy_info );
  return 0;
}

/*---------------------------------------------------------------------------*/

int __vfu_file_move( const char* src, const char* dst, CopyInfo* copy_info )
{
  errno = 0; /* clear error status */

  if ( device_is_same( src, dst ) == 0 )
    { /* same device */
    if ( ! over_if_exist( src, dst, copy_info ) )
      {
      copy_info->skipped_count++;
      return copy_info->abort ? CR_ABORT : CR_SKIP; /* ok */
      }

    if ( file_exist( dst ) )
      {
      if ( file_is_same( src, dst ) == 0 )
        {
        copy_info->skipped_count++;
        return CR_SKIP; /* dst is src actually */
        }
      /* FIXME: what if dst is symlink? */
      if ( __vfu_file_erase( dst ) ) return 1;
      }
    if ( rename( src, dst ) ) return 1;
    }
  else
    { /* src and dst devices are different */
    say2( "MOVING FILE" );
    int r;
    r = __vfu_file_copy( src, dst, copy_info );
    if ( r == CR_SKIP ) return CR_SKIP;
    if ( r == CR_ABORT ) return CR_ABORT;
    if ( r ) return 1;
    r = __vfu_file_erase( src );
    if ( r && r != CR_SKIP ) return 1;
    }
  return 0;
}

/*---------------------------------------------------------------------------*/

int __vfu_dir_copy( const char* src, const char* dst, CopyInfo* copy_info )
{
  errno = 0; /* clear error status */

  VString fname_src; /* used for directory entries */
  VString fname_dst; /* used for directory entries */

  if ( vfu_break_op() )
    return CR_ABORT; /* canceled */

  if ( file_is_same( src, dst ) == 0 )
    return CR_SKIP;

  if ( make_path( dst ) )
    {
    if ( ignore_copy_errors ) return CR_ABORT; /* cancel operation */

    say1( dst );
    say2errno();
    vfu_menu_box( L"Create dir error",
                  L"C Continue anyway,I Ignore further errors,  Abort (ESC)", -1 );
    if ( menu_box_info.ec != L'C' )
      return CR_ABORT; /* cancel operation */
    if ( menu_box_info.ec != L'I' )
      {
      ignore_copy_errors = 1;
      return CR_ABORT; /* cancel operation */
      }
    }

  DIR *dir;
  dirent *de;

  dir = opendir( src );
  if (!dir) return 1; /* FIXME: report error? */

  while( (de = readdir(dir)) )
    {
    if (strcmp( de->d_name, ".") == 0) continue;
    if (strcmp( de->d_name, "..") == 0) continue;

    fname_src  = src;
    fname_src += "/";
    fname_src += de->d_name;

    fname_dst  = dst;
    fname_dst += "/";
    fname_dst += de->d_name;

    while(4)
      {
      int r = __vfu_copy( fname_src, fname_dst, copy_info );

      if ( r == CR_ABORT )
        {
        closedir(dir);
        return CR_ABORT;
        }
      if ( r && r != CR_SKIP )
        {
        if ( ignore_copy_errors ) break;

        vfu_beep();
        say1( fname_dst );
        say2errno();
        vfu_menu_box( L"Copy/Move/SymLink error",
                      L"T Try again,S Skip/continue,I Ignore further errors,  Abort (ESC)" );
        if ( menu_box_info.ec == L'T' )
          continue; /* while(4) */
        else
        if ( menu_box_info.ec == L'S' )
          {
          copy_info->skipped_count++;
          break; /* consider it ok */
          };
        if ( menu_box_info.ec == L'I' )
          {
          ignore_copy_errors = 1;
          break;
          }
        else
          {
          closedir(dir);
          return CR_ABORT;
          }
        }
      else
        break; /* copy (r) is ok -- exit error (while(4)) loop */
      } /* while(4) */
    } /* readdir() */
  closedir( dir );

  if ( vfu_copy_mode( src, dst ) ) return 1;

  return 0;
}

/*---------------------------------------------------------------------------*/

int __vfu_dir_move( const char* src, const char* dst, CopyInfo* copy_info )
{
  errno = 0; /* clear error status */

  if ( file_is_same( src, dst ) == 0 )
    return CR_SKIP;

  if ( device_is_same( src, dst ) == 0 )
    { /* same device */
    if ( rename( src, dst ) ) return 1;
    }
  else
    { /* different devices */
    if ( __vfu_dir_copy( src, dst, copy_info ) || copy_info->skipped_count > 0 )
      {
      vfu_beep();
      say1( "There were errors or files were skipped! You have to erase dir manually." );
      vfu_menu_box( L"Copy/Move error", L"C Continue operation,  Abort (ESC)" );
      if ( menu_box_info.ec != L'C' )
        return CR_ABORT;
      else
        return CR_SKIP;
      }
    /* NOTE: whatever __vfu_dir_copy() returns it is considered
       as error even if it is CR_SKIP!, i.e. directory never
       erased unless everything went ok */
    if ( __vfu_dir_erase( src ) ) return 1;
    }
  return 0;
}

/*---------------------------------------------------------------------------*/

int __vfu_link_copy( const char* src, const char* dst, CopyInfo* copy_info )
{
  errno = 0; /* clear error status */

  if ( ! over_if_exist( src, dst, copy_info ) )
    {
    copy_info->skipped_count++;
    return copy_info->abort ? CR_ABORT : CR_SKIP; /* ok */
    }

  if ( file_exist( dst ) )
    { /* destination file exists */
    if ( file_is_same( src, dst ) == 0 )
      {
      copy_info->skipped_count++;
      return CR_SKIP; /* dst is src actually */
      }
    __vfu_file_erase( dst ); /* overwrite! */
    }

  fname_t t;
  int z = readlink( src, t, sizeof(t)-1);
  if (z < 1) return 1;
  t[z] = 0;
  if (symlink( t, dst ) == -1) return 1;
  /* FIXME: should we keep src mode? does links have this? */
  return 0;
}

/*---------------------------------------------------------------------------*/

int __vfu_link_move( const char* src, const char* dst, CopyInfo* copy_info )
{
  int r;
  r = __vfu_link_copy( src, dst, copy_info );
  if ( r == CR_SKIP ) return CR_SKIP;
  if ( r ) return 1;
  r = __vfu_link_erase( src );
  if ( r && r != CR_SKIP ) return 1;
  return 0;
}

/* erase *******************************************************************/

//  return 0 for ok, CR_ABORT for cancel, else for error

int __vfu_dir_erase( const char* target, fsize_t* bytes_freed )
{
  errno = 0; /* clear error status */

  VString fname; /* used for directory entries */
  VString s;

  if ( vfu_break_op() )
    return CR_ABORT; /* canceled */

  if (!file_exist( target )) return 1;

  /* make it writeable so we can erase files in it */
  file_set_mode_str( target, MODE_WRITE_ON );

  DIR *dir;
  dirent *de;

  dir = opendir( target );
  if (!dir) return 1; /* FIXME: report error? */

  while( (de = readdir(dir)) )
    {
    if (strcmp( de->d_name, ".") == 0) continue;
    if (strcmp( de->d_name, "..") == 0) continue;

    fname = target;
    fname += "/";
    fname += de->d_name;

    while(4)
      {
      /* progress report */
      say1( fname );

      int r = __vfu_erase( fname, bytes_freed );

      if ( r == CR_ABORT )
        {
        closedir(dir);
        return CR_ABORT;
        }
      if ( r && r != CR_SKIP )
        {
        if ( ignore_copy_errors ) break;

        vfu_beep();
        say1( fname );
        say2errno();
        vfu_menu_box( L"Erase error",
                      L"T Try again,S Skip/continue,I Ignore further errors,  Abort (ESC)" );
        if ( menu_box_info.ec == 'T' )
          continue; /* while(4) */
        else
        if ( menu_box_info.ec == 'S' )
          break; /* consider it ok */
        if ( menu_box_info.ec == 'I' )
          {
          ignore_copy_errors = 1;
          break;
          }
        else
          {
          closedir(dir);
          return CR_ABORT;
          }
        }
      else
        break;
      } /* while(4) */
    } /* readdir() */
  closedir( dir );

  /* show bytes freed if required */
  if ( bytes_freed )
    {
    VString t;
    t.fi( *bytes_freed );
    vfu_str_comma( t );
    VString tt;
    if( *bytes_freed > 1024*1024 )
      {
      tt = fsize_fmt( *bytes_freed, opt.use_gib_usage ).data();
      tt = "( " + tt + " ) ";
      }
    t = "ERASE: " + t + " bytes " + tt + "freed.";
    say2(t);
    }
  say1( target );
  return ( rmdir( target ) != 0 );
}

/*---------------------------------------------------------------------------*/

int __vfu_file_erase( const char* target, fsize_t* bytes_freed  )
{
  errno = 0; /* clear error status */

  fsize_t target_size = 0;
  if ( bytes_freed )
    target_size = file_size( target );

  int r = unlink( target );

  if ( r == 0 && bytes_freed )
    *bytes_freed += target_size;
  return (r != 0);
}

/*---------------------------------------------------------------------------*/

int __vfu_link_erase( const char* target, fsize_t* bytes_freed __attribute__((unused)) )
{
  errno = 0; /* clear error status */

  return (unlink( target ) != 0);
}


/* shells, call __*_*_*() above ********************************************/

int __vfu_copy( const char* src, const char* dst, CopyInfo* copy_info )
{
  int r = 0;
  if ( file_is_link( src ) )
    r = __vfu_link_copy( src, dst, copy_info ); /* symlink */
  else
  if ( file_is_dir( src ) )
    r = __vfu_dir_copy( src, dst, copy_info ); /* directory */
  else
    r = __vfu_file_copy( src, dst, copy_info ); /* regular file */

  return r;
}

/*---------------------------------------------------------------------------*/

int __vfu_move( const char* src, const char* dst, CopyInfo* copy_info )
{
  int r = 0;
  if ( file_is_link( src ) )
    r = __vfu_link_move( src, dst, copy_info ); /* symlink */
  else
  if ( file_is_dir( src ) )
    r = __vfu_dir_move( src, dst, copy_info ); /* directory */
  else
    r = __vfu_file_move( src, dst, copy_info ); /* regular file */

  return r;
}

/*---------------------------------------------------------------------------*/

int __vfu_symlink( const char* src, const char* dst, CopyInfo* copy_info )
{
  errno = 0; /* clear error status */

  if ( ! over_if_exist( src, dst, copy_info ) )
    {
    copy_info->skipped_count++;
    return copy_info->abort ? CR_ABORT : CR_SKIP; /* ok */
    }

  if ( file_exist( dst ) )
    {
    if ( file_is_same( src, dst ) == 0 )
      {
      copy_info->skipped_count++;
      return CR_SKIP; /* dst is src actually */
      }
    /* FIXME: what if dst is symlink? */
    if ( __vfu_file_erase( dst ) ) return 1;
    }
  int r = symlink( src, dst );
  return ( r != 0 );
}

/*---------------------------------------------------------------------------*/

int __vfu_erase( const char* target, fsize_t* bytes_freed )
{
  int r = 0;

  if ( file_is_link( target ) )
    r = __vfu_link_erase( target, bytes_freed ); /* symlink */
  else
  if ( file_is_dir( target ) )
    r = __vfu_dir_erase( target, bytes_freed ); /* directory */
  else
    r = __vfu_file_erase( target, bytes_freed ); /* regular file */

  return r;
}

/* high-level interface functions ******************************************/

void __copy_calc_totals( CopyInfo &copy_info, int a_one )
{
  if ( opt.copy_calc_totals )
    {
    say1( "Calculating files size. Press ESCAPE to cancel calculation." );
    copy_info.files_size = vfu_update_sel_size( a_one );
    copy_info.files_count = a_one ? 1 : sel_count; /* not used */
    copy_info.current_size = 0;
    copy_info.current_count = 0; /* not used */
    if ( copy_info.files_size == -1 )
      copy_info.no_info = 1;
    }
  else
    copy_info.no_info = 1;
}

void __copy_calc_totals_clipboard( CopyInfo &copy_info )
{
  if ( opt.copy_calc_totals )
    {
    say1( "Calculating files size. Press ESCAPE to cancel calculation." );
    copy_info.files_size = 0;
    VArray va = clipboard.keys();
    for( int i = 0; i < va.count(); i++ )
      {
      struct stat _st;
      stat( va[i], &_st );
      int _is_link = S_ISLNK(_st.st_mode );
      int _is_dir  = S_ISDIR(_st.st_mode );
      
      if( _is_dir && ! _is_link ) 
        copy_info.files_size += vfu_dir_size( va[i], 0 );
      else if( ! _is_link )
        copy_info.files_size += _st.st_size;  
      }
    copy_info.files_count = clipboard.count();
    copy_info.current_size = 0;
    copy_info.current_count = 0; /* not used */
    if ( copy_info.files_size == -1 )
      copy_info.no_info = 1;
    }
  else
    copy_info.no_info = 1;
}

void vfu_copy_files( int a_one, int a_mode )
{
  ignore_copy_errors = 0;
  if ( files_list_count() == 0 )
    {
    say1( "No files" );
    return;
    }

  fname_t t;

  ASSERT( a_mode == CM_COPY || a_mode == CM_MOVE || a_mode == CM_LINK );

  CopyInfo copy_info;

  if ( opt.copy_calc_totals == 2 ) /* PRELIMINARY copy calc totals */
    __copy_calc_totals( copy_info, a_one );

  VString target;
  if( opt.default_copy_to_cwd )
    target = work_path;
  else
    target = opt.last_copy_path[ a_mode ];
  VString str = CM_DESC[ a_mode ];
  if ( a_one )
    str = str + " '" + files_list_get(FLI)->name_ext() + "' to:";
  else
    str += " SELECTED FILES/DIRS to:";
  if ( !vfu_get_dir_name( str, target ) ) return;
  expand_path( target, t );
  /* str_reduce_path( NULL, t ); */
  str_fix_path( t );
  str_trim_right( t, 1 );
  if ( file_exists( t ) && !file_is_dir( t ) )
    {

    vfu_beep();
    say1( "Target is file, not directory!" );
    say2( t );

    vfu_menu_box( L"Error prompt",
                  L"C Continue anyway,E Erase first,  Abort (ESC)", -1 );
    if ( menu_box_info.ec == L'E' )
      {
      unlink( t );
      menu_box_info.ec = L'C';
      }
    if ( menu_box_info.ec != L'C' ) return; /* abort */
    }
  str_fix_path( t );
  target = t;
  strcpy( opt.last_copy_path[ a_mode ], target );

  if ( opt.copy_calc_totals == 1 ) /* copy calc totals if not PRELIMINARY */
    __copy_calc_totals( copy_info, a_one );

  copy_info.no_free_check = !opt.copy_free_space_check;
  copy_info.over_mode = OM_ASK; /* 0 */
  copy_info.abort = 0;

  VString dev_free_str;
  if ( !copy_info.no_free_check && !copy_info.no_info )
    {
    while(4)
      {
      fsize_t dev_free = device_avail_space( target );
      dev_free_str = size_str_compact( dev_free );
      dev_free_str = " | TARGET FREE: " + dev_free_str;
      if ( copy_info.files_size <= dev_free ) break;
      
      vfu_beep();
      say1( VString() + "Insufficient disk space! Free: " + vfu_str_comma( dev_free ) + ", Required: " + vfu_str_comma( copy_info.files_size ) );
      say2( target );

      vfu_menu_box( L"Error prompt",
                    L"C Continue anyway,P Space check,  Abort (ESC)", -1 );
      if ( menu_box_info.ec == L'C' ) break; 
      if ( menu_box_info.ec == L'P' ) continue; 
      return; /* abort */
      }  
    } /* free space check */

  copy_info.description = "";
  copy_info.description += CM_DESC[ a_mode ];
  copy_info.description += ": ";
  sprintf( t, "%.0f", copy_info.files_size );
  vfu_str_comma( t );
  copy_info.description += t;
  copy_info.description += " bytes.";
//  copy_info.description += dev_free_str;

  ASSERT( !copy_buff );
  copy_buff = new char[1024*1024];
  ASSERT( copy_buff );
  if (copy_buff == NULL)
    {
    say1( "Copy error: cannot allocate copy buffer" );
    return;
    }

  // vfu_set_title_info( VString() + CM_DESC[ a_mode ] + " to " + target );

  int z;
  for ( z = 0; z < files_list_count(); z++ )
    {
    if ( vfu_break_op() ) break; /* cancel operation */
    TF *fi = files_list_get(z);

    if ( a_one && z != FLI ) continue; /* if one and not current -- skip */
    if ( !a_one && !fi->sel ) continue; /* if not one and not selected -- skip */

    /*
      copy logic:
      src -- is full current item path
      dst -- is target + name-ext only!
    */
    VString src  = fi->full_name();
    VString dst  = target;
           dst += fi->name_ext();

    copy_info.current_count++;
    
    int r = 0;
    if ( a_mode == CM_COPY ) r = __vfu_copy( src, dst, &copy_info ); else
    if ( a_mode == CM_MOVE ) r = __vfu_move( src, dst, &copy_info ); else
    if ( a_mode == CM_LINK ) r = __vfu_symlink( src, dst, &copy_info ); else
    ASSERT(!"Bad copy mode");

    if ( r == 0 )
      {
      if ( a_mode == CM_MOVE )
        files_list_del(z);
      }
    else if ( r != CR_SKIP && r != CR_ABORT )
      {
      say1( target );
      say2errno();
      vfu_menu_box( L"Copy/Move error", L"C Continue operation,  Abort (ESC)" );
      if ( menu_box_info.ec != L'C' )
        r = CR_ABORT;
      }
    if ( r == CR_ABORT ) break; /* cancel operation */
    } /* for files_list[] */

  if ( a_mode == CM_MOVE )
    files_list_pack();
  update_status();
  do_draw = 2;

  ASSERT( copy_buff );
  delete [] copy_buff;
  copy_buff = NULL;

  say1( copy_info.description );
  /* show bytes copied */
  str = "";
  if ( copy_info.current_size > 0 )
    { 
    /* i.e. only if there *are* some bytes copied :) */
//    str = copy_info.description;
    str += "DONE: " + vfu_str_comma( copy_info.current_size ) + " bytes.";
    str += " SKIPPED: " + vfu_str_comma( copy_info.skipped_count );
    }
  else
    {
//    str = copy_info.description;
    str += "DONE";
    }
  say2( str + " TARGET AVAIL: " + size_str_compact( device_avail_space( target ) ) + ", FREE: " + size_str_compact( device_free_space( target ) ) + "" );

  ignore_copy_errors = 0;
}

/*---------------------------------------------------------------------------*/

void vfu_erase_files( int a_one )
{
  ignore_copy_errors = 0;
  if ( files_list_count() == 0 )
    {
    say1( "No files" );
    return;
    }

  fsize_t bytes_freed = 0;
  fsize_t *bytes_freed_ptr = opt.bytes_freed ? &bytes_freed : NULL;

  VString str;
  say1( "Calculating files size to be ERASED! Press ESCAPE to cancel calculation." );
  fsize_t erase_size = vfu_update_sel_size( a_one );
  if ( erase_size != -1 )
    {
    str.fi( erase_size );
    vfu_str_comma( str );
    }
  else
    {
    str = "( NO INFO! )";
    }

  vfu_beep();

  str = "ERASE: " + str + " bytes in: ( ENTER to confirm, other key to cancel )";
  say1( str );
  if ( a_one )
    say2( files_list_get(FLI)->full_name() );
  else
    say2( "SELECTED FILES/DIRS" );

  char ch = con_getch();
  say1("");
  say2("");
  if (ch != 13) return;

  int z;
  for ( z = 0; z < files_list_count(); z++ )
    {
    if ( vfu_break_op() ) break; /* cancel operation */
    TF *fi = files_list_get(z);

    if (   a_one && z != FLI  ) continue;
    if ( ! a_one && ! fi->sel ) continue;

    VString target  = fi->full_name();

    int r = __vfu_erase( target, bytes_freed_ptr );

    if ( r == 0 )
      {
      if ( fi->is_dir() ) 
        {
        str_fix_path( target );
        size_cache_clean( target );
        }
      files_list_del( z );
      }
    else if ( r != CR_ABORT && !ignore_copy_errors )
      {
      vfu_beep();
      say1( target );
      say2errno();
      vfu_menu_box( L"Erase error",
                    L"C Continue operation,I Ignore further errors,  Abort (ESC)" );
      if ( menu_box_info.ec != L'C' && menu_box_info.ec != L'I' )
        r = CR_ABORT;
      if ( menu_box_info.ec == L'I' )
        ignore_copy_errors = 1;
      }
    if ( r == CR_ABORT ) break; /* cancel operation */
    } /* for files_list[] */

  files_list_pack();
  update_status();
  do_draw = 2;

  say1("");
  /* show bytes freed if required */
  if ( opt.bytes_freed )
    {
    str.fi( bytes_freed );
    vfu_str_comma( str );
    str = "ERASE DONE: " + str + " bytes freed.";
    say2( str );
    }
  else
    say2( "ERASE DONE" );

  ignore_copy_errors = 0;
}

/***************************************************************************
**
** CLIPBOARD
**
****************************************************************************/

void clipboard_add_del( int del )
{
  if( files_list_count() == 0 )
    {
    say( 1, cINFO, "CLIPBOARD: no files to add, %d files already in clipboard",
                     clipboard.count() );
    return;
    }
  
  VString keep = "1";

  int za = 0;
  if( sel_count == 0 )
    {
    if( del )
      {
      if( clipboard.exists( files_list_get(FLI)->full_name() ) ) za++;
      clipboard.del( files_list_get(FLI)->full_name() );
      }
    else
      {  
      clipboard[ files_list_get(FLI)->full_name() ] = keep;
      za++;
      }
    }
  else
    {  
    for ( int z = 0; z < files_list_count(); z++ )
      {
      TF *fi = files_list_get(z);
      if ( !fi->sel ) continue;
      if( del )
        {
        if( clipboard.exists( fi->full_name() ) ) za++;
        clipboard.del( fi->full_name() );
        }
      else
        {
        clipboard[ fi->full_name() ] = keep;
        za++;
        }  
      }
    }  

  __copy_calc_totals_clipboard( clipboard_copy_info );

  clipboard_copy_info.no_free_check = !opt.copy_free_space_check;
  clipboard_copy_info.over_mode = OM_ASK; /* 0 */
  clipboard_copy_info.abort = 0;

  VString fsize = clipboard_copy_info.files_size;
  vfu_str_comma( fsize );
  say( 1, cINFO, "CLIPBOARD: %d file(s) %s. %d in the clipboard now = %s bytes.",
                 za, del ? "removed" : "added", clipboard.count(), fsize.data() );
  say2( "" );
}

void clipboard_paste( int mode )
{
  VArray va = clipboard.keys();

  ASSERT(    mode == CM_COPY
          || mode == CM_MOVE
          || mode == CM_LINK );

  ASSERT( !copy_buff );
  copy_buff = new char[1024*1024];
  ASSERT( copy_buff );
  if (copy_buff == NULL)
    {
    say1( "Copy error: cannot allocate copy buffer" );
    return;
    }

  va.reset();
  const char* ps;
  while( (ps = va.next()) )
    {
    VString dst = work_path + str_file_name_ext( ps );

    int r;
    if ( mode == CM_LINK )
      r = __vfu_symlink( ps, dst, &clipboard_copy_info );
    else if ( mode == CM_MOVE )
      r = __vfu_move( ps, dst, &clipboard_copy_info );
    else // if ( mode == CM_COPY )
      r = __vfu_copy( ps, dst, &clipboard_copy_info );
    if ( r == 0 ) clipboard_copy_info.ok_count++;
    if ( r != 0 && r != CR_SKIP && r != CR_ABORT )
      {
      vfu_beep();
      say1( dst + r );
      say2errno();
      vfu_menu_box( L"Copy/Move/Symlink error", L"C Continue operation,  Abort (ESC)" );
      if ( menu_box_info.ec != L'C' ) r = CR_ABORT;
      }
    if ( r == CR_ABORT ) break; // cancel operation
    }

  ASSERT( copy_buff );
  delete [] copy_buff;
  copy_buff = NULL;

  vfu_rescan_files( 0 );
  say( 1, cINFO, "CLIPBOARD: %s: %d of %d files processed ok",
                 CM_DESC[ mode ],
                 clipboard_copy_info.ok_count,
                 clipboard_copy_info.files_count );

  if ( mode == CM_MOVE ) clipboard_clear();
}

void clipboard_clear()
{
  if( clipboard_copy_info.files_count )
    say( 2, cINFO, "CLIPBOARD: %d files removed", clipboard_copy_info.files_count );
  else
    say( 2, cINFO, "CLIPBOARD: empty now" );

  clipboard.undef();
  clipboard_copy_info.reset();
}

void clipboard_view()
{
  VArray ca = clipboard.keys();
  int z;
  for( z = 0; z < ca.count(); z++ )
    {
    WString ws;
    ws.set_failsafe( ca[z] );
    mb.push( ws );
    }
  if( mb.count() == 0 )
    {
    say2( "CLIPBOARD: empty" );
    return;
    }
  vfu_menu_box( 5, 5, L"File Clipboard Content" );
}

void clipboard_menu( wchar_t act )
{
  if ( act == 0 )
    {
    mb.undef();
    mb.push( L"A clear & Add files to the clipboard" );
    mb.push( L"+ Add files to the clipboard" );
    mb.push( L"- Remove files from the clipboard" );
    mb.push( L"P Copy files here" );
    mb.push( L"O Move files here" );
    mb.push( L"L Symlink files here" );
    mb.push( L"E Clear clipboard" );
    mb.push( L"V View clipboard" );

    VString fcnt = clipboard_copy_info.files_count;
    vfu_str_comma( fcnt );
    VString fsize = clipboard_copy_info.files_size;
    vfu_str_comma( fsize );
    mb.push( L"---  " + WString( fcnt ) + L" files, " + WString( fsize ) + L" bytes" );

    if ( vfu_menu_box( 5, 5, L"File Clipboard " + WString( fcnt ) + " files, " + WString( fsize ) + " bytes" ) == -1 ) return;
    act = menu_box_info.ec;
    }
  act = towupper( act );
  switch( act )
    {
    case L'A': clipboard_clear(); clipboard_add_del(); break;
    case L'+': clipboard_add_del(); break;
    case L'-': clipboard_add_del( 1 ); break;
    case L'P': clipboard_paste( CM_COPY ); break;
    case L'O': clipboard_paste( CM_MOVE ); break;
    case L'L': clipboard_paste( CM_LINK ); break;
    case L'E': clipboard_clear(); break;
    case L'V': clipboard_view(); break;
    }
}

/*---------------------------------------------------------------------------*/

