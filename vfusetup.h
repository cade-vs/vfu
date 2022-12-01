/****************************************************************************
 *
 * Copyright (c) 1996-2022 Vladi Belperchinov-Shabanski "Cade" 
 * http://cade.noxrun.com/  <cade@noxrun.com> <cade@bis.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 ****************************************************************************/

#ifndef _SETUP_H_
#define _SETUP_H_

/*
 *
 * This file is used to setup some global parameters as
 * files locations and other similar things...
 * (it is separated from vfu.h just to keep it clear)
 *
 */

#define VFU_VERSION "4.24"

#define HEADER "VF/U v" VFU_VERSION " Copyright (c) 1996-2022 Vladi Belperchinov-Shabanski 'Cade'"
#define CONTACT "<cade@noxrun.com> <cade@bis.bg> [http://cade.noxrun.com]"

#ifdef _TARGET_GO32_
  #define FILENAME_OPT     "vfu.opt"
  #define FILENAME_CONF    "vfu.cfg"
  #define FILENAME_TREE    "vfu.tre"
  #define FILENAME_SIZE_CACHE    "vfu.siz"
  #define FILENAME_HISTORY "vfu.hst"
  #define FILENAME_FFR     "vfu.ffr"
#else
  #define FILENAME_OPT     "vfu.options"
  #define FILENAME_CONF    "vfu.conf"
  #define FILENAME_TREE    "vfu.tree"
  #define FILENAME_SIZE_CACHE    "vfu.size"
  #define FILENAME_HISTORY "vfu.history"
  #define FILENAME_FFR     "vfu.ffr"
#endif

// The SYSCONFDIR macro can be used here, which is defined when
// ../configure --sysconfdir=nnnn is used. It no option is given, it
// defaults to /usr/local/etc on most systems.
// The macro has been added to Makefile.am
//                                    -andy5995 2018-12-29
#ifndef FILENAME_CONF_GLOBAL0
#define FILENAME_CONF_GLOBAL0 "/etc/" FILENAME_CONF
#endif

#ifndef FILENAME_CONF_GLOBAL1
#define FILENAME_CONF_GLOBAL1 "/usr/local/etc/" FILENAME_CONF
#endif

#ifndef FILENAME_CONF_GLOBAL2
#define FILENAME_CONF_GLOBAL2 "/usr/libexec/vfu/" FILENAME_CONF
#endif

#define RX_TEMP_LIST "RX_TEMP_LIST"

/* colors */

#define cPLAIN   (cNORMAL)  // normal white
#define cHEADER  (chRED)    // files list headers
#define cINFO    (chYELLOW) // general info messages
#define cINFO2   (chYELLOW) // bottom information panel
#define cINPUT   (CONCOLOR(chWHITE,cBLUE)) // normal input lines
#define cINPUT2  (CONCOLOR(cBLACK,cWHITE)) // selected input lines
#define cMESSAGE (cWHITE)  // all messages
#define cSTATUS  (cCYAN)   // status messages (progress info)
#define cSTATUS2 (chCYAN)  // alt status messages (copy progress info)
#define cWARNING (CONCOLOR(chWHITE,cRED)) // warning messages

#define cBAR     (CONCOLOR(chWHITE,cBLUE)) // inverted select bar (dir tree)
#define cTAG     (CONCOLOR(cRED,cWHITE)) // currently selected file

#define cMENU_CN (CONCOLOR(chWHITE,cBLUE)) // menu normal
#define cMENU_CH (CONCOLOR(chWHITE,cGREEN)) // menu highlite
#define cMENU_TI (CONCOLOR(chWHITE,cMAGENTA)) // menu title

/* mono config -- never tested! */
/*
#define cPLAIN   (cWHITE)
#define cHEADER  (CONCOLOR(cBLACK,cWHITE))
#define cINFO    (chWHITE)
#define cINFO2   (CONCOLOR(cBLACK,cWHITE))
#define cINPUT   (CONCOLOR(cWHITE,cBLACK))
#define cINPUT2  (CONCOLOR(cBLACK,cWHITE))
#define cMESSAGE (chWHITE)
#define cSTATUS  (cWHITE)
#define cSTATUS2 (chWHITE)
#define cWARNING (CONCOLOR(cBLACK,cWHITE))

#define cBAR     (CONCOLOR(cBLACK,cWHITE))
#define cTAG     cBAR

#define cMENU_CN (CONCOLOR(cWHITE,cBLACK)) // menu normal
#define cMENU_CH (CONCOLOR(cBLACK,cWHITE)) // menu highlite
#define cMENU_TI (CONCOLOR(cBLACK,cWHITE)) // menu title
*/
/* colors setup end */

#endif //_SETUP_H_
