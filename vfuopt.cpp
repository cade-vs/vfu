/*
 *
 * (c) Vladi Belperchinov-Shabanski "Cade" 1996-2000
 * http://www.biscom.net/~cade  <cade@biscom.net>  <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 */

#include "vfu.h"
#include "vfuopt.h"
#include "vfuuti.h"
#include "vfudir.h"
#include "vfuview.h"
#include "vfumenu.h"

Options opt;

char *NOYES[] = { " - ", "YES", NULL };
char *NOYESPRECOPY[] = { " - ", "YES", "PRELIM", NULL };
char *FTIMETYPE[] = { "CHANGE", "MODIFY", "ACCESS", NULL };
#ifdef _TARGET_GO32_
char *TAGMARKS[] = { ">>", "=>", "->", "Í", "Ä", " ¯", "¯¯", NULL };
#else
char *TAGMARKS[] = { ">>", "=>", "->", NULL };
#endif

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
  { ' ', "Lowercase extensions for colors", &(opt.lower_case_ext_colors), NOYES },
  { '.', "Show hidden files", &(opt.show_hidden_files), NOYES },
  {  0 , "--navigation--", NULL, NULL },
  { 'i', "Use internal viewer", &(opt.internal_browser), NOYES },
  { 'I', "Use internal editor", &(opt.internal_editor), NOYES },
  { 'y', "Dynamic scroll ", &(opt.dynamic_scroll), NOYES },
  { ' ', "Use menu borders", &(opt.menu_borders), NOYES },
  {  0 , "--trees/dirs-- " , NULL, NULL },
  { ' ', "Compact DirTree", &(opt.tree_compact), NOYES },
  { ' ', "CDTree (cdpath) ", &(opt.tree_cd), NOYES },
  { ' ', "Fast dir size cache ", &(opt.fast_size_cache), NOYES },
  {  0 , "--troubleshooting--", NULL, NULL },
  { ' ', "Clear screen on shell", &(opt.shell_cls), NOYES },
  {  0 , "--compatibility--", NULL, NULL },
  { ' ', "Lynx style navigation",       &(opt.lynx_navigation), NOYES },
  { ' ', "Mask auto expand", &(opt.mask_auto_expand), NOYES },
  {  0 , "--other--", NULL, NULL },
/*  { ' ', "Can Zap/Erase READ-ONLY Files?!",       &(opt.zap_ro), NOYES }, ? */
  { 'b', "Allow beep!",       &(opt.allow_beep), NOYES },
  { 's', "Free space check on copy",  &(opt.copy_free_space_check), NOYES },
  { ' ', "Auto mount on change dir",  &(opt.auto_mount), NOYES},
  { ' ', "Preserve selection (after rescan)",  &(opt.keep_selection), NOYES},
  { ' ', "Preserve ownership/mode on copy?", &(opt.copy_keep_mode), NOYES },
  { ' ', "Show user's free space", &(opt.show_user_free), NOYES },
  { ' ', "Calc/Show bytes on copy", &(opt.copy_calc_totals), NOYESPRECOPY },
  { ' ', "Calc/Show bytes freed on erase", &(opt.bytes_freed), NOYES },
  {  -1, "---", NULL }
};

/*---------------------------------------------------------------------------*/

time_t vfu_opt_time( const struct stat st )
{
  if (opt.f_time_type == 0) return st.st_ctime; else
  if (opt.f_time_type == 1) return st.st_mtime; else
  if (opt.f_time_type == 2) return st.st_atime; else
  return 0;
};

