/*
 *
 * (c) Vladi Belperchinov-Shabanski "Cade" 1996-2000
 * http://www.biscom.net/~cade  <cade@biscom.net>  <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 * $Id: vfuarc.cpp,v 1.2 2001/10/28 13:56:40 cade Exp $
 *
 */


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
  
  String s;
  s = "rx_auto ";
  s += ( a_recursive ) ? "V " : "v \"";
  s += archive_name;
  s += "\" ";
  s += archive_path;
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
    if ( strncmp( line, "SIZE:", 5 ) == 0 )
      {
      st.st_size = atoi( line+5 );
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
};

/*---------------------------------------------------------------------------*/

void vfu_browse_archive_file()
{
  char tmpdir[MAX_PATH];
  strcpy( tmpdir, "/tmp/vfu.XXXXXX" );
  mktemp( tmpdir );
  if(mkdir( tmpdir, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH ))
    {
    say1( "error: cannot create temp directory" );
    say2( tmpdir );
    return;
    }
  chdir( tmpdir );

  String fn = files_list[FLI]->full_name();

  String s;
  s = "rx_auto x \"";
  s += work_path;
  s += archive_name;
  s += "\" ";
  s += fn;
  s += " 2> /dev/null";

  vfu_shell( s, "" );

  chdir( tmpdir ); /* FIXME: a little hack -- vfu_shell() changes current path */
  vfu_browse( fn );

  chdir( work_path );
  __vfu_dir_erase( tmpdir );
};

/*---------------------------------------------------------------------------*/

void vfu_user_external_archive_exec( String &shell_line  )
{
  char tmpdir[MAX_PATH];
  strcpy( tmpdir, "/tmp/vfu.XXXXXX" );
  mktemp( tmpdir );
  if(mkdir( tmpdir, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH ))
    {
    say1( "error: cannot create temp directory" );
    say2( tmpdir );
    return;
    }
  chdir( tmpdir );

  String fn = files_list[FLI]->full_name();

  String s;
  s = "rx_auto x \"";
  s += work_path;
  s += archive_name;
  s += "\" ";
  s += fn;
  s += " 2> /dev/null";

  vfu_shell( s, "" );

  chdir( tmpdir ); /* FIXME: a little hack -- vfu_shell() changes current path */
  str_replace( shell_line, "%f", fn );
  str_replace( shell_line, "%F", fn );
  vfu_shell( shell_line, "" );

  chdir( work_path );
  __vfu_dir_erase( tmpdir );
}

/*---------------------------------------------------------------------------*/

void vfu_extract_files( int one )
{
  if ( sel_count == 0 && one == 0 ) one = 1;
  char t[MAX_PATH];
  char tmpfile[MAX_PATH];
  String target;

  if ( one == 0 )
    sprintf( t, "EXTRACT SELECTION to: " );
  else
    sprintf( t, "EXTRACT `%s' to:", files_list[FLI]->full_name() );

  target = opt.last_copy_path[ CM_COPY ];
  if ( !vfu_get_dir_name( t, target ) ) return;

  strcpy( opt.last_copy_path[ CM_COPY ], target );

  PSZCluster sc; sc.create( 16, 16 );

  int z;
  for( z = 0; z < files_count; z++ )
    if ((files_list[z]->sel && one == 0) || (z == FLI && one != 0))
      sc.add( files_list[z]->full_name() );

  if (chdir(target))
    {
    sprintf( t, "Cannot chdir to: %s", target.data() );
    say1( t );
    say2errno();
    return;
    }

  strcpy( tmpfile, "/tmp/vfu.XXXXXX" );
  mktemp( tmpfile );
  if (SaveToFile( tmpfile, &sc ))
    {
    sprintf( t, "Error writing list file: %s", tmpfile );
    say1( t );
    return;
    }

  String s;
  s = "rx_auto x \"";
  s += work_path;
  s += archive_name;
  s += "\" @";
  s += tmpfile;
  s += " 2> /dev/null";

  vfu_shell( s, "" );

  if (unlink( tmpfile ))
    {
    /*
    sprintf( t, "Cannot unlink/erase temp file: %s", tmpfile );
    say2( t );
    */
    }
  if (chdir(work_path))
    {
    sprintf( t, "Cannot chdir back to to: %s", work_path.data() );
    say1( t );
    say2errno();
    return;
    }
  say1( "EXTRACT ok." );
};

