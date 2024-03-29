/****************************************************************************
 #
 # Copyright (c) 1996-2023 Vladi Belperchinov-Shabanski "Cade" 
 # https://cade.noxrun.com/  <cade@noxrun.com> <cade@bis.bg>
 # https://cade.noxrun.com/projects/vfu     https://github.com/cade-vs/vfu
 #
 # SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 #
 ****************************************************************************/


#include "vfuarc.h"
#include "vfuuti.h"
#include "vfuopt.h"
#include "vfudir.h"
#include "vfucopy.h"
#include "vfufiles.h"

/*---------------------------------------------------------------------------*/

VString find_rx_auto()
{
  if( file_exist( "/usr/libexec/vfu/rx_auto" ) ) return VString( "/usr/libexec/vfu/rx_auto" );
  if( file_exist( "/usr/lib/vfu/rx_auto"     ) ) return VString( "/usr/lib/vfu/rx_auto"     );
  return VString( "rx_auto" );
}

void vfu_read_archive_files( int a_recursive )
{
  char line[2048] = "";
  struct stat st;
  memset( &st, 0, sizeof(st));

  if ( a_recursive )
    archive_path = ""; /* cannot have path when recursing archive */

  VString an = archive_name;
  VString ap = archive_path;
  
  shell_escape( an );
  shell_escape( ap );

  VString s;
  s = find_rx_auto() + " ";
  s += ( a_recursive ) ? "v" : "l";
  s += " " + an + " " + ap + " 2> /dev/null";

  /* NOTE: calling rx_* should be safe and result should be proper.
     all bugs must be traced outside VFU ...
  */
  FILE *f = popen( s, "r" );
  s = "";
  if ( !f )
    {
    say2( "Archive cannot be recognized or cannot be read" );
    } else
  while( fgets(line, 2048-1, f) )
    {
    str_cut( line, "\n\r" );
    if ( strncmp( line, "NAME:", 5 ) == 0 )
      {
      s = line+5;
      if ( str_get_ch( s, -1 ) == '/' ) /* i.e. last char */
        {
        str_trim_right( s, 1 );
        st.st_mode |= S_IFDIR;
        }
      } else
    if ( strncmp( line, "MODE:", 5 ) == 0 )
      {
      VString ms = line + 5;
      if( ms[0] == 'd' )
        st.st_mode |= S_IFDIR;

      if (ms[1] == 'r') st.st_mode |= S_IRUSR;
      if (ms[2] == 'w') st.st_mode |= S_IWUSR;
      if (ms[3] == 'x') st.st_mode |= S_IXUSR;
      if (ms[4] == 'r') st.st_mode |= S_IRGRP;
      if (ms[5] == 'w') st.st_mode |= S_IWGRP;
      if (ms[6] == 'x') st.st_mode |= S_IXGRP;
      if (ms[7] == 'r') st.st_mode |= S_IROTH;
      if (ms[8] == 'w') st.st_mode |= S_IWOTH;
      if (ms[9] == 'x') st.st_mode |= S_IXOTH;

      if (ms[3] == 's') { st.st_mode |= S_ISUID; st.st_mode |= S_IXUSR; }
      if (ms[3] == 'S') st.st_mode |= S_ISUID;
      if (ms[6] == 's') { st.st_mode |= S_ISGID; st.st_mode |= S_IXGRP; }
      if (ms[6] == 'S') st.st_mode |= S_ISGID;
      if (ms[9] == 't') { st.st_mode |= S_ISVTX; st.st_mode |= S_IXOTH; }
      if (ms[9] == 'T') st.st_mode |= S_ISVTX;

      } else
    if ( strncmp( line, "SIZE:", 5 ) == 0 )
      {
      st.st_size = atoi( line+5 );
      } else
    if ( strncmp( line, "TIME:", 5 ) == 0 )
      {
      struct tm t;
      memset( &t, 0, sizeof(t) );
      VRegexp r( "^(....)(..)(..)(..)(..)(..)?" );
      r.m( line + 5 );
      t.tm_year = atoi( r[1] ) - 1900;
      t.tm_mon  = atoi( r[2] );
      t.tm_mday = atoi( r[3] );
      t.tm_hour = atoi( r[4] );
      t.tm_min  = atoi( r[5] );
      t.tm_sec  = atoi( r[6] );
      st.st_mtime = st.st_ctime = st.st_atime = mktime( &t );
      } else
    if ( line[0] == 0 )
      {
      if ( str_len( s ) > 0 )
        vfu_add_file( s, &st, 0 ); /* FIXME: there's no links for now */
      s = "";
      memset( &st, 0, sizeof(st) );
      }

    }
  pclose( f );
}

