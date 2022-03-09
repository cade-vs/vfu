/****************************************************************************
 *
 * Copyright (c) 1996-2021 Vladi Belperchinov-Shabanski "Cade" 
 * http://cade.datamax.bg/  <cade@biscom.net> <cade@bis.bg> <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 ****************************************************************************/

#include "vfu.h"
#include "vfuopt.h"
#include "vfuuti.h"
#include "vfudir.h"
#include "vfuview.h"
#include "vfumenu.h"

Options opt;

const char *NOYES[] = { " - ", "YES", NULL };
const char *NOYESPRECOPY[] = { " - ", "YES", "PRELIM", NULL };
const char *FTIMETYPE[] = { "CHANGE", "MODIFY", "ACCESS", NULL };
#ifdef _TARGET_GO32_
const char *TAGMARKS[] = { ">>", "=>", "->", "Í", "Ä", " ¯", "¯¯", NULL };
#else
const char *TAGMARKS[] = { ">>", "=>", "->", NULL };
#endif
const char *SIIEC[] = { "IEC", "SI ", NULL };
const char *COMMA_TYPES[] = { "'", "`", ",", " ", "_", NULL };
const char *PAGE_STEPS[] = { "1 LINE", "30% PG", "50% PG", NULL };

ToggleEntry Toggles[] =
{
//  { "[a] 1234567890123456", &(opt.some) },
  {  0 , "--screen--", NULL, NULL },
  { '1', "Show Mode  field", &(opt.f_mode), NOYES },
  { '2', "Show Owner field", &(opt.f_owner), NOYES },
  { '3', "Show Group field", &(opt.f_group), NOYES },
  { '4', "Show Time  field", &(opt.f_time), NOYES },
  { '5', "Show Size  field", &(opt.f_size), NOYES },
  { '6', "Show Type  field", &(opt.f_type), NOYES },
  { '7', "     Time Type ", &(opt.f_time_type), FTIMETYPE },
  { '8', "Long name view ", &(opt.long_name_view), NOYES },
  { ' ', "TagMark type   ", &(opt.tag_mark_type), TAGMARKS },
  { ' ', "Use colors     ", &(opt.use_colors), NOYES },
  { ' ', "Use /etc/DIR_COLORS", &(opt.use_dir_colors), NOYES },
  { ' ', "Lowercase extensions for configs", &(opt.lower_case_ext_config), NOYES },
  { '.', "Show hidden files", &(opt.show_hidden_files), NOYES },
  {  0 , "--navigation--", NULL, NULL },
  { 'i', "Use internal viewer", &(opt.internal_browser), NOYES },
  { 'I', "Use internal editor", &(opt.internal_editor), NOYES },
  { ' ', "Use menu borders", &(opt.menu_borders), NOYES },
  {  0 , "--trees/dirs-- " , NULL, NULL },
  { ' ', "Compact DirTree", &(opt.tree_compact), NOYES },
  { ' ', "CDTree (cdpath) ", &(opt.tree_cd), NOYES },
  {  0 , "--troubleshooting--", NULL, NULL },
  { ' ', "Clear screen on shell", &(opt.shell_cls), NOYES },
  {  0 , "--compatibility--", NULL, NULL },
  { ' ', "Lynx style navigation",       &(opt.lynx_navigation), NOYES },
  { ' ', "Mask auto expand", &(opt.mask_auto_expand), NOYES },
  { ' ', "Use CWD as target for COPY/MOVE", &(opt.default_copy_to_cwd), NOYES },
  {  0 , "--other--", NULL, NULL },
/*  { ' ', "Can Zap/Erase READ-ONLY Files?!",       &(opt.zap_ro), NOYES }, ? */
  { ' ', "Keep directories on top of the list",    &(opt.sort_top_dirs), NOYES },
  { ' ', "Smart HOME/END keys (only Top Dirs mode)",    &(opt.smart_home_end), NOYES },
  { ' ', "Case insensitive file/dir names matching",       &(opt.no_case_glob), NOYES },
  { 'b', "Allow beep!",       &(opt.allow_beep), NOYES },
  { 's', "Free space check on copy",  &(opt.copy_free_space_check), NOYES },
  { ' ', "Auto mount on change dir",  &(opt.auto_mount), NOYES},
  { ' ', "Preserve selection (after rescan)",  &(opt.keep_selection), NOYES},
  { ' ', "Preserve ownership/mode on copy?", &(opt.copy_keep_mode), NOYES },
  { ' ', "Show user's free space", &(opt.show_user_free), NOYES },
  { ' ', "Calc/Show bytes on copy", &(opt.copy_calc_totals), NOYESPRECOPY },
  { ' ', "Calc/Show bytes freed on erase", &(opt.bytes_freed), NOYES },
  { ' ', "Prefer GiB in disk usage status", &(opt.use_gib_usage), NOYES },
  { ' ', "Show file/dir sizes in units", &(opt.use_si_sizes), SIIEC },
  { ' ', "1000s separator type", &(opt.comma_type), COMMA_TYPES },
  { ' ', "Scrolling page step", &(opt.scroll_pagestep), PAGE_STEPS },
  {  -1, "---", NULL, NULL }
};