/*---------------------------------------------------------------------------*/

/*

  // NOTE: for extract commands need `%s%s' to construct CPath/AName 'cos
  // the current path will be changed to the target
  ArcItem      Arcs[] = {
  #ifdef _TARGET_UNIX_
  { ".zip", "unzip -v   %s" , "unzip      %s%s `cat %s`", "unzip      %s%s    %s" , "unzip -c   %s %s > %s", "^ *([0123456789]+).*-.*:.* ([^ ]*[^/]) *$", 2, 1 },
  #else
  { ".zip", "unzip -v   %s" , "unzip      %s%s    @%s"  , "unzip      %s%s    %s"   , "unzip -c   %s %s > %s", "^ *([0123456789]+).*-.*:.* ([^ ]*[^/]) *$", 2, 1 },
  #endif
  { ".arj", "arj v      %s" , "arj x   -y %s%s    !%s"  , "arj x   -y %s%s    %s"   , "arj p      %s %s > %s", "^\\+([^ ]+) +([0123456789]+) *$", 1, 2 },
  { ".tar", "tar tvf    %s" , "tar xvf    %s%s -T  %s"  , "tar xvf    %s%s    %s"   , "tar xOvf   %s %s > %s", ".* ([0123456789]+) .* ([^ ]+[^/]) *$", 2, 1 },
  { ".tgz", "tar tzvf   %s" , "tar xzvf   %s%s -T  %s"  , "tar xzvf   %s%s    %s"   , "tar xOzvf  %s %s > %s", ".* ([0123456789]+) .* ([^ ]+[^/]) *$", 2, 1 },
  { ".tar.gz" , "tar tzvf   %s" , "tar xzvf   %s%s -T  %s"  , "tar xzvf   %s%s    %s"   , "tar xOzvf  %s %s > %s", ".* ([0123456789]+) .* ([^ ]+[^/]) *$", 2, 1 },
  { ".tbz", "tar tIvf   %s" , "tar xIvf   %s%s -T  %s"  , "tar xIvf   %s%s    %s"   , "tar xOIvf  %s %s > %s", ".* ([0123456789]+) .* ([^ ]+[^/]) *$", 2, 1 },
  { ".tar.bz2", "tar tIvf   %s" , "tar xIvf   %s%s -T  %s"  , "tar xIvf   %s%s    %s"   , "tar xOIvf  %s %s > %s", ".* ([0123456789]+) .* ([^ ]+[^/]) *$", 2, 1 },
  { ".uc2", "uc \\d     %s" , "uc  es  -F %s%s    @%s"  , "uc  es  -F %s%s    %s"   , "uc $PRF    %s %s > %s", "^\\+([^ ]+) +([0123456789]+) *$", 1, 2 },
  { ".lzh", "lha v      %s" , "-"                       , "lha e      %s%s    %s"   , "lha p -p   %s %s > %s", "^\\+([^ ]+) +([0123456789]+) *$", 1, 2 },
  { ".rar", "rar v -std %s" , "rar x -std %s%s    @%s"  , "rar x -std %s%s    %s"   , "rar p -std %s %s > %s", "^\\+([^ ]+) +([0123456789]+) *$", 1, 2 },
  { ".ha" , "ha  lf     %s" , "-"                       , "ha  xy     %s%s    %s"   , "-", "^\\+([^ ]+) +([0123456789]+) *$", 1, 2 },
  { ".j"  , "jar v      %s" , "jar x   -y %s%s    @%s"  , "jar x   -y %s%s    %s"   , "-", "^\\+([^ ]+) +([0123456789]+) *$", 1, 2 },
  { ".lim", "limit l    %s" , "limit e -p %s%s    @%s"  , "limit e -p %s%s    %s"   , "-", "^\\+([^ ]+) +([0123456789]+) *$", 1, 2 },
  { ".ftp", "ftparc -l  %s" , "ftparc -x -q  %s%s    @%s"  , "ftparc -x -q %s%s %s"   , "ftparc -p -q %s %s > %s", ".* ([0123456789]+) [A-Z].* ([^ ]+[^/]) *$", 2, 1 },
  { ".deb", "debarc  l  %s" , "debarc  x     %s%s  -T %s"  , "debarc  x    %s%s %s"   , "debarc  p    %s %s > %s", ".* ([0123456789]+) .* ([^ ]+[^/]) *$", 2, 1 },
  { "---", "", "", "", "", "", 0, 0 }
//  { "arj", "arj l    %s" , "arj e      %s%s    !%s" , "arj p     %s %s > %s", "^ *([^ ]+) +([0123456789]+) +", 1, 2 },
//  { "j"  , "jar -l %a", "jar e %s !%s", "jar ", "", 0, 0 },
  };


////////////////////////////////////////////////////////////////////////////
//
//
//

  void ViewArchiveFile()
  {
    char tmp[MAX_PATH];
    char tmp2[MAX_PATH];
    mktemp--tmpnam( tmp );
    strcpy( tmp2, tmp );
    StrFixPath( tmp2 );

    const char *finame = Files[FLI]->name;

    if(mkdir( tmp, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH ))
      {
      sprintf(sss, "Cannot create temp path: %s", tmp);
      say1(sss);
      return;
      }

    chdir( tmp );

    if (strcasecmp(Arcs[AX].ext,"uc2") == 0 )
      { // uc2 hack
      char _fnonly[MAX_PATH];
      StrFNameExt( finame, _fnonly );
      sprintf( sss, Arcs[AX].extrs_cmd, CPath, AName, _fnonly );
      }
    else
      sprintf( sss, Arcs[AX].extrs_cmd, CPath, AName, finame );
    #ifdef _TARGET_GO32_
    StrTR( sss, "/", "\\" ); // BIG HACK -- sorry I'll fix it later
    #endif
    ConCS();
    say1( sss );
    Shell( sss, 0, 0 );
    Browse( finame );

    EraseDir( tmp );
  }

  void ArcUserExternal( const char *cmdline )
  {
    char tmp[MAX_PATH];
    char tmp2[MAX_PATH];
    mktemp--tmpnam( tmp );
    strcpy( tmp2, tmp );
    StrFixPath( tmp2 );

    const char *finame = Files[FLI]->name;

    if(mkdir( tmp, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH ))
      {
      sprintf(sss, "Cannot create temp path: %s", tmp);
      say1(sss);
      return;
      }

    chdir( tmp );

    // extracting file
    sprintf( sss, Arcs[AX].extrs_cmd, CPath, AName, finame );
    #ifdef _TARGET_GO32_
    StrTR( sss, "/", "\\" ); // BIG HACK -- sorry I'll fix it later
    #endif
    ConCS();
    say1( sss );
    Shell( sss, 0, 0 );
    // file extracted -- continue

    strcat( tmp2, finame );
    #ifdef _TARGET_GO32_
    if ( StrFind( cmdline, "%_" ) != -1 )
      {
      file_get_sfn( tmp2, sss );
      strcpy( tmp2, sss );
      }
    if ( StrFind( cmdline, "%\\" ))
      StrTR( tmp2, "/", "\\" ); // BIG HACK -- sorry I'll fix it later
    #endif
    strcpy( sss, cmdline );
    StrReplace( sss, "%f", "%F" );
    StrReplace( sss, "%F", tmp2 );

    ConCS();
    say1( sss );
    Shell( sss, 0, 0 );

    EraseDir( tmp );
  }

  void ViewArchiveFile2()
  {
    ASSERT( AX != -1 );

    if (Arcs[AX].view_cmd[0] == '-')
      {
      say1( "<BROWSE/VIEW> is not supported for this archive type!" );
      return;
      }

    ConCS();
    sprintf( sss, Arcs[AX].view_cmd, AName, Files[FLI]->name, ArcTempFile );
    Shell( sss, 0 );
    // String str = Browser; StrReplace( str,"%F", "%f" ); StrReplace( str,"%f", ArcTempFile );
    // Shell( str, 0 );
    Browse( ArcTempFile );
    unlink( ArcTempFile );
  }
*/

/* eof vfuarc.cpp */
