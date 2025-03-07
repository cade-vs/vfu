/****************************************************************************
 #
 # Copyright (c) 1996-2023 Vladi Belperchinov-Shabanski "Cade" 
 # https://cade.noxrun.com/  <cade@noxrun.com> <cade@bis.bg>
 # https://cade.noxrun.com/projects/vfu     https://github.com/cade-vs/vfu
 #
 # SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 #
 ****************************************************************************/

#include <stdarg.h>

#include "vfu.h"
#include "vfuopt.h"
#include "vfufiles.h"
#include "vfucopy.h"
#include "vfudir.h"
#include "vfuview.h"
#include "vfumenu.h"
#include "vfuuti.h"
#include "vfusys.h"
#include "vfuarc.h"
#include "vfutools.h"
#include "stdarg.h"
#include "see.h"
#include "locale.h"

/*######################################################################*/

  /* work context */
  int    work_mode;
  VString work_path;
  /* archive context */
  VString archive_name;
  VString archive_path;
  VArray archive_extensions;

  VString external_panelizer;
  VArray list_panelizer;

  /* file list statistics */
  fsize_t   files_size;
  int       sel_count;
  fsize_t   sel_size;
  /* file system statistics */
  fsize_t   fs_free;
  fsize_t   fs_total;
  fsize_t   fs_block_size;

  /* some world wide variables */
  VString startup_path;
  VString home_path;
  VString tmp_path;
  VString rc_path;

  /* files masks */
  VString   files_mask;
  VArray   files_mask_array;

  /* misc */
  int print_help_on_exit;

  VString last_inc_search;

/*############################################ GLOBAL STRUCTS  #########*/

  VArray   dir_tree;
  int      dir_tree_changed;
  VString   dir_tree_file;

  WArray file_find_results; // filefind results

  VArray path_bookmarks;

/*######################################################################*/

  VArray user_externals;
  VArray history;
  VArray see_filters;
  VArray panelizers;

  WArray mb; /* menu boxes */

  VArray trim_tree;

  VArray view_profiles;
  VString view_profile;

/*############################################ CONFIG SETTINGS #########*/

  VString ext_colors[16];

  VString shell_browser;
  VString shell_editor;
  VString shell_options;
  VString shell_diff;
  VString shell_prog;

  VString user_id_str;
  VString group_id_str;
  VString host_name_str;

  VString filename_opt;
  VString filename_conf;
  VString filename_history;
  VString filename_tree;
  VString filename_size_cache;
  VString filename_ffr; /* file find results */

/*######################################################################*/

  int do_draw;
  int do_draw_status;

/*######################################################################*/

/*
   messages printing
*/

void say( int line, int attr, const char* format, ... )
{
  char say_buf[4096];
  ASSERT( line == 1 || line == 2 );
  
  va_list vlist;
  va_start( vlist, format );
  vsnprintf( say_buf, sizeof(say_buf), format, vlist );
  va_end( vlist );

  WString ws = say_buf;
  VString os = str_dot_reduce( ws, con_max_x()-1 );
  vfu_con_out( 1, con_max_y() - ( (line == 1) ? 1 : 0 ), os, attr );
  con_ce( attr );
}

void say1(const char *a_str, int attr )
{
  say( 1, attr, "%s", a_str );
}

void say2(const char *a_str, int attr )
{
  say( 2, attr, "%s", a_str );
}

void say2errno()
{
  VString str = "error: ";
  str += strerror(errno);
  say2( str );
}

void saycenter( int line, int attr, const char *a_str )
{
  VString str = " ";
  int sl = str_len( a_str );
  if ( sl < con_max_x() )
    {
    sl = ( con_max_x() - sl ) / 2;
    str_mul( str, sl );
    str = str + a_str;
    }
  say( line, attr, "%s", str.data() );
}

void say1center(const char *a_str, int attr )
{
  saycenter( 1, attr, a_str );
}

void say2center(const char *a_str, int attr )
{
  saycenter( 2, attr, a_str );
}

void log_debug( const char* format, ... )
{
  char say_buf[4096];
  
  va_list vlist;
  va_start( vlist, format );
  vsnprintf( say_buf, sizeof( say_buf ), format, vlist );
  va_end( vlist );

  FILE* f = fopen( "/tmp/vfu.debug.txt", "at" );
  if (!f) return;
  if ( fwrite( say_buf, 1, strlen( say_buf ), f ) != strlen( say_buf ) ) return;
  if ( fwrite( "\n", 1, 1, f ) != 1 ) return;
  fclose(f);
}

/*######################################################################*/

VString get_user_id_str( int uid, int padw = 0 )
{
  VString str;
  if( struct passwd* _pwd = getpwuid( uid ) )
    str = _pwd->pw_name;
  else  
    str.i( uid );

  if( padw != 0 ) str_padw( str, padw );
  return str;
}

VString get_group_id_str( int gid, int padw = 0 )
{
  VString str;
  if( struct group*  _grp = getgrgid( gid ) )
    str = _grp->gr_name;
  else  
    str.i( gid );

  if( padw != 0 ) str_padw( str, padw );
  return str;
}

/*######################################################################*/

void TF::reset() /* reset -- NULL all fields */
{
  _name.undef();
  _name_ext.undef();
  _ext.undef();;
  memset( &_st, 0, sizeof(_st) );
  _type_str[0] = 0;
  _is_link = 0;
  _is_dir = 0;
  strcpy( _mode_str, MODE_OFF );
  _size = -1; /* unknown -- get from stat? */
  _view.undef();
  _color = cPLAIN;
  sel = 0;
}

/*-----------------------------------------------------------------------*/

TF::TF()
{
  reset();
}

/*-----------------------------------------------------------------------*/

TF::TF( const char* a_name, const struct stat* a_stat, int a_is_link )
{
  reset();
  set_name( a_name );
  update_stat( a_stat, a_is_link );
}

/*-----------------------------------------------------------------------*/

TF::~TF()
{
  reset();
}

/*-----------------------------------------------------------------------*/

const char* TF::full_name( int fix )
{
  ASSERT( _name );

  if ( _name[0] == '/' )
    _full_name = _name;
  else
    {
    if ( work_mode == WM_ARCHIVE )
      _full_name = archive_path;
    else
      _full_name = work_path;
    _full_name += _name;
    }
  if ( fix && _is_dir )
    _full_name += "/"; /* i.e. str_fix_path() */
  return _full_name.data();
}

/*-----------------------------------------------------------------------*/

void TF::set_name( const char* a_new_name )
{
  _name = a_new_name;

  int last_slash = str_rfind( _name, '/' );
  if ( last_slash == -1 )
    _name_ext = _name;
  else
    str_copy( _name_ext, _name, last_slash + 1 );

  int last_dot = str_rfind( _name, '.' );
  if ( last_dot == -1 || last_dot == 0 )
    _ext = ""; /* no dot or dot-file (hidden) */
  else
    str_copy( _ext, _name, last_dot ); /* including leading dot */

  drop_view();
}

/*-----------------------------------------------------------------------*/

void TF::set_size( fsize_t a_new_size )
{
  _size = a_new_size;
  drop_view();
}

/*-----------------------------------------------------------------------*/

int TF::color()
{
  if ( _color < 0 ) _color = get_item_color( this );
  return _color;
}

/*-----------------------------------------------------------------------*/

void TF::drop_view()
{
  _view.undef();
}

/*-----------------------------------------------------------------------*/

const char* TF::view()
{
  if ( str_len( _view ) == 0 ) refresh_view();
  return _view.data();
}

/*-----------------------------------------------------------------------*/

void TF::refresh_view()
{
  ASSERT( _name );
  ASSERT( _name_ext );
  ASSERT( _ext );

  char    stmode[16]      = ""; // 10 + 1sep
  VString stowner;
  VString stgroup;
  char    sttime[32]      = "";
  char    stsize[18]      = "";
  char    sttype[4]       = "";

  if ( !opt.long_name_view )
    {
    if (opt.f_mode)
      {
      strcpy( stmode, _mode_str );
      strcat( stmode, " " ); /* field separator */
      }

    if (opt.f_owner)
      stowner = get_user_id_str( _st.st_uid, 8 ) + " ";

    if (opt.f_group)
      stgroup = get_group_id_str( _st.st_gid, 8 ) + " ";

    if (opt.f_time )
      {
      time_t ftm = vfu_opt_time( _st );
      time_str_compact( ftm, sttime );
      long int tdiff = time( NULL ) - ftm;
      strcat( sttime, tdiff > 4 ? tdiff > 23*60*60 ? " " : "*" : "!" ); // TODO: use for something useful
      strcat( sttime, " " ); /* field separator */
      }

    if (opt.f_size)
      {
      VString str;
      if ( _is_dir && _size == -1 )
        str = "[DIR]";
      else
        str = fsize_fmt( _size );
      snprintf( stsize, sizeof(stsize), "%14s", (const char*)(str) );
      strcat( stsize, " " ); /* field separator */
      }
  } /* if ( !opt.LongNameView ) */

  if (opt.f_type || opt.long_name_view )
    {
    strcpy( sttype, _type_str );
    /* there is no field separator here! */
    }

  VString name_view = _name;

  #ifdef _TARGET_UNIX_
  /* links are supported only under UNIX */
  if ( _is_link )
    {
      name_view += " -> ";
      name_view += vfu_readlink( _name );
    }
  #endif

  /* the three space separator below is for the tag and selection marks `>>#' */
  VString  view;
  view = view + stmode + stowner + stgroup + sttime + stsize + sttype + "   " + name_view;

  WString wview;
  wview = view;

  int x = con_max_x() - 1; // 1 char for scroller, FIXME: TODO: should be optional
  if ( str_len( wview ) > x )
    {
    str_sleft( wview, x -1 );
    wview += L">";
    }
  else
    str_pad( wview, - x ); // +1 for the scroller
  
  _view = wview;
}

/*-----------------------------------------------------------------------*/

void TF::update_stat( const struct stat* a_new_stat, int a_is_link )
{
  if ( a_new_stat )
    memcpy( &_st, a_new_stat, sizeof(_st) );
  else
    stat( _name, &_st );

  _is_link = (a_is_link == -1) ? file_is_link( _name ) : a_is_link;
  _is_dir  = S_ISDIR(_st.st_mode );

  if( _is_link && opt.show_symlinks_stat )
    lstat( _name, &_st );

  strcpy( _type_str, file_type_str( _st.st_mode, _is_link ) );

  file_get_mode_str( _st.st_mode, _mode_str );
  if ( _is_dir )
    _size = -1; /* FIXME: some auto thing here? */
  else
    _size = _st.st_size;

  _color = -1;

  drop_view();
}

/*######################################################################*/

