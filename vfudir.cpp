/****************************************************************************
 *
 * (c) Vladi Belperchinov-Shabanski "Cade" 1996-2014
 * http://cade.datamax.bg/  <cade@biscom.net> <cade@bis.bg> <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 ****************************************************************************/

#include "vfudir.h"
#include "vfuopt.h"
#include "vfuuti.h"
#include "vfusys.h"
#include "vfufiles.h"
#include "vfuview.h"
#include "vfumenu.h"

VArray size_cache;

/*###########################################################################*/

  void __glob_gdn( const char* a_path, const char* a_fnpattern,
                   VArray &a_va ) // glob getdirname
  {
    VString pat = a_fnpattern;
    pat += "*";

    a_va.undef();
    DIR *dir;
    dirent *de;
    if ( !a_path || a_path[0] == 0 )
      dir = opendir(".");
    else
      dir = opendir( a_path );
    if (dir)
      {
      while ( (de = readdir(dir)) )
        {
        if ( strcmp( de->d_name, "." ) == 0 ||
             strcmp( de->d_name, ".." ) == 0 ) continue;
        int match_ok = 0;
        if( opt.no_case_glob )
          match_ok = FNMATCH_NC( pat, de->d_name) == 0;
        else
          match_ok = FNMATCH( pat, de->d_name) == 0;
        if ( a_fnpattern[0] == 0 || match_ok )
          {
          VString str; // = a_path;
          str += de->d_name;
          if ( file_is_dir( a_path + str ) )
            {
            str += "/";
            a_va.push(str);
            }
          }
        }
      closedir(dir);
      }
  }

/*
  FIXME: must call sayX() at the end to clear...
*/

