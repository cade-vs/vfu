/*
 *
 * (c) Vladi Belperchinov-Shabanski "Cade" 1996-2000
 * http://www.biscom.net/~cade  <cade@biscom.net>  <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 * $Id: vfu.cpp,v 1.4 2001/11/10 09:48:00 cade Exp $
 *
 */

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
  String work_path;
  /* archive context */
  String archive_name;
  String archive_path;
  PSZCluster archive_extensions;

  String external_panelizer;
  PSZCluster list_panelizer;
  
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
  TScrollPos file_list_index; 

  /* some world wide variables */
  String startup_path;
  String home_path;
  String tmp_path;
  String rc_path;

  /* files masks */
  String        files_mask;
  StrSplitter   files_mask_array( " " );

/*############################################ GLOBAL STRUCTS  #########*/

  PSZCluster dir_tree;
  int        dir_tree_changed;
  String     dir_tree_file;

  PSZCluster file_find_results; // filefind results

  String path_bookmarks[10];

/*######################################################################*/

  PSZCluster user_externals;
  PSZCluster history;
  PSZCluster see_filters;
  PSZCluster panelizers;

  PSZCluster mb; /* menu boxes */
  
  StrSplitter trim_tree( PATH_DELIMITER );

/*############################################ CONFIG SETTINGS #########*/

  String ext_colors[16];

  String shell_browser;
  String shell_editor;
  String shell_options;
  String shell_prog;
  
  String user_id_str;
  String group_id_str;
  String host_name_str;

  String filename_opt;
  String filename_conf;
  String filename_history;
  String filename_tree;
  String filename_size_cache;
  String filename_ffr; /* file find results */
  String filename_atl;

  char TF::_full_name[MAX_PATH]; 
  
/*######################################################################*/

  int do_draw;
  int do_draw_status;

/*######################################################################*/

/*
  Message issues
*/

void say1(const char *a_str, int attr )
{
  String s = a_str;
  str_dot_reduce( NULL, s, con_max_x()-1 );
  con_out( 1, con_max_y()-1, s, attr );
  con_ce( attr );
}

void say2(const char *a_str, int attr )
{
  String s = a_str;
  str_dot_reduce( NULL, s, con_max_x()-1 );
  con_out( 1, con_max_y(), s, attr );
  con_ce( attr );
}

void say2errno()
{
  String str = "error: ";
  str += strerror(errno);
  say2( str );
}

void say1center(const char *a_str, int attr )
{
  String str = " ";
  int sl = str_len( a_str );
  if ( sl < con_max_x() )
    {
    sl = ( con_max_x() - sl ) / 2;
    str_mul( str, sl );
    str = str + a_str;
    }
  say1( str, attr );
}

void say2center(const char *a_str, int attr )
{
  String str = " ";
  int sl = str_len( a_str );
  if ( sl < con_max_x() )
    {
    sl = ( con_max_x() - sl ) / 2;
    str_mul( str, sl );
    str = str + a_str;
    }
  say2( str, attr );
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
};

/*-----------------------------------------------------------------------*/

TF::TF()
{
  reset();
};

/*-----------------------------------------------------------------------*/

TF::TF( const char* a_name, const struct stat* a_stat, int a_is_link )
{
  reset();
  set_name( a_name );
  update_stat( a_stat, a_is_link );
};

/*-----------------------------------------------------------------------*/

TF::~TF()
{
  if ( _name ) delete _name;
  if ( _view ) delete _view;
  reset();
};

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
};

/*-----------------------------------------------------------------------*/

void TF::set_name( const char* a_new_name )
{
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
};

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
  delete _view;
  _view = NULL;
}

/*-----------------------------------------------------------------------*/

const char* TF::view()
{
  if ( !_view )
    refresh_view();
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
      String str;
      if ( _is_dir && _size == -1 )
        {
        str = "[DIR]";
        }
      else
        {
        str.setfi( _size );
        str_comma(str);
        }
      sprintf( stsize, "%14s", (const char*)(str) );
      strcat( stsize, " " ); /* field separator */
      }
  } /* if ( !opt.LongNameView ) */

  if (opt.f_type || opt.long_name_view )
    {
    strcpy( sttype, _type_str );
    /* there is no field separator here! */
    }

  String name_view = _name;
  
  #ifdef _TARGET_UNIX_
  /* links are supported only under UNIX */
  if ( _is_link )
    {
      name_view += " -> ";
      char t[MAX_PATH+1];
      int l = readlink( _name, t, MAX_PATH );
      if (l != -1) t[l] = 0;
      name_view += t;
    }
  #endif

  /* the three space separator below is for the tag and selection marks `>>#' */
  String view;
  view = view + stmode + stowner + stgroup + sttime + stsize + sttype + "   " + name_view;

  int x = con_max_x();
  if ( str_len( view ) > x )
    str_sleft( view, x );
  else
    str_pad( view, - x );  

  if ( _view ) delete _view;
  _view = new char[ con_max_x() + 1 ]; /* +1 for the zero :) */

  strcpy( _view, view );
  
  ASSERT( _view );
  ASSERT( strlen( _view ) == (size_t)con_max_x() );
}

