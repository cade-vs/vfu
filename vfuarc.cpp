/****************************************************************************
 *
 * (c) Vladi Belperchinov-Shabanski "Cade" 1996-2020
 * http://cade.datamax.bg/  <cade@biscom.net> <cade@bis.bg> <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 ****************************************************************************/


#include "vfuarc.h"
#include "vfuuti.h"
#include "vfuopt.h"
#include "vfudir.h"
#include "vfucopy.h"
#include "vfufiles.h"

/*---------------------------------------------------------------------------*/

void vfu_read_archive_files( int a_recursive )
{
  char line[2048] = "";
  struct stat st;
  memset( &st, 0, sizeof(st));

  if ( a_recursive )
    archive_path = ""; /* cannot have path when recursing archive */

  VString s;
  s = "rx_auto ";
  s += ( a_recursive ) ? "v" : "l";
  s += " '" + archive_name + "' ";
  s += " '" + archive_path + "' ";
  s += " 2> /dev/null";
  /* NOTE: calling rx_* should be safe and result should be proper
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
        /* FIXME: my man page for stat() says S_IFDIR is not POSIX?! */
      } else
    if ( strncmp( line, "MODE:", 5 ) == 0 )
      {
      VString ms = line + 5;
      if( ms[0] == 'd' )
        st.st_mode |= S_IFDIR;

      // FIXME: move this modeline parse to function and port back to vfufiles?
      if (ms[1] == 'r') st.st_mode |= S_IRUSR;
      if (ms[2] == 'w') st.st_mode |= S_IWUSR;
      if (ms[3] == 'x') st.st_mode |= S_IXUSR;
      if (ms[4] == 'r') st.st_mode |= S_IRGRP;
      if (ms[5] == 'w') st.st_mode |= S_IWGRP;
      if (ms[6] == 'x') st.st_mode |= S_IXGRP;
      if (ms[7] == 'r') st.st_mode |= S_IROTH;
      if (ms[8] == 'w') st.st_mode |= S_IWOTH;
      if (ms[9] == 'x') st.st_mode |= S_IXOTH;

      #ifndef _TARGET_GO32_
      if (ms[3] == 's') { st.st_mode |= S_ISUID; st.st_mode |= S_IXUSR; }
      if (ms[3] == 'S') st.st_mode |= S_ISUID;
      if (ms[6] == 's') { st.st_mode |= S_ISGID; st.st_mode |= S_IXGRP; }
      if (ms[6] == 'S') st.st_mode |= S_ISGID;
      if (ms[9] == 't') { st.st_mode |= S_ISVTX; st.st_mode |= S_IXOTH; }
      if (ms[9] == 'T') st.st_mode |= S_ISVTX;
      #endif

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
  VString tmpdir = vfu_temp();
  if(mkdir( tmpdir, S_IRUSR|S_IWUSR|S_IXUSR /*|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH*/ ))
    {
    say1( "error: cannot create temp directory" );
    say2( tmpdir );
    return;
    }
  if (chdir( tmpdir ));

  VString fn = files_list_get(FLI)->full_name();

  VString s;
  s = "/usr/lib/vfu/rx_auto x \"";
  s += work_path;
  s += archive_name;
  s += "\" ";
  s += fn;
  s += " 2> /dev/null";

  vfu_shell( s, "" );

  if (chdir( tmpdir )); /* FIXME: a little hack -- vfu_shell() changes current path */
  vfu_browse( fn );

  if (chdir( work_path ));
  __vfu_dir_erase( tmpdir );
  say1( "" );
}

/*---------------------------------------------------------------------------*/

void vfu_user_external_archive_exec( VString &shell_line  )
{
  VString tmpdir = vfu_temp();
  if(mkdir( tmpdir, S_IRUSR|S_IWUSR|S_IXUSR /*|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH*/ ))
    {
    say1( "error: cannot create temp directory" );
    say2( tmpdir );
    return;
    }
  if (chdir( tmpdir ));

  VString fn = files_list_get(FLI)->full_name();

  VString s;
  s = "/usr/lib/vfu/rx_auto x \"";
  s += work_path;
  s += archive_name;
  s += "\" ";
  s += fn;
  s += " 2> /dev/null";

  vfu_shell( s, "" );

  if (chdir( tmpdir )); /* FIXME: a little hack -- vfu_shell() changes current path */
  str_replace( shell_line, "%f", fn );
  str_replace( shell_line, "%F", fn );
  vfu_shell( shell_line, "" );

  if (chdir( work_path ));
  __vfu_dir_erase( tmpdir );
  say1( "" );
}

/*---------------------------------------------------------------------------*/

void vfu_extract_files( int one )
{
  if ( sel_count == 0 && one == 0 ) one = 1;
  char t[MAX_PATH];
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

  VString s;
  s = "/usr/lib/vfu/rx_auto x \"";
  s += work_path;
  s += archive_name;
  s += "\" @";
  s += tmpfile;
  s += " 2> /dev/null";

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