int vfu_get_dir_name( const char *prompt, VString &target, int should_exist )
{
  int res = -1;
  /*
  #ifdef _TARGET_UNIX_
  leaveok(stdscr, FALSE);
  #endif
  */
  VArray dir_list;

  say1(prompt);
  say2("");

  int pos = 0;
  int page = 0;
  int ch = 0;

  int insert = 1;
  int firsthit = 1;

  pos = str_len( target );

  //------------------------------------------------------------------

  con_cshow();
  say2( target, firsthit ? cINPUT2 : cINPUT );
  while(1)
    {
    int mx = con_max_x() - 1;
    VString target_out = target;
    if ( (pos < page) || (pos+1 > page + mx) || (page > 0 && pos == page) )
      page = pos - mx / 2;
    if ( page < 0 ) page = 0;

    str_trim_left( target_out, page );
    str_sleft( target_out, mx );
    str_pad( target_out, -mx );
    if ( page > 0 )
      str_set_ch( target_out, 0, '<' );
    if ( str_len( target ) - page > mx )
      str_set_ch( target_out, mx-1, '>' );

    say2( target_out, firsthit ? cINPUT2 : cINPUT );
    con_xy( pos-page+1, con_max_y() );


    if (ch == 0) ch = con_getch();
    if (ch == '\\') ch = '/'; /* dos hack :)) */
    if ( ch == '/' && str_find( target, '/' ) == -1 && target[0] == '~' )
      {
      target = tilde_expand( target );
      str_fix_path( target );
      pos = str_len( target );
      ch = 0;
      }

    if ((ch == 8 || ch == KEY_BACKSPACE) && pos > 0)
      {
      pos--;
      str_del( target, pos, 1 );
      }
    else
    if (ch == KEY_CTRL_A && str_len( target ) > 0)
      {
      int z = str_len( target )-1;
      if ( str_get_ch(target, z) == '/' ) z--;
      while ( z > 0 && str_get_ch(target,z) != '/' ) z--;
      z++;
      str_sleft(target,z);
      pos = z;
      }
    else
    if ( ch == 9 && str_len( target ) > 0)
      {
      int z;
      dir_list.undef();
      VString dmain; /* main/base path */
      VString dtail; /* item that should be expanded/glob */

      dmain = str_file_path( target );
      dtail = str_file_name_ext( target );

      /*
      int lastslash = str_rfind(target, '/');
      if ( lastslash == -1 )
        {
        dmain = "";
        dtail = target;
        }
      else
        {
        dmain = target;
        dtail = target;
        str_sleft( dmain, lastslash+1 );
        str_trim_left( dtail, lastslash+1 );
        }
      */

      __glob_gdn( dmain, dtail, dir_list );

      z = dir_list.count()-1;
      if (dir_list.count())
        {
        if ( dir_list.count() > 1)
          {
          int li; /* counter */
          int ll; /* longest directory entry */
          int xm = 0; /* exact match entry  */
          for ( li = 0; li < dir_list.count(); li++ )
            {
            int len = strlen( dir_list[li] );
            if( len > ll )
              ll = len;
            VString tmp1;
            if( dtail != str_copy( tmp1, dir_list[li], 0, str_len( dtail ) ) )
              continue;
            xm = li;
            break;
            }
          int mc = 0; /* match count        */
          int mi = 0; /* match letter index */
          while(4)
            {
            mc = 0;
            for ( li = 0; li < dir_list.count(); li++ )
              {
              char ch1 = str_get_ch( dir_list[xm], mi );
              char ch2 = str_get_ch( dir_list[li], mi );
              if( opt.no_case_glob )
                {
                ch1 = toupper( ch1 );
                ch2 = toupper( ch2 );
                }
              if ( ch1 == ch2 )
                mc++;
              }
            if ( mc != dir_list.count() )
              break;
            mi++;
            if( mi >= ll )
              break;
            }
          target.setn( dmain + dir_list[xm], str_len( dmain ) + mi );
          pos = str_len( target );
          say2( target, cINPUT );
          con_xy( pos+1, con_max_y() );

          vfu_beep();
          ch = con_getch();
          if ( ch != 9 ) { dir_list.undef(); continue; }
          dir_list.sort();
          con_chide();
          z = vfu_menu_box( 10, 5, "Complete...", &dir_list );
          con_cshow();
          ch = 0;
          }
        else
          ch = 0;
        if ( z != -1 )
          {
          while( str_len( target ) > 0 && target[-1] != '/' )
            str_chop( target );
          target += dir_list[z];
          }

        pos = str_len( target );

        dir_list.undef();
        if (ch != 0) continue;
        }
      else
        { /* no match found -- cannot complete */
        vfu_beep();
        }
      }
    else
    if (ch == 13)
      {
      res = 1;
      break;
      }
    else
    if (ch == 27)
      {
      target = "";
      res = 0;
      break;
      }
    if (ch == KEY_CTRL_U)
      {
      target = "";
      pos = 0;
      }
    else
    if (ch == KEY_CTRL_X)
      {
        char t[MAX_PATH];
        if ( target[0] == '~' )
          target = tilde_expand( target );
        expand_path( target, t );
        str_fix_path( t );
        target = t;
        pos = str_len( target );
      }
    else
    if (ch >= 32 && ch <= 255 ) // && pos < 70)
      {
      if (firsthit)
        {
        target = "";
        pos = 0;
        }
      if (!insert) str_del( target, pos, 1 );
      str_ins_ch( target, pos, ch );
      pos++;
      } else
    if( ch == KEY_LEFT  )
      {
      if (pos > 0)
        pos--;
      } else
    if( ch == KEY_RIGHT )
      {
      if (pos < str_len( target ))
        pos++;
      } else
    if ( ch == KEY_IC   ) insert = !insert; else
    if ( ch == KEY_HOME ) pos = 0; else
    if ( ch == KEY_END  ) pos = str_len(target); else
    if ( ch == KEY_DC  && pos < str_len(target) )
       str_del( target, pos, 1 ); else
    if ( ch == KEY_NPAGE || ch == KEY_PPAGE )
      {
      con_chide();
      int zz = vfu_hist_menu( 5, 5, ( ch == KEY_PPAGE ) ? "Dir Entry History" : "ChDir History",
                                    ( ch == KEY_PPAGE ) ? HID_GETDIR : HID_CHDIR );
      con_cshow();
      if (zz != -1)
        {
        const char* pc = vfu_hist_get( ( ch == KEY_PPAGE ) ? HID_GETDIR : HID_CHDIR, zz );
        if ( pc )
          {
          target = pc;
          pos = str_len( target );
          }
        }
      }
    ch = 0;
    firsthit = 0;
    }
  con_chide();

  //------------------------------------------------------------------
  str_cut_spc( target );
  if ( res == 1 && target[0] == '~' )
    {
    target = tilde_expand( target );
    str_fix_path( target );
    }
/*
  if ( target.len() > 0 )
    {
    // well this tmp is kind of s... ama k'vo da pravi chovek :)
    // FIXME: dos version?
    if ( __ExpandGetDirName && target[0] != '/'
       #ifdef _TARGET_GO32_
       && !( target[1] == ':' && target[2] == '/' )
       #endif
       )
      target = CPath + target;
    StrFixPath( target ); // add trailing slash if not exist
    }
*/
  /*
  #ifdef _TARGET_UNIX_
  leaveok(stdscr, TRUE);
  #endif
  */
  if ( res == 1 && str_len( target ) > 0 && should_exist && !dir_exist( target ))
    {
    vfu_beep();
    int ch = tolower( vfu_ask( "Directory does not exist! Create? "
                               "( ENTER=Yes, ESC=cancel )",
                               "\033\rcC" ));
    if ( ch == 27 )
      {
      res = 0;
      target = "";
      }
    else
    if ( ch == 13 )
       if (make_path( target ))
         {
         if(tolower(
            vfu_ask( "Cannot create path! ( ESC=cancel, C=continue-anyway )",
            "\033Cc" )) == 27)
            {
            res = 0;
            target = "";
            }
         }
    }

  say1(" ");
  say2(" ");
  if ( str_len( target ) > 0)
    {
    str_fix_path( target );
    vfu_hist_add( HID_GETDIR, target );
    }

  str_cut_spc( target );

  ASSERT( res == 0 || res == 1 );
  return res;
}