time_t vfu_opt_time( const struct stat* st )
{
  return vfu_opt_time( *st );
};

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

  int z;
  for ( z = 0; z < 16; z++ ) ext_colors[z] = "";
  
  PSZCluster sc;
  sc.create( 16, 16 );
  LoadFromFile( "/etc/DIR_COLORS", &sc, 1024 );
  if (sc.count() == 0) return;

  while( sc.count() )
    {
    String str = sc[0];
    sc.free( 0 );
    int comment = str_find( str, '#' );
    if ( comment != -1 ) str_sleft( str, comment );
    str_cut( str, " \t" );
    if ( str_len( str ) == 0 ) continue;

    if ( strncmp( str, "TERM "   , 5 ) == 0 ) continue;
    if ( strncmp( str, "COLOR "  , 6 ) == 0 ) continue;
    if ( strncmp( str, "OPTIONS ", 8 ) == 0 ) continue;

    int pos = -1;
    if ( str_find( str, "31" ) != -1 ) pos = cRED; else
    if ( str_find( str, "32" ) != -1 ) pos = cGREEN; else
    if ( str_find( str, "33" ) != -1 ) pos = cYELLOW; else
    if ( str_find( str, "34" ) != -1 ) pos = cBLUE; else
    if ( str_find( str, "35" ) != -1 ) pos = cMAGENTA; else
    if ( str_find( str, "36" ) != -1 ) pos = cCYAN; else
    if ( str_find( str, "37" ) != -1 ) pos = cWHITE; else
    ;

    int spc = str_find( str, ' ' );
    if ( spc == -1 || pos == -1 ) continue;
    str_sleft( str, spc );

    str_replace( str, "DIR", ".[].<>" );
    str_replace( str, "LINK", ".->" );
    str_replace( str, "FIFO", ".()" );
    str_replace( str, "SOCK", ".##" );
    str_replace( str, "BLK", ".==" );
    str_replace( str, "CHR", ".++" );
    str_replace( str, "EXEC", ".**" );

    str_ins( ext_colors[pos], 0, str );
    
    };

  for ( int z = 0; z < 16; z++ )
    if( str_len( ext_colors[z] ) > 0 )
      ext_colors[z] += ".";
    
  #endif /* _TARGET_UNIX_ */
};

/*---------------------------------------------------------------------------*/

int set_set( const char *line, const char *keyword, char *target )
{
  char temp[255];
  regexp *re = regcomp("^[ \011]*([a-zA-Z0-9]+)[ \011]*=[ \011]*(.+)");
  int r = (regexec(re, line) && !strcasecmp(keyword, regsubn(re, 1, temp)));
  if (r) regsubn(re, 2, target);
  free(re);
  return r;
}

/*---------------------------------------------------------------------------*/

int set_set( const char *line, const char *keyword, PSZCluster *target )
{
  char temp[255];
  regexp *re = regcomp("^[ \011]*([a-zA-Z0-9]+)[ \011]*=[ \011]*(.+)");
  int r = (regexec(re, line) && !strcasecmp(keyword, regsubn(re, 1, temp)));
  if (r)
    {
    regsubn(re, 2, temp);
    target->add( temp );
    }
  free(re);
  return r;
}

/*---------------------------------------------------------------------------*/

int set_set( const char *line, const char *keyword, String &target )
{
  char temp[255];
  regexp *re = regcomp("^[ \011]*([a-zA-Z0-9]+)[ \011]*=[ \011]*(.+)");
  int r = (regexec(re, line) && !strcasecmp(keyword, regsubn(re, 1, temp)));
  if (r)
    {
    regsubn(re, 2, temp);
    target = temp;
    }
  free(re);
  return r;
}

/*---------------------------------------------------------------------------*/

int set_set( const char *line, const char *keyword, int &target )
{
  char temp[255];
  regexp *re = regcomp("^[ \011]*([a-zA-Z0-9]+)[ \011]*=[ \011]*([0123456789]+)");
  int r = (regexec(re, line) && !strcasecmp(keyword, regsubn(re, 1, temp)));
  if (r)
    {
    regsubn(re, 2, temp);
    target = atoi(temp);
    }
  free(re);
  return r;
}

/*---------------------------------------------------------------------------*/

int set_set( const char *line, const char *keyword, StrSplitter &splitter )
{
  char temp[1024];
  regexp *re = regcomp("^[ \011]*([a-zA-Z0-9]+)[ \011]*=[ \011]*(.+)");
  int r = (regexec(re, line) && !strcasecmp(keyword, regsubn(re, 1, temp)));
  if (r)
    {
    regsubn(re, 2, temp);
    splitter.set_delimiter( PATH_DELIMITER );
    splitter.set( temp );
    }
  free(re);
  return r;
}

/*---------------------------------------------------------------------------*/