void vfu_help()
{
  say1center( HEADER  );
  say2center( CONTACT );
  mb.undef();
  mb.push( L"*keypad -- navigation keys" );
  mb.push( L"ENTER   -- enter into directory/View file ( `+' and `=' too )");
  mb.push( L"BACKSPC -- (BS) chdir to parent directory ( `-' and ^H too )"         );
  mb.push( L"Alt + - -- exit current archive ( Alt + BACKSPACE too )"     );
  mb.push( L"TAB     -- edit entry: filename, atrrib's/mode, owner, group");
  mb.push( L"R.Arrow -- rename current file " );
  mb.push( L"SPACE   -- select/deselect current list item"   );
  mb.push( L"ESC     -- exit menu");
  mb.push( L"ESC+ESC -- exit menu");
  mb.push( L"1       -- toggle `mode'  field on/off "    );
  mb.push( L"2       -- toggle `owner' field on/off "    );
  mb.push( L"3       -- toggle `group' field on/off "    );
  mb.push( L"4       -- toggle `time'  field on/off "    );
  mb.push( L"5       -- toggle `size'  field on/off "    );
  mb.push( L"6       -- toggle `type'  field on/off "    );
  mb.push( L"7       -- toggle `time type' field change/modify/access time "    );
  mb.push( L"8       -- turn on all fields"    );
  mb.push( L"0       -- toggle long name view ( show only type and file name )"    );
  mb.push( L"~       -- change current dir to HOME directory"     );
  mb.push( L"A       -- arrange/Sort file list"                   );
  mb.push( L"B       -- browse(view) selected (if selection) or current file"         );
  mb.push( L"Alt+B   -- browse(view) ONLY current file (regardless selection)"      );
  mb.push( L"C       -- copy selected (if selection) or current file(s)"             );
  mb.push( L"Alt+C   -- copy ONLY current file (regardless selection)"             );
  mb.push( L"D       -- change directory"                         );
  mb.push( L"Ctrl+D  -- directory tree "                          );
  mb.push( L"Alt+D   -- chdir history " );
  mb.push( L"E       -- erase(remove) selected (if selection) or current file(s)!"    );
  mb.push( L"Alt+E   -- erase(remove) ONLY current file (regardless selection)!"    );
  mb.push( L"F       -- change file masks (space-delimited)       ");
  mb.push( L"Ctrl+F  -- reset file mask to `*'"                    );
  mb.push( L"G       -- global select/deselect"                    );
  mb.push( L"H       -- this help text"                            );
  mb.push( L"I       -- edit file"                            );
  mb.push( L"Q       -- exit here (to the current directory)");
  mb.push( L"R       -- reload directory/refresh file list"       );
  mb.push( L"Ctrl+R  -- recursive reload... "                     );
  mb.push( L"Alt+R   -- reload/tree menu" );
  mb.push( L"J       -- jump to mountpoint"                       );
  mb.push( L"L       -- symlink selected/current file(s) into new directory" );
  mb.push( L"Ctrl+L  -- refresh/redraw entire screen" );
  mb.push( L"M       -- move selected (if selection) or current file(s)"             );
  mb.push( L"Alt+M   -- move ONLY current file (regardless selection)"             );
  mb.push( L"N       -- file find"                                );
  mb.push( L"Alt+N   -- file find menu"   );
  mb.push( L"O       -- options(toggles) menu"       );
  mb.push( L"P       -- file clipboard menu"       );
  mb.push( L"S       -- incremental filename search"       );
  mb.push( L"Alt+S   -- repeat last used incremental search entry (find next)"       );
  /*
  mb.push( L"Ctrl+C  -- copy files to clipboard"       );
  mb.push( L"Ctrl+X  -- cut  files to clipboard"       );
  mb.push( L"Ctrl+V  -- paste (copy) files from clipboard to current directory" );
  */
  mb.push( L"T       -- tools menu (including rename and classify tools)"                              );
  mb.push( L"U       -- user menu (user external commands bound to menu)  " );
  mb.push( L"X       -- exit to the old(startup) directory ");
  mb.push( L"Alt+X   -- exit to the old(startup) directory ");
  mb.push( L"Z       -- calculate directories sizes menu"       );
  mb.push( L"Ctrl+Z  -- show size of the current (under the cursor >>) directory");
  mb.push( L"Alt+Z   -- show all directories sizes ( or Alt+Z )" );
  mb.push( L"V       -- edit vfu.conf file");
  mb.push( L"!       -- shell (also available with '?')"                         );
  mb.push( L"/       -- command line"                                            );
  mb.push( L"`       -- bookmarks menu"                                          );

  mb.push( L"[       -- go to previous directory in the parent's list"       );
  mb.push( L"]       -- go to next     directory in the parent's list"       );
  mb.push( L"Alt+Up  -- same as ["       );
  mb.push( L"Alt+Dn  -- same as ]"       );

  mb.push( L"Alt+#   -- where # is 1..9, go to #'th bookmark"                    );
  mb.push( L"vfu uses these (one of) these config files:");
  mb.push( L"        1. $HOME/$RC_PREFIX/vfu/vfu.conf");
  mb.push( L"        2. $HOME/.vfu/vfu.conf");
  mb.push( L"        3. " FILENAME_CONF_GLOBAL0 );
  mb.push( L"        4. " FILENAME_CONF_GLOBAL1 );
  mb.push( L"        5. " FILENAME_CONF_GLOBAL2 );
  mb.push( L"" );
  vfu_menu_box( 1, 4, L"VFU Help ( PageUp/PageDown to scroll )" );
  mb.undef();
  do_draw = 1;
}

/*--------------------------------------------------------------------------*/

void vfu_init()
{
  char t[MAX_PATH];

  if( expand_path( "." ) == "" ) 
    if( chdir( "/" ) )
      {
      say1( "Cannot chdir to: /" );
      say2errno();
      }

  work_mode = WM_NORMAL;
  if( ! getcwd( t, sizeof(t) ) ) t[0] = 0;
  str_fix_path( t );
  work_path = t;

  archive_name = "";
  archive_path = ""; /* NOTE: archives' root directory is `' but not `/'! */

  external_panelizer = "";

  files_list_clear();

  files_size = 0;
  sel_count  = 0;
  sel_size   = 0;

  fs_free    = 0;
  fs_total   = 0;
  fs_block_size = 0;

  file_list_index.wrap = 0;
  /* file_list_index.* are setup by vfu_read_files() */

  user_id_str  = get_user_id_str( getuid() );
  group_id_str = get_group_id_str( getgid() );

  gethostname( t, MAX_PATH-1 );
  host_name_str = t;

  startup_path = work_path;

  tmp_path = "";
  if ( tmp_path == "" ) tmp_path = getenv( "TEMP" );
  if ( tmp_path == "" ) tmp_path = getenv( "TMP" );
  if ( tmp_path == "" ) tmp_path = "/tmp/";
  str_fix_path( tmp_path );

  if ( getenv( "HOME" ) )
    home_path = getenv( "HOME" );
  else
    {
    home_path = tmp_path + user_id_str + "/";
    make_path( home_path );
    }

  shell_diff = "/usr/bin/diff"; // TODO: FIXME: get from config file or environment

  /*
   FIXME: this should something relevant to the home_path
   from above if $HOME does not exist(?) well still can
   accept /tmp/ as it is default now
  */

  rc_path = get_rc_directory( "vfu" );

  /* setup config files locations */
  filename_opt        = rc_path + FILENAME_OPT;
  filename_conf       = rc_path + FILENAME_CONF;
  filename_tree       = rc_path + FILENAME_TREE;
  filename_size_cache = rc_path + FILENAME_SIZE_CACHE;
  filename_history    = rc_path + FILENAME_HISTORY;
  filename_ffr        = rc_path + FILENAME_FFR;

  VArray conf_data;
  if ( access( filename_conf, R_OK ) != 0 )
    { /* cannot find local/user conf file, try copy one */
    if      ( access( FILENAME_CONF_GLOBAL0, R_OK ) == 0 ) conf_data.fload( FILENAME_CONF_GLOBAL0 );
    else if ( access( FILENAME_CONF_GLOBAL1, R_OK ) == 0 ) conf_data.fload( FILENAME_CONF_GLOBAL1 );
    else if ( access( FILENAME_CONF_GLOBAL2, R_OK ) == 0 ) conf_data.fload( FILENAME_CONF_GLOBAL2 );
    conf_data.fsave( filename_conf );
    }

  /* shell setup */
  shell_prog = "";
  if ( shell_prog == "" ) shell_prog = getenv( "VFU_SHELL" );
  if ( shell_prog == "" ) shell_prog = getenv( "SHELL"     );

  /* this will load defaults first then load vfu.opt and at the
     end will load vfu.conf which will overwrite all if need to */
  vfu_settings_load( conf_data.count() > 0 ? &conf_data : NULL );
  size_cache_load();

  file_list_index.wrap = 0; /* just to be safe :) */

  files_mask = "*";
  files_mask_array = str_split( " ", files_mask );

  view_profiles.push( "123456" );
  view_profile = "123456";

  /* setup menu colors */
  menu_box_info.ti = 95; /* title */
  menu_box_info.cn = 23; /* normal */
  menu_box_info.ch = 47; /* selected */

  signal( SIGINT  , vfu_signal );
  signal( SIGHUP  , vfu_signal );
  signal( SIGTERM , vfu_signal );
  signal( SIGQUIT , vfu_signal );
  signal( SIGWINCH, vfu_signal ); // already set in unicon/vslib

  srand( time( NULL ) );
  do_draw = 1;

  vfu_read_files();
}

/*--------------------------------------------------------------------------*/

void vfu_exit_path( const char *a_path )
{
  if( chdir( a_path ) )
    {
    say1( VString( "Cannot chdir to: " ) + a_path );
    say2errno();
    }

  VString str;
  if ( getenv( "VFU_EXIT" ) )
    str = getenv( "VFU_EXIT" );
  else
    str = tmp_path + "vfu.exit." + user_id_str;

  unlink( str );
  int fdx = open( str, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR );
  if( fdx == -1 ) return;
  if( write( fdx, a_path, str_len( a_path ) ) ) // stupid, stupid -Wunused-result
  {
  }
  close( fdx );  
}

/*--------------------------------------------------------------------------*/
/* return 0 for exit-confirmed! */
int vfu_exit( const char* a_path )
{
  mb.undef();
  mb.push( L"X Exit (to startup path)" );
  mb.push( L"Q Quit (to work path)   " );

  if ( a_path == NULL )
    {
    vfu_beep();
    int z = vfu_menu_box( 50, 5, L"  Exit VFU?" );
    if ( z == -1 ) return 1;
    z ? vfu_exit_path( work_path ) : vfu_exit_path( startup_path );
    return 0;
    }
  else
    {
    vfu_exit_path( a_path );
    return 0;
    }
}

/*--------------------------------------------------------------------------*/