/*---------------------------------------------------------------------------*/

time_t vfu_opt_time( const struct stat st )
{
  if (opt.f_time_type == 0) return st.st_ctime; else
  if (opt.f_time_type == 1) return st.st_mtime; else
  if (opt.f_time_type == 2) return st.st_atime; else
  return 0;
}

time_t vfu_opt_time( const struct stat* st )
{
  return vfu_opt_time( *st );
}

time_t vfu_opt_time( time_t ctime, time_t mtime, time_t atime )
{
  if (opt.f_time_type == 0) return ctime; else
  if (opt.f_time_type == 1) return mtime; else
  if (opt.f_time_type == 2) return atime; else
  return 0;
}

/*---------------------------------------------------------------------------*/

void vfu_load_dir_colors()
{
  #ifdef _TARGET_UNIX_

  VArray va;
  va.fload( "/etc/DIR_COLORS" );
  if (va.count() == 0) return;

  while( va.count() )
    {
    VString str = va[0];
    va.del( 0 );
    int comment = str_find( str, '#' );
    if ( comment != -1 ) str_sleft( str, comment );
    str_cut( str, " \t" );
    if ( str_len( str ) == 0 ) continue;

    if ( strncmp( str, "TERM "   , 5 ) == 0 ) continue;
    if ( strncmp( str, "COLOR "  , 6 ) == 0 ) continue;
    if ( strncmp( str, "OPTIONS ", 8 ) == 0 ) continue;

    int pos = -1;
    if ( str_find( str, "31" ) != -1 ) pos = cRED;     else
    if ( str_find( str, "32" ) != -1 ) pos = cGREEN;   else
    if ( str_find( str, "33" ) != -1 ) pos = cYELLOW;  else
    if ( str_find( str, "34" ) != -1 ) pos = cBLUE;    else
    if ( str_find( str, "35" ) != -1 ) pos = cMAGENTA; else
    if ( str_find( str, "36" ) != -1 ) pos = cCYAN;    else
    if ( str_find( str, "37" ) != -1 ) pos = cWHITE;   else
    {};

    int spc = str_find( str, ' ' );
    if ( spc == -1 || pos == -1 ) continue;
    str_sleft( str, spc );

    str_replace( str, "DIR",  ".[].<>" );
    str_replace( str, "LINK", ".->" );
    str_replace( str, "FIFO", ".()" );
    str_replace( str, "SOCK", ".##" );
    str_replace( str, "BLK",  ".==" );
    str_replace( str, "CHR",  ".++" );
    str_replace( str, "EXEC", ".**" );

    str_ins( ext_colors[pos], 0, str );

    };

  for ( int z = 0; z < 16; z++ )
    if( str_len( ext_colors[z] ) > 0 )
      {
      ext_colors[z] += ".";
      if ( opt.lower_case_ext_config )
        str_low( ext_colors[z] );
      }

  #endif /* _TARGET_UNIX_ */
}

/*---------------------------------------------------------------------------*/

int set_arr( const char *line, const char *keyword, VArray &target )
{
  VRegexp re("^[ \011]*([a-zA-Z0-9]+)[ \011]*=[ \011]*(.+)");
  if ( ! re.m( line ) ) return 0;
  if ( str_low( re[1] ) != keyword ) return 0;
  target.push( re[2] );
  return 1;
}

/*---------------------------------------------------------------------------*/

int set_str( const char *line, const char *keyword, VString &target )
{
  VRegexp re("^[ \011]*([a-zA-Z0-9]+)[ \011]*=[ \011]*(.+)");
  if ( ! re.m( line ) ) return 0;
  if ( str_low( re[1] ) != keyword ) return 0;
  target = re[2];
  return 1;
}

/*---------------------------------------------------------------------------*/

int set_int( const char *line, const char *keyword, int &target )
{
  VRegexp re("^[ \011]*([a-zA-Z0-9]+)[ \011]*=[ \011]*([0123456789]+)");
  if ( ! re.m( line ) ) return 0;
  if ( str_low( re[1] ) != keyword ) return 0;
  target = atoi( re[2] );
  return 1;
}

/*---------------------------------------------------------------------------*/

int set_splitter( const char *line, const char *keyword, VArray &splitter )
{
  VRegexp re("^[ \011]*([a-zA-Z0-9]+)[ \011]*=[ \011]*(.+)");
  if ( ! re.m( line ) ) return 0;
  if ( str_low( re[1] ) != keyword ) return 0;
  splitter = str_split( PATH_DELIMITER, re[2] );
  return 1;
}