/*-----------------------------------------------------------------------*/

void vfu_chdir( const char *a_new_dir )
{
  char t[MAX_PATH];
  VString target;
  if ( a_new_dir && a_new_dir[0] )
    {
    target = a_new_dir;
    str_fix_path( target );
    }
  else
    {
    target = vfu_hist_get( HID_CHDIR, 0 );
    if (!vfu_get_dir_name( "ChDir to? (use keys: TAB, PageUp, PageDown, Ctrl+X, Ctrl+A)",
                           target, 0 ))
                           return; /* get_dir_name canceled */
    }
  /* get_dir_name confirmed */
  /*
  if ( work_path[0] != target[0] && DirTreeChanged && opt.AutoTree ) SaveTree();
  */
  if ( work_mode == WM_ARCHIVE )
    {
    archive_name = "";
    archive_path = "";
    }

  vfu_hist_add( HID_CHDIR, work_path );
  char ch = work_path[0];
  if (opt.tree_cd)
    if (!dir_exist( target ))
      {
      int z = 0;
      if ( dir_tree.count() == 0 ) tree_load();
      mb.undef();
      z = tree_find( target, &mb );
      if (z > 1)
        {
        z = vfu_menu_box( 10, 5, "Change dir to..." );
        if (z > -1)
          target = mb.get(z);
        else
          return;
        }
      else
      if (z == 1)
        target = mb.get(0);
      }
  VString str = target;
  str_cut_spc( str );
  #ifdef _TARGET_GO32_
    if ( str[0] == '/' )
      {
      str_ins_ch( str, 0, ':' );
      str_ins_ch( str, 0, work_path[0] );
      } else
    if ( str[1] == ':' && str[2] == 0 ) /* c: d: e: */
      {
      expand_path( str, t );
      str = t;
      }
    if ( str[1] == ':' && str[2] == '/' )
  #else /* _TARGET_GO32_ -> i.e. _TARGET_UNIX_ here*/
    if (str[0] == '/')
  #endif /* _TARGET_GO32_ */
    { /* root directory */
    target = str;
    }
  else
    {
    str = work_path + str;
    str_fix_path( str );
    target = str_reduce_path( str );
    }
  if (chdir( target ) != 0)
    {
    sprintf( t, "chdir: %s", target.data() );
    say1( t );
    say2errno();
    return;
    }
  else
    {
    work_path = target;
    if ( work_mode == WM_ARCHIVE )
      work_mode = WM_NORMAL;
    }
  if ( ch != work_path[0] ) tree_drop(); /* drop tree--it is for another drive*/
  vfu_read_files();
}