/*-----------------------------------------------------------------------*/

void TF::update_stat( const struct stat* a_new_stat = NULL,
                      int a_is_link = -1 )
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
    _size = _st.st_size;
  _color = get_item_color( this );
  
  drop_view();
};                      

/*######################################################################*/

void vfu_help()
{
  say1center( HEADER );
  mb.freeall();
  mb.add( "*keypad -- Navigation keys" );
  mb.add( "ENTER   -- Enter into directory/View file ( `+' and `=' too )");
  mb.add( "BACKSPC -- Chdir to prent directory ( `-' and ^H too )"         );
  mb.add( "TAB     -- Edit entry: filename, atrrib's/mode, owner, group");
  mb.add( "R.Arrow -- Rename current file " );
  mb.add( "SPACE   -- Select/deselect current list item"   );
  mb.add( "ESC+ESC -- exit menu");
  mb.add( "1       -- Toggle `mode'  field on/off "    );
  mb.add( "2       -- Toggle `owner' field on/off "    );
  mb.add( "3       -- Toggle `group' field on/off "    );
  mb.add( "4       -- Toggle `time'  field on/off "    );
  mb.add( "5       -- Toggle `size'  field on/off "    );
  mb.add( "6       -- Toggle `type'  field on/off "    );
  mb.add( "7       -- Toggle `time type' field change/modify/access time "    );
  mb.add( "8       -- Turn on all fields"    );
  mb.add( "0       -- Toggle long name view ( show only type and file name )"    );
  mb.add( "~       -- Change current dir to HOME directory"     );
  mb.add( "A       -- Arrange/Sort file list"                   );
  mb.add( "B       -- Browse/View selected/current file"         );
  mb.add( "Alt+B   -- Browse/View current file w/o filters"      );
  mb.add( "C       -- Copy selected/current file(s)"             );
  mb.add( "D       -- Change directory"                         );
  mb.add( "Ctrl+D  -- Directory tree "                          );
  mb.add( "Alt+D   -- Chdir history " );
  mb.add( "E       -- Erase/remove selected/current file(s)!"    );
  mb.add( "F       -- Change file masks (space-delimited)       ");
  mb.add( "Ctrl+F  -- reset file mask to `*'"                    );
  mb.add( "G       -- Global select/deselect"                    );
  mb.add( "H       -- This help text"                            );
  mb.add( "Q       -- exit here ( to the current directory)");
  mb.add( "R       -- reload directory/refresh file list"       );
  mb.add( "Ctrl+R  -- recursive reload... "                     );
  mb.add( "Alt+R   -- reload/tree menu" );
  mb.add( "J       -- jump to mountpoint"                       );
  mb.add( "L       -- symlink selected/currnet file(s) into new directory" );
  mb.add( "Ctrl+L  -- refresh/redraw entire screen" );
  mb.add( "M       -- move selected/current file(s)"             );
  mb.add( "N       -- file find"                                );
  mb.add( "Alt+N   -- file find menu"   );
  mb.add( "O       -- options/toggles menu"       );
  mb.add( "P       -- file clipboard menu"       );
  mb.add( "Ctrl+C  -- copy files to clipboard"       );
  mb.add( "Ctrl+X  -- cut  files to clipboard"       );
  mb.add( "Ctrl+V  -- paste (copy) files from clipboard to current directory" );
  mb.add( "T       -- tools menu"                              );
  mb.add( "U       -- UserMenu (user external commands bound to menu instead of key)  " );
  mb.add( "X       -- exit to old/startup directory ");
  mb.add( "Alt+X   -- exit to old/startup directory ");
  mb.add( "Z       -- calculate directories sizes menu"       );
  mb.add( "Ctrl+Z  -- show size of the current (under the cursor >>) directory");
  mb.add( "Alt+Z   -- show all directories sizes ( or Alt+Z )" );
  mb.add( "V       -- Edit vfu.conf file");
  mb.add( "!       -- Shell (also available with '?')"                         );
  mb.add( "/       -- Command line"                                            );
  mb.add( "vfu uses these (one of) these config files:");
  mb.add( "        1. $HOME/$RC_PREFIX/vfu/vfu.conf");
  mb.add( "        2. $HOME/.vfu/vfu.conf");
  mb.add( "        3. " FILENAME_CONF_GLOBAL0 );
  mb.add( "        4. " FILENAME_CONF_GLOBAL1 );
  mb.add( "        5. " FILENAME_CONF_GLOBAL2 );
  mb.add( "" );
  vfu_menu_box( 1, 4, "VFU Help ( PageUp/PageDown to scroll )" );
  mb.freeall();
  say1("");
  do_draw = 1;
};

/*--------------------------------------------------------------------------*/

