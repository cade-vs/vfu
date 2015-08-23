/****************************************************************************
 *
 * (c) Vladi Belperchinov-Shabanski "Cade" 1996-2015
 * http://cade.datamax.bg/  <cade@biscom.net> <cade@bis.bg> <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 ****************************************************************************/

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
#include "see.h"

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

  TF*       files_list[MAX_FILES];
  /* file list statistics */
  int       files_count;
  fsize_t   files_size;
  int       sel_count;
  fsize_t   sel_size;
  /* file system statistics */
  fsize_t   fs_free;
  fsize_t   fs_total;
  fsize_t   fs_block_size;
  /* index in the files list */
  ScrollPos file_list_index;

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

  VArray file_find_results; // filefind results

  VArray path_bookmarks;

/*######################################################################*/

  VArray user_externals;
  VArray history;
  VArray see_filters;
  VArray panelizers;

  VArray mb; /* menu boxes */

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

  char TF::_full_name[MAX_PATH];

/*######################################################################*/

  int do_draw;
  int do_draw_status;

/*######################################################################*/

/*
  Message issues
*/

char say_buf[1024];
VString say_str;
void say( int line, int attr, const char* format, ... )
{
  ASSERT( line == 1 || line == 2 );
  va_list vlist;
  va_start( vlist, format );
  vsnprintf( say_buf, sizeof(say_buf), format, vlist );
  va_end( vlist );
  say_str = str_dot_reduce( say_buf, con_max_x()-1 );
  con_out( 1, con_max_y() - ( (line == 1) ? 1 : 0 ), say_str, attr );
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


/*######################################################################*/

void TF::reset() /* reset -- NULL all fields */
{
  _name = _name_ext = _ext = NULL;
  memset( &_st, 0, sizeof(_st) );
  _type_str[0] = 0;
  _is_link = 0;
  _is_dir = 0;
  strcpy( _mode_str, MODE_OFF );
  _size = -1; /* unknown -- get from stat? */
  _view = NULL;
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
  if ( _name ) delete [] _name;
  if ( _view ) delete [] _view;
  reset();
}

/*-----------------------------------------------------------------------*/

const char* TF::full_name( int fix )
{
  ASSERT( _name );

  if ( _name[0] == '/' )
    {
    strcpy( _full_name, _name );
    }
  else
    {
    if ( work_mode == WM_ARCHIVE )
      strcpy( _full_name, archive_path );
    else
      strcpy( _full_name, work_path );
    strcat( _full_name, _name );
    }
  if ( fix && _is_dir )
    strcat( _full_name, "/" ); /* i.e. str_fix_path() */
  return _full_name;
}

/*-----------------------------------------------------------------------*/

void TF::set_name( const char* a_new_name )
{
  if ( _name ) delete [] _name;
  _name = new char[ strlen(a_new_name) + 1 ];
  ASSERT( _name ); /* this is run-time err but for now will be asserted */
  strcpy( _name, a_new_name );

  int last_slash = str_rfind( _name, '/' );
  if ( last_slash == -1 )
    _name_ext = _name;
  else
    _name_ext = _name + last_slash + 1;

  int last_dot = str_rfind( _name, '.' );
  if ( last_dot == -1 || last_dot == 0 ) /* no dot or dot-file (hidden) */
    _ext = _name + strlen( _name );
  else
    _ext = _name + last_dot;

  _color = get_item_color( this ); /* this is duplicated here and in update_stat() */

  drop_view();
}

/*-----------------------------------------------------------------------*/

void TF::set_size( fsize_t a_new_size )
{
  _size = a_new_size;
  drop_view();
}

/*-----------------------------------------------------------------------*/

void TF::drop_view()
{
  if ( !_view ) return;
  delete [] _view;
  _view = NULL;
}

/*-----------------------------------------------------------------------*/

const char* TF::view()
{
  if ( !_view ) refresh_view();
  ASSERT(_view);
  return (const char*)_view;
}

/*-----------------------------------------------------------------------*/

void TF::refresh_view()
{
  ASSERT( _name );
  ASSERT( _name_ext );
  ASSERT( _ext );

  char stmode[16]  = ""; // 10 + 1sep
  char stowner[16+64] = ""; /* +64 just to keep safe (not too much anyway) */
  char stgroup[16+64] = ""; /* +64 just to keep safe (not too much anyway) */
  char sttime[32]  = "";
  char stsize[16]  = "";
  char sttype[4]   = "";

  if ( !opt.long_name_view )
    {
    if (opt.f_mode)
      {
      strcpy( stmode, _mode_str );
      strcat( stmode, " " ); /* field separator */
      }

    if (opt.f_owner)
      {
      struct passwd* _pwd = getpwuid(_st.st_uid);
      if (_pwd)
        sprintf( stowner, "%8s", _pwd->pw_name );
      else
        sprintf( stowner, "%8d", _st.st_uid);
      stowner[8] = 0; /* safe */
      strcat( stowner, " " ); /* field separator */
      }

    if (opt.f_group)
      {
      struct group*  _grp = getgrgid(_st.st_gid);
      if (_grp)
        sprintf( stgroup, "%8s", _grp->gr_name );
      else
        sprintf( stgroup, "%8d", _st.st_gid);
      stgroup[8] = 0; /* safe */
      strcat( stgroup, " " ); /* field separator */
      }

    if (opt.f_time )
      {
      time_str_compact( vfu_opt_time( _st ), sttime );
      strcat( sttime, " " ); /* field separator */
      }

    if (opt.f_size)
      {
      VString str;
      if ( _is_dir && _size == -1 )
        str = "[DIR]";
      else
        str = fsize_fmt( _size );
      sprintf( stsize, "%14s", (const char*)(str) );
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
  VString view;
  view = view + stmode + stowner + stgroup + sttime + stsize + sttype + "   " + name_view;

  int x = con_max_x();
  if ( str_len( view ) > x )
    str_sleft( view, x );
  else
    str_pad( view, - x );

  if ( _view ) delete [] _view;
  _view = new char[ con_max_x() + 1 ]; /* +1 for the zero :) */

  strcpy( _view, view );

  ASSERT( _view );
  ASSERT( strlen( _view ) == (size_t)con_max_x() );
}

/*-----------------------------------------------------------------------*/

void TF::update_stat( const struct stat* a_new_stat, int a_is_link )
{
  ASSERT( _name );
  ASSERT( _name_ext );
  ASSERT( _ext );
  if ( a_new_stat )
    memcpy( &_st, a_new_stat, sizeof(_st) );
  else
    stat( _name, &_st );

  _is_link = (a_is_link == -1) ? file_is_link( _name ) : a_is_link;
  _is_dir = S_ISDIR(_st.st_mode );
  strcpy( _type_str, file_type_str( _st.st_mode, _is_link ) );

  file_get_mode_str( _st.st_mode, _mode_str );
  if ( _is_dir )
    _size = -1; /* FIXME: some auto thing here? */
  else
    _size = file_st_size( &_st );
  _color = get_item_color( this );

  drop_view();
}

/*######################################################################*/

void vfu_help()
{
  say1center( HEADER  );
  say2center( CONTACT );
  mb.undef();
  mb.push( "*keypad -- navigation keys" );
  mb.push( "ENTER   -- enter into directory/View file ( `+' and `=' too )");
  mb.push( "BACKSPC -- chdir to prent directory ( `-' and ^H too )"         );
  mb.push( "TAB     -- edit entry: filename, atrrib's/mode, owner, group");
  mb.push( "R.Arrow -- rename current file " );
  mb.push( "SPACE   -- select/deselect current list item"   );
  mb.push( "ESC     -- exit menu");
  mb.push( "ESC+ESC -- exit menu");
  mb.push( "1       -- toggle `mode'  field on/off "    );
  mb.push( "2       -- toggle `owner' field on/off "    );
  mb.push( "3       -- toggle `group' field on/off "    );
  mb.push( "4       -- toggle `time'  field on/off "    );
  mb.push( "5       -- toggle `size'  field on/off "    );
  mb.push( "6       -- toggle `type'  field on/off "    );
  mb.push( "7       -- toggle `time type' field change/modify/access time "    );
  mb.push( "8       -- turn on all fields"    );
  mb.push( "0       -- toggle long name view ( show only type and file name )"    );
  mb.push( "~       -- change current dir to HOME directory"     );
  mb.push( "A       -- arrange/Sort file list"                   );
  mb.push( "B       -- browse/View selected/current file"         );
  mb.push( "Alt+B   -- browse/View current file w/o filters"      );
  mb.push( "C       -- copy selected/current file(s)"             );
  mb.push( "D       -- change directory"                         );
  mb.push( "Ctrl+D  -- directory tree "                          );
  mb.push( "Alt+D   -- chdir history " );
  mb.push( "E       -- erase/remove selected/current file(s)!"    );
  mb.push( "F       -- change file masks (space-delimited)       ");
  mb.push( "Ctrl+F  -- reset file mask to `*'"                    );
  mb.push( "G       -- global select/deselect"                    );
  mb.push( "H       -- this help text"                            );
  mb.push( "I       -- edit file"                            );
  mb.push( "Q       -- exit here ( to the current directory)");
  mb.push( "R       -- reload directory/refresh file list"       );
  mb.push( "Ctrl+R  -- recursive reload... "                     );
  mb.push( "Alt+R   -- reload/tree menu" );
  mb.push( "J       -- jump to mountpoint"                       );
  mb.push( "L       -- symlink selected/currnet file(s) into new directory" );
  mb.push( "Ctrl+L  -- refresh/redraw entire screen" );
  mb.push( "M       -- move selected/current file(s)"             );
  mb.push( "N       -- file find"                                );
  mb.push( "Alt+N   -- file find menu"   );
  mb.push( "O       -- options/toggles menu"       );
  mb.push( "P       -- file clipboard menu"       );
  mb.push( "S       -- incremental filename search"       );
  mb.push( "Alt+S   -- find next incremental search entry"       );
  /*
  mb.push( "Ctrl+C  -- copy files to clipboard"       );
  mb.push( "Ctrl+X  -- cut  files to clipboard"       );
  mb.push( "Ctrl+V  -- paste (copy) files from clipboard to current directory" );
  */
  mb.push( "T       -- tools menu"                              );
  mb.push( "U       -- user menu (user external commands bound to menu)  " );
  mb.push( "X       -- exit to old/startup directory ");
  mb.push( "Alt+X   -- exit to old/startup directory ");
  mb.push( "Z       -- calculate directories sizes menu"       );
  mb.push( "Ctrl+Z  -- show size of the current (under the cursor >>) directory");
  mb.push( "Alt+Z   -- show all directories sizes ( or Alt+Z )" );
  mb.push( "V       -- edit vfu.conf file");
  mb.push( "!       -- shell (also available with '?')"                         );
  mb.push( "/       -- command line"                                            );
  mb.push( "vfu uses these (one of) these config files:");
  mb.push( "        1. $HOME/$RC_PREFIX/vfu/vfu.conf");
  mb.push( "        2. $HOME/.vfu/vfu.conf");
  mb.push( "        3. " FILENAME_CONF_GLOBAL0 );
  mb.push( "        4. " FILENAME_CONF_GLOBAL1 );
  mb.push( "        5. " FILENAME_CONF_GLOBAL2 );
  mb.push( "" );
  vfu_menu_box( 1, 4, "VFU Help ( PageUp/PageDown to scroll )" );
  mb.undef();
  do_draw = 1;
}

/*--------------------------------------------------------------------------*/

void vfu_init()
{
  char t[MAX_PATH];

  if( expand_path( "." ) == "" ) chdir( "/" );

  work_mode = WM_NORMAL;
  getcwd( t, MAX_PATH-1 );
  str_fix_path( t );
  work_path = t;

  archive_name = "";
  archive_path = ""; /* NOTE: archives' root directory is `' but not `/'! */

  external_panelizer = "";

  memset( &files_list, 0, sizeof(files_list) );
  files_count = 0;
  files_size = 0;
  sel_count = 0;
  sel_size = 0;

  fs_free = 0;
  fs_total = 0;
  fs_block_size = 0;

  file_list_index.wrap = 0;
  /* file_list_index.* are setup by vfu_read_files() */

#ifdef _TARGET_GO32_
  user_id_str = "dosuser";
  group_id_str = "dos";
  gethostname( t, MAX_PATH-1 );
  host_name_str = t;
  __opendir_flags = __OPENDIR_FIND_HIDDEN;
#else /* _TARGET_UNIX_ */
  uid_t _uid = getuid();
  gid_t _gid = getgid();
  struct passwd* _pwd = getpwuid(_uid);
  struct group*  _grp = getgrgid(_gid);
  if ( _pwd )
    user_id_str  = _pwd->pw_name;
  else
    user_id_str = (int)_uid;
  if ( _grp )
    group_id_str = _grp->gr_name;
  else
    group_id_str = (int)_gid;
  gethostname( t, MAX_PATH-1 );
  host_name_str = t;
#endif

  startup_path = work_path;

  tmp_path = "";
  if ( getenv( "TEMP" ) ) tmp_path = getenv( "TEMP" );
  if ( getenv( "TMP"  ) ) tmp_path = getenv( "TMP" );
  if ( tmp_path == "" )
    {
    #ifdef _TARGET_GO32_
    tmp_path = "c:/tmp/";
    #else
    tmp_path = "/tmp/";
    #endif
  }
  else
    str_fix_path( tmp_path );

  if ( getenv( "HOME" ) )
    home_path = getenv( "HOME" );
  else
    {
    home_path = tmp_path;
  home_path += user_id_str;
  home_path += "/";
  make_path( home_path );
  }
  #ifdef _TARGET_GO32_
  str_tr( home_path, "\\", "/" );
  #endif


  #ifdef _TARGET_GO32_
  shell_diff = "fc";
  #else
  shell_diff = "/usr/bin/diff";
  #endif

  /*
     FIXME: this should something relevant to the home_path
     from above if $HOME does not exist(?) well still can
   accept /tmp/ as it is default now
  */

  rc_path = get_rc_directory( "vfu" );

  /* setup config files locations */
  filename_opt = rc_path;
  filename_conf = rc_path;
  filename_tree = rc_path;
  filename_size_cache = rc_path;
  filename_history = rc_path;
  filename_ffr = rc_path;

  filename_opt += FILENAME_OPT;
  filename_conf += FILENAME_CONF;
  filename_tree += FILENAME_TREE;
  filename_size_cache += FILENAME_SIZE_CACHE;
  filename_history += FILENAME_HISTORY;
  filename_ffr += FILENAME_FFR;

  if ( access( filename_conf, R_OK ) != 0 )
    { /* cannot find local/user conf file, try copy one */
    if ( access( FILENAME_CONF_GLOBAL0, R_OK ) == 0 )
      {
      VArray va;
      va.fload( FILENAME_CONF_GLOBAL0 );
      va.fsave( filename_conf );
      }
    else if ( access( FILENAME_CONF_GLOBAL1, R_OK ) == 0 )
      {
      VArray va;
      va.fload( FILENAME_CONF_GLOBAL1 );
      va.fsave( filename_conf );
      }
    else if ( access( FILENAME_CONF_GLOBAL2, R_OK ) == 0 )
      {
      VArray va;
      va.fload( FILENAME_CONF_GLOBAL2 );
      va.fsave( filename_conf );
      }
    }

  if ( access( filename_conf, R_OK ) != 0 )
    { /* cannot find local/user conf file, try globals */
    if ( access( FILENAME_CONF_GLOBAL0, R_OK ) == 0 )
      filename_conf = FILENAME_CONF_GLOBAL0;
    else if ( access( FILENAME_CONF_GLOBAL1, R_OK ) == 0 )
      filename_conf = FILENAME_CONF_GLOBAL1;
    else if ( access( FILENAME_CONF_GLOBAL2, R_OK ) == 0 )
      filename_conf = FILENAME_CONF_GLOBAL2;
    /* if we get here then no readable conf file found */
    }

  /* shell setup */
  shell_prog = "";
  if (getenv("SHELL")) shell_prog = getenv("SHELL");
  #ifdef _TARGET_GO32_
  if ( shell_prog == "" ) if(getenv("COMSPEC")) shell_prog = getenv("COMSPEC");
  #endif
  if (getenv("VFU_SHELL")) shell_prog = getenv("VFU_SHELL");

  /* this will load defaults first then load vfu.opt and at the
     end will load vfu.conf which will overwrite all if need to */
  vfu_settings_load();

  file_list_index.wrap = 0; /* just to be safe :) */

  files_mask = "*";
  files_mask_array = str_split( " ", files_mask );

  view_profiles.push( "123456" );
  view_profile = "123456";

  /* setup menu colors */
  menu_box_info.ti = 95; /* title */
  menu_box_info.cn = 23; /* normal */
  menu_box_info.ch = 47; /* selected */

  //////////////////////////////////////////
  // setup signals to VFUdone
  // this is a patch but at least will reset terminal and save settings
  signal( SIGINT  , vfu_signal );
  signal( SIGHUP  , vfu_signal );
  signal( SIGTERM , vfu_signal );
  signal( SIGQUIT , vfu_signal );
  // signal( SIGWINCH, vfu_signal );
  // this is for xterm resize refresh handle
  // signal( SIGWINCH, VFUsignal );
  // still doesn't work?...
  // HELP: I tried (as it is said in the curses-intro doc)
  // that I have to do endwin and wrefresh and all will be ok...
  // but it is not... :(
  //////////////////////////////////////////

  srand( time( NULL ) );
  do_draw = 1;

  vfu_read_files();
}

/*--------------------------------------------------------------------------*/

void vfu_exit_path( const char *a_path )
{
  chdir( a_path );

  #ifdef _TARGET_GO32_
  return; // this is meaningless under DOS
  #else

  VString str;
  if ( getenv( "VFU_EXIT" ) )
    str = getenv( "VFU_EXIT" );
  else
    {
    str = tmp_path;
    str_fix_path( str );
    str += "vfu.exit.";
    str += user_id_str;
    }

  FILE *f = fopen( str, "wt" );
  file_set_mode_str( str, "-rw-------" );
  if (!f) return;
  fputs( a_path, f);
  fclose(f);
  #endif
}

/*--------------------------------------------------------------------------*/
/* return 0 for exit-confirmed! */
int vfu_exit( const char* a_path )
{
  int z;
  mb.undef();
  mb.push( "X Exit (to startup path)" );
  mb.push( "Q Quit (to work path)   " );

  if ( a_path == NULL )
    {
    vfu_beep();
    z = vfu_menu_box( 50, 5, "  Exit VFU?" );
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
  int ch = 0;
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
    if ( work_mode == WM_NORMAL && files_count > 0 && oldFLI != FLI )
      {
      oldFLI = FLI;
      const char* fn = files_list[FLI]->full_name();
      file_save( "/tmp/vfu-quick-view", (void*)fn, strlen( fn ) );
      }
    */
    show_pos( FLI+1, files_count ); /* FIXME: should this be in vfu_redraw()? */

    ch = con_getch();
    if( ch == 0 ) ch = KEY_CTRL_L;
    if ( ch >= 'A' && ch <= 'Z' ) ch = tolower( ch );
    say1( "" );
    if ( user_id_str == "root" )
      say2center( "*** WARNING: YOU HAVE GOT ROOT PRIVILEGES! ***" );
    else
      say2( "" );

    if ( work_mode == WM_NORMAL || work_mode == WM_ARCHIVE ) switch (ch)
      { /* actually this is ANY work_mode (since there are two modes only) */
      case '1'       :
      case '2'       :
      case '3'       :
      case '4'       :
      case '5'       :
      case '6'       :
      case '7'       :
      case '8'       :
      case '0'       : vfu_toggle_view_fields( ch ); break;

      case '.'       : vfu_toggle_view_fields( ch );
                       vfu_rescan_files( 0 ); break;

      case 's'       : vfu_inc_search( 0 ); break;
      case KEY_ALT_S : vfu_inc_search( 1 ); break;

      case KEY_CTRL_L: do_draw = 3; break;

      case 'q'       : if( vfu_exit( work_path ) == 0 ) return; break;

      case KEY_ALT_X :
      case 'x'       : if( vfu_exit( startup_path ) == 0 ) return; break;

      case 27        : if( vfu_exit( NULL ) == 0 ) return; break;

      case KEY_UP    : vfu_nav_up(); break;
      case KEY_DOWN  : vfu_nav_down(); break;
      case KEY_PPAGE : vfu_nav_ppage(); break;
      case KEY_NPAGE : vfu_nav_npage(); break;

      case KEY_CTRL_A    :
      case KEY_HOME  : vfu_nav_home(); break;
      case KEY_CTRL_E    :
      case KEY_END   : vfu_nav_end(); break;

      case 'h' : vfu_help(); break;

      case 'f'        : vfu_change_file_mask( NULL ); break;
      case KEY_CTRL_F : vfu_change_file_mask( "*" ); break;

      case KEY_CTRL_D : tree_view(); break;
      case KEY_ALT_R  : vfu_read_files_menu(); break;

      /* this will be in alt+r menu
      case 'R' : con_cs(); vfu_refresh_all_views(); do_draw = 1; break;
      */
      case KEY_CTRL_R : vfu_rescan_files( 1 ); break;
      case 'r'        : vfu_rescan_files( 0 ); break;

      case ' ' : vfu_nav_select(); break;

  #ifdef _TARGET_UNIX_
      case KEY_BACKSPACE :
  #endif
      case 8   :
      case '-' : vfu_action_minus(); break;

      case 13  :
      case '+' :
      case '=' : vfu_action_plus( ch ); break;

      case KEY_LEFT  : if (opt.lynx_navigation) vfu_action_minus(); break;
      case KEY_RIGHT : if (opt.lynx_navigation)
                         vfu_action_plus( '+' );
                       else
                         if ( work_mode == WM_NORMAL )
                           vfu_rename_file_in_place();
                       break;

      case 'd' : vfu_chdir( NULL ); break;
      case KEY_ALT_D : vfu_chdir_history(); break;

      case KEY_ALT_EQ :
      case '>' : opt.long_name_view = !opt.long_name_view;
                 vfu_drop_all_views();
                 do_draw = 1;
                 break;

      case 'a' : vfu_arrange_files(); break;

      case 'g' : vfu_global_select(); break;

      case 'o' : vfu_options(); break;

      case 'v' : vfu_edit_conf_file(); break;

      case '!' :
      case '?' : con_cs();
                 vfu_shell( shell_prog, 0 );
                 do_draw = 1;
                 break;

      case 'u'        : vfu_user_menu(); break;


      /* not documented unless here :) */
      case KEY_CTRL_T  :
        {
        char s[128];
        say1( "Timing screen draws (x1000)..." );
        clock_t t = clock();
        for(int z = 0; z < 1000; z++) vfu_redraw();
        t = clock() - t;
        sprintf(s,"Draw speed: %f dps.",(100.0/((double)t/CLOCKS_PER_SEC)));
        say1(s);
        break;
        }

      case '*' : FGO( rand() % files_count );
                 do_draw = 1;
                 break;

      case 'z'        : vfu_directories_sizes(  0  ); break;
      case KEY_ALT_Z  : vfu_directories_sizes( 'A' ); break;
      case KEY_CTRL_Z : vfu_directories_sizes( 'Z' ); break;
      }
    if ( work_mode == WM_ARCHIVE ) switch (ch)
      {
      case 'c' : vfu_extract_files( 0 ); break;
      case KEY_ALT_C : vfu_extract_files( 1 ); break;
      }
    if ( work_mode == WM_NORMAL ) switch (ch)
      {
      case 'b' :
      case KEY_ALT_B : if ( ch == 'b' && sel_count > 0 )
                         vfu_browse_selected_files();
                       else
                         {
                         if ( files_count > 0 )
                           vfu_browse( files_list[FLI]->name(), ch == KEY_ALT_B );
                         else
                           say1( "No files" );
                         }
                       break;

      case 'n' : vfu_file_find( 0 ); break;
      case KEY_ALT_N  : vfu_file_find( 1 ); break;

      case '~' : vfu_chdir( home_path ); break;

      case '/' : vfu_command(); break;

      case 'i' : if ( files_count > 0 )
                   vfu_edit( files_list[FLI]->name() );
                 else
                   say1( "No files");
                 break;

      case 'm'        : vfu_copy_files(sel_count == 0, CM_MOVE); break;
      case KEY_ALT_M  : vfu_copy_files(1, CM_MOVE); break;

      case 'c'        : vfu_copy_files(sel_count == 0, CM_COPY); break;
      case KEY_ALT_C  : vfu_copy_files(1, CM_COPY); break;

      case 'l'        : vfu_copy_files(sel_count == 0, CM_LINK); break;
      case KEY_ALT_L  : vfu_copy_files(1, CM_LINK); break;

      case 'e'        : vfu_erase_files(sel_count == 0); break;
      case KEY_ALT_E  : vfu_erase_files(1); break;

      case 'j'        : vfu_jump_to_mountpoint( 0 ); break;
      case KEY_ALT_J  : vfu_jump_to_mountpoint( 1 ); break;

      case KEY_ALT_1  : bookmark_goto( '1' ); break;
      case KEY_ALT_2  : bookmark_goto( '2' ); break;
      case KEY_ALT_3  : bookmark_goto( '3' ); break;
      case KEY_ALT_4  : bookmark_goto( '4' ); break;
      case KEY_ALT_5  : bookmark_goto( '5' ); break;
      case KEY_ALT_6  : bookmark_goto( '6' ); break;
      case KEY_ALT_7  : bookmark_goto( '7' ); break;
      case KEY_ALT_8  : bookmark_goto( '8' ); break;
      case KEY_ALT_9  : bookmark_goto( '9' ); break;
      case '`'        : bookmark_goto(-1 ); break;

      case 9          : vfu_edit_entry(); break;

      case 't'        : vfu_tools(); break;

      case 'p'        : clipboard_menu( 0 ); break;
      /*
      case KEY_CTRL_C : vfu_clipboard( 'C' ); break; // copy
      case KEY_CTRL_X : vfu_clipboard( 'X' ); break; // cut
      case KEY_CTRL_V : vfu_clipboard( 'V' ); break; // paste
      */

      }
    if (  ( KEY_F1 <= ch && ch <= KEY_F10)
       || ( KEY_SH_F1 <= ch && ch <= KEY_SH_F10)
       || ( KEY_ALT_F1 <= ch && ch <= KEY_ALT_F10)
       || ( KEY_CTRL_F1 <= ch && ch <= KEY_CTRL_F10)
       || ( ch == KEY_IC) )
           vfu_user_external_exec( ch );
    }
}

/*--------------------------------------------------------------------------*/

void vfu_help_cli()
{
  printf( "%s",
    HEADER
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
    "  compile date: " __DATE__ "\n"
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
            #ifdef _TARGET_GO32_
          str_tr( temp, "\\", "/" );
          #endif
          vfu_chdir( temp );
          break;
      case 'r'  : con_out(1,1,HEADER,cINFO);
                temp = "Rebuilding directory tree ( work_path is";
          temp += work_path;
          temp += " )";
          say2( temp );
          tree_rebuild();
          break;
      case 't'  : con_out(1,1,HEADER,cINFO);
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

void vfu_reset_screen()
{
  con_done();
  con_init();
  con_chide();

  /* update scroll parameters */
  file_list_index.set_min_max( 0, files_count - 1 );
  file_list_index.set_pagesize( con_max_y() - 7 );
  FGO( file_list_index.pos() );

  vfu_drop_all_views();
  vfu_redraw();
  vfu_redraw_status();
}

void vfu_signal( int sig )
{
  /* there is no simple solution... :/
  if ( sig == SIGWINCH )
    {
    signal( SIGWINCH, vfu_signal ); // (re)setup signal handler
    do_draw = 3;
    return;
    }
  */
  vfu_done();

  con_beep();
  con_cs();
  con_cshow();
  con_done();

  printf( "vfu: signal received: %d -- terminated\n", sig );
  exit(200);
}

/*--------------------------------------------------------------------------*/

void vfu_toggle_view_fields( int ch )
{
  switch( ch )
    {
    case '1' : opt.f_mode = !opt.f_mode; break;
    case '2' : opt.f_owner = !opt.f_owner; break;
    case '3' : opt.f_group = !opt.f_group; break;
    case '4' : opt.f_time = !opt.f_time; break;
    case '5' : opt.f_size = !opt.f_size; break;
    case '6' : opt.f_type = !opt.f_type; break;
    case '7' : opt.f_time_type++;
               if (opt.f_time_type > 2)
                 opt.f_time_type = 0;
               break;
    case '8' : opt.f_mode  =
               opt.f_owner =
               opt.f_group =
               opt.f_time  =
               opt.f_size  =
               opt.f_type  = 1; break;
    case '0' : opt.long_name_view = !opt.long_name_view; break;
    case '.' : opt.show_hidden_files = !opt.show_hidden_files; break;
    default  : return; /* cannot be reached really */
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
    sl = str_dot_reduce( sl, con_max_x()-1 );
    say2( sl );
    con_getch();
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

  chdir( work_path ); /* in case SHELL changed directory... (DOS only :)) */

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
  for( z = 0; z < files_count; z++ )
    {
    s = files_list[z]->size();
    if ( files_list[z]->sel )
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
  if ( files_count == 0 )
    {
    say1( "No files");
    return;
    }
  if ( files_list[FLI]->is_dir() )
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
      str_replace( str, "%f", fname );
      str_replace( str, "%F", fname );
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

  for ( z = 0; z < files_count; z++ )
    if ( files_list[z]->sel )
      if ( !files_list[z]->is_dir() )
        SeeAddFile( files_list[z]->full_name() );
  //------
  int z;
  for ( z = 0; z < files_count; z++ )
    if ( files_list[z]->sel )
      if ( !files_list[z]->is_dir() )
        SeeAddFile( files_list[z]->full_name() );
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

  if ( !no_filters && see_filters.count() > 0 )
    {
    char full_fname[MAX_PATH];
    expand_path( fname, full_fname );
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
      str_replace( str, "%f", fname );
      str_replace( str, "%F", full_fname );
      str += " > ";
      str += tmp_name;
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
      str_replace( str, "%f", fname );
      str_replace( str, "%F", fname );
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

void vfu_action_plus( int key )
{
  if ( files_count == 0 ) return;

  TF *fi = files_list[FLI];

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
      if ( key == KEY_ENTER && vfu_user_external_find( KEY_ENTER, fi->ext(), fi->type_str(), NULL ) != -1 )
        vfu_user_external_exec( KEY_ENTER );
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
    if ( key == KEY_ENTER && vfu_user_external_find( KEY_ENTER, fi->ext(), fi->type_str(), NULL ) != -1 )
        vfu_user_external_exec( KEY_ENTER );
    else
      { /* file */
      vfu_browse_archive_file();
      }
    }
}

/*--------------------------------------------------------------------------*/

void vfu_action_minus()
{
  VString o = work_path; /* save old path i.e. current */

  if ( work_mode == WM_NORMAL )
    {
      #ifdef _TARGET_GO32_
        if ( work_path[1] == ':' && work_path[2] == '/' && work_path[3] == 0 )
           return;
      #else
        if ( work_path[0] == '/' && work_path[1] == 0 )
          return;
      #endif
      vfu_chdir( ".." );
    } else
  if ( work_mode == WM_ARCHIVE )
    {
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
  for ( z = 0; z < files_count; z++ )
    {
    VString n;
    if ( work_mode == WM_ARCHIVE )
      n = archive_path;
    else
      n = work_path;
    n += files_list[z]->name();
    n += "/";
    if ( pathcmp( o, n ) == 0 )
      {
      FGO(z);
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
  char fn1[MAX_PATH];
  char fn2[MAX_PATH];
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

TF* fi = files_list[FLI];

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
for (z = 0; z < files_count; z++)
  {
  fi = files_list[z];
  int sel = 0;
  switch( same_mode )
    {
    case GSAME_NAME  : sel = (pathcmp(same_str, fi->name()) == 0);
                       break;
    case GSAME_EXT   : sel = (pathcmp(same_str, fi->ext()) == 0);
                       break;
    case GSAME_SIZE  : sel = (same_fsize == fi->size());
                       if ( fi->is_dir() ) sel = 0; break;
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
    case GSAME_OWNER : sel = ((unsigned int)same_int == fi->st()->st_uid); break;
    case GSAME_GROUP : sel = ((unsigned int)same_int == fi->st()->st_gid); break;
    case GSAME_MODE  : sel = ((unsigned int)same_int == fi->st()->st_mode); break;
    case GSAME_TYPE  : sel = ( same_str == fi->type_str()); break;
    }
  fi->sel = sel;
  }
}

void vfu_global_select()
{
  char ch;

  mb.undef();
  mb.push( "S All" );
  mb.push( "A All (+Dirs)" );
  mb.push( "R Reverse" );
  mb.push( "C Clear" );
  mb.push( "P Pack" );
  mb.push( "H Hide" );
  mb.push( "D Different" );
  mb.push( ". Hide dirs" );
  mb.push( ", Hide dotfiles" );
  mb.push( "= Mask add (+dirs)" );
  mb.push( "+ Mask add (-dirs)" );
  mb.push( "- Mask sub        " );
  mb.push( "L Same..." );
  mb.push( "X EXtended select..." );
  if ( vfu_menu_box( 50, 5, "Global Select" ) == -1 ) return;
  ch = menu_box_info.ec;
  if (ch == 'X')
    {
    if ( work_mode != WM_NORMAL )
      {
      say1( "GlobalSelect/Extended not available in this mode." );
      return;
      }
    mb.undef();
    mb.push( "A Select to begin" );
    mb.push( "E Select to end" );
    mb.push( "--searching--" );
    mb.push( "F Find string (no case)" );
    mb.push( "S Scan string (case sense)" );
    mb.push( "H Hex  string" );
    mb.push( "/ Regular expression" );
    mb.push( "\\ Reg.exp (no case)" );
//    mb.push( "--other--" );
//    mb.push( "M Mode/Attributes" );
    if ( vfu_menu_box( 50, 5, "Extended G.Select" ) == -1 ) return;
    ch = menu_box_info.ec;
    if (ch == 'S') ch = 'B'; /* 'B' trans scan */
    if (ch == 'H') ch = 'E'; /* 'E' trans hex  */
    if (ch == 'A') ch = '<'; /* '<' trans to begin  */
    if (ch == 'E') ch = '>'; /* '>' trans to end    */
    }

  switch(ch)
    {
    case 'S' : {
               for (int z = 0; z < files_count; z++)
                 if (!files_list[z]->is_dir())
                   files_list[z]->sel = 1;
               } break;
    case 'A' : {
               for (int z = 0; z < files_count; z++)
                 files_list[z]->sel = 1;
               } break;
    case 'R' : {
               int z;
               for (z = 0; z < files_count; z++)
                 if (!files_list[z]->is_dir())
                   files_list[z]->sel = !files_list[z]->sel;
               } break;
    case 'C' : {
               int z;
               for (z = 0; z < files_count; z++)
                   files_list[z]->sel = 0;
               } break;
    case 'P' :
               {
               int z;
               for (z = 0; z < files_count; z++)
                 {
                 if (!files_list[z]->sel)
                   {
                   delete files_list[z];
                   files_list[z] = NULL;
                   }
                 }
               vfu_pack_files_list();
               } break;
    case 'H' :
               {
               int z;
               for (z = 0; z < files_count; z++)
                 {
                 if (files_list[z]->sel)
                   {
                   delete files_list[z];
                   files_list[z] = NULL;
                   }
                 }
               vfu_pack_files_list();
               } break;
    case '.' :
               {
               int z;
               for (z = 0; z < files_count; z++)
                 {
                 if (files_list[z]->is_dir())
                   {
                   delete files_list[z];
                   files_list[z] = NULL;
                   }
                 }
               vfu_pack_files_list();
               } break;
    case ',' :
               {
               int z;
               for (z = 0; z < files_count; z++)
                 {
                 if (files_list[z]->name()[0] == '.')
                   {
                   delete files_list[z];
                   files_list[z] = NULL;
                   }
                 }
               vfu_pack_files_list();
               } break;
    case '+' :
    case '=' :
    case '-' :
              {
              VString m;
              int selaction = 0;
              if (ch != '-') selaction = 1;
              if (ch == '+')
                say1("Select by mask: (w/o directories)");
              else
              if (ch == '=')
                say1("Select by mask: (with directories)");
              else
                say1("Deselect by mask:");
              if ( vfu_get_str( "", m, HID_GS_MASK ))
                {
                VArray sm;
                sm = str_split( " +", m );
                while( (m = sm.pop()) != "" )
                  {
                  if (opt.mask_auto_expand)
                    vfu_expand_mask( m );
                  int z = 0;
                  for (z = 0; z < files_count; z++)
                    {
                    if (files_list[z]->is_dir() && ch == '+') continue;
                    if (FNMATCH(m,files_list[z]->name_ext()) == 0)
                      files_list[z]->sel = selaction;
                    }
                  }
                }
              say1( " " );
              say2( " " );
              } break;
    case 'D' :
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
                for (z = 0; z < files_count; z++)
                  {
                  if ( files_list[z]->is_dir() ) continue;
                  say1( files_list[z]->name() );
                  files_list[z]->sel =
                      (vfu_cmp_files_crc32( work_path, target,
                        files_list[z]->name() ) != 0 );
                  }
                }
              say1( "Done." );
              say2( " " );
              } break;
    case '/':
    case '\\':
    case 'E':
    case 'F':
    case 'B': {
              say1("");
              VString pat;
              if ( vfu_get_str( "Search string: ", pat, HID_GS_GREP ) )
                {
                fsize_t size = 0;
                say1("");
                say2("");

                size = 0;
                for ( int z = 0; z < files_count; z++ )
                  {
                  size += files_list[z]->size();
                  if ( files_list[z]->is_dir() ) continue;

                  int pos = -1;
                  switch( ch )
                    {
                    case 'F':
                       pos = file_string_search( pat, files_list[z]->name(), "i" );
                       break;
                    case 'B':
                       pos = file_string_search( pat, files_list[z]->name(), "" );
                       break;
                    case 'E':
                       pos = file_string_search( pat, files_list[z]->name(), "h" );
                       break;
                    case '/':
                       pos = file_string_search( pat, files_list[z]->name(), "r" );
                       break;
                    case '\\':
                       pos = file_string_search( pat, files_list[z]->name(), "ri" );
                       break;
                    }

                  files_list[z]->sel = ( pos > -1 );

                  char s[128];
                  snprintf( s, sizeof(s),
                              "Scanning %4.1f%% (%12.0f bytes in %s ) ",
                              (100.0 * size) / (files_size+1.0),
                              files_list[z]->size(),
                              files_list[z]->name() );
                  say1( s );
                  }
                }
              say1("");
              say2("");
              break;
              }

    case 'L':
              {
              mb.undef();
              mb.push( "N Name" );
              mb.push( "E Extension" );
              mb.push( "S Size" );
              mb.push( "T Time" );
              mb.push( "I Time (1 min.round)" );
              mb.push( "D Date" );
              mb.push( "M Date+Time" );
              mb.push( "A Attr/Mode" );
              #ifndef _TARGET_GO32_
              mb.push( "O Owner" );
              mb.push( "G Group" );
              #endif
              mb.push( "Y Type (TP)" );

              vfu_menu_box( 50, 5, "Select Same..." );
              ch = menu_box_info.ec;
              switch ( ch )
                {
                case 'N' : vfu_global_select_same( GSAME_NAME  ); break;
                case 'E' : vfu_global_select_same( GSAME_EXT   ); break;
                case 'S' : vfu_global_select_same( GSAME_SIZE  ); break;
                case 'M' : vfu_global_select_same( GSAME_DATETIME  ); break;
                case 'T' : vfu_global_select_same( GSAME_TIME  ); break;
                case 'I' : vfu_global_select_same( GSAME_TIME1 ); break;
                case 'D' : vfu_global_select_same( GSAME_DATE  ); break;
                case 'O' : vfu_global_select_same( GSAME_OWNER ); break;
                case 'G' : vfu_global_select_same( GSAME_GROUP ); break;
                case 'A' : vfu_global_select_same( GSAME_MODE  ); break;
                case 'Y' : vfu_global_select_same( GSAME_TYPE  ); break;
                }
              } break;
    case 'M': {
              mode_str_t mode_str;
              strcpy( mode_str, MODE_STRING );
              if(vfu_edit_attr( mode_str, 0 ))
                {
                for ( int z = 0; z < files_count; z++ )
                  files_list[z]->sel =
                     (strcmp( files_list[z]->mode_str()+1, mode_str+1 ) == 0);
                do_draw = 1;
                }
              } break;
    case '<' : {
               if( files_count > 0)
                 for (int z = 0; z <= FLI; z++)
                   if (!files_list[z]->is_dir())
                     files_list[z]->sel = 1;
               } break;
    case '>' : {
               if( files_count > 0)
                 for (int z = FLI; z < files_count; z++)
                   if (!files_list[z]->is_dir())
                     files_list[z]->sel = 1;
               } break;
    }
  update_status();
  do_draw = 1;
}

/*--------------------------------------------------------------------------*/

int vfu_user_external_find( int key, const char* ext, const char* type, VString *shell_line )
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
    if ( key_by_name( split[1] ) != key ) continue; /* if key not the same -- skip */
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

void vfu_user_external_exec( int key )
{
  if ( files_count == 0 )
    {
    say1( "Directory is empty: user externals are disabled!" );
    return;
    }
  VString shell_line;
  TF *fi = files_list[FLI];
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
    char t[128];
    snprintf( t, sizeof(t), "No user external defined for this key and extension (%d,%s)", key, fi->ext() );
    say1( t );
    }
}

/*--------------------------------------------------------------------------*/

VString tools_last_target;
void vfu_tools()
{
  mb.undef();
  mb.push( "R Real path" );
  mb.push( "D ChDir to symlink path" );
  mb.push( "G Go to symlink target" );
  mb.push( "B Go back to last target" );
  mb.push( "T Make directory" );
  mb.push( "P Path bookmarks" );
  mb.push( "A Rename tools..." );
  mb.push( "C Classify files" );
  if ( vfu_menu_box( 50, 5, "Tools" ) == -1 ) return;
  switch( menu_box_info.ec )
    {
    case 'R' : {
               char s[MAX_PATH];
               expand_path(files_list[FLI]->name(), s);
               say1( s );
               break;
               }
    case 'D' : {
               if( ! files_list[FLI]->is_link() ) break;
               tools_last_target = files_list[FLI]->full_name();
               if( ! files_list[FLI]->is_dir() ) break;
               vfu_chdir( expand_path( files_list[FLI]->name() ) );
               break;
               }
    case 'G' : {
               if( ! files_list[FLI]->is_link() ) break;
               tools_last_target = files_list[FLI]->full_name();
               VString target = vfu_readlink( files_list[FLI]->full_name() );
               vfu_chdir( expand_path( str_file_path( target ) ) );
               vfu_goto_filename( str_file_name_ext( target ) );
               break;
               }
    case 'B' : {
               if( tools_last_target == "" ) break;
               VString target = tools_last_target;
               tools_last_target = files_list[FLI]->full_name();
               vfu_chdir( expand_path( str_file_path( target ) ) );
               vfu_goto_filename( str_file_name_ext( target ) );
               break;
               }
    case 'T' : {
               VString str;
               if (vfu_get_str( "Make directory(ies) (use space for separator)",
                                str, HID_MKPATH ))
                 {
                 int err = 0;
                 int z;
                 VArray ms;
                 ms = str_split( " +", str );
                 for ( z = 0; z < ms.count(); z++ )
                   if( make_path(ms[z]) )
                     {
                     say1( "Cannot create directory:" );
                     say2( ms[z] );
                     con_getch();
                     err++;
                     }
                 if ( err == 0 ) say1( "MKDIR: ok." );
                 break;
                 }
               }
               break;
    case 'P' : bookmark_goto( -1 ); break;
    case 'A' : vfu_tool_rename(); break;
    case 'C' : vfu_tool_classify(); break;
    }
}

/*--------------------------------------------------------------------------*/

void bookmark_goto( int n )
{
  VString t;
  if ( n == -1 )
    {
    int z;
    mb.undef();
    mb.push( "A Bookmark current directory" );
    mb.push( "` Change working directory" );
    mb.push( "---" );
    for( z = 1; z < 10; z++ )
      {
      const char* ps = path_bookmarks.get( z-1 );
      if( !ps ) break;
      sprintf(t, "%d %s", z%10, ps );
      mb.push( str_dot_reduce( t, 60 ) );
      }
    n = vfu_menu_box( 5, 5, "Path bookmarks");
    if ( n == -1 ) return;
    n = menu_box_info.ec;
    }
  // FIXME: neshto ne raboti :/
  switch( n )
    {
    case '`' : vfu_chdir( NULL ); return;
    case 'A' : bookmark_hookup(); return;
    }
  if ( n >= '1' && n <= '9' && str_len( path_bookmarks[ n - '1' ] ) > 0 )
    {
    vfu_chdir( path_bookmarks[ n - '1' ] );
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
  if ( files_count <= 0 )
    {
    say1( "No files" );
    return;
    }
  
  
  TF *fi = files_list[FLI];

  int y = (file_list_index.pos() - file_list_index.page()) + 4;
  int x = tag_mark_pos + 3;

  VString str = fi->name();

  if(TextInput( x, y, "", MAX_PATH, con_max_x() - tag_mark_pos - 4, &str ) &&
     strcmp( fi->name(), str.data() )
    )
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
void vfu_directories_sizes( int n )
{
  int z;
  char t[256];
  n = toupper( n );
  if ( n == 0 )
    {
    mb.undef();
    mb.push( "E Specify directory" );
    mb.push( "Z Directory under cursor" );
    mb.push( ". Current directory `.'" );
    mb.push( "S Selected directories" );
    mb.push( "A All dir's in the list" );
    if ( vfu_menu_box( 5, PS - 4, "Directory size of:" ) == -1 ) return;
    n = menu_box_info.ec;
    }

  say1( "Calculating files size. Press ESCAPE to cancel calculation." );
  if ( n == 'E' || n == '.' ) /* specific directory */
    {
    VString target = work_path;
    if ( n == '.' )
      target = work_path;
    else
      if ( !vfu_get_dir_name( "Calculate size of directory: ", target ) ) return;
    fsize_t dir_size = vfu_dir_size( target );
    if ( dir_size == -1 ) return;
    VString dir_size_str;
    dir_size_str.fi( dir_size );
    vfu_str_comma( dir_size_str );
    snprintf( t, sizeof(t), "Dir size of: %s", target.data() );
    say1( t );
    snprintf( t, sizeof(t), "Size: %s bytes", dir_size_str.data() );
    say2( t );
    } else
  if ( n == 'A' || n == 'S' ) /* all or selected  */
    {
    for( z = 0; z < files_count; z++)
      {
      TF *fi = files_list[z];
      if ( fi->is_dir() ) /* dirs */
        {
        if ( n == 'S' && !fi->sel ) continue; /* if not sel'd and required -- skip */
        /* if ( n == 'A' ) continue; // all */
        fsize_t dir_size = vfu_dir_size( fi->name(), 0 );
        if ( dir_size == -1 )
          break;
        fi->set_size( dir_size );
        }
      }
    size_cache_sort();
    say1("");
    say2("");
    }  else
  if ( n == 'Z' ) /* single one, under cursor  */
    {
    VFU_CHECK_LIST_POS( FLI );
    if ( files_list[FLI]->is_dir() )
      {
      files_list[FLI]->set_size( vfu_dir_size( files_list[FLI]->name() ) );
      say1("");
      say2("");
      }
    else
      say1("This is not directory...");
    }

  do_draw = 1;
  update_status();
  if ( opt.sort_order == 'S' && n < 0 ) vfu_sort_files();
}

/*--------------------------------------------------------------------------*/

void vfu_edit_entry( )
{
  char errstr[128];

  int one = ( sel_count == 0 );
  int z;
  VString str;

  mb.undef();
  mb.push( "M Mode" );
  mb.push( "A Octal Mode" );
  mb.push( "O Owner/Group" );
  mb.push( "N Name (TAB)" );
  mb.push( "T Time/Touch Mod+Acc Times" );
  mb.push( "I Modify Time" );
  mb.push( "E Access Time" );
  mb.push( "L Edit SymLink Reference" );
  if ( sel_count )
    { /* available only when selection exist */
    mb.push( "--");
    mb.push( "+ Target: Toggle" );
    mb.push( "C Target: Current File" );
    mb.push( "S Target: Selection" );
    }

  while(1)
    {
    while(1)
      {
      str = "Edit entry: ";
      str += one ? "current file" : "[ SELECTION ] ";
      menu_box_info.ac = 9;
      z = vfu_menu_box( 50, 5, str );
      if ( z == -1 ) return; /* canceled */
      if (menu_box_info.ac == -2 ) menu_box_info.ec = 'N';
      if (menu_box_info.ec == '+') { one = !one; continue; }
      if (menu_box_info.ec == 'S') { one = 0; continue; }
      if (menu_box_info.ec == 'C') { one = 1; continue; }
      break;
      }

    if ( menu_box_info.ec == 'N' ) /* name (rename) */
      {
      vfu_rename_file_in_place();
      break;
      } else
    if ( menu_box_info.ec == 'M' ||
         menu_box_info.ec == 'A' ) /* attributes/mode */
      {
        mode_str_t new_mode;
        int ok = 1;
        int err = 0;
        if ( menu_box_info.ec == 'M' )
          {
          if (one)
            {
            strcpy( new_mode, files_list[FLI]->mode_str() );
            file_get_mode_str( files_list[FLI]->name(), new_mode);
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
          sscanf( str, "%o", &m );
          file_get_mode_str( m, new_mode );
          ok = (z && str_len(str) > 0);
          }
        if( ok )
          {
          for ( z = 0; z < files_count; z++ )
            if ( (one && FLI == z) || (!one && files_list[z]->sel) )
              {
              TF *fi = files_list[z];
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
    if ( menu_box_info.ec == 'T' ||
         menu_box_info.ec == 'I' ||
         menu_box_info.ec == 'E' )
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
        for ( z = 0; z < files_count; z++ )
          if ( (one && FLI == z) || (!one && files_list[z]->sel) )
            {
            TF *fi = files_list[z];
            tb.actime  = fi->st()->st_atime;
            tb.modtime = fi->st()->st_mtime;
            if (menu_box_info.ec == 'M') tb.modtime = new_time;
            if (menu_box_info.ec == 'S') tb.actime  = new_time;
            if (menu_box_info.ec == 'T') tb.modtime = new_time;
            if (menu_box_info.ec == 'T') tb.actime  = new_time;
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
    if ( menu_box_info.ec == 'O' )
      {
        #ifdef _TARGET_GO32_
        say1( "Change owner/group function is not supported under DOS filesystem" );
        break;
        #endif

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
        for ( z = 0; z < files_count; z++ )
          if ( (one && FLI == z) || (!one && files_list[z]->sel) )
            {
            TF *fi = files_list[z];
            int u = uid;
            int g = gid;
            if (u == -1) u = fi->st()->st_uid;
            if (g == -1) g = fi->st()->st_gid;
            if(chown(fi->name(), u, g) == 0)
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
      #ifdef _TARGET_GO32_
      say1( "Edit SymLink reference is not supported under DOS filesystem" );
      #else
      if (!one)
        {
        say1( "Cannot edit symlink reference for selection..." );
        break;
        }
      TF* fi = files_list[FLI];
      if ( !fi->is_link() )
        {
        say1( "This is not a symlink..." );
        break;
        }
      char t[MAX_PATH] = "";
      t[ readlink( fi->name(), t, MAX_PATH-1 ) ] = 0;
      VString str = t;
      if ( vfu_get_str( "", str, 0 ) )
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
      #endif
      break;
      } else
    ;
    }
  return;
}

/*--------------------------------------------------------------------------*/

void vfu_jump_to_mountpoint( int all )
{
  VString str;
  char t[2048];
  int z;
  VArray va;
#ifdef _TARGET_UNIX_
  if ( va.fload( "/etc/mtab" ) ) return;
#endif
#ifdef _TARGET_GO32_
  str = home_path;
  str += "_vfu.mtb";
  if ( va.fload( str ) return;
  if (all)
    {
    va.ins( 0, "-  b:/" );
    va.ins( 0, "-  a:/" );
    }
#endif
  if (va.count() < 1) return;

  mb.undef();
  for(z = 0; z < va.count(); z++)
    {
    str = va[z];
    str_cut( str, " \t");
    str_word( str, " \t", t ); /* get device name */
    str_cut( str, " \t");
    str_word( str, " \t", t ); /* get mount point */
    va.set( z, t ); /* replace line with mount point only */

    struct statfs stafs;
    statfs( t, &stafs );
    int hk = ('A'+ z); /* hot key */
    #ifdef _TARGET_GO32_
    if (toupper(t[0]) >= 'A' && toupper(t[0]) <= 'Z' && toupper(t[1]) == ':')
      hk = toupper(t[0]);
    #endif
    sprintf( str, "%c | %9s | %9s | %-20s ",
             hk,
             (const char*)size_str_compact( stafs.f_bsize * ( opt.show_user_free ? stafs.f_bavail : stafs.f_bfree ) ),
             (const char*)size_str_compact( stafs.f_bsize * stafs.f_blocks ),
             //stafs.f_bsize * ( opt.show_user_free ? stafs.f_bavail : stafs.f_bfree ) / (1024.0*1024.0),
             //stafs.f_bsize * stafs.f_blocks / (1024.0*1024.0),
             t
             );

    mb.push(str);
    }
  menu_box_info.ac = KEY_CTRL_U;
  z = vfu_menu_box( 20, 5, "Jump to mount-point (free/total) Ctrl+U=umount" );
  if ( z == -1 )   return;
  if (menu_box_info.ac == -2)
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
    vfu_chdir( va[z] );
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
    mb.push( des );
    }

  if ( mb.count() == 0 )
    {
    say1("No menu user externals defined...");
    return;
    }
  z = vfu_menu_box( 5, 5, "User menu (externals) " );
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
  int z = con_full_box( 1, 1, "VFU File find results", &file_find_results, &bi );

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
    for( z = 0; z < files_count; z++ )
      if ( pathcmp( fname, files_list[z]->name_ext() ) == 0 )
        {
        FGO(z);
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

int __ff_process( const char* origin,    /* origin path */
                  const char* fname,     /* full file name */
                  const struct stat* st, /* stat struture or NULL */
                  int is_link,           /* 1 if link */
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
  file_find_results.push( str );
  str = str_dot_reduce( str, con_max_x()-1 );
  con_puts( "\r" );
  con_puts( str, cSTATUS );
  con_puts( "\n" );
  return 0;
}

void vfu_file_find( int menu )
{
  VString str;
  char ch;

  if (menu)
    {
    if ( vfu_menu_box("File find", "L Last find results,D Drop find results,N File find,F Find string (no case),S Scan string (case),B Scan string (case),E Hex string,/ Regular expresion,\\ Reg.exp (no case)", 5 ) == -1 ) return;
    ch = menu_box_info.ec;
    }
  else
    ch = 'N';
  if ( ch == 'L' )
    {
    if ( file_find_results.count() == 0 )
      file_find_results.fload( filename_ffr );
    vfu_file_find_results();
    return;
    }
  if ( ch == 'D' )
    {
    file_find_results.undef();
    vfu_file_find_results(); /* FIXME: this will show `no results' warning */
    return;
    }

  __ff_pattern = "";
  if ( str_find( "FSB/\\", ch ) != -1 ) /* we want search for pattern */
    {
    __ff_pattern = vfu_hist_get( HID_FFGREP, 0 );
    if (!vfu_get_str( "Enter search pattern: ", __ff_pattern, HID_FFGREP )) return;
    if (ch == 'F' ) __ff_opt = "i "; else
    if (ch == 'S' ) __ff_opt = "  "; else
    if (ch == 'B' ) __ff_opt = "  "; else
    if (ch == 'E' ) __ff_opt = "h "; else
    if (ch == '/' ) __ff_opt = "r "; else
    if (ch == '\\') __ff_opt = "ri"; else
    ;
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
  con_out( 1, 1, HEADER );
  sprintf( str, "Find mask: %s", vfu_hist_get( HID_FFMASK, 0 ) );
  con_out( 1, 2, str );
  sprintf( str, "Start path: %s", __ff_path.data() );
  con_out( 1, 3, str );
  if ( __ff_pattern != "" )
    {
    sprintf( str, "Containing pattern: %s", __ff_pattern.data() );
    con_out( 1, 4, str );
    }

  file_find_results.undef();
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
  mb.push( "T Rescan DirTree" );           list.push("");
  mb.push( "F Rescan Files" );             list.push("");
  mb.push( "R Rescan Files Recursive" );   list.push("");
  mb.push( "L Refresh all views/screen (Ctrl+L)" ); list.push("");
  if ( panelizers.count() > 0 )
    {
    mb.push( "--panelizers---" );         list.push("");
    for ( z = 0; z < panelizers.count(); z++ )
      {
      str = panelizers[z];
      str_word( str, ",", t );
      /* fix menu hotkeys */
      str_ins( t, 1, " " );
      str_set_ch( t, 0, toupper(str_get_ch(t, 0)) );
      mb.push(t);
      list.push(str);
      }
    }
  z = vfu_menu_box( 25, 5, "Read/Rescan Files" );
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
  switch(menu_box_info.ec)
    {
    case 'T' : tree_rebuild(); break;
    case 'F' : vfu_read_files( 0 ); break;
    case 'R' : vfu_read_files( 1 ); break;
    case 'L' : con_cs(); vfu_drop_all_views(); do_draw = 2; break;
    }
}

/*--------------------------------------------------------------------------*/

void vfu_inc_search( int use_last_one )
{
  VString str;
  int key;
  if( use_last_one && last_inc_search == "" )
    use_last_one = 0;
  if( use_last_one && last_inc_search != "" )
    str = last_inc_search;
  if( use_last_one )
    {
    say1( "Incremental search pattern: ( use ALT+S to find next matching entry )" );
    key = 9;
    }
  else
    {
    say1( "Enter incremental search pattern: ( use TAB to find next matching entry )" );
    key = con_getch();
    }
  VRegexp size_re("^size:(\\d+)$");
  while( ( key >= 32 && key <= 255 ) || key == 8 || key == KEY_BACKSPACE || key == 9 )
    {
    if ( key == 8 || key == KEY_BACKSPACE )
      str_trim_right( str, 1 );
    else
    if ( key != 9 )
      str_add_ch( str, key );
    say2( str );

    if ( files_count == 0 ) { key = con_getch(); continue; }

    int z;
    if ( key == 9 )
      {
      z = FLI+1;
      if ( z > file_list_index.max() ) z = file_list_index.min();
      }
    else
      z = FLI;

    int direction = 1;
    int found = 0;
    int loops = 0;
    VString s_mask = str;
    int s_size = 0;
    if( size_re.m( str ) )
      s_size = atoi( size_re[1] );
    else
      vfu_expand_mask( s_mask );
    while(1)
      {
      if ( z > file_list_index.max() ) z = file_list_index.min();
      if ( z < file_list_index.min() ) z = file_list_index.max();
      if( s_size )
        found = files_list[z]->size() == s_size;
      else
        if( opt.no_case_glob )
          found = ( FNMATCH_NC( s_mask, files_list[z]->name_ext() ) == 0 );
        else
          found = ( FNMATCH( s_mask, files_list[z]->name_ext() ) == 0 );
      if ( found ) break;
      z += direction;
      if ( loops++ > files_count ) break;
      }
    if (found)
      {
      FGO(z);
      vfu_redraw();
      show_pos( FLI+1, files_count );
      }
    if( use_last_one )
      break;
    else
      key = con_getch();
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
  if ( files_count == 0 ) return;

  for (int z = 0; z < files_count; z++)
    {
    if( strcmp( fname, files_list[z]->name_ext() ) ) continue;
    FGO(z);
    return;
    }
}

/*######################################################################*/

// #include <mcheck.h> /* memory allocation debug */
int main( int argc, char* argv[] )
{

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
  printf("%s\n<cade@biscom.net> [http://soul.datamax.bg/~cade/vfu]\nThank You for using VFU!\n\n", HEADER );
  */
  return 0;
}

/*######################################################################*/