void vfu_run()
{
  say1center( HEADER );
  say2center( CONTACT );

  /* int oldFLI = -1; // quick view */
  while (4)
    {
    if (do_draw)
      {
      if ( do_draw > 2 ) vfu_reset_screen();
      if ( do_draw > 1 ) do_draw_status = 1;
      vfu_redraw();
      do_draw = 0;
      }
    if (do_draw_status)
      {
      vfu_redraw_status();
      do_draw_status = 0;
      }
    /*
    TODO: quick view?...
    if ( work_mode == WM_NORMAL && files_list_count() > 0 && oldFLI != FLI )
      {
      oldFLI = FLI;
      const char* fn = files_list_get(FLI)->full_name();
      file_save( "/tmp/vfu-quick-view", (void*)fn, strlen( fn ) );
      }
    */
    show_pos( FLI+1, files_list_count() ); /* FIXME: should this be in vfu_redraw()? */

    wchar_t wch = con_getwch();

    if( wch == 0 || wch == UKEY_RESIZE ) wch = UKEY_CTRL_L;
    if ( wch >= 'A' && wch <= 'Z' ) wch = towlower( wch );
    say1( "" );
    if ( user_id_str == "root" )
      say2center( "*** WARNING: YOU HAVE GOT ROOT PRIVILEGES! ***" );
    else
      say2( "" );
    // say( 1, cMESSAGE, "key/wch: %lX", wch );

    if ( work_mode == WM_NORMAL || work_mode == WM_ARCHIVE ) switch (wch)
      { /* actually this is ANY work_mode (since there are two modes only) */
      case L'1'       :
      case L'2'       :
      case L'3'       :
      case L'4'       :
      case L'5'       :
      case L'6'       :
      case L'7'       :
      case L'8'       :
      case L'0'       : vfu_toggle_view_fields( wch ); break;

      case L'.'       : vfu_toggle_view_fields( wch );
                        vfu_rescan_files( 0 ); break;

      case L's'       : vfu_inc_search( 0 ); break;
      case UKEY_ALT_S : vfu_inc_search( 1 ); break;

      case UKEY_CTRL_L: do_draw = 3; break;

      case L'q'       : if( vfu_exit( work_path ) == 0 ) return; break;

      case UKEY_ALT_X :
      case L'x'       : if( vfu_exit( startup_path ) == 0 ) return; break;

      case 27        : if( vfu_exit( NULL ) == 0 ) return; break;

      case UKEY_UP    : vfu_nav_up(); break;
      case UKEY_DOWN  : vfu_nav_down(); break;
      case UKEY_PGUP  : vfu_nav_ppage(); break;
      case UKEY_PGDN  : vfu_nav_npage(); break;

      case UKEY_CTRL_A:
      case UKEY_HOME  : vfu_nav_home(); break;
      
      case UKEY_CTRL_E:
      case UKEY_END   : vfu_nav_end(); break;

      case L'h' : vfu_help(); break;

      case L'f'        : vfu_change_file_mask( NULL ); break;
      case UKEY_CTRL_F : vfu_change_file_mask( "*" ); break;

      case UKEY_CTRL_D : tree_view(); break;
      case UKEY_ALT_R  : vfu_read_files_menu(); break;

      /* this will be in alt+r menu
      case 'R' : con_cs(); vfu_refresh_all_views(); do_draw = 1; break;
      */
      case UKEY_CTRL_R : vfu_rescan_files( 1 ); break;
      case L'r'        : vfu_rescan_files( 0 ); break;

      case L' ' : vfu_nav_select(); break;

      case UKEY_BACKSPACE :
      case 8   :
      case L'-' : vfu_action_minus(); break;

      case UKEY_ALT_BACKSPACE :
      case UKEY_ALT_MINUS :
                 vfu_action_minus( 2 ); break;

      case 13  :
      case L'+' :
      case L'=' : vfu_action_plus( wch ); break;

      case UKEY_ALT_UP:
      case L'[' : vfu_action_minus();
                  vfu_nav_up();
                  vfu_action_plus( 13 );
                  break;

      case UKEY_ALT_DOWN:
      case L']' : vfu_action_minus();
                  vfu_nav_down();
                  vfu_action_plus( 13 );
                  break;

      case UKEY_LEFT  : if (opt.lynx_navigation) vfu_action_minus(); break;
      case UKEY_RIGHT : if (opt.lynx_navigation)
                         vfu_action_plus( L'+' );
                       else
                         if ( work_mode == WM_NORMAL )
                           vfu_rename_file_in_place();
                       break;

      case L'd' : vfu_chdir( NULL ); break;
      case UKEY_ALT_D : vfu_chdir_history(); break;

      case UKEY_ALT_EQ :
      case L'>' : opt.long_name_view = !opt.long_name_view;
                  vfu_drop_all_views();
                  do_draw = 1;
                  break;

      case L'a' : vfu_arrange_files(); break;

      case L'g' : vfu_global_select(); break;

      case L'o' : vfu_options(); 
                  vfu_drop_all_views();
                  do_draw = 1;
                  break;

      case L'v' : vfu_edit_conf_file(); break;

      case L'!' :
      case L'?' : con_cs();
                 vfu_shell( shell_prog, 0 );
                 do_draw = 3;
                 break;

      case L'u'        : vfu_user_menu(); break;


      /* not documented unless here :) */
      case UKEY_CTRL_T  :
        {
        char s[128];
        say1( "Timing screen draws (x1000)..." );
        clock_t t = clock();
        for(int z = 0; z < 1000; z++) vfu_redraw();
        t = clock() - t;
        snprintf(s, sizeof(s), "Draw speed: %f dps.",(1000.0/((double)t/CLOCKS_PER_SEC)));
        say1(s);
        break;
        }

      case L'*' : FLGO( rand() % files_list_count() );
                 do_draw = 1;
                 break;

      case L'z'        : vfu_directory_sizes(  0  ); break;
      case UKEY_ALT_Z  : vfu_directory_sizes( L'A' ); break;
      case UKEY_CTRL_Z : vfu_directory_sizes( L'Z' ); break;
      }
    if ( work_mode == WM_ARCHIVE ) switch (wch)
      {
      case L'c' : vfu_extract_files( 0 ); break;
      case UKEY_ALT_C : vfu_extract_files( 1 ); break;
      }
    if ( work_mode == WM_NORMAL ) switch (wch)
      {
      case L'b' :
      case UKEY_ALT_B : if ( wch == L'b' && sel_count > 0 )
                         vfu_browse_selected_files();
                       else
                         {
                         if ( files_list_count() > 0 )
                           vfu_browse( FLCUR->name(), wch == UKEY_ALT_B );
                         else
                           say1( "No files" );
                         }
                       break;

      case L'n' : vfu_file_find( 0 ); break;
      case UKEY_ALT_N  : vfu_file_find( 1 ); break;

      case L'~' : vfu_chdir( home_path ); break;

      case L'/' : vfu_command(); break;

      case 'i' : if ( files_list_count() > 0 )
                   vfu_edit( files_list_get(FLI)->name() );
                 else
                   say1( "No files");
                 break;

      case 'm'        : vfu_copy_files(sel_count == 0, CM_MOVE); break;
      case UKEY_ALT_M : vfu_copy_files(1, CM_MOVE); break;

      case 'c'        : vfu_copy_files(sel_count == 0, CM_COPY); break;
      case UKEY_ALT_C : vfu_copy_files(1, CM_COPY); break;

      case 'l'        : vfu_copy_files(sel_count == 0, CM_LINK); break;
      case UKEY_ALT_L : vfu_copy_files(1, CM_LINK); break;

      case 'e'        : vfu_erase_files(sel_count == 0); break;
      case UKEY_ALT_E : vfu_erase_files(1); break;

      case 'j'        : vfu_jump_to_mountpoint( 0 ); break;
      case UKEY_ALT_J : vfu_jump_to_mountpoint( 1 ); break;

      case UKEY_ALT_1 : bookmark_goto( L'1' ); break;
      case UKEY_ALT_2 : bookmark_goto( L'2' ); break;
      case UKEY_ALT_3 : bookmark_goto( L'3' ); break;
      case UKEY_ALT_4 : bookmark_goto( L'4' ); break;
      case UKEY_ALT_5 : bookmark_goto( L'5' ); break;
      case UKEY_ALT_6 : bookmark_goto( L'6' ); break;
      case UKEY_ALT_7 : bookmark_goto( L'7' ); break;
      case UKEY_ALT_8 : bookmark_goto( L'8' ); break;
      case UKEY_ALT_9 : bookmark_goto( L'9' ); break;
      case '`'        : bookmark_goto( 0 ); break;

      case 9          : vfu_edit_entry(); break;

      case 't'        : vfu_tools(); break;

      case 'p'        : clipboard_menu( 0 ); break;
      /*
      case KEY_CTRL_C : vfu_clipboard( 'C' ); break; // copy
      case KEY_CTRL_X : vfu_clipboard( 'X' ); break; // cut
      case KEY_CTRL_V : vfu_clipboard( 'V' ); break; // paste
      */

      }
    if (  ( UKEY_F1      <= wch && wch <= UKEY_F10      )
       || ( UKEY_SH_F1   <= wch && wch <= UKEY_SH_F10   )
       || ( UKEY_ALT_F1  <= wch && wch <= UKEY_ALT_F10  )
       || ( UKEY_CTRL_F1 <= wch && wch <= UKEY_CTRL_F10 )
       || ( wch == UKEY_INS) )
           vfu_user_external_exec( wch );
    }
}

/*--------------------------------------------------------------------------*/

void vfu_help_cli()
{
  printf( "%s",
    HEADER "\n"
    "Command line switches:\n"
    "  none    -- run in interactive mode (DEFAULT)\n"
    "  -h      -- this help screen\n"
    "  -i      -- go temporarily into interactive mode\n"
    "  -d path -- change current path to `path'\n"
    "  -r      -- rebuild DirTree (under DOS -- tip: see -d)\n"
    "  -t      -- view DirTree\n"
    "  -v      -- version information\n"
    "tips:\n"
    "  1. command line switches are executed in order!\n"
    "  2. example: vfu -d c:/dos/ -r -i\n"
    "  3. example: vfu -d c:/ -r -d d:/ -r -d e:/ -r\n"
    "compile information:\n"
    "  target description: " _TARGET_DESCRIPTION_ "\n"
    );
}

/*--------------------------------------------------------------------------*/

void vfu_cli( int argc, char* argv[] )
{
  VString temp;
  GETOPT((char*)"hrd:ti")
    {
    switch(optc)
      {
      case 'h'  : print_help_on_exit = 1; break;
      case 'i'  : vfu_run(); break;
      case 'd'  : temp = optarg;
                  vfu_chdir( temp );
                  break;
      case 'r'  : con_out(1,1,HEADER,cINFO);
                  temp = "Rebuilding directory tree ( work_path is";
                  temp += work_path;
                  temp += " )";
                  say2( temp );
                  tree_rebuild();
                  break;
      case 't'  : vfu_con_out(1,1,HEADER,cINFO);
                  tree_view();
                  vfu_exit( work_path );
                  break;
      default:
                  vfu_help_cli();
                  break;
      }
    }
}

/*--------------------------------------------------------------------------*/

void vfu_done()
{
  /* if dir_tree.count is 0 don't save -- there's nothing to be saved */
  if ( dir_tree.count() && dir_tree_changed ) tree_save();

  vfu_settings_save();
}

/*--------------------------------------------------------------------------*/

void vfu_soft_reset_screen()
{
  /* update scroll parameters */
  file_list_index.set_min_max( 0, files_list_count() - 1 );
  file_list_index.set_pagesize( con_max_y() - 7 );
  FLGO( file_list_index.pos() );

  vfu_drop_all_views();
  vfu_redraw();
  vfu_redraw_status();

  say1( "" );
  say2( "" );
}

void vfu_reset_screen()
{
  con_done();
  con_init();
  con_chide();

  vfu_soft_reset_screen();
}

void vfu_signal( int sig )
{
  if( sig == SIGWINCH ) 
      return vfu_reset_screen();
  
  vfu_done();

  con_beep();
  con_cs();
  con_cshow();
  con_done();

  printf( "vfu: signal received: %d -- terminated\n", sig );
  exit(200);
}

/*--------------------------------------------------------------------------*/

void vfu_toggle_view_fields( wchar_t wch )
{
  switch( wch )
    {
    case L'1' : opt.f_mode  = !opt.f_mode; break;
    case L'2' : opt.f_owner = !opt.f_owner; break;
    case L'3' : opt.f_group = !opt.f_group; break;
    case L'4' : opt.f_time  = !opt.f_time; break;
    case L'5' : opt.f_size  = !opt.f_size; break;
    case L'6' : opt.f_type  = !opt.f_type; break;
    case L'7' : opt.f_time_type++;
               if (opt.f_time_type > 2)
                 opt.f_time_type = 0;
               break;
    case L'8' : opt.f_mode  =
                opt.f_owner =
                opt.f_group =
                opt.f_time  =
                opt.f_size  =
                opt.f_type  = 1; break;
    case L'0' : opt.long_name_view = !opt.long_name_view; break;
    case L'.' : opt.show_hidden_files = !opt.show_hidden_files; break;
    default   : return; /* unreachable */
    }
  vfu_drop_all_views();
}

/*--------------------------------------------------------------------------*/