void vfu_init()
{
  char t[MAX_PATH];
  
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
  file_list_index.type = opt.dynamic_scroll;
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
  
  /* 
     FIXME: this should something relevant to the home_path 
     from above if $HOME does not exist(?) well still can
   accept /tmp/ as it is default now
  */
  
  get_rc_directory( "vfu", t );
  rc_path = t;
  
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
    { /* cannot find local/user conf file, try globals */
    if ( access( FILENAME_CONF_GLOBAL0, R_OK ) == 0 ) 
      filename_conf = FILENAME_CONF_GLOBAL0;
    if ( access( FILENAME_CONF_GLOBAL1, R_OK ) == 0 ) 
      filename_conf = FILENAME_CONF_GLOBAL1;
    if ( access( FILENAME_CONF_GLOBAL2, R_OK ) == 0 ) 
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

  /* now init some dynamic structs that should be */
  dir_tree.create(16,16);
  file_find_results.create( 32, 32 );
  user_externals.create(16,16);
  history.create(16,16);
  see_filters.create(16,16);
  panelizers.create(16,16);
  mb.create( 32, 16 ); /* menu box items init */
  list_panelizer.create( 32, 32 );
  archive_extensions.create( 32, 32 );

  /* this will load defaults first then load vfu.opt and at the
     end will load vfu.conf which will overwrite all if need to */
  vfu_settings_load();
 
  file_list_index.type = ( opt.dynamic_scroll != 0 );
  file_list_index.wrap = 0; /* just to be safe :) */

  files_mask = "*";
  files_mask_array.set( files_mask );

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

  String str;
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
  mb.freeall();
  mb.add( "X Exit (to startup path)" );
  mb.add( "Q Quit (to work path)   " );

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
  char t[128];
  sprintf( t, "Build date: %s, target: %s", __DATE__, _TARGET_DESCRIPTION_ );
  say1center( HEADER );
  say2center( t );

  int ch = 0;
  while (4)
    {
    if (do_draw) 
      { 
      if (do_draw > 1) do_draw_status = 1;
      vfu_redraw(); 
      do_draw = 0; 
      }
    if (do_draw_status)  
      {
      vfu_redraw_status();
      do_draw_status = 0;
      }
    show_pos( FLI+1, files_count ); /* FIXME: should this be in vfu_redraw()? */  
    
    ch = con_getch();
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

      case 's'       : vfu_inc_search(); break;

      case KEY_CTRL_L: 
                       con_cs(); 
                       
                       /* update scroll parameters */
                       file_list_index.min = 0;
                       file_list_index.max = files_count - 1;
                       file_list_index.pagesize = con_max_y() - 7;
                       
                       vfu_drop_all_views(); 
                       do_draw = 2; 
                       break;
      
      case 'q'       : if( vfu_exit( work_path ) == 0 ) return; break;
  
      case KEY_ALT_X :
      case 'x'       : if( vfu_exit( startup_path ) == 0 ) return; break;
  
      case 27        : if( vfu_exit( NULL ) == 0 ) return; break;
  
      case KEY_UP    : vfu_nav_up(); break;
      case KEY_DOWN  : vfu_nav_down(); break;
      case KEY_PPAGE : vfu_nav_ppage(); break;
      case KEY_NPAGE : vfu_nav_npage(); break;
      case KEY_HOME  : vfu_nav_home(); break;
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
  
      case KEY_RIGHT : if (opt.lynx_navigation)
                         vfu_action_plus( '+' );
                       else
                         vfu_rename_file_in_place();
                       break;
      
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
  
      case 'k'        : bookmark_hookup(); break;
      case KEY_ALT_0  : bookmark_goto( 0 ); break;
      case KEY_ALT_1  : bookmark_goto( 1 ); break;
      case KEY_ALT_2  : bookmark_goto( 2 ); break;
      case KEY_ALT_3  : bookmark_goto( 3 ); break;
      case KEY_ALT_4  : bookmark_goto( 4 ); break;
      case KEY_ALT_5  : bookmark_goto( 5 ); break;
      case KEY_ALT_6  : bookmark_goto( 6 ); break;
      case KEY_ALT_7  : bookmark_goto( 7 ); break;
      case KEY_ALT_8  : bookmark_goto( 8 ); break;
      case KEY_ALT_9  : bookmark_goto( 9 ); break;
  
      case 9          : vfu_edit_entry(); break;
  
      case 't'        : vfu_tools(); break;
  
      case 'p'        : vfu_clipboard( 0 ); break;
      case KEY_CTRL_C : vfu_clipboard( 'c' ); break; /* copy  */
      case KEY_CTRL_X : vfu_clipboard( 'x' ); break; /* cut   */
      case KEY_CTRL_V : vfu_clipboard( 'v' ); break; /* paste */
      
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
  con_out( 1, 1, 
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
    "\n*** press a key ***", cSTATUS
    );
  con_getch();
}

/*--------------------------------------------------------------------------*/

void vfu_cli( int argc, char* argv[] )
{
  String temp;
  GETOPT("hrd:ti")
    {
    switch(optc)
      {
      case 'h'  : vfu_help_cli(); break;
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

  dir_tree.done();
  file_find_results.done();
  user_externals.done();
  history.done();
  see_filters.done();
  panelizers.done();
  mb.done();
  list_panelizer.done();
  archive_extensions.done();

  if ( access( filename_atl, F_OK ) == 0 )
    unlink( filename_atl );
  
}

/*--------------------------------------------------------------------------*/

void vfu_signal( int sig )
{
/* this seems not to work... why?!
  if ( sig == SIGWINCH )
    {
    ...here should update screen size information...
    signal( SIGWINCH, vfu_signal ); // resetup signal handler
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
};

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
    default  : return; /* cannot be reached really */
    }
  vfu_drop_all_views();
};

/*--------------------------------------------------------------------------*/

void vfu_shell( const char* a_command, const char* a_options )
{
  String shell_line = a_command;
  String o = a_options;
  
  String status = "*** exec ok ***";
  
  int res = vfu_update_shell_line( shell_line, o );
  if (res) return;

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
    };
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
    String str = shell_editor;
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
  String new_name = fname;

  if ( !no_filters && see_filters.count() > 0 )
    {
    char t[MAX_PATH];
    strcpy( t, "/tmp/vfu.XXXXXX" );
    mktemp( t );
    new_name = t;
    int z;
    for ( z = 0; z < see_filters.count(); z++ )
      {
      StrSplitter split( "," );
      split.set( see_filters[z] );
      String mask = split[0];
      String str  = split[1];
      if ( FNMATCH( mask, str_file_name_ext( fname, t ) ) ) continue;
      /* found */
      str_replace( str, "%f", fname );
      str_replace( str, "%F", fname );
      str += " > ";
      str += new_name;
      vfu_shell( str, "" );
      break;
      }
    if ( z >= see_filters.count() )
      new_name = fname; /* not found */
    }

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
    String str = shell_browser;
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
        if ( FNMATCH( archive_extensions[z], fi->name_ext() ) == 0 )
          {
          z = -1; /* FIXME: this shouldn't be -1 for TRUE really :) */
          break;
          }
      if ( z == -1 )
        { /* yep this is archive */
        work_mode = WM_ARCHIVE;
        archive_name = fi->name();
        archive_path = ""; /* NOTE: archives' root dir is `' but not `/'! */
        char t[MAX_PATH];
        strcpy( t, "/tmp/vfu.XXXXXX" );
        mktemp( t );
        filename_atl = t; /* this is help for rx_*'s */
        setenv( RX_TEMP_LIST, t, 1 );
        vfu_read_files();
        say1( "ARCHIVE mode activated. ( some keys/commands are disabled! )" );
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
      String p = fi->name();
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
    };
}

/*--------------------------------------------------------------------------*/

void vfu_action_minus()
{
  String o = work_path; /* save old path i.e. current */
  
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
      #ifndef _TARGET_GO32_
      unsetenv( RX_TEMP_LIST );
      #endif
      if ( access( filename_atl, F_OK ) == 0 )
        unlink( filename_atl );
      vfu_chdir( "." );
      }
    }

  int z = 0;
  for ( z = 0; z < files_count; z++ )
    {
    String n;
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

String same_str;
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
    case GSAME_SIZE  : sel = (same_fsize == fi->size()); break;
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

  mb.freeall();
  mb.add( "S All" );
  mb.add( "A All (+Dirs)" );
  mb.add( "R Reverse" );
  mb.add( "C Clear" );
  mb.add( "P Pack" );
  mb.add( "H Hide" );
  mb.add( "D Different" );
  mb.add( ". Hide dirs" );
  mb.add( ", Hide dotfiles" );
  mb.add( "= Mask add (+dirs)" );
  mb.add( "+ Mask add (-dirs)" );
  mb.add( "- Mask sub        " );
  mb.add( "L Same..." );
  mb.add( "X EXtended select..." );
  if ( vfu_menu_box( 50, 5, "Global Select" ) == -1 ) return;
  ch = menu_box_info.ec;
  if (ch == 'X')
    {
    if ( work_mode != WM_NORMAL ) 
      { 
      say1( "GlobalSelect/Extended not available in this mode." ); 
      return; 
      };
    mb.freeall();
    mb.add( "--searching--" );
    mb.add( "F Find string (no case)" );
    mb.add( "S Scan string (case sense)" );
    mb.add( "H Hex  string" );
    mb.add( "/ Regular expression" );
    mb.add( "\\ Reg.exp (no case)" );
//    mb.add( "--other--" );
//    mb.add( "M Mode/Attributes" );
    if ( vfu_menu_box( 50, 5, "Extended G.Select" ) == -1 ) return;
    ch = menu_box_info.ec;
    if (ch == 'S') ch = 'B'; /* 'B' trans */
    if (ch == 'H') ch = 'E'; /* 'E' trans */
    }

  switch(ch)
    {
    case 'S' : {
               for (int z = 0; z < files_count; z++)
                 if (!files_list[z]->is_dir())
                   files_list[z]->sel = 1;
               }; break;
    case 'A' : {
               for (int z = 0; z < files_count; z++)
                 files_list[z]->sel = 1;
               }; break;
    case 'R' : {
               int z;
               for (z = 0; z < files_count; z++)
                 if (!files_list[z]->is_dir())
                   files_list[z]->sel = !files_list[z]->sel;
               }; break;
    case 'C' : {
               int z;
               for (z = 0; z < files_count; z++)
                   files_list[z]->sel = 0;
               }; break;
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
               }; break;
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
               }; break;
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
               }; break;
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
               }; break;
    case '+' :
    case '=' :
    case '-' :
              {
              String m;
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
                StrSplitter sm( " " );
                str_squeeze( m, " " );
                sm.set( m );
                while( sm.pop( m ) != "" )
                  {
                  if (opt.mask_auto_expand)
                    vfu_expand_mask( m );
                  int z = 0;
                  for (z = 0; z < files_count; z++)
                    {
                    if (files_list[z]->is_dir() && ch == '+') continue;
                    if (FNMATCH(m,files_list[z]->name()) == 0)
                      files_list[z]->sel = selaction;
                    }
                  }
                }
              say1( " " );
              say2( " " );
              }; break;
    case 'D' :
              {
              if ( work_mode != WM_NORMAL ) 
                { 
                say1( "GlobalSelect/Different not available in this mode." ); 
                break; 
                };
              String target;
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
              }; break;
    case '/':
    case '\\':
    case 'E':
    case 'F':
    case 'B': {
              say1("");
              String pat;
              if ( vfu_get_str( "Search string: ", pat, HID_GS_GREP ) )
                {
                fsize_t size = 0;
                say1("");
                say2("");
                if (ch == 'F' ) str_ins_ch( pat, 0, '\\'); else
                if (ch == 'B' ) str_ins_ch( pat, 0, '\\'); else
                if (ch == 'E' ) str_ins_ch( pat, 0, '$'); else
                if (ch == '/' ) str_ins_ch( pat, 0, '~'); else
                if (ch == '\\') str_ins_ch( pat, 0, '~'); else
                   ;
                int nocase = ( ( ch == '\\' ) || ( ch == 'F' ) );
                size = 0;
                for ( int z = 0; z < files_count; z++ )
                  {
                  size += files_list[z]->size();
                  if ( files_list[z]->is_dir() ) continue;
                  files_list[z]->sel = 
                      (file_find_string( pat,files_list[z]->name(),nocase) >= 0);
                  char s[128];
                  sprintf( s, "Scanning %4.1f%% (%12.0f bytes in %s ) ", 
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
              mb.freeall();
              mb.add( "N Name" );
              mb.add( "E Extension" );
              mb.add( "S Size" );
              mb.add( "T Time" );
              mb.add( "I Time (1 min.round)" );
              mb.add( "D Date" );
              mb.add( "M Date+Time" );
              mb.add( "A Attr/Mode" );
              #ifndef _TARGET_GO32_
              mb.add( "O Owner" );
              mb.add( "G Group" );
              #endif
              mb.add( "P Type (TP)" );

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
                case 'P' : vfu_global_select_same( GSAME_TYPE  ); break;
                }
              }; break;
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
              }; break;
    }
  update_status();
  do_draw = 1;
}