/*-----------------------------------------------------------------------*/

void vfu_chdir_history()
{
  int z = vfu_hist_menu( 5, 5, "ChDir History", HID_CHDIR );
  if (z == -1) return;
  do_draw = 1;
  //strcpy(opt.LastDir, CPath);
  vfu_chdir( vfu_hist_get( HID_CHDIR, z ) );
}

/*###########################################################################*/

void tree_load()
{
  if( dir_tree.fload( filename_tree ) )
    say1( "DirTree load error." );
  else
    {
    say1( "DirTree loaded ok." );
    dir_tree_changed = 0;
    }
}

/*-----------------------------------------------------------------------*/

void tree_save()
{
  if( dir_tree.fsave( filename_tree ) )
    say1( "DirTree save error." );
  else
    {
    say1( "DirTree saved ok." );
    dir_tree_changed = 0;
    }
}

/*-----------------------------------------------------------------------*/

void tree_drop()
{
  if ( dir_tree_changed )
    tree_save();
  dir_tree.undef();
  dir_tree_changed = 0;
}

/*-----------------------------------------------------------------------*/

void tree_fix()
{
  int z;
  for( z = dir_tree.count() - 1; z >= 0; z-- )
    {
    VString s1 = dir_tree[z];
    VString s2;
    if (z < dir_tree.count() - 1)
      s2 = dir_tree[z+1];
    else
      s2 = "";
    int i = -1;
    int n = str_count( s1, "/" );
    int p = 0;
    while(n > 2)
      {
      i = str_find( s1, '/', i+1 );
      int q = 0;
      if ( str_len( s2 ) > i )
        q = s1[i] != s2[i];
      if ( q || ( str_count(s2,"/",i+1) < 2))
        {
          p = 1;
          str_set_ch(s1, i, '\\');
        }
      n--;
      }
    if ( p )
      dir_tree.set( z, s1 );
    }
}

/*-----------------------------------------------------------------------*/

fsize_t __tree_rebuild_process( const char* path )
{
  if ( vfu_break_op() ) return -1;

  DIR* dir;
  dirent* de;
  struct stat st;
  fsize_t size = 0;
  char new_name[MAX_PATH];

  dir = opendir( path );
  if ( !dir ) return 0;
  while( (de = readdir(dir)) )
    {
    if ( strcmp( de->d_name, "." ) == 0 ||
         strcmp( de->d_name, ".." ) == 0 ) continue;

    sprintf(new_name, "%s%s", path, de->d_name);
    lstat(new_name, &st);
    int is_link = int(S_ISLNK(st.st_mode));

    if (is_link) continue;

    #ifdef _TARGET_GO32_
    dosstat(dir, &st);
    #else
    stat(new_name, &st);
    #endif

    int is_dir = S_ISDIR(st.st_mode);

    if ( is_dir )
      { /* directory */
      strcat( new_name, "/" );

      int z;
      int trim = 0;
      for ( z = 0; z < trim_tree.count(); z++ )
        {
        VString trim_temp = trim_tree[z];
        str_fix_path( trim_temp );
        if ( pathcmp(trim_temp, new_name) == 0 )
          { /* trim_tree item found */
          trim = 1;
          break;
          }
        }
      if (trim)
        continue; /* cut this branch */

      int pos = dir_tree.count();
      fsize_t dir_size = __tree_rebuild_process( new_name );
      if ( dir_size < 0 )
        { /* canceled */
        closedir(dir);
        return -1;
        }
      dir_tree.ins( pos, new_name );
      size_cache_set( new_name, dir_size );
      size += dir_size;
      }
    else
      { /* file */
      size += file_st_size( &st );
      }

    }
  closedir(dir);

  /* show some progress :) */
  say2( str_dot_reduce( path, con_max_x()-1 ) );
  return size;
}

