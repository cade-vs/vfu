/*
 *
 * (c) Vladi Belperchinov-Shabanski "Cade" 1996-2000
 * http://www.biscom.net/~cade  <cade@biscom.net>  <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 * $Id: vfuview.h,v 1.2 2001/10/28 13:56:40 cade Exp $
 *
 */

#ifndef _VFUVIEW_H_
#define _VFUVIEW_H_

#include "vfuopt.h"

extern int tag_mark_pos;
extern int sel_mark_pos;

int get_item_color( TF* fi );

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

