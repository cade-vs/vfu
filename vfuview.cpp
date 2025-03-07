/****************************************************************************
 #
 # Copyright (c) 1996-2023 Vladi Belperchinov-Shabanski "Cade" 
 # https://cade.noxrun.com/  <cade@noxrun.com> <cade@bis.bg>
 # https://cade.noxrun.com/projects/vfu     https://github.com/cade-vs/vfu
 #
 # SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 #
 ****************************************************************************/

#include "vfu.h"
#include "vfufiles.h"
#include "vfuview.h"
#include "vfuopt.h"
#include "vfuuti.h"
#include "vfusys.h"

int sel_mark_pos;
int tag_mark_pos;

int get_item_color( TF *fi )
{
  VString str;
  int z;

  ASSERT( fi );

  if ( ! opt.use_colors ) return cNORMAL; /* don't use colors */

  /* try to find file type color first */

  str = fi->type_str();
  str = "." + str + ".";

  if ( str != ".." )
    {
    for ( z = cBLACK; z <= chWHITE; z++ )
      if ( str_find( ext_colors[z], str ) != -1 )
        return z;
    }

  /* regular file, try to find extension color */

  str = fi->name();
  if ( str_get_ch( str, 0 ) == '.' )
    str = ".dotfiles";
  else
    {
    str = fi->ext();
    if ( str == "" ) str = ".";
    }
  str += ".";

  if ( opt.lower_case_ext_config ) str_low( str ); // lowercase extension

  if ( str != ".." )
    {
    for ( z = cBLACK; z <= chWHITE; z++ )
      if ( str_find( ext_colors[z], str ) != -1)
        return z;
    }

  /* type string not found too return std color */
  return cNORMAL;
}

/*-----------------------------------------------------------------------*/

// 999,999,999 -- up to 1GB
// 999.999 XXX -- above 1GB with metric

VString fsize_fmt( fsize_t fs, int use_gib ) /* return commified number */
{
  VString str;
  const char* fix = "";

  int units_size = opt.use_si_sizes ? 1000 : 1024;

  if( fs > pow( units_size, 5 ) )
    {
    sprintf( str, "%.3f", fs / pow( units_size, 5 ) );
    fix = opt.use_si_sizes ? " PB !" : " PiB !";
    }
  else if( fs > pow( units_size, 4 ) )
    {
    sprintf( str, "%.3f", fs / pow( units_size, 4 ) );
    fix = opt.use_si_sizes ? " TB *" : " TiB *";
    }
  else if( fs >= pow( 1000, 3 ) || use_gib ) // 1000 to use only 999'999'999 positions for bytes display
    {
    sprintf( str, "%.3f", fs / pow( units_size, 3 ) );
    fix = opt.use_si_sizes ? " GB +" : " GiB +";
    }
  else
    {
    str.fi( fs );
    fix = "";
    }

  vfu_str_comma( str );
  str += fix;

  return str;  
}

/*-----------------------------------------------------------------------*/

void show_pos( int curr, int all )
{
  char t[64];
  sprintf( t, all > 999999 ? "%16d" : "%6d of %6d", curr, all );
  vfu_con_out( con_max_x() - 15, 3, t, cHEADER );

  // draw scroller

  int x  = con_max_x();
  int y1 = 4;
  int y2 = con_max_y() - 2;

  int y = y2 - y1;

  if( all <= y ) all = 0; // hide scroller
  
  int ss  = all > 0 ? y * y / all : 0;   // scroller size in scroll space
  if( ss < 1 ) ss = 1;     // min scroller size
  int vss = y - ss - 1;    // available scroll space without scroller size

  int s1 = all > 0 ? vss * curr / all : 0; 
  int s2 = s1 + ss;

  // fprintf( stderr, "y %d, ss %d, vss %d, s1 %d, s2 %d\n", y, ss, vss, s1, s2 );

  for( int z = 0; z < y; z++ )
    {
    vfu_con_out( x, y1 + z, " ", ( all > 0 && s1 <= z and z <= s2 ) ? CONCOLOR(cBLACK,cCYAN) : cNORMAL );
    //vfu_con_out( x, y1 + z, ( s1 <= z and z <= s2 ) ? " " : ".", ( s1 <= z and z <= s2 ) ? CONCOLOR(cBLACK,cCYAN) : cCYAN );
    }
}