void tree_rebuild()
{
#ifdef _TARGET_GO32_
// we do need only files sizes -- so the other stuff under dos is unneeded :)
_djstat_flags = _STAT_INODE | _STAT_EXEC_EXT | _STAT_EXEC_MAGIC |
                _STAT_EXEC_MAGIC | _STAT_DIRSIZE | _STAT_ROOT_TIME |
                _STAT_WRITEBIT;
// _djstat_flags = 0;
#endif
  dir_tree.undef();
  size_cache.undef();
  say1( "Rebuilding tree..." );

  __tree_rebuild_process( "/" );

  tree_fix();
#ifdef _TARGET_GO32_
 _djstat_flags = 0;
#endif

  dir_tree_changed = 1;
  tree_save();
}

/*-----------------------------------------------------------------------*/

void tree_draw_item( int page, int index, int hilite )
{
  if ( page + index >= dir_tree.count() ) return;
  VString s1 = dir_tree[page+index];
  str_trim_right(s1,1);
  VString s2 = s1;
  int j = str_rfind( s1,'/');
  str_trim_right(s1,str_len(s2)-j-1);
  str_trim_left(s2,j+1);
  for(j = 0; j < str_len(s1); j++)
    {
    if (s1[j] == '/')
      str_set_ch(s1,j, '|');
    else
    if (s1[j] == '\\')
      str_set_ch(s1,j, '\\');
    else
      str_set_ch(s1,j, '+');
    }
  if (opt.tree_compact)
    {
    str_replace(s1,"+", "");
    str_replace(s1,"|", "|  ");
    str_replace(s1,"\\","   ");
    str_trim_right(s1,2);
    s1 += "--";
    }
  else
    {
    str_replace(s1,"+", " ");
    str_replace(s1,"\\", " ");
    s1 += "--";
    }

  VString str = dir_tree[page+index];
  str_tr( str,"\\", "/" );

  VString sz;
  sz.fi( size_cache_get( str ) );
  if ( sz == "-1" )
    sz = "n/a";
  else
    str_comma( sz );
  str_pad( sz, 14 );

  s1 = sz + " " + s1;

  int m = con_max_x() - 1; /* doesn't speed the code... :) */
  if ( str_len( s1 ) > m )
    {
    str_sleft( s1, m );
    s2 = "";
    }
  else if ( str_len( s1 ) + str_len( s2 ) > m )
    {
    str_sleft( s2, m - str_len( s1 ) );
    }

  con_xy(1,3+1+index);
  if (hilite)
    {
    con_puts( s1, cBAR );
    con_puts( s2, cBAR );
    con_ce( cBAR );
    }
  else
    {
    con_puts( s1, cSTATUS );
    con_puts( s2, cMESSAGE );
    con_ce( cSTATUS );
    }
}

/*-----------------------------------------------------------------------*/

void tree_draw_page( ScrollPos &scroll )
{
  VString str = " ";
  str_mul( str, con_max_x() );
  str = "          SiZE DiRECTORY" + str;
  str_sleft( str, con_max_x()-16 );
  con_out(1,3, str, cHEADER );
  int z = 0;
  for(z = 0; z < scroll.pagesize(); z++)
    {
    if (scroll.page() + z <= scroll.max())
      {
      tree_draw_item( scroll.page(), z );
      }
    else
      {
      con_xy( 1, 3+1+z );
      con_puts( "~", cCYAN );
      con_ce( cCYAN );
      }
    }
}

/*-----------------------------------------------------------------------*/