/*--------------------------------------------------------------------------*/

int vfu_user_external_find( int key, const char* ext, const char* type, String *shell_line )
{
  StrSplitter split(",");
  String str;
  String ext_str = ext;
  String type_str = type;
  if ( ext_str == "" )
    ext_str = ".";
  ext_str += ".";
  type_str = "." + type_str + ".";
  int z;
  for ( z = 0; z < user_externals.count(); z++ )
    {
    split.set( user_externals[z] );
    if ( key_by_name( split[1] ) != key ) continue; /* if key not the same -- skip */
    if ( strcmp( split[2], "*" ) != 0 ) /* if we should match and extension */
      if ( str_find( split[2], ext_str ) == -1 &&
           str_find( split[2], type_str ) == -1 ) continue; /* not found -- next one */
    if ( shell_line ) /* if not NULL -- store shell line into it */
      (*shell_line) = split[3];
    return z;
    }
  return -1;
};

/*--------------------------------------------------------------------------*/

void vfu_user_external_exec( int key )
{
  if ( files_count == 0 )
    {
    say1( "Directory is empty: user externals are disabled!" );
    return;
    }
  String shell_line;
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
    sprintf( t, "No user external defined for this key and extension (%d,%s)", key, fi->ext() );
    say1( t );
    }
}

