/****************************************************************************
 *
 * Copyright (c) 1996-2022 Vladi Belperchinov-Shabanski "Cade" 
 * http://cade.noxrun.com/  <cade@noxrun.com> <cade@bis.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 ****************************************************************************/

#include <vslib.h>
#include "vfu.h"
#include "vfuopt.h"
#include "vfumenu.h"
#include "vfuview.h"

int vfu_toggle_box( int x, int y, const wchar_t *title, ToggleEntry* toggles )
{
  menu_box_info.bo = opt.menu_borders;
  menu_box_info.cn = cMENU_CN;
  menu_box_info.ch = cMENU_CH;
  menu_box_info.ti = cMENU_TI;
  int z = con_toggle_box( x, y, title, toggles, &menu_box_info );
  vfu_redraw();
  return z;
}

int vfu_menu_box( int x, int y, const wchar_t *title, WArray *wa )
{
  menu_box_info.bo = opt.menu_borders;
  menu_box_info.cn = cMENU_CN;
  menu_box_info.ch = cMENU_CH;
  menu_box_info.ti = cMENU_TI;
  int z = con_menu_box( x, y, title, wa, 0, &menu_box_info );
  vfu_redraw();
  return z;
}

int vfu_menu_box( const wchar_t* title, const wchar_t* menustr, int row )
{
  WArray wmb = str_split( L",", menustr );
  if ( row == -1 ) row = con_max_y() - 5 - wmb.count();
  return vfu_menu_box( 50, row, title, &wmb );
}

// eof vfumenu.cpp