void vfu_shell( const char* a_command, const char* a_options )
{
  VString shell_line = a_command;
  VString o = a_options;

  VString status = "";

  int res = vfu_update_shell_line( shell_line, o );
  if (res) return;

  if ( str_find( o, '!' ) > -1 )
    {
    // review shell_line
    say1( "Review shell line to be executed:" );
    VString sl = shell_line;
    sl = str_dot_reduce( sl, con_max_x() - 1 );
    say2( sl );
    con_getwch();
    }

  if ( str_find( o, 'n' ) == -1 ) /* [n]o console suspend */
    {
    con_cs();
    con_xy( 1, 1 );
    con_cshow();
    con_suspend();
    }

  res = system( shell_line );
  if ( res )
    {
    sprintf( status, "*** execution failed, system() == %d ***", res );
    }

  if ( str_find( o, 'w' ) != -1 ) /* [w]ait after shell */
    {
    printf( "*** press enter ***" );
    fflush( stdout );
    fgetc( stdin );
    }

  if ( str_find( o, 'n' ) == -1 ) /* [n]o console suspend */
    {
    con_restore();
    con_chide();
    con_cs();
    }

  if( chdir( work_path ) ) /* in case SHELL changed directory... (DOS only :)) */
    {
    say1( VString( "Cannot chdir to: " ) + work_path );
    say2errno();
    }

  if ( str_find( o, 'r' ) != -1 ) vfu_rescan_files();

  do_draw = 2;

  if ( str_find( o, 'n' ) != -1 ) do_draw = 0;
  if ( str_find( o, 'i' ) != -1 ) vfu_nav_down();

  say1("");
  say2( status );
}

/*--------------------------------------------------------------------------*/

void update_status()
{
  int z;
  fsize_t s;
  sel_count = 0;
  sel_size = 0;
  files_size = 0;
  /* files list statistics */
  for( z = 0; z < files_list_count(); z++ )
    {
    s = files_list_get(z)->size();
    if ( files_list_get(z)->sel )
      {
      sel_size += s;
      sel_count++;
      }
    files_size += s;
    }
  /* current fs statistics */
  struct statfs stafs;
  statfs( ".", &stafs );
  fs_free  = (fsize_t)(stafs.f_bsize) * (opt.show_user_free?stafs.f_bavail:stafs.f_bfree);
  fs_total = (fsize_t)(stafs.f_bsize) * stafs.f_blocks;
  fs_block_size = (fsize_t)(stafs.f_bsize);
  do_draw_status = 1;
}

/*--------------------------------------------------------------------------*/

void vfu_edit( const char *fname )
{
  if ( files_list_count() == 0 )
    {
    say1( "No files");
    return;
    }
  if ( FLCUR->is_dir() )
    {
    say1( "Cannot edit directory");
    return;
    }
  con_cs();
  if ( opt.internal_editor )
    {
    opt.seo.cs = cINFO;
    SeeEditor editor( &opt.seo );
    if( editor.open( fname ) == 0 )
      {
      int r = 1;
      while ( r )
        {
        editor.run();
        r = editor.request_quit();
        }
      }
    else
      say1( "Error loading file..." );
    editor.close();
    }
  else
    {
    VString str = shell_editor;
    if ( fname )
      {
      str_replace( str, "%f", shell_escape( fname ) );
      str_replace( str, "%F", shell_escape( fname ) );
      }
    vfu_shell( str, "" );
    }
  do_draw = 2;
  say1("");
  say2("");
}

/*--------------------------------------------------------------------------*/

void vfu_browse_selected_files()
{
  /*
  FIXME: multiple files browsing should be back ASAP
  #define VFU_BROWSE_MAX_FILES 10
  int i;  // index
  int ic; // count

  for ( z = 0; z < files_list_count(); z++ )
    if ( files_list_get(z)->sel )
      if ( !files_list_get(z)->is_dir() )
        SeeAddFile( files_list_get(z)->full_name() );
  //------
  int z;
  for ( z = 0; z < files_list_count(); z++ )
    if ( files_list_get(z)->sel )
      if ( !files_list_get(z)->is_dir() )
        SeeAddFile( files_list_get(z)->full_name() );
  SeeSLColor = cINFO;
  See();
  do_draw = 2;
  say1("");
  say2("");
  */
}

void vfu_browse( const char *fname, int no_filters )
{
  VString new_name = fname;
  VString tmp_name;

  char full_fname[MAX_PATH];
  expand_path( fname, full_fname );

  if ( ! no_filters && see_filters.count() > 0 )
    {
    int z;
    for ( z = 0; z < see_filters.count(); z++ )
      {
      VArray split;
      split = str_split( ",", see_filters[z] );
      VString mask = split[0];
      VString str  = split[1];
      if ( FNMATCH( mask, str_file_name_ext( fname ) ) ) continue;
      /* found */
      tmp_name = vfu_temp();
      str_replace( str, "%f", shell_escape( fname     ) );
      str_replace( str, "%F", shell_escape( full_fname ) );
      str += " > " + tmp_name;
      vfu_shell( str, "" );
      chmod( tmp_name, S_IRUSR | S_IWUSR );
      break;
      }
    }

  if ( tmp_name != "" )
    new_name = tmp_name;

  if ( opt.internal_browser )
    {
    opt.svo.cs = cINFO;
    SeeViewer viewer( &opt.svo );

    if( viewer.open( new_name ) == 0 )
      viewer.run();
    else
      say1( "Error loading file..." );
    viewer.close();
    }
  else
    {
    VString str = shell_browser;
    if ( fname )
      {
      str_replace( str, "%f", shell_escape( fname     ) );
      str_replace( str, "%F", shell_escape( full_fname ) );
      }
    vfu_shell( str, "" );
    }
  do_draw = 2;
  say1("");
  say2("");

  if ( tmp_name != "" )
    unlink( tmp_name );
}

/*--------------------------------------------------------------------------*/

void vfu_action_plus( wchar_t wch )
{
  if ( files_list_count() == 0 ) return;

  TF *fi = FLCUR;

  if ( work_mode == WM_NORMAL )
    {
    if ( fi->is_dir() )
      { /* directory */
      vfu_chdir( fi->name() );
      }
    else
      { /* file */
      int z;
      for ( z = 0; z < archive_extensions.count(); z++ )
        if ( FNMATCH_OC( archive_extensions[z], fi->name_ext(), opt.lower_case_ext_config ) == 0 )
          {
          FNMATCH_OC( archive_extensions[z], fi->name_ext(), opt.lower_case_ext_config );
          z = -1; /* FIXME: this shouldn't be -1 for TRUE really :) */
          break;
          }
      if ( z == -1 )
        { /* yep this is archive */
        work_mode = WM_ARCHIVE;
        archive_name = fi->name();
        archive_path = ""; /* NOTE: archives' root dir is `' but not `/'! */
        vfu_read_files();
        say1( "ARCHIVE mode activated ( some keys/commands are disabled! )" );
        } else
      if ( wch == UKEY_ENTER && vfu_user_external_find( UKEY_ENTER, fi->ext(), fi->type_str(), NULL ) != -1 )
        vfu_user_external_exec( UKEY_ENTER );
      else
        vfu_browse( fi->name() );
      }
    }
  else
    { /* work_mode == WM_ARCHIVE */
    if ( fi->is_dir() )
      { /* directory */
      VString p = fi->name();
      str_fix_path( p );
      archive_path += p;
      vfu_read_files();
      }
    else
    if ( wch == UKEY_ENTER && vfu_user_external_find( UKEY_ENTER, fi->ext(), fi->type_str(), NULL ) != -1 )
        vfu_user_external_exec( UKEY_ENTER );
    else
      { /* file */
      vfu_browse_archive_file();
      }
    }
}

/*--------------------------------------------------------------------------*/

void vfu_action_minus( int mode )
{
  VString o = work_path; /* save old path i.e. current */

  if ( work_mode == WM_NORMAL )
    {
      if ( work_path[0] == '/' && work_path[1] == 0 ) return;
      vfu_chdir( ".." );
    } else
  if ( work_mode == WM_ARCHIVE )
    {
    if( mode == 2 ) archive_path = "";
    
    if ( str_len( archive_path ) > 0 )
      {
      str_cut_right( archive_path, "/" );
      int z = str_rfind( archive_path, "/" );
      o  = archive_path;
      o += "/";
      if ( z != -1 )
        str_sleft(archive_path,z+1);
      else
        archive_path = "";
      vfu_read_files();
      }
    else
      {
      o += archive_name;
      o += "/"; /* FIXME: is this required ? */
      work_mode = WM_NORMAL;
      archive_name = "";
      archive_path = ""; /* NOTE: this shouldn't be `/'! */
      vfu_chdir( "." );
      }
    }

  int z = 0;
  for ( z = 0; z < files_list_count(); z++ )
    {
    VString n;
    if ( work_mode == WM_ARCHIVE )
      n = archive_path;
    else
      n = work_path;
    n += files_list_get(z)->name();
    n += "/";
    if ( pathcmp( o, n ) == 0 )
      {
      FLGO(z);
      break;
      }
    }
  do_draw = 1;
}

/*--------------------------------------------------------------------------*/

// global select / different
// returns 0 for ok
int vfu_cmp_files_crc32( const char* src, const char* dst, const char* name )
{
  fname_t fn1;
  fname_t fn2;
  struct stat stat_src;
  struct stat stat_dst;
  strcpy( fn1, src ); strcat( fn1, name );
  strcpy( fn2, dst ); strcat( fn2, name );

  if (access( fn1, F_OK )) return 1;
  if (access( fn2, F_OK )) return 2;

  if (stat( fn1, &stat_src )) return 3;
  if (stat( fn2, &stat_dst )) return 4;

  if (S_ISDIR( stat_src.st_mode )) return 5;
  if (S_ISDIR( stat_dst.st_mode )) return 6;

  if ( stat_src.st_size != stat_dst.st_size ) return 7;

  if ( file_crc32( fn1 ) != file_crc32( fn2 ) ) return 8;

  return 0;
}

#define GSAME_NAME      1
#define GSAME_EXT       2
#define GSAME_SIZE      3
#define GSAME_DATETIME  4
#define GSAME_DATE      5
#define GSAME_TIME      6
#define GSAME_TIME1     7
#define GSAME_OWNER     8
#define GSAME_GROUP     9
#define GSAME_MODE     10
#define GSAME_TYPE     11

#define TIMECMP_DT     0 // compare date and time
#define TIMECMP_D      1 // compare only date
#define TIMECMP_T      2 // compare only time
#define TIMECMP_T1     3 // compare only time (to 1 minute round)

// return 0=don't match and 1=match
int vfu_time_cmp( time_t t1, time_t t2, int type = TIMECMP_DT )
{
  char tmp1[32];
  char tmp2[32];
  strcpy( tmp1, ctime(&t1) );
  strcpy( tmp2, ctime(&t2) );
  if ( type == TIMECMP_T )
    {
    strcpy( tmp1, tmp1+11 ); tmp1[8] = 0;
    strcpy( tmp2, tmp2+11 ); tmp2[8] = 0;
    } else
  if ( type == TIMECMP_T1 )
    {
    strcpy( tmp1, tmp1+11 ); tmp1[5] = 0;
    strcpy( tmp2, tmp2+11 ); tmp2[5] = 0;
    } else
  if ( type == TIMECMP_D )
    {
    strcpy( tmp1+10, tmp1+19 );
    strcpy( tmp2+10, tmp2+19 );
    }
  return (strcmp( tmp1, tmp2 ) == 0);
}