/*---------------------------------------------------------------------------*/

int key_by_name( const char* key_name )
{
  if ( strcmp( key_name, "IC"     ) == 0 ) return KEY_IC;
  if ( strcmp( key_name, "INS"    ) == 0 ) return KEY_IC;
  if ( strcmp( key_name, "INSERT" ) == 0 ) return KEY_IC;
  if ( strcmp( key_name, "ENTER"  ) == 0 ) return KEY_ENTER;
  if ( strcmp( key_name, "RETURN" ) == 0 ) return KEY_ENTER;
  /*
  if (strcmp (key_name, "MENU"  ) == 0) ux.key = - menucount;
  */
  VRegexp reFKEYS( "[\\@\\^\\#]?[fF][01234567890]+" );

  if ( reFKEYS.m( key_name ) )
    {
    if ( toupper(key_name[0]) == 'F' )
      return KEY_F1 + atoi( key_name + 1 ) - 1; else
    if ( toupper(key_name[0]) == '@' )
      return KEY_ALT_F1 + atoi( key_name + 2 ) - 1; else
    if ( toupper(key_name[0]) == '^' )
      return KEY_CTRL_F1 + atoi( key_name + 2 ) - 1; else
    if ( toupper(key_name[0]) == '#' )
      return KEY_SH_F1 + atoi( key_name + 2 ) - 1;
    }
  return 0;
}

/*---------------------------------------------------------------------------*/