/*---------------------------------------------------------------------------*/

void vfu_browse_archive_file()
{
  VString tmpdir = vfu_temp_dir();
  if( tmpdir == "" || chdir( tmpdir ) )
    {
    say1( "error: cannot create temp directory: " + tmpdir );
    say2errno();
    return;
    }

  VString fn = files_list_get(FLI)->full_name();

  VString wp = work_path;
  VString an = archive_name;
  
  shell_escape( wp );
  shell_escape( an );
  shell_escape( fn );

  VString s;
  s = find_rx_auto() + " x " + wp + an + " " + fn + " 2> /dev/null";

  vfu_shell( s, "" );

  if( chdir( tmpdir ) ) /* FIXME: a little hack -- vfu_shell() changes current path */
    {
    say1( "error: cannot chdir to temp directory: " + tmpdir );
    say2errno();
    return;
    }
  vfu_browse( fn );

  if(chdir( work_path ))
    {
    say1( "error: cannot chdir back to work directory: " + work_path );
    say2errno();
    return;
    }
  __vfu_dir_erase( tmpdir );
  say1( "" );
}

/*---------------------------------------------------------------------------*/

void vfu_user_external_archive_exec( VString &shell_line  )
{
  VString tmpdir = vfu_temp_dir();
  if( tmpdir == "" || chdir( tmpdir ) )
    {
    say1( "error: cannot create temp directory: " + tmpdir );
    say2errno();
    return;
    }

  VString fn = files_list_get(FLI)->full_name();

  VString wp = work_path;
  VString an = archive_name;
  
  shell_escape( wp );
  shell_escape( an );
  shell_escape( fn );

  VString s;
  s = find_rx_auto() + " x " + wp + an + " " + fn + " 2> /dev/null";

  vfu_shell( s, "" );

  if(chdir( tmpdir )) /* FIXME: a little hack -- vfu_shell() changes current path */
    {
    say1( "error: cannot chdir to temp directory: " + tmpdir );
    say2errno();
    return;
    }
  str_replace( shell_line, "%f", shell_escape( fn ) );
  str_replace( shell_line, "%F", shell_escape( fn ) );
  vfu_shell( shell_line, "" );

  if(chdir( work_path ))
    {
    say1( "error: cannot chdir back to work directory: " + work_path );
    say2errno();
    return;
    }
  __vfu_dir_erase( tmpdir );
  say1( "" );
}

/*---------------------------------------------------------------------------*/

void vfu_extract_files( int one )
{
  if ( sel_count == 0 && one == 0 ) one = 1;
  fname_t t;
  VString target;

  if ( one == 0 )
    sprintf( t, "EXTRACT SELECTION to: " );
  else
    snprintf( t, sizeof(t), "EXTRACT `%s' to:", files_list_get(FLI)->full_name() );

  target = opt.last_copy_path[ CM_COPY ];
  if ( !vfu_get_dir_name( t, target ) ) return;

  strcpy( opt.last_copy_path[ CM_COPY ], target );

  VArray va;

  int z;
  for( z = 0; z < files_list_count(); z++ )
    if ((files_list_get(z)->sel && one == 0) || (z == FLI && one != 0))
      va.push( files_list_get(z)->full_name() );

  if (chdir(target))
    {
    snprintf( t, sizeof(t), "Cannot chdir to: %s", target.data() );
    say1( t );
    say2errno();
    return;
    }

  VString tmpfile = vfu_temp();
  if (va.fsave( tmpfile ))
    {
    snprintf( t, sizeof(t), "Error writing list file: %s", tmpfile.data() );
    say1( t );
    return;
    }
  chmod( tmpfile, S_IRUSR|S_IWUSR );

  VString wp = work_path;
  VString an = archive_name;
  
  shell_escape( wp );
  shell_escape( an );

  VString s;
  s = find_rx_auto() + " x " + wp + an + " @" + tmpfile + " 2> /dev/null";

  vfu_shell( s, "" );

  if (unlink( tmpfile ))
    {
    /*
    snprintf( t, sizeof(t), "Cannot unlink/erase temp file: %s", tmpfile );
    say2( t );
    */
    }
  if (chdir(work_path))
    {
    snprintf( t, sizeof(t), "Cannot chdir back to to: %s", work_path.data() );
    say1( t );
    say2errno();
    return;
    }
  say1( "EXTRACT ok." );
}

/*---------------------------------------------------------------------------*/

/* eof vfuarc.cpp */