void tree_draw_pos( ScrollPos &scroll, int opos )
{
  int z = scroll.pos() - scroll.page();
  if ( opos != -1 ) tree_draw_item( scroll.page(), opos );
  tree_draw_item( scroll.page(), z, 1 );
  VString str;
  str = dir_tree[scroll.pos()];
  str_tr( str,"\\", "/" );

  VString sz;
  sz.fi( size_cache_get( str ) );
  str_comma( sz );
  str_pad( sz, 14 );
  str = sz + " " + str;

  str = str_dot_reduce( str, con_max_x()-1 );

  say1( str, cINFO );
  say2( "         Help: R Rebuild, S Incremental search, Z Recalc directory size", cINFO );
  show_pos( scroll.pos()+1, scroll.max()+1 );
}

/*-----------------------------------------------------------------------*/

void tree_view()
{
  VString str;
  if (dir_tree.count() == 0)
    {
    tree_load();
    if (dir_tree.count() == 0)
      tree_rebuild();
    }
  say1( " " );
  int new_pos = tree_index( work_path );
  if ( new_pos == -1 )
    new_pos = 0;

  BSet set; /* used for searching */
  set.set_range1( 'a', 'z' );
  set.set_range1( 'A', 'Z' );
  set.set_range1( '0', '9' );
  set.set_str1( "._-~" );
  set.set_str1( "?*>[]" );

  ScrollPos scroll;
  scroll.set_min_max( 0, dir_tree.count()-1 );
  scroll.set_pagesize( PS+2 );
  scroll.go( new_pos );

  int key = 0;
  int opos = -1;
  int opage = -1;
  while( key != 27 && key != 13 && key != '-' &&
         toupper( key ) != 'Q' && toupper( key ) != 'X' && key != KEY_ALT_X )
    {
    if ( key >= 'A' && key <= 'Z' ) key = tolower( key );
    if ( key == 's' )
      {
        str = "";
        say1( "Enter search pattern: ( use TAB to advance )" );
        key = con_getch();
        while( set.in( key ) || key == 8 || key == KEY_BACKSPACE || key == 9 )
          {
          if ( key == 8 || key == KEY_BACKSPACE )
            str_trim_right( str, 1 );
          else
          if ( key != 9 )
            str_add_ch( str, key );
          say2( str );

          if ( dir_tree.count() == 0 ) { key = con_getch(); continue; }

          int z;
          if ( key == 9 )
            {
            z = scroll.pos() + 1;
            if (z > scroll.max() ) z = scroll.min();
            }
          else
            z = scroll.pos();
          int direction = 1;
          int found = 0;
          int loops = 0;
          VString s_mask = str;
          vfu_expand_mask( s_mask );
          while(1)
            {
            if ( z > scroll.max() ) z = scroll.min();
            if ( z < scroll.min() ) z = scroll.max();

            VString str1 = dir_tree[z];
            str_trim_right( str1, 1 );
            int j = str_rfind(str1,'/');
            if (j < 0)
              str1 = "";
            else
              str_trim_left( str1, j+1 );
            found = ( FNMATCH( s_mask, str1 ) == 0 );
            if ( found ) break;
            z += direction;
            if ( loops++ > dir_tree.count() ) break;
            }
          if (found)
            {
            scroll.go(z);
            tree_draw_page( scroll );
            tree_draw_pos( scroll, opos );
            }
          key = con_getch();
          }
        say1( "" );
        say2( "" );
      }
    else
    switch( key )
      {
      case KEY_UP     : scroll.up(); break;
      case KEY_DOWN   : scroll.down(); break;
      case KEY_PPAGE  : scroll.ppage(); break;
      case KEY_NPAGE  : scroll.npage(); break;
      case KEY_HOME   : scroll.home(); break;
      case KEY_END    : scroll.end(); break;
      case 'r'        : tree_rebuild();
                        scroll.set_min_max( 0, dir_tree.count()-1 );
                        scroll.home();
                        say1( "Rebuild done." );
                        break;
      case 'w'        : tree_save(); break;
      case 'l'        : tree_load();
                        scroll.set_min_max( 0, dir_tree.count()-1 );
                        scroll.home();
                        break;
      case 'z'        :
      case KEY_CTRL_Z :
                        str = dir_tree[scroll.pos()];
                        str_tr( str, "\\", "/" );
                        size_cache_set( str, vfu_dir_size( str ) );
                        tree_draw_page( scroll );
                        tree_draw_pos( scroll, opos );
                        say1( "Done." );
                        break;
      }
    if (opage != scroll.page())
      tree_draw_page( scroll );
    if (opos != scroll.pos() - scroll.page() || opage != scroll.page())
      tree_draw_pos( scroll, opos );
    opos = scroll.pos() - scroll.page();
    opage = scroll.page();
    key = con_getch();
    }
  if ( key == 13 )
    {
    str = dir_tree[scroll.pos()];
    str_tr( str, "\\", "/" );
    vfu_chdir( str );
    }
  do_draw = 2;
}

