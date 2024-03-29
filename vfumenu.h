/****************************************************************************
 #
 # Copyright (c) 1996-2023 Vladi Belperchinov-Shabanski "Cade" 
 # https://cade.noxrun.com/  <cade@noxrun.com> <cade@bis.bg>
 # https://cade.noxrun.com/projects/vfu     https://github.com/cade-vs/vfu
 #
 # SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 #
 ****************************************************************************/

#ifndef _VFUMENU_H_
#define _VFUMENU_H_

#include <vslib.h>
#include "vfuuti.h"

#define menu_box_info con_default_menu_info

int vfu_toggle_box( int x, int y, const wchar_t *title, ToggleEntry* toggles );
int vfu_menu_box( int x, int y, const wchar_t *title, WArray *wa = &mb );
int vfu_menu_box( const wchar_t* title, const wchar_t* menustr, int row = -1 );


#endif /* _VFUMENU_H_ */

/* eof vfumenu.h */
