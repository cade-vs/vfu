/****************************************************************************
 *
 * Copyright (c) 1996-2020 Vladi Belperchinov-Shabanski "Cade" 
 * http://cade.datamax.bg/  <cade@biscom.net> <cade@bis.bg> <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 ****************************************************************************/

#include <vslib.h>
#include "vfu.h"
#include "vfuopt.h"
#include "vfumenu.h"
#include "vfuview.h"

int vfu_toggle_box( int x, int y, const char *title, ToggleEntry* toggles )
{
  menu_box_info.bo = opt.menu_borders;
  menu_box_info.cn = cMENU_CN;
  menu_box_info.ch = cMENU_CH;
  menu_box_info.ti = cMENU_TI;
  int z = con_toggle_box( x, y, title, toggles, &menu_box_info );
  vfu_redraw();
  return z;
}

int vfu_menu_box( int x, int y, const char *title, VArray *va )
{
  menu_box_info.bo = opt.menu_borders;
  menu_box_info.cn = cMENU_CN;
  menu_box_info.ch = cMENU_CH;
  menu_box_info.ti = cMENU_TI;
  int z = con_menu_box( x, y, title, va, 0, &menu_box_info );
  vfu_redraw();
  return z;
}

int vfu_menu_box( const char* title, const char* menustr, int row )
{
  char t[256];
  mb.undef();
  VString str = menustr;
  while( str_len(str) )
    {
    str_word(str,",", t);
    mb.push(t);
    }
  if ( row == -1 ) row = con_max_y() - 5 - mb.count();
  return vfu_menu_box( 50, row, title, &mb );
}

// eof vfumenu.cpp