/*--------------------------------------------------------------------------*/

void vfu_tools()
{
  mb.freeall();
  mb.add( "R Real path" );
  mb.add( "D ChDir to Real path" );
  mb.add( "T Make directory" );
  mb.add( "P Preset dirs menu" );
  mb.add( "A Rename tools..." );
  mb.add( "C Classify files" );
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
               char s[MAX_PATH];
               expand_path(files_list[FLI]->name(), s);
               vfu_chdir( s );
               break;
               }
    case 'T' : {
               String str;
               if (vfu_get_str( "Make directory(ies) (use space for separator)", 
                                str, HID_MKPATH ))
                 {
                 int err = 0;
                 int z;
                 StrSplitter ms( " " );
                 str_squeeze( str, " " );
                 ms.set( str );
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
    case 'P' : bookmark_goto( -1 ); break;
    case 'A' : vfu_tool_rename(); break;
    case 'C' : vfu_tool_classify(); break;
    }
}

/*--------------------------------------------------------------------------*/

void bookmark_goto( int n )
{
  char t[MAX_PATH];
  if ( n == -1 )
    {
    int z;
    mb.freeall();
    for(z = 1; z < 10; z++)
      {
      sprintf(t, "%d %-60s", z%10, path_bookmarks[z].data());
      if ( str_len(t) > 60 )
        str_dot_reduce( NULL, t, 60 );
      mb.add(t);
      }
    n = vfu_menu_box( 5, 5, "Path bookmarks");
    if ( n == -1 ) return;
    }
  if ( n < 0 || n > 9 ) return;
  if ( str_len( path_bookmarks[n] ) > 0 )
    vfu_chdir( path_bookmarks[n] );
  else
    say1( "No bookmark set" );  
}

