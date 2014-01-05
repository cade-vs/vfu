/****************************************************************************
 *
 * (c) Vladi Belperchinov-Shabanski "Cade" 1996-2014
 * http://cade.datamax.bg/  <cade@biscom.net> <cade@bis.bg> <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 ****************************************************************************/

#include "vfu.h"
#include "vfudir.h"
#include "vfumenu.h"
#include "vfufiles.h"
#include "vfuview.h"
#include "vfusys.h"
#include "vfucopy.h"
#include <errno.h>

/****************************************************************************
**
** globals
**
****************************************************************************/

const char *CM_DESC[] = { "COPY", "MOVE", "LINK" };
char *copy_buff = NULL;

int ignore_copy_errors = 0; /* actually it is used for copy/move/erase */

/* clipboard ***************************************************************/

const char *CB_DESC[] = { "COPY", "MOVE", "SYMLINK" };

VTrie Clipboard;
CopyInfo clipboard_copy_info;

/****************************************************************************
**
** utilities
**
****************************************************************************/

fsize_t device_free_space( const char *target ) /* TODO: get bavail/bfree depending on current user */
{
  struct statfs stafs;
  int res = statfs( str_file_path( target ), &stafs );
  if( res == 0 ) return ((fsize_t)(stafs.f_bsize)) * stafs.f_bfree;
  return 0;
}

/*
 *  return 0 if src and dst are actually the same file
 */
int file_is_same( const char *src, const char *dst )
{
  #ifdef _TARGET_GO32_
    char _f1[MAX_PATH];
    char _f2[MAX_PATH];
    _fixpath( src, _f1 );
    _fixpath( dst, _f2 );
    ASSERT( _f1[1] == ':' && _f2[1] == ':' );
    return (strcasecmp( _f1, _f2 ) != 0);
  #else
    struct stat st1;
    struct stat st2;
    if(stat( src, &st1 )) return 1;
    if(stat( dst, &st2 )) return 1;
    return !( st1.st_dev == st2.st_dev && /* same device */
              st1.st_ino == st2.st_ino ); /* same inode */
  #endif
}

/*
  return 0 if src and dst are on the same device
*/
int device_is_same( const char *src, const char *dst )
{
  #ifdef _TARGET_GO32_
    char _f1[MAX_PATH];
    char _f2[MAX_PATH];
    _fixpath( src, _f1 );
    _fixpath( dst, _f2 );
    ASSERT( _f1[1] == ':' && _f2[1] == ':' );
    return ( _f1[0] != _f2[0] );
  #else
    char *ch;
    struct stat st1;
    struct stat st2;
    char _f1[MAX_PATH];
    char _f2[MAX_PATH];
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
  #endif
}

#ifdef _TARGET_GO32_
  int fast_stat( const char* s, struct stat *st )
  {
      /*
        NOTE: vfu does not use this info, so don't simulate it
        under DOS/WinXX -- otherwise it is too slow
      */
      _djstat_flags =  _STAT_INODE       /* don't simulate inode info */
                      |_STAT_EXEC_EXT    /* don't try recognize exe's */
                      |_STAT_EXEC_MAGIC  /* don't try recognize exe's */
                      |_STAT_DIRSIZE     /* don't get dir sizes */
                      |_STAT_ROOT_TIME;  /* don't get root time */
      int r = stat( s, st );
      _djstat_flags = 0;
      return r;
  }
#else
  #define fast_stat stat
#endif


/*###########################################################################*/