/*###########################################################################*/

#define SIZE_CACHE_OFFSET 12
#define SIZE_CACHE_OFFSET_CLEAN (12+8+1)

int size_cache_cmp( const char* s1, const char* s2 )
{
  return strcmp( s1 + SIZE_CACHE_OFFSET, s2 + SIZE_CACHE_OFFSET );
}

VString size_cache_compose_key( const char *s, fsize_t size )
{
  const char *ps;
  char ss[MAX_PATH];
  expand_path( s, ss );
  ps = ss;

  char s_adler[16];
  char s_size[32];

  sprintf( s_size, "%012.0f", size ); //WARNING: THIS IS SIZE_CACHE_OFFSET
  sprintf( s_adler, "%08X", (unsigned int)str_adler32( ps ) );

  VString str;
  str = str + s_size + "|" + s_adler + "|" + ps;

  return str;
}

int size_cache_index( const char *s )
{
  if ( size_cache.count() == 0 ) return -1;
  VString str = size_cache_compose_key( s, 0 );
  int l = 0;
  int h = size_cache.count() - 1;
  int m = h;
  while(4)
    {
    int r =  size_cache_cmp( size_cache[m], str );
    if ( l == m && r != 0 )
      return -1;
    if ( r > 0 )
      h = m; else
    if ( r < 0 )
      l = m;
    else
      return m;
    m = ( l + h ) / 2;
    }
}

fsize_t size_cache_get( const char *s )
{
  int z = size_cache_index( s );
  if ( z != -1 )
    {
    VString str = size_cache[z];
    str_sleft( str, SIZE_CACHE_OFFSET );
    return atof( str );
    }
  else
    return -1;
}

void size_cache_set( const char *s, fsize_t size, int sort )
{
  int z = size_cache_index( s );
  if ( z != -1 )
    {
    size_cache.set( z, size_cache_compose_key( s, size ) );
    }
  else
    {
    size_cache.push( size_cache_compose_key( s, size ) );
    if( sort )
      size_cache.sort( 0, size_cache_cmp );
    }
}

// this function is used to add quickly entries to the cache
// it REQUIRES that size_cache_sort() is called after it!
void size_cache_append( const char *s, fsize_t size )
{
  size_cache.push( size_cache_compose_key( s, size ) );
}

void size_cache_clean( const char *s )
{
  VString str = size_cache_compose_key( s, 0 );
  str_trim_left( str, SIZE_CACHE_OFFSET_CLEAN );
  int sl = str_len( str );
  int z = 0;
  while( z < size_cache.count() )
    {
    const char* ps = size_cache[z].data() + SIZE_CACHE_OFFSET_CLEAN;
    if ( ( strncmp( ps, str, sl ) == 0 && (ps[sl] == '/' || ps[sl] == 0) )
         ||
         ( size_cache[z][SIZE_CACHE_OFFSET_CLEAN] != '|' ) )
      size_cache.del( z );
    else
      z++;
    }
}

void size_cache_sort()
{
  size_cache.sort( 0, size_cache_cmp );
}

/*###########################################################################*/