void vfu_global_select_same( int same_mode )
{

VString same_str;
long same_int = 0;
fsize_t same_fsize = 0;

TF* fi = FLCUR;

switch( same_mode )
  {
  case GSAME_NAME  : same_str = fi->name(); break;
  case GSAME_EXT   : same_str = fi->ext(); break;
  case GSAME_SIZE  : same_fsize = fi->size(); break;
  case GSAME_DATETIME  :
  case GSAME_DATE      :
  case GSAME_TIME      :
  case GSAME_TIME1     :
                     same_int = vfu_opt_time( fi->st() );
                     break;
  case GSAME_OWNER : same_int = fi->st()->st_uid; break;
  case GSAME_GROUP : same_int = fi->st()->st_gid; break;
  case GSAME_MODE  : same_int = fi->st()->st_mode; break;
  case GSAME_TYPE  : same_str = fi->type_str(); break;
  default          : return;
  }

int z = 0;
for (z = 0; z < files_list_count(); z++)
  {
  fi = files_list_get(z);
  int sel = 0;
  switch( same_mode )
    {
    case GSAME_NAME  : sel = (pathcmp(same_str, fi->name()) == 0);
                       break;
    case GSAME_EXT   : sel = (pathcmp(same_str, fi->ext()) == 0);
                       break;
    case GSAME_SIZE  : sel = (same_fsize == fi->size());
                       if ( fi->is_dir() ) sel = 0; 
                       break;
    case GSAME_DATETIME  :
                       sel = vfu_time_cmp(same_int, vfu_opt_time( fi->st()));
                       break;
    case GSAME_DATE      :
                       sel = vfu_time_cmp(same_int, vfu_opt_time( fi->st() ),
                                    TIMECMP_D );
                       break;
    case GSAME_TIME      :
                       sel = vfu_time_cmp(same_int, vfu_opt_time( fi->st() ),
                                    TIMECMP_T );
                       break;
    case GSAME_TIME1     :
                       sel = vfu_time_cmp(same_int, vfu_opt_time( fi->st() ),
                                      TIMECMP_T1 );
                       break;
    case GSAME_OWNER : sel = ((unsigned int)same_int == fi->st()->st_uid); 
                       break;
    case GSAME_GROUP : sel = ((unsigned int)same_int == fi->st()->st_gid); 
                       break;
    case GSAME_MODE  : sel = ((unsigned int)same_int == fi->st()->st_mode); 
                       break;
    case GSAME_TYPE  : sel = ( same_str == fi->type_str()); 
                       break;
    }
  fi->sel = sel;
  }
}

void vfu_global_select()
{
  mb.undef();
  mb.push( L"S All" );
  mb.push( L"A All (+Dirs)" );
  mb.push( L"R Reverse selection" );
  mb.push( L"C Clear selection" );
  mb.push( L"P Pack (list selected only)" );
  mb.push( L"H Hide selected" );
  mb.push( L"D Different" );
  mb.push( L". Hide dotfiles" );
  mb.push( L", Hide dirs" );
  mb.push( L"= Select by mask (with directories)" );
  mb.push( L"+ Select by mask (w/o  directories)" );
  mb.push( L"- Deselect by mask" );
  mb.push( L"L Select Same..." );
  mb.push( L"X Extended Select..." );
  if ( vfu_menu_box( 30, 5, L"Select Files and Directories" ) == -1 ) return;
  wchar_t wch = menu_box_info.ec;
  if ( wch == L'X')
    {
    if ( work_mode != WM_NORMAL )
      {
      say1( "Extended Select is not available in this mode." );
      return;
      }
    mb.undef();
    mb.push( L"A Select to the begin of the list" );
    mb.push( L"E Select to the end of the list" );
    mb.push( L"--searching--" );
    mb.push( L"F Find string (no case)" );
    mb.push( L"S Scan string (case sense)" );
    mb.push( L"H Hex  string" );
    mb.push( L"/ Regular expression" );
    mb.push( L"\\ Reg.exp (no case)" );
//    mb.push( "--other--" );
//    mb.push( "M Mode/Attributes" );
    if ( vfu_menu_box( 32, 6, L"Extended Select" ) == -1 ) return;
    wch = menu_box_info.ec;
    if (wch == L'S') wch = L'B'; /* 'B' trans scan */
    if (wch == L'H') wch = L'E'; /* 'E' trans hex  */
    if (wch == L'A') wch = L'<'; /* '<' trans to begin  */
    if (wch == L'E') wch = L'>'; /* '>' trans to end    */
    }

  switch(wch)
    {
    case L'S' : {
               for (int z = 0; z < files_list_count(); z++)
                 if (!files_list_get(z)->is_dir())
                   files_list_get(z)->sel = 1;
               } break;
    case L'A' : {
               for (int z = 0; z < files_list_count(); z++)
                 files_list_get(z)->sel = 1;
               } break;
    case L'R' : {
               int z;
               for (z = 0; z < files_list_count(); z++)
                 if (!files_list_get(z)->is_dir())
                   files_list_get(z)->sel = !files_list_get(z)->sel;
               } break;
    case L'C' : {
               int z;
               for (z = 0; z < files_list_count(); z++)
                   files_list_get(z)->sel = 0;
               } break;
    case L'P' :
               {
               int z;
               for (z = 0; z < files_list_count(); z++)
                 {
                 if (!files_list_get(z)->sel)
                   files_list_del(z);
                 }
               files_list_pack();
               } break;
    case L'H' :
               {
               int z;
               for (z = 0; z < files_list_count(); z++)
                 {
                 if (files_list_get(z)->sel)
                   files_list_del(z);
                 }
               files_list_pack();
               } break;
    case L',' :
               {
               int z;
               for (z = 0; z < files_list_count(); z++)
                 {
                 if (files_list_get(z)->is_dir())
                   files_list_del(z);
                 }
               files_list_pack();
               } break;
    case L'.' :
               {
               int z;
               for (z = 0; z < files_list_count(); z++)
                 {
                 if (files_list_get(z)->name()[0] == '.')
                   files_list_del(z);
                 }
               files_list_pack();
               } break;
    case L'+' :
    case L'=' :
    case L'-' :
              {
              VString mask;
              int selaction = 0;
              if (wch != L'-') selaction = 1;
              if (wch == L'+')
                say1("Select by mask: (w/o directories)");
              else
              if (wch == '=')
                say1("Select by mask: (with directories)");
              else
                say1("Deselect by mask:");
              if ( vfu_get_str( "", mask, HID_GS_MASK ))
                {
                VArray mask_array = str_split( " +", mask );
                while( ( mask = mask_array.pop() ) != "" )
                  {
                  if (opt.mask_auto_expand)
                    vfu_expand_mask( mask );
                  int z = 0;
                  for (z = 0; z < files_list_count(); z++)
                    {
                    if ( files_list_get(z)->is_dir() && wch == L'+') continue;
                    if ( FNMATCH( mask, files_list_get(z)->name_ext() ) == 0)
                      files_list_get(z)->sel = selaction;
                    }
                  }
                }
              say1( " " );
              say2( " " );
              } break;
    case L'D' :
              {
              if ( work_mode != WM_NORMAL )
                {
                say1( "GlobalSelect/Different not available in this mode." );
                break;
                }
              VString target;
              if ( vfu_get_dir_name( "Target directory:", target ))
                {
                str_fix_path( target );
                int z = 0;
                for (z = 0; z < files_list_count(); z++)
                  {
                  if ( files_list_get(z)->is_dir() ) continue;
                  if ( vfu_break_op() ) break;
                  say1( files_list_get(z)->name() );
                  files_list_get(z)->sel =
                      (vfu_cmp_files_crc32( work_path, target,
                        files_list_get(z)->name() ) != 0 );
                  }
                }
              say1( "Done." );
              say2( " " );
              } break;
    case L'/':
    case L'\\':
    case L'E':
    case L'F':
    case L'B': {
              say1("");
              VString pat;
              if ( vfu_get_str( "Search string: ", pat, HID_GS_GREP ) )
                {
                fsize_t size = 0;
                say1("");
                say2("");

                size = 0;
                for ( int z = 0; z < files_list_count(); z++ )
                  {       
                  size += files_list_get(z)->size();
                  if ( files_list_get(z)->is_dir() ) continue;

                  int pos = -1;
                  switch( wch )
                    {
                    case L'F':
                       pos = file_string_search( pat, files_list_get(z)->name(), "i" );
                       break;
                    case L'B':
                       pos = file_string_search( pat, files_list_get(z)->name(), "" );
                       break;
                    case L'E':
                       pos = file_string_search( pat, files_list_get(z)->name(), "h" );
                       break;
                    case L'/':
                       pos = file_string_search( pat, files_list_get(z)->name(), "r" );
                       break;
                    case L'\\':
                       pos = file_string_search( pat, files_list_get(z)->name(), "ri" );
                       break;
                    }

                  files_list_get(z)->sel = ( pos > -1 );

                  char s[128];
                  snprintf( s, sizeof(s),
                              "Scanning %4.1f%% (%12.0f bytes in %s ) ",
                              (100.0 * size) / (files_size+1.0),
                              files_list_get(z)->size(),
                              files_list_get(z)->name() );
                  say1( s );
                  }
                }
              say1("");
              say2("");
              break;
              }

    case L'L':
              {
              mb.undef();
              mb.push( L"N Name" );
              mb.push( L"E Extension" );
              mb.push( L"S Size" );
              mb.push( L"T Time" );
              mb.push( L"I Time (1 min.round)" );
              mb.push( L"D Date" );
              mb.push( L"M Date+Time" );
              mb.push( L"A Attr/Mode" );
              mb.push( L"O Owner" );
              mb.push( L"G Group" );
              mb.push( L"Y Type (TP)" );

              vfu_menu_box( 32, 6, L"Select Same..." );
              wch = menu_box_info.ec;
              switch ( wch )
                {
                case L'N' : vfu_global_select_same( GSAME_NAME  ); break;
                case L'E' : vfu_global_select_same( GSAME_EXT   ); break;
                case L'S' : vfu_global_select_same( GSAME_SIZE  ); break;
                case L'M' : vfu_global_select_same( GSAME_DATETIME  ); break;
                case L'T' : vfu_global_select_same( GSAME_TIME  ); break;
                case L'I' : vfu_global_select_same( GSAME_TIME1 ); break;
                case L'D' : vfu_global_select_same( GSAME_DATE  ); break;
                case L'O' : vfu_global_select_same( GSAME_OWNER ); break;
                case L'G' : vfu_global_select_same( GSAME_GROUP ); break;
                case L'A' : vfu_global_select_same( GSAME_MODE  ); break;
                case L'Y' : vfu_global_select_same( GSAME_TYPE  ); break;
                }
              } break;
    case L'M': {
              mode_str_t mode_str;
              strcpy( mode_str, MODE_STRING );
              if(vfu_edit_attr( mode_str, 0 ))
                {
                for ( int z = 0; z < files_list_count(); z++ )
                  files_list_get(z)->sel =
                     (strcmp( files_list_get(z)->mode_str()+1, mode_str+1 ) == 0);
                do_draw = 1;
                }
              } break;
    case L'<' : {
               if( files_list_count() > 0)
                 for (int z = 0; z <= FLI; z++)
                   if (!files_list_get(z)->is_dir())
                     files_list_get(z)->sel = 1;
               } break;
    case L'>' : {
               if( files_list_count() > 0)
                 for (int z = FLI; z < files_list_count(); z++)
                   if (!files_list_get(z)->is_dir())
                     files_list_get(z)->sel = 1;
               } break;
    }
  update_status();
  do_draw = 1;
}

/*--------------------------------------------------------------------------*/

int vfu_user_external_find( wchar_t key, const char* ext, const char* type, VString *shell_line )
{
  VArray split;
  VString str;
  VString ext_str = ext;
  VString type_str = type;
  if ( ext_str == "" )
    ext_str = ".";
  ext_str += ".";
  type_str = "." + type_str + ".";
  int z;
  for ( z = 0; z < user_externals.count(); z++ )
    {
    split = str_split( ",", user_externals[z] );
    if ( (wchar_t)(key_by_name( split[1] )) != key ) continue; /* if key not the same -- skip */
    if ( split[2] != "*" ) /* if we should match and extension */
      {
      if ( opt.lower_case_ext_config )
        {
        str_low( split[2] );
        str_low( ext_str );
        str_low( type_str );
        }
      if ( str_find( split[2], ext_str ) == -1 &&
           str_find( split[2], type_str ) == -1 ) continue; /* not found -- next one */
      }
    if ( shell_line ) /* if not NULL -- store shell line into it */
      (*shell_line) = split[3];
    return z;
    }
  return -1;
}