void show_copy_pos( fsize_t a_fc, /* file under copy current pos */
                    fsize_t a_fa, /* file under copy all size */
                    CopyInfo *copy_info ) /* totals info */
{
  char t[16];

  fsize_t c1 = a_fc;
  fsize_t a1 = a_fa;
  fsize_t c2 = copy_info->current_size;
  fsize_t a2 = copy_info->files_size;

  ASSERT( a1 >= 0 && a2 >= 0 );

  if ( a1 < 1 ) a1 = 1;
  if ( a2 < 1 ) a2 = 1;
  if ( c1 == a1 ) /* hack, every single 100% each is not meaningfull really */
    sprintf( t, "     %%%5.1f", (100.0*(c1+c2))/a2);
  else
    sprintf( t, "%5.1f%%%5.1f", (100.0*c1)/a1, (100.0*(c1+c2))/a2);
  con_out( con_max_x() - 12, con_max_y(), t, cSTATUS2 );
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
  fast_stat( src, &stat_src );
  fast_stat( dst, &stat_dst );

  if ( copy_info->over_mode == OM_ALWAYS_IF_MTIME &&
       stat_src.st_mtime > stat_dst.st_mtime ) return 1; /* newer mtime, do it! */

  int ch = 0;
  while(4)
    {
    vfu_redraw();
    vfu_redraw_status();
    VString str;
    char sttime[32];

    char s_t = (stat_src.st_mtime == stat_dst.st_mtime)?'*':' '; // same time
    char s_s = (stat_src.st_size  == stat_dst.st_size)?'*':' '; // same size

    char t[MAX_PATH];

    time_str_compact( stat_src.st_mtime, sttime);
    str = file_st_size( &stat_src );
    str_comma(str);
    sprintf(t, "SRC: %s%c %11s%c %s", sttime, s_t, str.data(), s_s, src );
    say1(t);

    time_str_compact(stat_dst.st_mtime, sttime);
    str = file_st_size( &stat_dst );
    str_comma(str);
    sprintf(t, "DST: %s%c %11s%c %s", sttime, s_t, str.data(), s_s, dst );
    say2(t);

    vfu_beep();
    vfu_menu_box( "Overwrite", "Y Yes,N No,A Always overwrite,V Never overwrite,I If newer (MODIFY),W Always if newer (MODIFY),D View differences,  Abort (ESC)", -1 );
    ch = menu_box_info.ec;
    if( ch == 'D' )
      {
      VString diff = vfu_temp();
      VString cmd;
      cmd = shell_diff + " '" + dst + "' '" + src + "' > " + diff;
      system( cmd );
      vfu_browse( diff );
      unlink( diff );
      continue;
      }
    break;
    }
  say1( "" );
  say2( "" );

  switch (ch)
    {
    case 'Y' : return 1;
    case 'N' : return 0;
    case 'A' : copy_info->over_mode = OM_ALWAYS; return 1;
    case 'V' : copy_info->over_mode = OM_NEVER; return 0;
    case 'I' : return ( stat_src.st_mtime > stat_dst.st_mtime ); break;
    case 'W' : copy_info->over_mode = OM_ALWAYS_IF_MTIME; return 0;
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
    chown( dst, st.st_uid, st.st_gid );

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
  con_out( 1, con_max_y(), copy_info->description, cMESSAGE );


  if ( !copy_info->no_free_check && !copy_info->no_info )
    {
    fsize_t dev_free = device_free_space( dst );
    if (size > dev_free )
      {
      char t[128];
      vfu_beep();
      sprintf(t, "Insufficient disk space! Free: %.0f, Required: %.0f",
                  dev_free, size );
      say1( t );
      say2( dst );

      vfu_menu_box( "Error prompt", "C Continue anyway,S Skip file,N No free space check,  Abort (ESC)", -1 );
      switch (menu_box_info.ec)
        {
        case 'C' : break;

        case 'S' : copy_info->skipped_count++;
                   return CR_SKIP;
                   break; /* skip it */

        case 'N' : copy_info->no_free_check = 1;
                   break;

        default  : copy_info->abort = 1;
                   return CR_ABORT;
                   break; /* abort! */
        }
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

  do
    {
    if ( vfu_break_op() )
      {
      aborted = 1;
      break;
      }
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
    ASSERT( cp <= size );
    show_copy_pos( cp, size, copy_info );
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
  ASSERT( cp == size );

  if ( vfu_copy_mode( src, dst ) ) return 1;

  copy_info->current_size += size;
  show_copy_pos( 1, 1, copy_info );
  return 0;
}

/*---------------------------------------------------------------------------*/

int __vfu_file_move( const char* src, const char* dst, CopyInfo* copy_info )
{
  errno = 0; /* clear error status */

  if ( device_is_same( src, dst ) == 0 )
    { /* same device */
    if (!over_if_exist( src, dst, copy_info ))
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
    vfu_menu_box( "Create dir error",
                  "C Continue anyway,I Ignore further errors,  Abort (ESC)", -1 );
    if ( menu_box_info.ec != 'C' )
      return CR_ABORT; /* cancel operation */
    if ( menu_box_info.ec != 'I' )
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
        vfu_menu_box( "Copy/Move/SymLink error",
                      "T Try again,S Skip/continue,I Ignore further errors,  Abort (ESC)" );
        if ( menu_box_info.ec == 'T' )
          continue; /* while(4) */
        else
        if ( menu_box_info.ec == 'S' )
          {
          copy_info->skipped_count++;
          break; /* consider it ok */
          };
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
      vfu_menu_box( "Copy/Move error", "C Continue operation,  Abort (ESC)" );
      if ( menu_box_info.ec != 'C' )
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
  #ifdef _TARGET_GO32_
    ASSERT(!"Symlinks are not supported for this platform!");
    return 0;
  #else
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

    char t[MAX_PATH];
    int z = readlink( src, t, sizeof(t)-1);
    if (z < 1) return 1;
    t[z] = 0;
    if (symlink( t, dst ) == -1) return 1;
    /* FIXME: should we keep src mode? does links have this? */
    return 0;
  #endif /* _TARGET_UNIX_ */
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
        vfu_menu_box( "Erase error",
                      "T Try again,S Skip/continue,I Ignore further errors,  Abort (ESC)" );
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
    str_comma( t );
    t = "ERASE: " + t + " bytes freed.";
    say2(t);
    }
  say1( target );
  return ( rmdir( target ) != 0 );
}

/*---------------------------------------------------------------------------*/

int __vfu_file_erase( const char* target, fsize_t* bytes_freed  )
{
  errno = 0; /* clear error status */

  #ifdef _TARGET_GO32_
  /* under dos write access rules and delete protection
     so we have to remove it first */
  file_set_mode_str( target, MODE_WRITE_ON );
  #endif
  fsize_t target_size = 0;
  if ( bytes_freed )
    target_size = file_size( target );

  int r = unlink( target );

  if ( r == 0 && bytes_freed )
    *bytes_freed += target_size;
  return (r != 0);
}

/*---------------------------------------------------------------------------*/

int __vfu_link_erase( const char* target, fsize_t* bytes_freed )
{
  errno = 0; /* clear error status */

  #ifdef _TARGET_GO32_
    return 0; /* always ok under dos */
  #else
    return (unlink( target ) != 0);
  #endif
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

  if (!over_if_exist( src, dst, copy_info ))
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

void vfu_copy_files( int a_one, int a_mode )
{
  ignore_copy_errors = 0;
  if ( files_count == 0 )
    {
    say1( "No files" );
    return;
    }

  char t[MAX_PATH];

  ASSERT( a_mode == CM_COPY || a_mode == CM_MOVE || a_mode == CM_LINK );

  #ifdef _TARGET_GO32_
  if ( a_mode == CM_LINK )
    {
    say1( "LINK: SymLinks are NOT supported on this platform!" );
    return;
    }
  #endif

  CopyInfo copy_info;

  if ( opt.copy_calc_totals == 2 ) /* PRELIMINARY copy calc totals */
    __copy_calc_totals( copy_info, a_one );

  VString target;
  if( opt.default_copy_to_cwd )
    target = work_path;
  else
    target = opt.last_copy_path[ a_mode ];
  const char* cm_mode_str[] = { "COPY", "MOVE", "LINK" };
  VString str = cm_mode_str[ a_mode ];
  if ( a_one )
    str = str + " `" + files_list[FLI]->name_ext() + "' to:";
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

    vfu_menu_box( "Error prompt",
                  "C Continue anyway,E Erase first,  Abort (ESC)", -1 );
    if ( menu_box_info.ec == 'E' )
      {
      unlink( t );
      menu_box_info.ec = 'C';
      }
    if ( menu_box_info.ec != 'C' ) return; /* abort */
    }
  str_fix_path( t );
  target = t;
  strcpy( opt.last_copy_path[ a_mode ], target );

  if ( opt.copy_calc_totals == 1 ) /* copy calc totals if not PRELIMINARY */
    __copy_calc_totals( copy_info, a_one );

  copy_info.no_free_check = !opt.copy_free_space_check;
  copy_info.over_mode = OM_ASK; /* 0 */
  copy_info.abort = 0;

  if ( !copy_info.no_free_check && !copy_info.no_info )
    {
    fsize_t dev_free = device_free_space( target );
    if (copy_info.files_size > dev_free )
      {
      vfu_beep();
      sprintf(t, "Insufficient disk space! Free: %.0f, Required: %.0f",
                 dev_free, copy_info.files_size );
      say1( t );
      say2( target );

      vfu_menu_box( "Error prompt",
                    "C Continue anyway,  Abort (ESC)", -1 );
      if ( menu_box_info.ec != 'C' ) return; /* abort */
      }
    } /* free space check */

  copy_info.description = "FILE OPERATION: ";
  copy_info.description += cm_mode_str[ a_mode ];
  copy_info.description += ": ";
  sprintf( t, "%.0f", copy_info.files_size );
  str_comma( t );
  copy_info.description += t;
  copy_info.description += " bytes.";

  ASSERT( !copy_buff );
  copy_buff = new char[1024*1024];
  ASSERT( copy_buff );
  if (copy_buff == NULL)
    {
    say1( "Copy error: cannot allocate copy buffer" );
    return;
    }

  int z;
  for ( z = 0; z < files_count; z++ )
    {
    if ( vfu_break_op() ) break; /* cancel operation */
    TF *fi = files_list[z];

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

    int r = 0;
    if ( a_mode == CM_COPY ) r = __vfu_copy( src, dst, &copy_info ); else
    if ( a_mode == CM_MOVE ) r = __vfu_move( src, dst, &copy_info ); else
    if ( a_mode == CM_LINK ) r = __vfu_symlink( src, dst, &copy_info ); else
    ASSERT(!"Bad copy mode");

    if ( r == 0 )
      {
      if ( a_mode == CM_MOVE )
        {
        delete fi;
        files_list[z] = NULL;
        }
      }
    else if ( r != CR_SKIP && r != CR_ABORT )
      {
      say1( target );
      say2errno();
      vfu_menu_box( "Copy/Move error", "C Continue operation,  Abort (ESC)" );
      if ( menu_box_info.ec != 'C' )
        r = CR_ABORT;
      }
    if ( r == CR_ABORT ) break; /* cancel operation */
    } /* for files_list[] */

  if ( a_mode == CM_MOVE )
    vfu_pack_files_list();
  update_status();
  do_draw = 2;

  ASSERT( copy_buff );
  delete [] copy_buff;
  copy_buff = NULL;

  say1( "" );
  /* show bytes copied */
  if ( copy_info.current_size > 0 )
    { /* i.e. only if there *are* some bytes copied :) */
    str.fi( copy_info.current_size );
    str_comma( str );
    str = copy_info.description + " DONE: " + str + " bytes copied.";
    }
  else
    {
    str = copy_info.description;
    str += " DONE";
    }
  say2( str );

  ignore_copy_errors = 0;
}

/*---------------------------------------------------------------------------*/

void vfu_erase_files( int a_one )
{
  ignore_copy_errors = 0;
  if ( files_count == 0 )
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
    str_comma( str );
    }
  else
    {
    str = "( NO INFO! )";
    }

  vfu_beep();

  str = "ERASE: " + str + " bytes in: ( ENTER to confirm, other key to cancel )";
  say1( str );
  if ( a_one )
    say2( files_list[FLI]->full_name() );
  else
    say2( "SELECTED FILES/DIRS" );

  char ch = con_getch();
  say1("");
  say2("");
  if (ch != 13) return;

  int z;
  for ( z = 0; z < files_count; z++ )
    {
    if ( vfu_break_op() ) break; /* cancel operation */
    TF *fi = files_list[z];

    if ( a_one && z != FLI ) continue;
    if ( !a_one && !fi->sel ) continue;

    VString target  = fi->full_name();

    int r = __vfu_erase( target, bytes_freed_ptr );

    if ( r == 0 )
      { delete fi; files_list[z] = NULL; }
    else if ( r != CR_ABORT && !ignore_copy_errors )
      {
      vfu_beep();
      say1( target );
      say2errno();
      vfu_menu_box( "Erase error",
                    "C Continue operation,I Ignore further errors,  Abort (ESC)" );
      if ( menu_box_info.ec != 'C' && menu_box_info.ec != 'I' )
        r = CR_ABORT;
      if ( menu_box_info.ec == 'I' )
        ignore_copy_errors = 1;
      }
    if ( r == CR_ABORT ) break; /* cancel operation */
    } /* for files_list[] */

  vfu_pack_files_list();
  update_status();
  do_draw = 2;

  say1("");
  /* show bytes freed if required */
  if ( opt.bytes_freed )
    {
    str.fi( bytes_freed );
    str_comma( str );
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

void clipboard_add()
{
  if( sel_count == 0 )
    {
    say( 1, cINFO, "CLIPBOARD: no files selected, %d files already in clipboard",
                     clipboard_copy_info.files_count );
    return;
    }

  Clipboard.undef();
  clipboard_copy_info.reset();

  VString keep = "1";

  int z;
  for ( z = 0; z < files_count; z++ )
    {
    TF *fi = files_list[z];
    if ( !fi->sel ) continue;
    Clipboard[ fi->full_name() ] = keep;
    }

  __copy_calc_totals( clipboard_copy_info, 0 );

  clipboard_copy_info.no_free_check = !opt.copy_free_space_check;
  clipboard_copy_info.over_mode = OM_ASK; /* 0 */
  clipboard_copy_info.abort = 0;

  say( 1, cINFO, "CLIPBOARD: %d files added.",
                 clipboard_copy_info.files_count );
  say2( "" );
}

void clipboard_paste( int mode )
{
  VArray va = Clipboard.keys();

  ASSERT(    mode == CLIPBOARD_COPY
          || mode == CLIPBOARD_MOVE
          || mode == CLIPBOARD_SYMLINK );

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
    if ( mode == CLIPBOARD_SYMLINK )
      r = __vfu_symlink( ps, dst, &clipboard_copy_info );
    else if ( mode == CLIPBOARD_MOVE )
      r = __vfu_move( ps, dst, &clipboard_copy_info );
    else // if ( mode == CLIPBOARD_COPY )
      r = __vfu_copy( ps, dst, &clipboard_copy_info );
    if ( r == 0 ) clipboard_copy_info.ok_count++;
    if ( r != 0 && r != CR_SKIP && r != CR_ABORT )
      {
      vfu_beep();
      say1( dst + r );
      say2errno();
      vfu_menu_box( "Copy/Move/Symlink error", "C Continue operation,  Abort (ESC)" );
      if ( menu_box_info.ec != 'C' ) r = CR_ABORT;
      }
    if ( r == CR_ABORT ) break; // cancel operation
    }

  ASSERT( copy_buff );
  delete [] copy_buff;
  copy_buff = NULL;

  vfu_rescan_files( 0 );
  say( 1, cINFO, "CLIPBOARD: %s: %d of %d files processed ok",
                 CB_DESC[ mode ],
                 clipboard_copy_info.ok_count,
                 clipboard_copy_info.files_count );

  if ( mode == CLIPBOARD_MOVE ) clipboard_clear();
}

void clipboard_clear()
{
  if( clipboard_copy_info.files_count )
    say( 2, cINFO, "CLIPBOARD: %d files removed", clipboard_copy_info.files_count );
  else
    say( 2, cINFO, "CLIPBOARD: empty" );
  Clipboard.undef();
  clipboard_copy_info.reset();
}

void clipboard_view()
{
  mb = Clipboard.keys();
  if( mb.count() == 0 )
    {
    say2( "CLIPBOARD: empty" );
    return;
    }
  vfu_menu_box( 5, 5, "File Clipboard Content" );
}

void clipboard_menu( int act )
{
  if ( act == 0 )
    {
    mb.undef();
    mb.push( "A Add files to the clipboard" );
    mb.push( "P Copy files here" );
    mb.push( "O Move files here" );
    mb.push( "L Symlink files here" );
    mb.push( "E Clear clipboard" );
    mb.push( "V View clipboard" );

    VString fcnt = clipboard_copy_info.files_count;
    str_comma( fcnt );
    VString fsize = clipboard_copy_info.files_size;
    str_comma( fsize );
    mb.push( "---  " + fcnt + " files, " + fsize + " bytes" );

    if ( vfu_menu_box( 5, 5, "File Clipboard " + fcnt + " files, " + fsize + " bytes" ) == -1 ) return;
    act = menu_box_info.ec;
    }
  act = toupper( act );
  switch( act )
    {
    case 'A': clipboard_add(); break;
    case 'P': clipboard_paste( CLIPBOARD_COPY ); break;
    case 'O': clipboard_paste( CLIPBOARD_MOVE ); break;
    case 'L': clipboard_paste( CLIPBOARD_SYMLINK ); break;
    case 'E': clipboard_clear(); break;
    case 'V': clipboard_view(); break;
    }
}

/*---------------------------------------------------------------------------*/