/*#######################################################################*/

void vfu_drop_all_views()
{
  for( int z = 0; z < files_list_count(); z++ ) files_list_get( z )->drop_view();
  do_draw++;
}

/*#######################################################################*/

void vfu_draw( int n )
{
  if ( n < FLP || n > FLP + FLPS )
    return; /* we are out of screen -- don't draw */

  TF* fi = files_list_get(n);

  int c = fi->color(); /* color to be used */
  VString view = fi->view();
  if ( fi->sel )
    {
    str_set_ch( view, sel_mark_pos, '#' );
    c = CONCOLOR( cBLACK, cWHITE );
    }
  if ( n == FLI )
    {
    str_set_ch( view, tag_mark_pos  , TAGMARKS[opt.tag_mark_type][0] );
    str_set_ch( view, tag_mark_pos+1, TAGMARKS[opt.tag_mark_type][1] );
    c += cBOLD;
    /* this is a hack, can be removed w/o warning :) -- more visibility */
    if ( c == 120 ) c = cTAG; // 116; // 123; // 63; // 123
    //str_replace( view, " ", "_" );
    //c += A_UNDERLINE;
    //c += A_STANDOUT;
    //c += YAS_BOLD;
    //str_replace( view, " ", "-" );
    //c = CONCOLOR(cWHITE,cBLUE);
    }
  vfu_con_out( 1, n - FLP + 4, view, c );
  //  con_ce( c );
}

/*#######################################################################*/

extern const wchar_t *FTIMETYPE[]; /* in vfuopt.cpp */
void vfu_redraw() /* redraw file list and header */
{
  fname_t t;
  VString str;

  str  = "Mask: ";
  str += files_mask;
  vfu_con_out( 1, 1, VString(str), cINFO );
  con_ce( cINFO );
  if ( work_mode == WM_ARCHIVE )
    vfu_con_out( con_max_x()-34, 1, " [-ARCHIVE-] ", cWARNING );
  vfu_con_out( con_max_x()-17, 1, "Press H for help", cINFO);
  vfu_con_out( con_max_x()-20, 1, "VFU " VFU_VERSION " <H> for help",cINFO);

  str = "Path: ";
  str += work_path;
  if ( work_mode == WM_ARCHIVE )
    str += "[" + archive_name + "]/" + archive_path; /* NOTE: to simulate root dir visually */
  str = str_dot_reduce( str, con_max_x()-1 );
  vfu_con_out( 1, 2, str, cINFO );
  con_ce( cINFO );

  str = "";

  if ( opt.sort_order == 'N' ) str = "NAME";
  if ( opt.sort_order == 'M' ) str = "NAME#";
  if ( opt.sort_order == 'E' ) str = "EXT";
  if ( opt.sort_order == 'A' ) str = "MODE";
  if ( opt.sort_order == 'O' ) str = "OWNER";
  if ( opt.sort_order == 'G' ) str = "GROUP";
  if ( opt.sort_order == 'T' ) str = "MTiME";
  if ( opt.sort_order == 'H' ) str = "CTiME";
  if ( opt.sort_order == 'C' ) str = "ATiME";
  if ( opt.sort_order == 'S' ) str = "SiZE";
  if ( opt.sort_order == 'Y' ) str = "TYPE";
  str += opt.sort_direction == 'A' ? "+" : "-";
  str = "(SORT:" + str + ")";
  vfu_con_out( con_max_x() - str_len( str ) + 1, 2, str, cHEADER );

  str = "";

  t[0] = 0;
  char *spos = t;
  if (opt.sort_order == 'D') opt.sort_order = 'T'; /* hack anyway */
  if (!opt.long_name_view)
    {
    //if ( opt.f_mode  ) spos += sprintf( spos, "%10s ", MODE_STRING );
    if ( opt.f_mode  ) spos += sprintf( spos, "MODE       " );
    if ( opt.f_owner ) spos += sprintf( spos, "   OWNER " );
    if ( opt.f_group ) spos += sprintf( spos, "   GROUP " );
    if ( opt.f_time  ) spos += sprintf( spos, " %s TiME  ", VString( FTIMETYPE[opt.f_time_type] ).data() );
    if ( opt.f_size  ) spos += sprintf( spos, "          SiZE " );
    };
  if ( opt.f_mode + opt.f_owner + opt.f_group + opt.f_time + opt.f_size + opt.f_type == 0 )
    opt.f_type = 1; /* a hack really :) if all fields are off -- turn on type one */
  if ( opt.f_type || opt.long_name_view ) 
    spos += sprintf( spos, "TY" );
  tag_mark_pos = strlen( t );
  sel_mark_pos = tag_mark_pos + 2;

  /* spos += */ sprintf( spos, "  #NAME  %s",
                         opt.long_name_view ? "( long name view, press 0 to disable )" : "" );

  str_pad( t, - con_max_x() );
  str_sleft( t, con_max_x() );

  vfu_con_out( 1, 3, t, cHEADER );
  show_pos( FLI+1, files_list_count() );

  int z;

  for ( z = 0; z < FLPS; z++ )
    {
    ASSERT( FLP + z >= 0 );
    if ( FLP + z >= files_list_count() || files_list_is_empty( FLP + z ) )
      {
      vfu_con_out( 1, z + 4, "~", cPLAIN );
      con_ce( cPLAIN );
      }
    else
      vfu_draw( FLP + z );
    }
  if ( files_list_count() <= 0 )
    vfu_con_out( ( con_max_x() - 20 ) / 2, 10, " *** No files found *** ", cHEADER);

}