void bookmark_hookup()
{
  say1( "FIXME: not implemented yet" );
}

/*--------------------------------------------------------------------------*/

void vfu_command()
{
  String cmd;
  if ( vfu_get_str( "Command: ", cmd, HID_COMMANDS ) ) vfu_shell( cmd, "" );
}

/*--------------------------------------------------------------------------*/

void vfu_rename_file_in_place()
{
  TF *fi = files_list[FLI];
  
  int y = (file_list_index.pos - file_list_index.page) + 4;
  int x = tag_mark_pos + 3;
  
  String str = fi->name();
  
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
  String tmp = files_mask;
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
    str_squeeze( files_mask, " " );
    files_mask_array.set( files_mask );
    
    if ( opt.mask_auto_expand )
      {
      int z;
      files_mask = "";
      tmp = "";
      for ( z = 0; z < files_mask_array.count(); z++ )
        {
        tmp = files_mask_array[z];
        vfu_expand_mask( tmp );
        files_mask += tmp + " ";
        }
      files_mask_array.set( files_mask );  
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
    mb.freeall();
    mb.add( "E Specify directory" );
    mb.add( "Z Directory under cursor" );
    mb.add( ". Current directory `.'" );
    mb.add( "S Selected directories" );
    mb.add( "A All dir's in the list" );
    if ( vfu_menu_box( 5, PS - 4, "Directory size of:" ) == -1 ) return;
    n = menu_box_info.ec;
    }
  
  if ( n == 'E' || n == '.' ) /* specific directory */
    {
    String target = work_path;
    if ( n == '.' )
      target = work_path;
    else
      if ( !vfu_get_dir_name( "Calculate size of directory: ", target ) ) return;
    fsize_t dir_size = vfu_dir_size( target );
    if ( dir_size == -1 ) return;
    String dir_size_str;
    dir_size_str.setfi( dir_size );
    str_comma( dir_size_str );
    sprintf( t, "Dir size of: %s", target.data() ); 
    say1( t );
    sprintf( t, "Size: %s bytes", dir_size_str.data() );
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
        if ( n == 'A' &&  fi->is_link() ) continue; /* all but not for symlinks */
        say1( fi->name() );
        fsize_t dir_size = vfu_dir_size( fi->name() );
        if ( dir_size == -1 )
          {
          say1(""); /* clear status text */
          return;
          }
        fi->set_size( dir_size );
        }
      }
    say1("");
    say2("");
    }  else
  if ( n == 'Z' ) /* all or selected  */
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
  String str;
  
  mb.freeall();
  mb.add( "M Mode" );
  mb.add( "O Owner/Group" );
  mb.add( "N Name (TAB)" );
  mb.add( "T Time/Touch Mod+Acc Times" );
  mb.add( "I Modify Time" );
  mb.add( "E Access Time" );
  mb.add( "L Edit SymLink Reference" );
  if ( sel_count )
    { /* available only when selection exist */
    mb.add( "--");
    mb.add( "+ Target: Toggle" );
    mb.add( "C Target: Current File" );
    mb.add( "S Target: Selection" );
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
    if ( menu_box_info.ec == 'M' ) /* attributes/mode */
      {
        mode_str_t new_mode;
        int err = 0;
        if (one)
          {
          strcpy( new_mode, files_list[FLI]->mode_str() );
          file_get_mode_str( files_list[FLI]->name(), new_mode);
          }
        else
          strcpy(new_mode, MODE_MASK);
        if( vfu_edit_attr(new_mode, !one ) )
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
        
        String str = t;
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

        String str;
        if (one)
          say1("Enter new `user.group | user | .group' for current file:");
        else
          say1("Enter new `user.group | user | .group' for all SELECTED files:");
        if( !(vfu_get_str( "", str, HID_EE_OWNER ) && str_len(str) > 0) ) break;

        int uid = 0;
        int gid = 0;
        char temp[100];
        regexp *re = regcomp("^ *([^\\.]*)(\\.([^\\.]*))? *$");
        if( !regexec(re, str) )
          {
          say1("Format is 'uid.gid', for example 'cade.users', 'cade.', '.users'");
          free(re);
          break;
          }

        struct passwd *pwd; regsubn(re, 1, temp);
        uid = -1; (pwd = getpwnam(temp))? uid = pwd->pw_uid : -2;
        struct group  *grp; regsubn(re, 3, temp);
        gid = -1; (grp = getgrnam(temp))? gid = grp->gr_gid : -2;
        free(re);

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
      String str = t;
      if ( vfu_get_str( "", str, 0 ) )
        {
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
  String str;
  char t[2048];
  int z;
  PSZCluster sc;
  sc.create( 8, 8 );
#ifdef _TARGET_UNIX_
  if (LoadFromFile( "/etc/mtab", &sc, 1024 )) return;
#endif
#ifdef _TARGET_GO32_
  str = home_path;
  str += "_vfu.mtb";
  if (LoadFromFile( str, &sc, 1024 )) return;
  if (all)
    {
    sc.ins( 0, "-  b:/" );
    sc.ins( 0, "-  a:/" );
    }
#endif
  if (sc.count() < 1) return;

  mb.freeall();
  for(z = 0; z < sc.count(); z++)
    {
    str = sc[z];
    str_cut( str, " \t");
    str_word( str, " \t", t ); /* get device name */
    str_cut( str, " \t");
    str_word( str, " \t", t ); /* get mount point */
    sc.put( z, t ); /* replace line with mount point only */

    struct statfs stafs;
    statfs( t, &stafs );
    int hk = ('A'+ z); /* hot key */
    #ifdef _TARGET_GO32_
    if (toupper(t[0]) >= 'A' && toupper(t[0]) <= 'Z' && toupper(t[1]) == ':')
      hk = toupper(t[0]);
    #endif
    sprintf( str, "%c | %8.1fM | %8.1fM | %-20s ", 
             hk, 
             stafs.f_bsize * (opt.show_user_free?stafs.f_bavail:stafs.f_bfree) 
                              / (1024.0*1024.0),
             stafs.f_bsize * stafs.f_blocks / (1024.0*1024.0),
             t
             );

    mb.add(str);
    }
  menu_box_info.ac = KEY_CTRL_U;
  z = vfu_menu_box( 20, 5, "Jump to mount-point (free/total) Ctrl+U=umount" );
  if ( z == -1 )   return;
  if (menu_box_info.ac == -2)
    {
    str = sc.get(z);
    str_fix_path( str );
    if ( pathcmp( str, work_path ) == 0 )
      {
      say1( "Warning: cannot unmount current directory" );
      return;
      }
    str = "umount " + str + " 2> /dev/null";
    sprintf( t, "Unmounting, exec: %s", str.data() );
    say1( t );
    if (system( str ) == 0)
      say1( "umount ok" );
    else
      say1( "umount failed" );
    }
  else
    vfu_chdir( sc.get(z) );
}

/*--------------------------------------------------------------------------*/

void vfu_user_menu()
{
  StrSplitter split(",");
  PSZCluster lines;
  lines.create( 16, 16 );
  String des;
  int z;
  
  mb.freeall();

  for ( z = 0; z < user_externals.count(); z++ )
    {
    split.set( user_externals[z] );
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

    lines.add( split[3] );
    mb.add( des );
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
    String str = lines[z];
    vfu_user_external_archive_exec( str );
    }

  lines.done();
};

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
  bi.st = opt.dynamic_scroll;
  bi.ac = 'p';

  say1center("------- ESC Exit ----- ENTER Chdir to target ----- P Panelize all results -----", cINFO );
  say2("");
  int z = con_full_psz_box( 1, 1, "VFU File find results", &file_find_results, &bi );

  if ( bi.ec == 13 )
    {
    String fname;
    String str = file_find_results[z];
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
    list_panelizer.freeall();
    for ( z = 0; z < file_find_results.count(); z++ )
      {
      String str = file_find_results[z];
      str_trim_left( str, str_find( str, " | " ) + 3 );
      list_panelizer.add( str );
      }
    vfu_read_files( 0 );
    }

  SaveToFile( filename_ffr, &file_find_results );
  con_cs();
};

/*--------------------------------------------------------------------------*/

StrSplitter __ff_masks( " " );
String      __ff_path;
String      __ff_pattern;
int         __ff_nocase;

int __ff_process( const char* origin,    /* origin path */
                  const char* fname,     /* full file name */
                  const struct stat* st, /* stat struture or NULL */
                  int is_link,           /* 1 if link */
                  int flag )
{
  String str;

  if ( flag == FTWALK_DX ) return 0;
  if ( vfu_break_op() ) return 1;
  if ( flag == FTWALK_D )
    {
    str = fname;
    str_dot_reduce( NULL, str, con_max_x()-1 );
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
    if ( FNMATCH( __ff_masks[z], pc ) == 0 )
      {
      add = 1;
      break;
      }
  if ( add && __ff_pattern != "" )
    add = ( file_find_string( __ff_pattern, fname, __ff_nocase ) > -1 );
  if (!add) return 0;

  char time_str[32];
  char size_str[32];
  time_str_compact( st->st_mtime, time_str );
  if ( flag == FTWALK_D )
    strcpy( size_str, "[DIR]" );
  else
    size_str_compact( st->st_size, size_str );
  str_pad( size_str, 5 );
  str = "";
  str = str + time_str + " " + size_str + " | " + fname;
  file_find_results.add( str );
  str_dot_reduce( NULL, str, con_max_x()-1 );
  con_puts( "\r" );
  con_puts( str, cSTATUS );
  con_puts( "\n" );
  return 0;
}

void vfu_file_find( int menu )
{
  String str;
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
      LoadFromFile( filename_ffr, &file_find_results, MAX_PATH+64 );
    vfu_file_find_results();
    return;
    }
  if ( ch == 'D' )
    {
    file_find_results.freeall();
    vfu_file_find_results(); /* FIXME: this will show `no results' warning */
    return;
    }

  __ff_pattern = "";
  if ( str_find( "FSB/\\", ch ) != -1 ) /* we want search for pattern */
    {
    __ff_pattern = vfu_hist_get( HID_FFGREP, 0 );
    if (!vfu_get_str( "Enter search pattern: ", __ff_pattern, HID_FFGREP )) return;
    if (ch == 'F' ) str_ins_ch( __ff_pattern, 0, '\\'); else
    if (ch == 'S' ) str_ins_ch( __ff_pattern, 0, '\\'); else
    if (ch == 'B' ) str_ins_ch( __ff_pattern, 0, '\\'); else
    if (ch == 'E' ) str_ins_ch( __ff_pattern, 0, '$'); else
    if (ch == '\\') str_ins_ch( __ff_pattern, 0, '~'); else
    if (ch == '/' ) str_ins_ch( __ff_pattern, 0, '~'); else;
    __ff_nocase = ( str_find( "F\\", ch ) != -1 );
    }

  str = vfu_hist_get( HID_FFMASK, 0 );
  if ( str == "" ) str = "*";
  if (!vfu_get_str( "Enter find masks (space separated): ", str, HID_FFMASK )) return;
  str_squeeze( str, " " );
  __ff_masks.set( str );

  str = work_path;
  if (!vfu_get_dir_name( "Enter start path: ", str )) return;
  __ff_path = str;

  /*--------------------------------------*/

  if ( opt.mask_auto_expand )
    { /* well, I have to (finally) fix that mask-expand API :(( */
    str = "";
    int z;
    for ( z = 0; z < __ff_masks.count(); z++ )
      {
      String tmp = __ff_masks[z];
      vfu_expand_mask( tmp );
      str += tmp;
      str += " ";
      }
    __ff_masks.set( str );
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
  
  file_find_results.freeall();
  ftwalk( __ff_path, __ff_process );
  vfu_file_find_results();
};

/*--------------------------------------------------------------------------*/

void vfu_clipboard( int act )
{
  if ( act == 0 )
    {
    mb.freeall();
    mb.add( "P Clipboard files list" );
    mb.add( "C Copy files to clipboard" );
    mb.add( "X Cut  files to clipboard" );
    mb.add( "V Paste files from clipbrd" );
    mb.add( "E Clear/flush clipboard" );
    if ( vfu_menu_box( 50, 5, "File Clipboard" ) ) return;
    act = menu_box_info.ec;
    }
}

/*--------------------------------------------------------------------------*/

void vfu_read_files_menu()
{
  char t[1024];
  PSZCluster list;
  list.create( 16, 16 );

  int z;
  String str;
  mb.freeall();
  /* I don't format src like this but it gives clear idea what is all about */
  mb.add( "T Rescan DirTree" );           list.add("");
  mb.add( "F Rescan Files" );             list.add("");
  mb.add( "R Rescan Files Recursive" );   list.add("");
  mb.add( "L Refresh all views/screen (Ctrl+L)" ); list.add("");
  if ( panelizers.count() > 0 )
    {
    mb.add( "--panelizers---" );         list.add("");
    for ( z = 0; z < panelizers.count(); z++ )
      {
      str = panelizers[z];
      str_word( str, ",", t );
      /* fix menu hotkeys */
      str_ins( t, 1, " " );
      str_set_ch( t, 0, toupper(str_get_ch(t, 0)) );
      mb.add(t);
      list.add(str);
      };
    }
  z = vfu_menu_box( 25, 5, "Read/Rescan Files" );
  if ( z == -1 )
    {
    list.done();
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
  list.done();
};

/*--------------------------------------------------------------------------*/

void vfu_inc_search()
{
  BSet set; /* used for searching */
  set.set_range1( 'a', 'z' );
  set.set_range1( 'A', 'Z' );
  set.set_range1( '0', '9' );
  set.set_str1( "._-~" );
  set.set_str1( "?*>[]" );

  String str;
  say1( "Enter search pattern: ( use TAB to advance )" );
  int key = con_getch();
  while( set.in( key ) || key == 8 || key == KEY_BACKSPACE || key == 9 )
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
      if ( z > file_list_index.max ) z = file_list_index.min;
      }
    else
      z = FLI;
      
    int direction = 1;
    int found = 0;
    int loops = 0;
    String s_mask = str;
    vfu_expand_mask( s_mask );
    while(1)
      {
      if ( z > file_list_index.max ) z = file_list_index.min;
      if ( z < file_list_index.min ) z = file_list_index.max;
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
    key = con_getch();
    }
  say1( "" );
  say2( "" );
}

/*######################################################################*/

// #include <mcheck.h> /* memory allocation debug */
int main( int argc, char* argv[] )
{
  
  #ifndef NDEBUG
  // mtrace(); /* memory allocation debug */
  #endif
  
  printf( HEADER );

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
  
  /*
  printf("%s\n<cade@biscom.net> [http://www.biscom.net/~cade/vfu]\nThank You for using VFU!\n\n", HEADER );
  */
  
  return 0;
}

/*######################################################################*/