/*--------------------------------------------------------------------------*/

void vfu_user_external_exec( wchar_t key )
{
  if ( files_list_count() == 0 )
    {
    say1( "Directory is empty: user externals are disabled!" );
    return;
    }
  VString shell_line;
  TF *fi = FLCUR;
  if (vfu_user_external_find( key, fi->ext(), fi->type_str(), &shell_line ) != -1)
    {
    if ( work_mode == WM_NORMAL )
      vfu_shell( shell_line, "" );
    else
    if ( work_mode == WM_ARCHIVE )
      {
      vfu_user_external_archive_exec( shell_line );
      }
    }
  else
    {
    say( 1, cNORMAL, "No user external defined for this key and extension (%lx,%s)", key, fi->ext() );
    }
}

/*--------------------------------------------------------------------------*/

VString tools_last_target;
void vfu_tools()
{
  mb.undef();
  mb.push( L"R Real path" );
  mb.push( L"D ChDir to symlink path" );
  mb.push( L"G Go to symlink target" );
  mb.push( L"B Go back to last target" );
  mb.push( L"T Make directory" );
  mb.push( L"P Path bookmarks" );
  mb.push( L"A Rename tools..." );
  mb.push( L"C Classify files" );
  if ( vfu_menu_box( 30, 5, L"Tools" ) == -1 ) return;

  switch( menu_box_info.ec )
    {
    case L'T' : {
               VString str;
               if (vfu_get_str( "Make directory(ies) (use space for separator)", str, HID_MKPATH ))
                 {
                 int err = 0;
                 int z;
                 VArray ms;
                 ms = str_split( " +", str );
                 for ( z = 0; z < ms.count(); z++ )
                   if( make_path( ms[z] ) )
                     {
                     say1( "Cannot create directory:" );
                     say2( ms[z] );
                     con_getwch();
                     err++;
                     }
                 if ( err == 0 ) say1( "MKDIR: ok." );
                 return;
                 }
               }
               return;
    case L'P' : bookmark_goto( 0 );  return;
    }

  if( files_list_count() < 1 )
    {
    say1( "No files..." );
    return;
    }
  
  TF *fi = FLCUR;

  switch( menu_box_info.ec )
    {
    case L'R' : {
               say1( expand_path( fi->name() ) );
               return;
               }
    case L'D' : {
               if( ! fi->is_link() ) return;
               tools_last_target = fi->full_name();
               if( ! fi->is_dir() ) return;
               vfu_chdir( expand_path( fi->name() ) );
               return;
               }
    case L'G' : {
               if( ! fi->is_link() ) return;
               tools_last_target = fi->full_name();
               VString target = vfu_readlink( fi->full_name() );
               vfu_chdir( expand_path( str_file_path( target ) ) );
               vfu_goto_filename( str_file_name_ext( target ) );
               return;
               }
    case L'B' : {
               if( tools_last_target == "" ) return;
               VString target = tools_last_target;
               tools_last_target = fi->full_name();
               vfu_chdir( expand_path( str_file_path( target ) ) );
               vfu_goto_filename( str_file_name_ext( target ) );
               return;
               }
    case L'A' : vfu_tool_rename();   return;
    case L'C' : vfu_tool_classify(); return;
    }
}

/*--------------------------------------------------------------------------*/

void bookmark_goto( wchar_t wch )
{
  VString t;
  WString wt;
  if ( wch == 0 )
    {
    int z;
    mb.undef();
    mb.push( L"A Bookmark current directory" );
    mb.push( L"` Change working directory" );
    mb.push( L"---" );
    for( z = 0; z < 10; z++ )
      {
      const char* ps = path_bookmarks.get( z );
      if( !ps ) break;
      sprintf( t, "%d %s", ( z + 1 ) % 10, ps );
      wt = t;
      mb.push( str_dot_reduce( wt, 60 ) );
      }
    if ( vfu_menu_box( 5, 5, L"Path bookmarks") == -1 ) return;
    wch = menu_box_info.ec;
    }
  switch( wch )
    {
    case L'`' : vfu_chdir( NULL ); return;
    case L'A' : bookmark_hookup(); return;
    }
  if ( wch >= L'1' && wch <= L'9' && str_len( path_bookmarks[ wch - L'1' ] ) > 0 )
    {
    vfu_chdir( path_bookmarks[ wch - L'1' ] );
    return;
    }
}

void bookmark_hookup()
{
  int found = -1;
  for( int z = 0; z < path_bookmarks.count(); z++ )
    {
    if( work_path == path_bookmarks[z] ) found = z;
    }
  if( found > -1 )
    {
    say1( "Current directory is already hooked", chRED );
    return;
    }
  path_bookmarks.push( work_path );
  if ( path_bookmarks.count() > 10 )
    path_bookmarks.shift();
}

/*--------------------------------------------------------------------------*/

void vfu_command()
{
  VString cmd;
  if ( vfu_get_str( "Command: ", cmd, HID_COMMANDS ) ) vfu_shell( cmd, "" );
}

/*--------------------------------------------------------------------------*/

void vfu_rename_file_in_place()
{
  if ( files_list_count() <= 0 )
    {
    say1( "No files" );
    return;
    }

  TF *fi = FLCUR;

  int y = ( FLI - FLP ) + 4;
  int x = tag_mark_pos + 3;

  WString www = fi->name();
  TextInput( x, y, "", MAX_PATH, con_max_x() - tag_mark_pos - 3, www );
  // FIXME: check return res
  VString str = www;
  
  if( str != fi->name() )
    {
    if ( file_exist(str) )
      say1( "Cannot rename: destination name exists!" );
    else
    if(rename(fi->name(), str.data()) == 0)
      {
      fi->set_name( str );
      say1("RENAME: ok.");
      }
    else
      {
      say1("RENAME: failed.");
      say2errno();
      }
    }

  do_draw = 1;
}

/*--------------------------------------------------------------------------*/

void vfu_change_file_mask( const char* a_new_mask )
{
  VString tmp = files_mask;
  int new_mask = 0;

  if ( a_new_mask )
    {
    tmp = a_new_mask;
    new_mask = 1;
    }
  else
    {
    new_mask = vfu_get_str( "", tmp, HID_FMASK, 6, 1 );
    do_draw = 1;
    }

  if(new_mask)
    {
    str_cut_spc( tmp );
    if ( str_len( tmp ) < 1 ) tmp = "*";

    files_mask = tmp;
    files_mask_array = str_split( " +", files_mask );

    if ( opt.mask_auto_expand )
      {
      int z;
      for ( z = 0; z < files_mask_array.count(); z++ )
        vfu_expand_mask( files_mask_array[z] );
      }
    vfu_read_files();
    }
}

/*--------------------------------------------------------------------------*/

/*
  n == 0..nnn for nnn only, -1 current only, -2 all
*/

int __vfu_dir_size_followsymlinks = 0;
int __vfu_dir_size_samedevonly    = 0;

void vfu_directory_sizes( wchar_t wch )
{
  int z;

  int dir_size_mode = __vfu_dir_size_followsymlinks | __vfu_dir_size_samedevonly;

  wch = toupper( wch );
  if ( wch == 0 )
    {
    mb.undef();
    mb.push( L"E Specify directory" );
    mb.push( L"Z Directory under cursor" );
    mb.push( L". Current directory `.'" );
    mb.push( L"S Selected directories" );
    mb.push( L"A All dir's in the list" );
    mb.push( L"M Missing sizes dirs" );
    mb.push( L"P Drop dir sizes cache (WARNING!)" );
    mb.push( L"--directory size options--" );
    mb.push( L"N Normal" );
    mb.push( L"Y Follow symlinks (WARNING: may loop!)" );
    mb.push( L"V Keep on the same device/filesystem only" );
    if ( vfu_menu_box( 5, FLPS - 8, L"Directory size of:" ) == -1 ) return;
    wch = menu_box_info.ec;
    }

  DirSizeInfo size_info;
  say1( "Calculating files size. Press ESCAPE to cancel calculation." );
  if ( wch == L'E' || wch == L'.' ) /* specific directory */
    {
    VString target = work_path;
    if ( wch == L'.' )
      target = work_path;
    else
      if ( !vfu_get_dir_name( "Calculate size of directory: ", target ) ) return;
    fsize_t dir_size = vfu_dir_size( target, 1, dir_size_mode, &size_info );
    if ( dir_size == -1 ) return;
    say1( "Path: " + target );
    say2( size_info.str() );
    } else
  if ( wch == L'A' || wch == L'S' || wch == L'M' ) /* all, selected or missing sizes  */
    {
    size_cache_sort( 1 );
    for( z = 0; z < files_list_count(); z++)
      {
      TF *fi = files_list_get(z);
      if ( ! fi->is_dir() ) continue;
      if ( wch == L'S' && ! fi->sel        ) continue; /* if not sel'd and required -- skip */
      if ( wch == L'M' &&   fi->size() > 0 ) continue; /* if not sel'd and required -- skip */
      fsize_t dir_size = -1;
      if ( fi->is_link() )
        dir_size = size_cache_get_pending( fi->name() );
      if( dir_size == -1 )
        dir_size = vfu_dir_size( fi->name(), 0, dir_size_mode, &size_info );
      if ( dir_size == -1 ) break; // break requested
      fi->set_size( dir_size );
      }
    size_cache_sort();
    say1( wch == L'S' ? "Path: *** selected dirs in the list ***" : "Path: *** all dirs in the list ***" );
    say2( size_info.str() );
    }  else
  if ( wch == L'Z' ) /* single one, under cursor  */
    {
    if ( FLCUR->is_dir() )
      {
      FLCUR->set_size( vfu_dir_size( FLCUR->name(), 1, dir_size_mode, &size_info ) );
      say1( "Path: " + work_path + VString( FLCUR->name() ) );
      say2( size_info.str() );
      }
    else
      say1("This is not directory...");
    } else
  if( wch == L'N' ) /* normal traverse mode */
    {
    __vfu_dir_size_followsymlinks = 0;
    __vfu_dir_size_samedevonly    = 0;
    say1( "Directory size calculation mode set to NORMAL (all dev/fs, no symlinks)" );
    } else
  if( wch == L'Y' ) /* follow symlinks */
    {
    __vfu_dir_size_followsymlinks = __vfu_dir_size_followsymlinks ? 0 : DIR_SIZE_FOLLOWSYMLINKS;
    if( __vfu_dir_size_followsymlinks )
      say1( "Directory size calculation will FOLLOW symlinks! LOOP WARNING!" );
    else
      say1( "Directory size calculation will NOT follow symlinks" );
    } else
  if( wch == L'V' ) /* traverse same device/filesystem only */
    {
    __vfu_dir_size_samedevonly = __vfu_dir_size_samedevonly ? 0 : DIR_SIZE_SAMEDEVONLY;
    if( __vfu_dir_size_samedevonly )
      say1( "Directory size calculation will KEEP THE SAME device/filesystem only!" );
    else
      say1( "Directory size calculation will follow ALL devices/filesystems" );
    }
  if( wch == L'P' )
    {
    int scc = size_cache.count();
    wchar_t wch = 0;
    if( scc > 0 )
      {
      vfu_beep();
      say2( VString() + "Entries to be removed: " + scc );
      wch = towlower( vfu_ask( L"Directory sizes cache will be dropped? "
                               L"( D=Yes, drop data!, ESC=cancel )",
                               L"d" ));
      say1( "" );
      say2( "" );
      }
    else
      {
      say1( "Directory sizes cache is empty." );
      }  
                               
    if( wch == L'd')
      {
      size_cache.undef();
      say1( VString() + "Directory sizes cache dropped. Removed entries: " + scc );
      }
    }

  do_draw = 1;
  update_status();
  if ( opt.sort_order == 'S' ) vfu_sort_files();
}