int key_by_name( const char* key_name )
{
  if (strcmp (key_name, "IC"    ) == 0) return KEY_IC;
  if (strcmp (key_name, "INS"   ) == 0) return KEY_IC;
  if (strcmp (key_name, "INSERT") == 0) return KEY_IC;
  if (strcmp (key_name, "ENTER" ) == 0) return KEY_ENTER;
  if (strcmp (key_name, "RETURN") == 0) return KEY_ENTER;
  /*
  if (strcmp (key_name, "MENU"  ) == 0) ux.key = - menucount;
  */
  regexp *reFKEYS = regcomp( "[\\@\\^\\#]?[fF][01234567890]+" );
  if (!reFKEYS) return 0;
  
  if (regexec( reFKEYS, key_name ))
    {
    free(reFKEYS); /* so I can return below */
    if ( toupper(key_name[0]) == 'F' )
      return KEY_F1 + atoi( key_name + 1 ) - 1; else
    if ( toupper(key_name[0]) == '@' )
      return KEY_ALT_F1 + atoi( key_name + 2 ) - 1; else
    if ( toupper(key_name[0]) == '^' )
      return KEY_CTRL_F1 + atoi( key_name + 2 ) - 1; else
    if ( toupper(key_name[0]) == '#' )
      return KEY_SH_F1 + atoi( key_name + 2 ) - 1;
    }
  else  
    free(reFKEYS);  
  return 0;    
};

/*---------------------------------------------------------------------------*/