/* return index in the dir_tree of directory named `s' or -1 if not found */
int __tree_index_last_cache = 0; /* cache-like -- keeps the last found index */
int tree_index( const char *s )
{
  int z = 0;
  int i = 0;
  int sl1;
  int sl2;

  if ( dir_tree.count() == 0 ) return -1;

  const char *s1 = s
  #ifdef _TARGET_GO32_
  + 2; // to cut `d:' chars len
  #endif
  ;
  sl1 = strlen( s1 );

  z = __tree_index_last_cache + 1;
  while(4)
    {
    if ( z >= dir_tree.count() ) z = 0;
    if ( z == __tree_index_last_cache ) break;

    const char *s2 = dir_tree[z];
    sl2 = strlen( s2 );

    if ( sl1 == sl2 )
      {
      i = sl1; // or sl2 ...
      while ( i >= 0 && (s1[i] == s2[i] || (s1[i] == '/' && s2[i] == '\\')) ) i--;
      if ( i < 0 )
        {
        __tree_index_last_cache = z;
        return z;
        }
      }
    ASSERT( z < dir_tree.count() );
    ASSERT( z >= 0 );
    z++;
    }
  return -1;
}

/*-----------------------------------------------------------------------*/

const char* tree_find( const char *s ) // return full path by dirname
{
  VString str;
  int z = 0;
  int sl = strlen( s );
  for ( z = 0; z < dir_tree.count(); z++ )
    {
    str = dir_tree[z];
    if ( str_len( str ) < sl ) continue;
    str_sright( str, sl );
    if (pathcmp( (const char*)str, s ) == 0)
      {
      return dir_tree[z];
      }
    }
  return NULL;
}

/*-----------------------------------------------------------------------*/
/* return count of found dirnames and stores them to va */

int tree_find( const char *s, VArray *va )
{
  VString str;
  int z = 0;
  int sl = strlen( s );
  for ( z = 0; z < dir_tree.count(); z++ )
    {
    str = dir_tree[z];
    if ( str_len( str ) < sl ) continue;
    str_sright( str, sl );
    if (pathcmp( str, s ) == 0)
      {
      str = dir_tree[z];
      str_tr( str, "\\", "/" );
      va->push( str );
      }
    }
  return va->count();
}

/*###########################################################################*/

fsize_t __dir_size_process( const char* path )
{
  if ( vfu_break_op() )
    return -1;

  DIR* dir;
  dirent* de;
  struct stat st;
  fsize_t size = 0;
  char new_name[MAX_PATH];

  dir = opendir( path );
  if ( !dir ) return 0;
  while( (de = readdir(dir)) )
    {
    if ( strcmp( de->d_name, "." ) == 0 ||
         strcmp( de->d_name, ".." ) == 0 ) continue;

    sprintf(new_name, "%s%s", path, de->d_name);
    lstat(new_name, &st);
    int is_link = int(S_ISLNK(st.st_mode));
    memset( &st, 0, sizeof(st) );
    #ifdef _TARGET_GO32_
      dosstat(dir, &st);
    #else
      stat(new_name, &st);
    #endif
    int is_dir = int(S_ISDIR(st.st_mode));

    if ( is_link ) continue; /* skip links */
    if ( is_dir )
      {
      strcat( new_name, "/" );
      fsize_t dir_size = __dir_size_process( new_name );
      if ( dir_size == -1 )
        {
        closedir(dir);
        return -1;
        }
      else
        {
        size += dir_size;
        size_cache_append( new_name, dir_size );
        }
      }
    else
      size += file_st_size( &st );

    }
  closedir(dir);

  /* show some progress :) */
  say2( str_dot_reduce( path, con_max_x()-1 ) );
  return size;
}

fsize_t vfu_dir_size( const char *s, int sort )
{
  char t[MAX_PATH];
  expand_path( s, t );
  size_cache_clean( t );
  str_fix_path( t );
  size_cache_clean( t );
  fsize_t size = __dir_size_process( t );
  size_cache_set( t, size, sort );
  return size;
}

/*###########################################################################*/

/* eof vfudir.cpp */