/*--------------------------------------------------------------------------*/

void vfu_edit_entry( )
{
  char errstr[128];

  int one = ( sel_count == 0 );
  int z;
  WString str;

  mb.undef();
  mb.push( L"M Mode" );
  mb.push( L"A Octal Mode" );
  mb.push( L"O Owner/Group" );
  mb.push( L"N Name (TAB)" );
  mb.push( L"T Time/Touch Mod+Acc Times" );
  mb.push( L"I Modify Time" );
  mb.push( L"E Access Time" );
  mb.push( L"L Edit SymLink Reference" );
  if ( sel_count )
    { /* available only when selection exist */
    mb.push( L"--");
    mb.push( L"+ Target: Toggle" );
    mb.push( L"C Target: Current File" );
    mb.push( L"S Target: Selection" );
    }

  while(1)
    {
    while(1)
      {
      str = L"Edit entry: ";
      str += one ? L"current file" : L"[ SELECTION ] ";
      menu_box_info.ac = 9;
      z = vfu_menu_box( 50, 5, str );
      if ( z == -1 ) return; /* canceled */
      if (menu_box_info.ec ==   9 ) menu_box_info.ec = L'N';
      if (menu_box_info.ec == L'+') { one = !one; continue; }
      if (menu_box_info.ec == L'S') { one = 0; continue; }
      if (menu_box_info.ec == L'C') { one = 1; continue; }
      break;
      }

    if ( menu_box_info.ec == L'N' ) /* name (rename) */
      {
      vfu_rename_file_in_place();
      break;
      } else
    if ( menu_box_info.ec == L'M' ||
         menu_box_info.ec == L'A' ) /* attributes/mode */
      {
        if( one && FLCUR->is_link() && opt.show_symlinks_stat )
          {
          say1( "Symlinks' mode cannot be changed." );
          return;
          }

        mode_str_t new_mode;
        int ok = 1;
        int err = 0;
        if ( menu_box_info.ec == L'M' )
          {
          if(one)
            {
            strcpy( new_mode, FLCUR->mode_str() );
            file_get_mode_str( FLCUR->st()->st_mode, new_mode);
            }
          else
            strcpy(new_mode, MODE_MASK);
          ok = vfu_edit_attr(new_mode, !one );
          }
        else
          {
          say1( "Enter octal mode (i.e. 755, 644, 1777, etc.)" );
          VString str;
          int z = vfu_get_str( "", str, HID_OMODE );
          str_cut_spc( str );
          mode_t m;
          unsigned int im;
          sscanf( str, "%o", &im );
          m = im;
          file_get_mode_str( m, new_mode );
          ok = (z && str_len(str) > 0);
          }
        if( ok )
          {
          for ( z = 0; z < files_list_count(); z++ )
            if ( (one && FLI == z) || (!one && files_list_get(z)->sel) )
              {
              TF *fi = files_list_get(z);
              if( opt.show_symlinks_stat && fi->is_link() ) continue; // symlinks' mode cannot be changed
              if(file_set_mode_str(fi->name(), new_mode) == 0)
                {
                fi->update_stat();
                do_draw = 1;
                }
              else
                err++;
              }
          }

        if (err)
          sprintf( errstr, "Change attr/mode errors: %d", err );
        else
          strcpy( errstr, "Change attr/mode ok." );
        say1( errstr );
        if (err)
          say2errno();
        break;
      } else
    if ( menu_box_info.ec == L'T' ||
         menu_box_info.ec == L'I' ||
         menu_box_info.ec == L'E' )
      {
        char t[128];
        strcpy( t, "Change times: " );
        strcat( t, (menu_box_info.ec == 'T') ? "MODIFY,ACCESS" : ( (menu_box_info.ec == 'M') ? "MODIFY" : "ACCESS" ) );
        strcat( t, one ? " for the current file:" : " for SELECTED FILES/DIRS:" );
        strcat( t, "    PLEASE KEEP THE FORMAT!" );
        say1( t );

        strcpy( t, time2str(time(NULL)));
        t[24] = 0;

        VString str = t;
        int z = vfu_get_str( "", str, HID_EE_TIME );
        if( !(z && str_len(str) > 0) ) break;

        time_t new_time = str2time( str );
        if ( new_time == 0 ) // well, this is 1.1.1970 but I consider it wrong
          {
          say1( "Wrong time string format." );
          break;
          }
        int err = 0;
        struct utimbuf tb;
        for ( z = 0; z < files_list_count(); z++ )
          if ( (one && FLI == z) || (!one && files_list_get(z)->sel) )
            {
            TF *fi = files_list_get(z);
            tb.actime  = fi->st()->st_atime;
            tb.modtime = fi->st()->st_mtime;
            if (menu_box_info.ec == L'M') tb.modtime = new_time;
            if (menu_box_info.ec == L'S') tb.actime  = new_time;
            if (menu_box_info.ec == L'T') tb.modtime = new_time;
            if (menu_box_info.ec == L'T') tb.actime  = new_time;
            if (utime( fi->name(), &tb ) == 0)
              {
              fi->update_stat();
              do_draw = 1;
              }
            else
              err++;
            }
        if (err)
          sprintf( errstr, "Time touch errors: %d", err );
        else
          strcpy( errstr, "Time touch ok." );
        say1( errstr );
        if (err)
          say2errno();
        break;
      } else
    if ( menu_box_info.ec == L'O' )
      {
        VString str;
        if (one)
          say1("Enter new `user.group | user | .group' for current file:");
        else
          say1("Enter new `user.group | user | .group' for all SELECTED files:");
        if( !(vfu_get_str( "", str, HID_EE_OWNER ) && str_len(str) > 0) ) break;

        VRegexp re( "^ *([^\\.]*)(\\.([^\\.]*))? *$" );
        if( ! re.m( str ) )
          {
          say1("Format is 'uid.gid', for example 'cade.users', 'cade.', '.users'");
          break;
          }

        int uid = -1;
        int gid = -1;

        struct passwd *pwd = getpwnam(re[1]);
        if ( pwd ) uid = pwd->pw_uid;

        struct group  *grp = getgrnam(re[3]);
        if ( grp ) gid = grp->gr_gid;

        int err = 0;
        for ( z = 0; z < files_list_count(); z++ )
          if ( (one && FLI == z) || (!one && files_list_get(z)->sel) )
            {
            TF *fi = files_list_get(z);
            int u = uid;
            int g = gid;
            if (u == -1) u = fi->st()->st_uid;
            if (g == -1) g = fi->st()->st_gid;
            int cr = 0;

            if( opt.show_symlinks_stat && fi->is_link() )
              cr = lchown(fi->name(), u, g);
            else
              cr = chown(fi->name(), u, g);
            
            if( cr == 0 )
              {
              fi->update_stat();
              do_draw = 1;
              }
            else
              err++;
            }

        if (err)
          sprintf( errstr, "Change owner/group errors: %d", err );
        else
          strcpy( errstr, "Change owner/group ok." );
        say1( errstr );
        if (err)
          say2errno();
        break;
      } else
    if ( menu_box_info.ec == 'L' )
      {
      if ( ! one )
        {
        say1( "Cannot edit symlink reference for selection..." );
        break;
        }
      TF* fi = FLCUR;
      if ( ! fi->is_link() )
        {
        say1( "This is not a symlink..." );
        break;
        }
      fname_t t = "";
      t[ readlink( fi->name(), t, MAX_PATH - 1 ) ] = 0;
      VString str = t;
      //if ( vfu_get_str( "", str, 0 ) )
      if ( vfu_get_dir_name( "SymLink Target:", str, 1, 'A' ) )
        {
        fi->drop_view();
        do_draw = 1;
        say2( "" );
        if ( unlink( fi->name() ) || symlink( str, fi->name() ) )
          {
          say1( "Edit SymLink reference error..." );
          say2errno();
          }
        else
          {
          say1( "Edit SymLink reference ok." );
          }
        }
      break;
      }
    }
  return;
}

/*--------------------------------------------------------------------------*/

void vfu_jump_to_mountpoint( int all __attribute__((unused)) )
{
  VString str;
  char t[2048];
  int z;
  VArray va;
  if ( va.fload( "/etc/mtab" ) ) return;
  if (va.count() < 1) return;

  mb.undef();
  for(z = 0; z < va.count(); z++)
    {
    str = va[z];
    str_cut( str, " \t");
    str_word( str, " \t", t ); /* get device name */
    str_cut( str, " \t");
    str_word( str, " \t", t ); /* get mount point */
    //va.set( z, t ); /* replace line with mount point only */
    va[z] = t; /* replace line with mount point only */

    struct statfs stafs;
    statfs( t, &stafs );
    int hk = ('A'+ z); /* hot key */
    
    fsize_t fs_free   = stafs.f_bsize * ( opt.show_user_free ? stafs.f_bavail : stafs.f_bfree );
    fsize_t fs_total  = stafs.f_bsize * stafs.f_blocks;
    VString str_free  = opt.use_gib_usage ? fsize_fmt( fs_free,  1 ) : size_str_compact( fs_free  );
    VString str_total = opt.use_gib_usage ? fsize_fmt( fs_total, 1 ) : size_str_compact( fs_total );
    
    sprintf( str, "%c | %15s | %15s | %-30s ",
             hk,
             (const char*)str_free,
             (const char*)str_total,
             //stafs.f_bsize * ( opt.show_user_free ? stafs.f_bavail : stafs.f_bfree ) / (1024.0*1024.0),
             //stafs.f_bsize * stafs.f_blocks / (1024.0*1024.0),
             (const char*)(str_dot_reduce( t, 30 ))
             );

    mb.push(WString(str));
    }
  menu_box_info.ac = UKEY_CTRL_U;
  z = vfu_menu_box( 5, 5, L"Jump to mount-point (free/total) Ctrl+U=umount" );
  if ( z == -1 )   return;
  if ( menu_box_info.ec == UKEY_CTRL_U )
    {
    str = va[z];
    str_fix_path( str );
    if ( pathcmp( str, work_path ) == 0 )
      {
      say1( "Warning: cannot unmount current directory" );
      return;
      }
    str = "umount " + str + " 2> /dev/null";
    snprintf( t, sizeof(t), "Unmounting, exec: %s", str.data() );
    say1( t );
    if (system( str ) == 0)
      say1( "umount ok" );
    else
      say1( "umount failed" );
    }
  else
    vfu_chdir( VString( va[z] ) );
}

/*--------------------------------------------------------------------------*/

void vfu_user_menu()
{
  VArray split;
  VArray lines;
  VString des;
  int z;

  mb.undef();

  for ( z = 0; z < user_externals.count(); z++ )
    {
    split = str_split( ",", user_externals[z] );
    if ( strcasecmp( split[1], "menu" ) ) continue; /* not menu item -- skip */
    /* FIXME: should we care about ext's or user will override this? */
    /* split[2]; // extensions */

    des = split[0];
    if ( des != "---" ) /* not separator */
      {
      /* fix menu hotkeys */
      str_ins( des, 1, " " );
      str_set_ch( des, 0, toupper(str_get_ch(des, 0)) );
      }

    lines.push( split[3] );
    mb.push( WString( des ) );
    }

  if ( mb.count() == 0 )
    {
    say1("No menu user externals defined...");
    return;
    }
  z = vfu_menu_box( 5, 5, L"User menu (externals) " );
  if ( z == -1 ) return;

  if ( work_mode == WM_NORMAL )
    vfu_shell( lines[z], "" );
  else
  if ( work_mode == WM_ARCHIVE )
    {
    VString str = lines[z];
    vfu_user_external_archive_exec( str );
    }
}