void vfu_settings_load()
{
  String str;
  char t[1024];
  
  user_externals.freeall();
  history.freeall();
  see_filters.freeall();
  panelizers.freeall();
  archive_extensions.freeall();
  
  /***** LOAD DEFAULTS *******/

  memset( &opt, 0, sizeof( opt ) );
  
  opt.svo.reset();
  opt.seo.reset();
  
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
  opt.fast_size_cache = 0;

  opt.show_hidden_files = 1;
  
  opt.allow_beep = 1;

  opt.use_colors = 1;
  opt.use_dir_colors = 1;
  opt.lower_case_ext_colors = 0;

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
  opt.dynamic_scroll = 1;

  opt.auto_mount = 1;
  opt.keep_selection = 1;

  opt.bytes_freed = 1;

  /***** LOAD DEFAULTS END ***/
  
  FILE *fsett;
  
  Options tmp_opt;
  memset( &tmp_opt, 0, sizeof( tmp_opt ) );
  if ( file_load_crc32( filename_opt, &tmp_opt, sizeof( tmp_opt ) ) == 0 )
    memcpy( &opt, &tmp_opt, sizeof(Options) );
  else
    say1( "warning: bad vfu.options file, loading defaults..." );
  
  LoadFromFile( filename_history, &history, MAX_PATH );
  
  if (getenv("EDITOR"))
    {
    shell_editor  = getenv("EDITOR");
    shell_editor += " %f";
    }
  if (getenv("PAGER") )
    {
    shell_browser  = getenv("PAGER");
    shell_browser += " %f";
    } else
  if (getenv("BROWSER") )
    {
    shell_browser  = getenv("BROWSER");
    shell_browser += " %f";
    } else
  if (getenv("VIEWER") )
    {
    shell_browser  = getenv("VIEWER");
    shell_browser  += " %f";
    }
  
  regexp *re_ux  = regcomp("^[ \011]*u?x[ \011]*=[ \011]*([^,]*)[ \011]*,[ \011]*([^, \011]*)[ \011]*,[ \011]*([^, \011]*)[ \011]*,(.*)$");
  regexp *re_see = regcomp( "^[ \011]*see[ \011]*=[ \011]*([^, \011]*)[ \011]*,(.*)$" );
  regexp *re_pan = regcomp( "^[ \011]*panelize[ \011]*=[ \011]*([^,]*)[ \011]*,(.*)$" );

  ASSERT( re_ux );
  ASSERT( re_see );
  ASSERT( re_pan );
  
  #define opti(key,target) if(set_set(line, key, target)) continue
  
  char line[1024];
  if ( (fsett = fopen( filename_conf, "r")) )
    {
    while(fgets(line, 1024, fsett))
      {
      if ( line[0] == '#' ) continue;
      if ( line[0] == ';' ) continue;
      str_cut( line, "\n\r" );
      if ( strlen( line ) == 0 ) continue;
      
      opti("browser", shell_browser);
      opti("pager",   shell_browser);
      opti("viewer",  shell_browser);

      opti("archive",  &archive_extensions);
      
      opti("editor", shell_editor);
      
      opti("bookmark1", path_bookmarks[1]);
      opti("bookmark2", path_bookmarks[2]);
      opti("bookmark3", path_bookmarks[3]);
      opti("bookmark4", path_bookmarks[4]);
      opti("bookmark5", path_bookmarks[5]);
      opti("bookmark6", path_bookmarks[6]);
      opti("bookmark7", path_bookmarks[7]);
      opti("bookmark8", path_bookmarks[8]);
      opti("bookmark9", path_bookmarks[9]);
            
   /* opti("cBLACK"   , ext_colors[0]); */
      opti("cGREEN"   , ext_colors[cGREEN]);
      opti("cRED"     , ext_colors[cRED]);
      opti("cCYAN"    , ext_colors[cCYAN]);
      opti("cWHITE"   , ext_colors[cWHITE]);
      opti("cMAGENTA" , ext_colors[cMAGENTA]);
      opti("cBLUE"    , ext_colors[cBLUE]);
      opti("cYELLOW"  , ext_colors[cYELLOW]);
      opti("chBLACK"  , ext_colors[chBLACK]);
      opti("chGREEN"  , ext_colors[chGREEN]);
      opti("chRED"    , ext_colors[chRED]);
      opti("chCYAN"   , ext_colors[chCYAN]);
      opti("chWHITE"  , ext_colors[chWHITE]);
      opti("chMAGENTA", ext_colors[chMAGENTA]);
      opti("chBLUE"   , ext_colors[chBLUE]);
      opti("chYELLOW" , ext_colors[chYELLOW]);
    
      opti("TrimTree",   trim_tree  );
  
      /* following code is used to clean input data */
      if( regexec( re_ux, line ) )
        {
        str = "";
        regsubn( re_ux, 1, t ); /* get description */
        str = str + t + ",";
        regsubn( re_ux, 2, t ); /* get key name */
        str = str + t + ",";
        regsubn( re_ux, 3, t ); /* get extensions */
        if ( strlen( t ) > 0 && strcmp( t, "*" ) != 0 )
          if ( str_get_ch( t, -1 ) != '.' ) 
            str_add_ch( t, '.' );
        str = str + t + ",";
        regsubn( re_ux, 4, t ); /* get shell line */
        str += t;
        user_externals.add( str );
        continue;
        } else
      if ( regexec( re_see, line ) )
        {
        str = "";
        regsubn( re_see, 1, t ); /* get fnmatch mask */
        str = str + t + ",";
        regsubn( re_see, 2, t ); /* get shell line */
        str += t;
        see_filters.add( str );
        continue;
        } else
      if ( regexec( re_pan, line ) )
        {
        str = "";
        regsubn( re_pan, 1, t ); /* get description */
        str = str + t + ",";
        regsubn( re_pan, 2, t ); /* get shell line */
        str += t;
        panelizers.add( str );
        continue;
        }
      }
    fclose(fsett);
    }
  
  free( re_ux );
  free( re_see );
  free( re_pan );
    
  #ifdef _TARGET_GO32_
  int z;
  for ( z = 0; z < 16; z++ ) str_low( ext_colors[z] );
  #endif
  
  if (opt.use_dir_colors) vfu_load_dir_colors();
//  if (file_load_crc32( filename_size_cache, &size_cache, sizeof(size_cache)))
//    memset( &size_cache, 0, sizeof(size_cache) );
  size_cache.zap();
  size_cache.fload( filename_size_cache );
}

/*---------------------------------------------------------------------------*/

void vfu_settings_save()
{
  file_save_crc32( filename_opt, &opt, sizeof(opt));
//  file_save_crc32( filename_size_cache, &size_cache, sizeof(size_cache));
  SaveToFile( filename_history, &history );
  size_cache.fsave( filename_size_cache );
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
    String line = shell_editor;
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
  vfu_toggle_box( 30, 5, "Options/Toggles   (scroll down...)", Toggles );
  file_list_index.type = opt.dynamic_scroll;
  vfu_drop_all_views();
  vfu_redraw(); 
  vfu_redraw_status();
};