void vfu_settings_load()
{
  VString str;

  user_externals.undef();
  history.undef();
  see_filters.undef();
  panelizers.undef();
  archive_extensions.undef();
  path_bookmarks.undef();

  /***** LOAD DEFAULTS *******/

  memset( &opt, 0, sizeof( opt ) );

  opt.svo.reset();
  opt.seo.reset();
  opt.seo.handle_tab = 1;

  opt.sort_order = 'N';
  opt.sort_direction = 'A';
  opt.sort_top_dirs = 1;

  opt.f_size = 1;
  opt.f_time = 1;
  opt.f_mode = 1;
  opt.f_group = 1;
  opt.f_owner = 1;
  opt.f_type = 1;
  opt.f_time_type = 1;

  opt.long_name_view = 0;
  opt.tree_compact = 0;
  opt.tree_cd = 1;

  opt.show_hidden_files = 1;

  opt.allow_beep = 1;

  opt.use_colors = 1;
  opt.use_dir_colors = 1;
  opt.lower_case_ext_config = 1;

  opt.copy_free_space_check = 1;
  opt.copy_calc_totals = 1;
  opt.copy_keep_mode = 1;

  opt.tag_mark_type = 0;

  opt.internal_browser = 1;
  opt.internal_editor = 1;

  opt.mask_auto_expand = 1;
  opt.shell_cls = 1;

  opt.zap_ro = 0;

  opt.show_user_free = 1;
  opt.menu_borders = 0;

  opt.lynx_navigation = 0;

  opt.auto_mount = 1;
  opt.keep_selection = 1;

  opt.bytes_freed = 1;

  opt.use_si_sizes = 0;

  opt.smart_home_end = 1;

  opt.scroll_pagestep = 1;
  /***** LOAD DEFAULTS END ***/

  FILE *fsett;

  Options tmp_opt;
  memset( &tmp_opt, 0, sizeof( tmp_opt ) );
  if ( file_load_crc32( filename_opt, &tmp_opt, sizeof( tmp_opt ) ) == 0 )
    memcpy( &opt, &tmp_opt, sizeof(Options) );
  else
    say1( "warning: bad vfu.options file, loading defaults..." );

  history.fload( filename_history );
  file_list_index.set_pagestep( OPT_SCROLL_PAGESTEP(opt.scroll_pagestep) );

  if ( getenv("EDITOR"))
    {
    shell_editor  = getenv("EDITOR");
    shell_editor += " %f";
    }
  if ( getenv("PAGER") )
    {
    shell_browser  = getenv("PAGER");
    shell_browser += " %f";
    } else
  if ( getenv("BROWSER") )
    {
    shell_browser  = getenv("BROWSER");
    shell_browser += " %f";
    } else
  if ( getenv("VIEWER") )
    {
    shell_browser  = getenv("VIEWER");
    shell_browser  += " %f";
    }

  VRegexp re_ux("^\\s*u?x\\s*=\\s*([^,]*)[ \011]*,\\s*([^, \011]*)\\s*,\\s*([^, \011]*)\\s*,(.*)$", "i");
  VRegexp re_see( "^\\s*see\\s*=\\s*([^, \011]*)\\s*,(.*)$", "i" );
  VRegexp re_pan( "^\\s*panelize\\s*=\\s*([^,]*)\\s*,(.*)$", "i" );

  char line[1024];
  if ( (fsett = fopen( filename_conf, "r")) )
    {
    while(fgets(line, 1024, fsett))
      {
      if ( line[0] == '#' ) continue;
      if ( line[0] == ';' ) continue;
      str_cut( line, "\n\r" );
      if ( strlen( line ) == 0 ) continue;

      if(set_str( line, "browser", shell_browser))continue;
      if(set_str( line, "pager", shell_browser))continue;
      if(set_str( line, "viewer", shell_browser))continue;

      if(set_arr( line, "archive",  archive_extensions))continue;

      if(set_str( line, "editor", shell_editor))continue;
      if(set_str( line, "diff",   shell_diff))continue;

      if(set_arr( line, "bookmark",  path_bookmarks))continue;

   /* if(set_str( line, "cblack"   , ext_colors[0]); */
      if(set_str( line, "cgreen"   , ext_colors[cGREEN]))continue;
      if(set_str( line, "cred"     , ext_colors[cRED]))continue;
      if(set_str( line, "ccyan"    , ext_colors[cCYAN]))continue;
      if(set_str( line, "cwhite"   , ext_colors[cWHITE]))continue;
      if(set_str( line, "cmagenta" , ext_colors[cMAGENTA]))continue;
      if(set_str( line, "cblue"    , ext_colors[cBLUE]))continue;
      if(set_str( line, "cyellow"  , ext_colors[cYELLOW]))continue;
      if(set_str( line, "chblack"  , ext_colors[chBLACK]))continue;
      if(set_str( line, "chgreen"  , ext_colors[chGREEN]))continue;
      if(set_str( line, "chred"    , ext_colors[chRED]))continue;
      if(set_str( line, "chcyan"   , ext_colors[chCYAN]))continue;
      if(set_str( line, "chwhite"  , ext_colors[chWHITE]))continue;
      if(set_str( line, "chmagenta", ext_colors[chMAGENTA]))continue;
      if(set_str( line, "chblue"   , ext_colors[chBLUE]))continue;
      if(set_str( line, "chyellow" , ext_colors[chYELLOW]))continue;

      if(set_splitter( line, "trimtree",  trim_tree  ))continue;

      /* following code is used to clean input data */
      if( re_ux.m( line ) )
        {
        str = "";
        str = str + re_ux[1] + ","; /* get description */
        str = str + re_ux[2] + ","; /* get key name */
        VString t = re_ux[3]; /* get extensions */
        if ( t != "*" && t[-1] != '.' ) t += ".";
        str = str + t + ",";
        str += re_ux[4]; /* get shell line */
        user_externals.push( str );
        continue;
        } else
      if( re_see.m( line ) )
        {
        str = "";
        see_filters.push( str + re_see[1] + "," + re_see[2] );
        continue;
        } else
      if( re_pan.m( line ) )
        {
        str = "";
        panelizers.push( str + re_pan[1] + "," + re_pan[2] );
        continue;
        }
      }
    fclose(fsett);
    }

  #ifdef _TARGET_GO32_
  int z;
  for ( z = 0; z < 16; z++ ) str_low( ext_colors[z] );
  #endif

  if (opt.use_dir_colors) vfu_load_dir_colors();
//  if (file_load_crc32( filename_size_cache, &size_cache, sizeof(size_cache)))
//    memset( &size_cache, 0, sizeof(size_cache) );
  size_cache_load();
}

/*---------------------------------------------------------------------------*/

void vfu_settings_save()
{
  file_save_crc32( filename_opt, &opt, sizeof(opt));
//  file_save_crc32( filename_size_cache, &size_cache, sizeof(size_cache));
  history.fsave( filename_history );
  size_cache_save();
}

/*---------------------------------------------------------------------------*/

void vfu_edit_conf_file()
{
  if (opt.internal_editor)
    {
    opt.seo.cs = cINFO;
    SeeEditor editor( &opt.seo );
    if( editor.open( filename_conf ) == 0 )
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
    VString line = shell_editor;
    str_replace( line, "%f", filename_conf );
    str_replace( line, "%F", filename_conf );
    vfu_shell( line.data(), 0 );
    }
  vfu_settings_save();
  vfu_settings_load();
  do_draw = 2;
  say1("");
  say2("");
}

/*---------------------------------------------------------------------------*/

void vfu_options()
{
  say1("press SPACE to toggle, ENTER or ESC to exit");
  say2("");
  vfu_toggle_box( 30, 5, "Options/Toggles (scroll down, SPACE selects)", Toggles );
  vfu_settings_save();
  vfu_settings_load();
  vfu_drop_all_views();
  vfu_redraw();
  vfu_redraw_status();
  say1("");
  say2("");
}

