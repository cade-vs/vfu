/*
 *
 * (c) Vladi Belperchinov-Shabanski "Cade" 1996-2014
 * http://cade.datamax.bg/  <cade@biscom.net> <cade@bis.bg> <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 */

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

  if (!opt.use_colors)
    return cNORMAL; /* don't use colors -- option */

  /* try to find file type color first */

  str = fi->type_str();
  str = "." + str + ".";

  if ( str != ".." )
    {
    for ( z = cBLACK; z <= chWHITE; z++ )
      if (str_find( ext_colors[z], str ) != -1)
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

  #ifdef _TARGET_GO32_
  /* under dos/windows file names are not case sensitive */
  str_low( str );
  #endif

  if ( str != ".." )
    {
    for ( z = cBLACK; z <= chWHITE; z++ )
      if (str_find( ext_colors[z], str ) != -1)
        return z;
    }

  /* type string not found too return std color */
  return cNORMAL;
}

/*-----------------------------------------------------------------------*/

VString fsize_fmt( fsize_t fs ) /* return commified number */
{
  VString str;
  if( fs > 99999999999.0 ) // 99_999_999_999 11 positions + 3 comma = 14 chars
    {
    str.fi( int( fs / ( 1024*1024 ) ) );
    str_comma( str );
    str += " MiB";
    }
  else
    {
    str.fi( fs );
    str_comma( str );
    }
  return str;
}

/*-----------------------------------------------------------------------*/

void show_pos( int curr, int all )
{
  char t[64];
  sprintf( t, "%5d of %5d", curr, all );
  con_out( con_max_x() - 13, 3, t, cHEADER );
}

/*#######################################################################*/

void vfu_drop_all_views()
{
  int z = 0;
  for( z = 0; z < files_count; z++ ) files_list[z]->drop_view();
  do_draw += 1;
}

/*#######################################################################*/

void vfu_draw( int n )
{
  VFU_CHECK_LIST_POS( n );
  if ( n < FLP || n > FLP + PS )
    return; /* we are out of screen -- don't draw */

  TF* fi = files_list[n];

  int c = fi->color(); /* color to be used */
  VString view = fi->view();
  if ( fi->sel )
    {
    str_set_ch( view, sel_mark_pos, '#' );
    c = CONCOLOR(cBLACK,cWHITE);
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
    //str_replace( view, " ", "-" );
    //c = CONCOLOR(cWHITE,cBLUE);
    }
  con_out( 1, n - FLP + 4, view, c );
  //  con_ce( c );
}

/*#######################################################################*/

