/****************************************************************************
 *
 * Copyright (c) 1996-2022 Vladi Belperchinov-Shabanski "Cade" 
 * http://cade.noxrun.com/  <cade@noxrun.com> <cade@bis.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 ****************************************************************************/

#ifndef _VFUVIEW_H_
#define _VFUVIEW_H_

#include "vfuopt.h"

extern int tag_mark_pos;
extern int sel_mark_pos;

int get_item_color( TF* fi );
VString fsize_fmt( fsize_t fs, int use_gib = 0 ); /* return commified number */

void show_pos( int curr, int all );

void vfu_drop_all_views();

void vfu_draw(int n);

void vfu_redraw(); /* redraw file list and header */
void vfu_redraw_status(); /* redraw bottom status, total,free,selected... */

void vfu_nav_up();
void vfu_nav_down();
void vfu_nav_ppage();
void vfu_nav_npage();
void vfu_nav_home();
void vfu_nav_end();
void vfu_nav_select();
void vfu_nav_update_pos();

#endif //_VFUVIEW_H_