/*--------------------------------------------------------------------------*/

void vfu_file_find_results()
{
  do_draw = 2;
  if ( file_find_results.count() == 0 )
    {
    say1("No file find results...");
    return;
    }

  ConMenuInfo bi;
  bi.cn = cSTATUS;
  bi.ch = 31;
  bi.ti = cINFO;
  bi.ac = 'p';

  say1center("------- ESC Exit ----- ENTER Chdir to target ----- P Panelize all results -----", cINFO );
  say2("");
  int z = con_full_box( 1, 1, L"VFU File find results", &file_find_results, &bi );

  if ( bi.ec == 13 )
    {
    VString fname;
    VString str = file_find_results[z];
    str_trim_left( str, str_find( str, " | " ) + 3 );
    z = str_rfind( str, '/' );
    fname = str;
    str_sleft( str, z+1 );
    str_trim_left( fname, z+1 );
    vfu_chdir( str );
    for( z = 0; z < files_list_count(); z++ )
      if ( pathcmp( fname, files_list_get(z)->name_ext() ) == 0 )
        {
        FLGO(z);
        vfu_nav_update_pos();
        break;
        }
    }
  else if ( tolower(bi.ec) == 'p' )
    {
    list_panelizer.undef();
    for ( z = 0; z < file_find_results.count(); z++ )
      {
      VString str = file_find_results[z];
      str_trim_left( str, str_find( str, " | " ) + 3 );
      list_panelizer.push( str );
      }
    vfu_read_files( 0 );
    }

  file_find_results.fsave( filename_ffr );
  con_cs();
}

/*--------------------------------------------------------------------------*/

VArray      __ff_masks;
VString     __ff_path;
VString     __ff_pattern;
VString     __ff_opt;
int         __ff_rescount;

int __ff_process( const char* origin __attribute__((unused)),    /* origin path */
                  const char* fname,     /* full file name */
                  const struct stat* st, /* stat struture or NULL */
                  int is_link __attribute__((unused)),           /* 1 if link */
                  int flag )
{
  VString str;

  if ( flag == FTWALK_DX ) return 0;
  if ( vfu_break_op() ) return 1;
  if ( flag == FTWALK_D )
    {
    str = fname;
    str = str_dot_reduce( str, con_max_x()-1 );
    say2( str );
    }

  const char *pc = strrchr( fname, '/' );
  if (pc)
    pc++;
  else
    pc = fname;

  int add = 0;
  int z;
  for ( z = 0; z < __ff_masks.count(); z++ )
    if ( opt.no_case_glob ? FNMATCH_NC( __ff_masks[z], pc ) == 0 : FNMATCH( __ff_masks[z], pc ) == 0 )
      {
      add = 1;
      break;
      }
  if ( add && __ff_pattern != "" )
    add = ( file_string_search( __ff_pattern, fname, __ff_opt ) > -1 );
  if (!add) return 0;

  __ff_rescount++;
  char time_str[32];
  VString size_str;
  time_str_compact( st->st_mtime, time_str );
  if ( flag == FTWALK_D )
    size_str = "[DIR]";
  else
    size_str = size_str_compact( st->st_size );
  str_pad( size_str, 7 );
  str = "";
  str = str + time_str + " " + size_str + " | " + fname;
  WString wstr;
  wstr.set_failsafe( str );
  file_find_results.push( wstr );
  wstr = str_dot_reduce( wstr, con_max_x()-1 );
  con_puts( "\r" );
  con_puts( VString( wstr ), cSTATUS );
  con_puts( "\n" );

  str = "Found items: ";
  str += __ff_rescount;
  str += " | ";
  str += fname;
  say1( str );
  return 0;
}

void vfu_file_find( int menu )
{
  VString str;
  wchar_t wch;

  if (menu)
    {
    if ( vfu_menu_box( L"File find", L"L Last find results,D Drop find results,N File find,F Find string (no case),S Scan string (case),B Scan string (case),E Hex string,/ Regular expresion,\\ Reg.exp (no case)", 5 ) == -1 ) return;
    wch = menu_box_info.ec;
    }
  else
    wch = L'N';
  if ( wch == L'L' )
    {
    if ( file_find_results.count() == 0 )
      file_find_results.fload( filename_ffr );
    vfu_file_find_results();
    return;
    }
  if ( wch == L'D' )
    {
    file_find_results.undef();
    vfu_file_find_results(); /* FIXME: this will show `no results' warning */
    return;
    }

  __ff_pattern = "";
  if ( str_find( "FSB/\\", wch ) != -1 ) /* we want search for pattern */
    {
    __ff_pattern = vfu_hist_get( HID_FFGREP, 0 );
    if (!vfu_get_str( "Enter search pattern: ", __ff_pattern, HID_FFGREP )) return;
    if (wch == L'F' ) __ff_opt = "i "; else
    if (wch == L'S' ) __ff_opt = "  "; else
    if (wch == L'B' ) __ff_opt = "  "; else
    if (wch == L'E' ) __ff_opt = "h "; else
    if (wch == L'/' ) __ff_opt = "r "; else
    if (wch == L'\\') __ff_opt = "ri"; else
    {};
    }

  str = vfu_hist_get( HID_FFMASK, 0 );
  if ( str == "" ) str = "*";
  if (!vfu_get_str( "Enter find masks (space separated): ", str, HID_FFMASK )) return;
  __ff_masks = str_split( " +", str );

  str = work_path;
  if (!vfu_get_dir_name( "Enter start path: ", str )) return;
  __ff_path = str;

  /*--------------------------------------*/

  if ( opt.mask_auto_expand )
    {
    int z;
    for ( z = 0; z < __ff_masks.count(); z++ )
      vfu_expand_mask( __ff_masks[z] );
    }
  con_cs();
  con_ta( cINFO );
  vfu_con_out( 1, 1, HEADER );
  sprintf( str, "Find mask: %s", vfu_hist_get( HID_FFMASK, 0 ) );
  vfu_con_out( 1, 2, str );
  sprintf( str, "Start path: %s", __ff_path.data() );
  vfu_con_out( 1, 3, str );
  if ( __ff_pattern != "" )
    {
    sprintf( str, "Containing pattern: %s", __ff_pattern.data() );
    vfu_con_out( 1, 4, str );
    }

  file_find_results.undef();
  __ff_rescount = 0;
  ftwalk( __ff_path, __ff_process );
  vfu_file_find_results();
}

/*--------------------------------------------------------------------------*/

void vfu_read_files_menu()
{
  char t[1024];
  VArray list;

  int z;
  VString str;
  mb.undef();
  /* I don't format src like this but it gives clear idea what is all about */
  mb.push( L"T Rescan DirTree" );           list.push("");
  mb.push( L"F Rescan Files" );             list.push("");
  mb.push( L"R Rescan Files Recursive" );   list.push("");
  mb.push( L"L Refresh all views/screen (Ctrl+L)" ); list.push("");
  if ( panelizers.count() > 0 )
    {
    mb.push( L"--panelizers---" );         list.push("");
    for ( z = 0; z < panelizers.count(); z++ )
      {
      str = panelizers[z];
      str_word( str, ",", t );
      /* fix menu hotkeys */
      str_ins( t, 1, " " );
      str_set_ch( t, 0, toupper( str_get_ch( t, 0 ) ) );
      mb.push( WString( t ) );
      list.push(str);
      }
    }
  z = vfu_menu_box( 25, 5, L"Read/Rescan Files" );
  if ( z == -1 )
    {
    return;
    }
  if ( str_len( list[z] ) )
    { /* so panelizer has been choosed */
    external_panelizer = list[z];
    str = ""; /* no shell options by default */
    vfu_update_shell_line( external_panelizer, str );
    vfu_read_files( 0 );
    }
  else
  switch( menu_box_info.ec )
    {
    case L'T' : tree_rebuild(); break;
    case L'F' : vfu_read_files( 0 ); break;
    case L'R' : vfu_read_files( 1 ); break;
    case L'L' : vfu_reset_screen(); break;
    }
}

/*--------------------------------------------------------------------------*/

void vfu_inc_search( int use_last_one )
{
  WString str;
  wchar_t wch;
  if( use_last_one && last_inc_search == "" )
    use_last_one = 0;
  if( use_last_one && last_inc_search != "" )
    str = last_inc_search;

  VString no_case_opt_str = opt.no_case_glob ? " no-case " : " ";
  if( use_last_one )
    {
    say1( "Incremental" + no_case_opt_str + "search: ( ALT+S for next match )" );
    wch = 9;
    }
  else
    {
    say1( "Incremental" + no_case_opt_str + "search: ( TAB for next or 'size:NNN' search )" );
    wch = con_getwch();
    }
  WRegexp size_re( L"^size:\\s*(\\d+)$" ); // TODO: allow "size:1024+"
  while( ( wch >= 32 && ( ! UKEY_IS_WIDE_CTRL( wch ) ) ) || wch == 8 || wch == UKEY_BACKSPACE || wch == 9 )
    {
    if ( wch == 8 || wch == UKEY_BACKSPACE )
      str_trim_right( str, 1 );
    else
    if ( wch != 9 )
      str_add_ch( str, wch );
    say2( VString( str ) );

    if ( files_list_count() == 0 ) { wch = con_getwch(); continue; }

    int z;
    if ( wch == 9 )
      {
      z = FLI + 1;
      if ( z > FLMAX ) z = FLMIN;
      }
    else
      z = FLI;

    int direction = 1;
    int found = 0;
    int loops = 0;
    VString s_mask = str;
    int s_size = 0;
    if( size_re.m( str ) )
      s_size = VString( size_re[1] ).i();
    else
      vfu_expand_mask( s_mask );
    while(1)
      {
      if ( z > FLMAX ) z = FLMIN;
      if ( z < FLMIN ) z = FLMAX;
      if( s_size )
        found = files_list_get(z)->size() == s_size;
      else
        if( opt.no_case_glob )
          found = ( FNMATCH_NC( s_mask, files_list_get(z)->name_ext() ) == 0 );
        else
          found = ( FNMATCH( s_mask, files_list_get(z)->name_ext() ) == 0 );
      if ( found ) break;
      z += direction;
      if ( loops++ > files_list_count() ) break;
      }
    if (found)
      {
      FLGO(z);
      vfu_redraw();
      show_pos( FLI + 1, files_list_count() );
      }
    if( use_last_one )
      break;
    else
      wch = con_getwch();
    }
  last_inc_search = str;
  if( use_last_one )
    return;

  say1( "" );
  say2( "" );
}

/*--------------------------------------------------------------------------*/

void vfu_goto_filename( const char* fname )
{
  if ( files_list_count() == 0 ) return;

  for (int z = 0; z < files_list_count(); z++)
    {
    if( strcmp( fname, files_list_get(z)->name_ext() ) ) continue;
    FLGO(z);
    return;
    }
}

/*######################################################################*/

// #include <mcheck.h> /* memory allocation debug */
int main( int argc, char* argv[] )
{

  setlocale(LC_ALL,"");
  #ifndef NDEBUG
  // mtrace(); /* memory allocation debug */
  #endif

  print_help_on_exit = 0;

  con_init();
  con_cs();
  con_fg( cNORMAL );
  con_bg( cBLACK );
  con_chide();

  vfu_init();
  argc > 1 ? vfu_cli( argc, argv ) : vfu_run(); /* ... :) */
  vfu_done();

  con_cs();
  con_cshow();
  con_done();

  if( print_help_on_exit ) vfu_help_cli();
  /*
  printf("%s\n<cade@noxrun.com> [http://cade.noxrun.com/]\nThank You for using VFU!\n\n", HEADER );
  */
  return 0;
}

/*######################################################################*/