extern const char *FTIMETYPE[]; /* in vfuopt.cpp */
void vfu_redraw() /* redraw file list and header */
{
  char t[MAX_PATH];
  VString str;

  str  = "Mask: ";
  str += files_mask;
  con_out(1,1,str,cINFO);
  con_ce(cINFO);
  if ( work_mode == WM_ARCHIVE )
    con_out( con_max_x()-34, 1, " [-ARCHIVE-] ", cWARNING );
  con_out(con_max_x()-17,1,"Press H for help",cINFO);
  con_out(con_max_x()-20,1,"VFU " VFU_VERSION " <H> for help",cINFO);

  str = "Path: ";
  str += work_path;
  if ( work_mode == WM_ARCHIVE )
    {
    str += "[";
    str += archive_name;
    str += "]/"; /* NOTE: to simulate root dir visually */
    str += archive_path;
    }
  str = str_dot_reduce( str, con_max_x()-1 );
  con_out(1,2,str,cINFO);
  con_ce(cINFO);

  str = "";

  if ( opt.sort_order == 'N' ) str = "NAME";
  if ( opt.sort_order == 'M' ) str = "NAME#";
  if ( opt.sort_order == 'E' ) str = "EXT";
  if ( opt.sort_order == 'A' ) str = "MODE";
  if ( opt.sort_order == 'O' ) str = "OWNER";
  if ( opt.sort_order == 'G' ) str = "GROUP";
  if ( opt.sort_order == 'T' ) str = "MTIME";
  if ( opt.sort_order == 'H' ) str = "CTIME";
  if ( opt.sort_order == 'C' ) str = "ATIME";
  if ( opt.sort_order == 'S' ) str = "SIZE";
  if ( opt.sort_order == 'Y' ) str = "TYPE";
  str += opt.sort_direction == 'A' ? "+" : "-";
  str = "(SORT:" + str + ")";
  con_out( con_max_x() - str_len( str ) + 1, 2, str, cHEADER );

  str = "";

  t[0] = 0;
  char *spos = t;
  if (opt.sort_order == 'D') opt.sort_order = 'T'; /* hack anyway */
  if (!opt.long_name_view)
    {
    if (opt.f_mode  ) spos += sprintf( spos, "%10s ", MODE_STRING );
    if (opt.f_owner ) spos += sprintf( spos, "   OWNER " );
    if (opt.f_group ) spos += sprintf( spos, "   GROUP " );
    if (opt.f_time  ) spos += sprintf( spos, "%s  TiME ", FTIMETYPE[opt.f_time_type] );
    if (opt.f_size  ) spos += sprintf( spos, "          SiZE " );
    };
  if ( opt.f_mode + opt.f_owner + opt.f_group + opt.f_time + opt.f_size + opt.f_type == 0 )
    opt.f_type = 1; /* a hack really :) if all fields are off -- turn on type one */
  if (opt.f_type || opt.long_name_view) spos += sprintf( spos, "TP" );
  tag_mark_pos = strlen( t );
  sel_mark_pos = tag_mark_pos + 2;

  spos += sprintf( spos, "  #NAME    %s",
                   opt.long_name_view ? "( long name view )" : "" );

  str_pad( t, - con_max_x() );
  str_sleft( t, con_max_x() );

  con_out(1,3, t, cHEADER );
  show_pos( FLI+1, files_count );

  int z;

  for ( z = 0; z < PS; z++ )
    {
    ASSERT( FLP + z >= 0 );
    if ( FLP + z >= files_count )
      {
      con_out( 1, z+4, "~", cPLAIN );
      con_ce( cPLAIN );
      }
    else if ( files_list[FLP+z] == NULL )  /* FIXME: if NULL?! */
      {
      con_out( 1, z+4, "~", cPLAIN );
      con_ce( cPLAIN );
      }
    else
      vfu_draw( FLP + z );
    }
  if ( files_count <= 0 )
    con_out( ( con_max_x() - 20 ) / 2, 10, " *** No files found *** ", cHEADER);

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
  str_comma(tmp);
  str_pad(tmp,14);
  s1 += tmp;

  s1 += "  Free: ";
  tmp = fsize_fmt( fs_free );
  str_pad(tmp,14);
  s1 += tmp;
  if (fs_total == 0 || fs_free > fs_total)
    tmp = "  n/a%";
  else
    sprintf( 64, tmp, "%5.1f%%", (double)100 * ((double)fs_free / (double)fs_total));

  s1 += "  " + tmp + "  FSize:";
  tmp = fsize_fmt( files_size );
  str_pad(tmp,14);
  s1 += tmp;
  if (fs_total == 0 || files_size > fs_total)
    tmp = " n/a%";
  else
    sprintf(tmp,"%4.1f%%", (double)100 * ((double)files_size / (double)fs_total));
  s1 += " " + tmp;

  /* second line here */
  s2  = "S.Size:";

  tmp = fsize_fmt( sel_size );
  str_pad(tmp,14);
  s2 += tmp;
  s2 += "  Total:";
  tmp = fsize_fmt( fs_total );
  str_pad(tmp,14);
  s2 += tmp;

  tmp = fs_block_size; str_pad( tmp,5 ); s2 += " [" + tmp + "]";
  sprintf( tmp,"  %s.%s@%s ", user_id_str.data(), group_id_str.data(),
                              host_name_str.data() );
  s2 += tmp;

  str_pad( s1, - con_max_x() );
  str_pad( s2, - con_max_x() );

  con_out( 1, con_max_y()-3, s1, cINFO2 );
  con_out( 1, con_max_y()-2, s2, cINFO2 );
}

/*#######################################################################*/

void vfu_nav_up()
{
  if ( files_count == 0) return;
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
  if ( files_count == 0 ) return;
  if ( FLI == files_count - 1 ) return;

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
if ( files_count == 0 ) return;
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
  if ( files_count == 0 ) return;
  if ( FLP >= files_count - PS && FLI == files_count - 1 ) return;

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
  if ( files_count == 0 ) return;
  ASSERT( FLI >= 0 && FLI <= files_count - 1 );
  if ( opt.sort_top_dirs && opt.smart_home_end && FLI == 0 ) 
    {
    int z = 0;
    while( z < files_count )
      {
      TF *fi = files_list[z];
      if( ! fi->is_dir() ) break;
      z++;
      }
    if( z < files_count )
      FGO( z );
    }
  else
    FGO( 0 );
  vfu_nav_update_pos();
  do_draw = 1;
/*
  if ( files_count == 0 ) return;
  if ( FLI == 0 ) return;
  FGO( 0 );
  vfu_nav_update_pos();
  do_draw = 1;
*/  
}

/*-----------------------------------------------------------------------*/

void vfu_nav_end()
{
  if ( files_count == 0 ) return;
  ASSERT( FLI >= 0 && FLI <= files_count - 1 );
  if ( opt.sort_top_dirs && opt.smart_home_end && FLI == files_count - 1 )
    {
    int z = FLI;
    while( z >= 0 )
      {
      TF *fi = files_list[z];
      if( fi->is_dir() ) break;
      z--;
      }
    if( z >= 0 )
      FGO( z );
    }
  else
    FGO( files_count - 1 );
  vfu_nav_update_pos();
  do_draw = 1;
/*
  if ( files_count == 0 ) return;
  if ( FLI >= files_count - 1 ) return;
  FGO( files_count - 1 );
  vfu_nav_update_pos();
  do_draw = 1;
*/
}

/*-----------------------------------------------------------------------*/

void vfu_nav_select()
{
  if ( files_count == 0 ) return;
  TF *fi = files_list[FLI];

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
 ASSERT( files_count >= 0 );
 if ( FLI < 0 ) FGO( 0 );
 if ( files_count == 0 ) FGO( 0 );
 if ( files_count > 0 && FLI > files_count - 1 ) FGO( files_count - 1 );
}

/* eof vfuview.cpp */