/*-----------------------------------------------------------------------*/

void vfu_redraw_status() /* redraw bottom status, total,free,selected... */
{
  VString s1;
  VString s2;
  VString tmp;

  /* first line here */
  s1  = "Select:";
  tmp = sel_count;
  vfu_str_comma(tmp);
  str_pad(tmp,15);
  s1 += tmp;

  s1 += "  Free: ";
  //tmp = size_str_compact( fs_free );
  tmp = fsize_fmt( fs_free, opt.use_gib_usage );
  str_pad( tmp, 14 );
  s1 += tmp;
  if (fs_total == 0 || fs_free > fs_total)
    tmp = "  n/a%";
  else
    sprintf( 64, tmp, "%4.1f%%", (double)100 * ((double)fs_free / (double)fs_total));

  s1 += "  " + tmp + " FSize:";
  //tmp = size_str_compact( files_size );
  tmp = fsize_fmt( files_size );
  str_pad( tmp, 15 );
  s1 += tmp;
  if (fs_total == 0 || files_size > fs_total)
    tmp = " n/a%";
  else
    sprintf( 64, tmp,"%4.1f%%", (double)100 * ((double)files_size / (double)fs_total));
  s1 += " " + tmp;

  /* second line here */
  s2  = "S.Size:";
  //tmp = size_str_compact( sel_size );
  tmp = fsize_fmt( sel_size );
  str_pad(tmp,15);
  s2 += tmp;
  
  s2 += "  Total:";
  //tmp = size_str_compact( fs_total );
  tmp = fsize_fmt( fs_total, opt.use_gib_usage );
  str_pad( tmp, 14 );
  s2 += tmp;

  tmp = fs_block_size; 
  str_pad( tmp, 5 ); 
  s2 += " [" + tmp + "]";
  
  
  tmp = " ";
  tmp = tmp + user_id_str + "." + group_id_str + "@" + host_name_str;
  str_padw( tmp, con_max_x() - str_len( s2 ) - 1 );
  
  s2 += tmp;

  str_pad( s1, - con_max_x() );
  str_pad( s2, - con_max_x() );

  vfu_con_out( 1, con_max_y()-3, s1, cINFO2 );
  vfu_con_out( 1, con_max_y()-2, s2, cINFO2 );
}

/*#######################################################################*/

void vfu_nav_up()
{
  if ( files_list_count() == 0) return;
  if ( FLI == 0 ) return;

  int old_flp = FLP;
  file_list_index.up();
  if ( old_flp != FLP )
    do_draw = 1;
  else
    {
    vfu_draw(FLI+1);
    vfu_draw(FLI);
    }
}

/*-----------------------------------------------------------------------*/

void vfu_nav_down()
{
  if ( files_list_count() == 0 ) return;
  if ( FLI == files_list_count() - 1 ) return;

  int old_flp = FLP;
  file_list_index.down();
  if ( old_flp != FLP )
    do_draw = 1;
  else
    {
    vfu_draw( FLI-1 );
    vfu_draw( FLI   );
    }
}

/*-----------------------------------------------------------------------*/

void vfu_nav_ppage()
{
  if ( files_list_count() == 0 ) return;
  if ( FLP == 0 && FLI == 0 ) return;

  int old_fli = FLI;
  int old_flp = FLP;
  file_list_index.pageup();
  if ( old_flp != FLP )
    do_draw = 1;
  else
    {
    vfu_draw( old_fli );
    vfu_draw( FLI   );
    }
}

/*-----------------------------------------------------------------------*/

void vfu_nav_npage()
{
  if ( files_list_count() == 0 ) return;
  if ( FLP >= files_list_count() - FLPS && FLI == files_list_count() - 1 ) return;

  int old_fli = FLI;
  int old_flp = FLP;
  file_list_index.pagedown();
  if ( old_flp != FLP )
    do_draw = 1;
  else
    {
    vfu_draw( old_fli );
    vfu_draw( FLI   );
    }
}

/*-----------------------------------------------------------------------*/

void vfu_nav_home()
{
  if ( files_list_count() == 0 ) return;
  ASSERT( FLI >= 0 && FLI <= files_list_count() - 1 );
  if ( opt.sort_top_dirs && opt.smart_home_end && FLI == 0 ) 
    {
    int z = 0;
    while( z < files_list_count() )
      {
      TF *fi = files_list_get(z);
      if( ! fi->is_dir() ) break;
      z++;
      }
    if( z < files_list_count() )
      FLGO( z );
    }
  else
    FLGO( 0 );
  vfu_nav_update_pos();
  do_draw = 1;
/*
  if ( files_list_count() == 0 ) return;
  if ( FLI == 0 ) return;
  FLGO( 0 );
  vfu_nav_update_pos();
  do_draw = 1;
*/  
}

/*-----------------------------------------------------------------------*/

void vfu_nav_end()
{
  if ( files_list_count() == 0 ) return;
  ASSERT( FLI >= 0 && FLI <= files_list_count() - 1 );
  if ( opt.sort_top_dirs && opt.smart_home_end && FLI == files_list_count() - 1 )
    {
    int z = FLI;
    while( z >= 0 )
      {
      TF *fi = files_list_get(z);
      if( fi->is_dir() ) break;
      z--;
      }
    if( z >= 0 )
      FLGO( z );
    }
  else
    FLGO( files_list_count() - 1 );
  vfu_nav_update_pos();
  do_draw = 1;
/*
  if ( files_list_count() == 0 ) return;
  if ( FLI >= files_list_count() - 1 ) return;
  FLGO( files_list_count() - 1 );
  vfu_nav_update_pos();
  do_draw = 1;
*/
}

/*-----------------------------------------------------------------------*/

void vfu_nav_select()
{
  if ( files_list_count() == 0 ) return;
  TF *fi = files_list_get(FLI);

  fsize_t f = fi->size();
  if ( f < 0 ) f = 0; /* dirs w/o size i.e. -1 */

  if ( fi->sel )
    { /* unselect */
    sel_count --;
    sel_size -= f;
    }
  else
    { /* select */
    sel_count ++;
    sel_size += f;
    }

  fi->sel = - ( fi->sel - 1 ); /* I know -- !!(fi->sel) ... :) */

  vfu_draw( FLI );
  vfu_nav_down();
  vfu_draw( FLI );
  do_draw_status = 1;
}

/*-----------------------------------------------------------------------*/

void vfu_nav_update_pos()
{
 ASSERT( files_list_count() >= 0 );
 if ( FLI < 0 ) FLGO( 0 );
 if ( files_list_count() == 0 ) FLGO( 0 );
 if ( files_list_count() > 0 && FLI > files_list_count() - 1 ) FLGO( files_list_count() - 1 );
}

/* eof vfuview.cpp */
