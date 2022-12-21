/****************************************************************************
 *
 * Copyright (c) 1996-2022 Vladi Belperchinov-Shabanski "Cade" 
 * http://cade.noxrun.com/  <cade@noxrun.com> <cade@bis.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 ****************************************************************************/

#include "vfu.h"
#include "vfusys.h"
#include "vfuuti.h"
#include "vfumenu.h"

#ifdef _TARGET_GO32_
  #include <io.h>
#endif

/*###########################################################################*/

void file_get_mode_str( const mode_t tm, mode_str_t &mod_str )
{
  strcpy( mod_str, MODE_OFF );
  if (S_ISDIR(tm) ) mod_str[0] = 'd';else
  if (S_ISLNK(tm) ) mod_str[0] = 'l';else
  if (S_ISBLK(tm) ) mod_str[0] = 'b';else
  if (S_ISCHR(tm) ) mod_str[0] = 'c';else
  if (S_ISFIFO(tm)) mod_str[0] = 'f';else
  if (S_ISSOCK(tm)) mod_str[0] = 's';

  if ((tm & S_IRUSR) != 0) mod_str[1] = 'r';
  if ((tm & S_IWUSR) != 0) mod_str[2] = 'w';
  if ((tm & S_IXUSR) != 0) mod_str[3] = 'x';
  if ((tm & S_IRGRP) != 0) mod_str[4] = 'r';
  if ((tm & S_IWGRP) != 0) mod_str[5] = 'w';
  if ((tm & S_IXGRP) != 0) mod_str[6] = 'x';
  if ((tm & S_IROTH) != 0) mod_str[7] = 'r';
  if ((tm & S_IWOTH) != 0) mod_str[8] = 'w';
  if ((tm & S_IXOTH) != 0) mod_str[9] = 'x';

  #ifndef _TARGET_GO32_
  if ((tm & S_ISUID) != 0) mod_str[3] = ((tm & S_IXUSR) != 0) ? 's' : 'S';
  if ((tm & S_ISGID) != 0) mod_str[6] = ((tm & S_IXGRP) != 0) ? 's' : 'S';
  if ((tm & S_ISVTX) != 0) mod_str[9] = ((tm & S_IXOTH) != 0) ? 't' : 'T';
  #endif

  #ifdef _TARGET_GO32_
  mod_str[4]=mod_str[5]=mod_str[6]=mod_str[7]=mod_str[8]=mod_str[9]='-';
  #endif
}

/*---------------------------------------------------------------------------*/

int  file_get_mode_str( const char *filename, mode_str_t &mod_str )
{
  strcpy( mod_str, MODE_OFF );
  struct stat st;
  if ( stat(filename, &st) ) return 1;
  file_get_mode_str(st.st_mode, mod_str);
  return 0;
}

/*---------------------------------------------------------------------------*/

int  file_set_mode_str( const char *filename, const mode_str_t mod_str )
{
  mode_str_t old_mod_str;
  mode_str_t new_mod_str;
  mode_t new_mode = 0;
  strcpy( new_mod_str, mod_str );

  if (strchr( new_mod_str, '?' ))
    {
    if (file_get_mode_str(filename, old_mod_str)) return 1;

    if (new_mod_str[1] == '?') new_mod_str[1] = old_mod_str[1];
    if (new_mod_str[2] == '?') new_mod_str[2] = old_mod_str[2];
    if (new_mod_str[3] == '?') new_mod_str[3] = old_mod_str[3];
    if (new_mod_str[4] == '?') new_mod_str[4] = old_mod_str[4];
    if (new_mod_str[5] == '?') new_mod_str[5] = old_mod_str[5];
    if (new_mod_str[6] == '?') new_mod_str[6] = old_mod_str[6];
    if (new_mod_str[7] == '?') new_mod_str[7] = old_mod_str[7];
    if (new_mod_str[8] == '?') new_mod_str[8] = old_mod_str[8];
    if (new_mod_str[9] == '?') new_mod_str[9] = old_mod_str[9];
    }

  if (new_mod_str[1] == 'r') new_mode |= S_IRUSR;
  if (new_mod_str[2] == 'w') new_mode |= S_IWUSR;
  if (new_mod_str[3] == 'x') new_mode |= S_IXUSR;
  if (new_mod_str[4] == 'r') new_mode |= S_IRGRP;
  if (new_mod_str[5] == 'w') new_mode |= S_IWGRP;
  if (new_mod_str[6] == 'x') new_mode |= S_IXGRP;
  if (new_mod_str[7] == 'r') new_mode |= S_IROTH;
  if (new_mod_str[8] == 'w') new_mode |= S_IWOTH;
  if (new_mod_str[9] == 'x') new_mode |= S_IXOTH;

  #ifndef _TARGET_GO32_
  if (new_mod_str[3] == 's') { new_mode |= S_ISUID; new_mode |= S_IXUSR; }
  if (new_mod_str[3] == 'S') new_mode |= S_ISUID;
  if (new_mod_str[6] == 's') { new_mode |= S_ISGID; new_mode |= S_IXGRP; }
  if (new_mod_str[6] == 'S') new_mode |= S_ISGID;
  if (new_mod_str[9] == 't') { new_mode |= S_ISVTX; new_mode |= S_IXOTH; }
  if (new_mod_str[9] == 'T') new_mode |= S_ISVTX;
  #endif

  return ( chmod( filename, new_mode ) != 0 );
}

/*---------------------------------------------------------------------------*/

int  vfu_edit_attr( mode_str_t mod_str, int allow_masking )
{
  int mode_i[16];
  if (allow_masking == 0)
    { /* "-rwxrwxrwx" */
    for ( int z = 0; z < 16; z++ ) mode_i[z] = 1;
    mode_i[ 1] = (mod_str[1] != 'r');
    mode_i[ 2] = (mod_str[2] != 'w');
    mode_i[ 3] = (mod_str[3] != 'x');
    mode_i[ 4] = (mod_str[4] != 'r');
    mode_i[ 5] = (mod_str[5] != 'w');
    mode_i[ 6] = (mod_str[6] != 'x');
    mode_i[ 7] = (mod_str[7] != 'r');
    mode_i[ 8] = (mod_str[8] != 'w');
    mode_i[ 9] = (mod_str[9] != 'x');

    mode_i[10] = !((mod_str[3] == 's') || (mod_str[3] == 'S'));
    mode_i[11] = !((mod_str[6] == 's') || (mod_str[6] == 'S'));
    mode_i[12] = !((mod_str[9] == 't') || (mod_str[9] == 'T'));
    if (mode_i[3]) mode_i[ 3] = !(mod_str[3] == 's');
    if (mode_i[6]) mode_i[ 6] = !(mod_str[6] == 's');
    if (mode_i[9]) mode_i[ 9] = !(mod_str[9] == 't');
    }
  else
    {
    for ( int z = 0; z < 16; z++ ) mode_i[z] = 2;
    }

  const wchar_t* AONOFF1[] = { L"YES", L" - ", L" ? ", NULL };
  const wchar_t* AONOFF2[] = { L"YES", L" - ", NULL };
  #define AONOFF  ( allow_masking ? AONOFF1 : AONOFF2 )
  ToggleEntry mode_toggles[] =
  {
  { L' ', L"Read      Owner", &mode_i[ 1], AONOFF },
  { L' ', L"Write     Owner", &mode_i[ 2], AONOFF },
  { L' ', L"Exec/Srch Owner", &mode_i[ 3], AONOFF },
  { L' ', L"Read      Group", &mode_i[ 4], AONOFF },
  { L' ', L"Write     Group", &mode_i[ 5], AONOFF },
  { L' ', L"Exec/Srch Group", &mode_i[ 6], AONOFF },
  { L' ', L"Read      Other", &mode_i[ 7], AONOFF },
  { L' ', L"Write     Other", &mode_i[ 8], AONOFF },
  { L' ', L"Exec/Srch Other", &mode_i[ 9], AONOFF },
  { L' ', L"Set user  id",    &mode_i[10], AONOFF },
  { L' ', L"Set group id",    &mode_i[11], AONOFF },
  { L' ', L"Sticky  Bit",     &mode_i[12], AONOFF },
  {    0, L"---",             NULL,        NULL },
  };

  if ( !vfu_toggle_box( 50, 5, L"Change file Mode", mode_toggles ) ) return 0;

  if (mode_i[ 1] < 2) mod_str[1] = mode_i[ 1] == 0 ? 'r' : '-';
  if (mode_i[ 2] < 2) mod_str[2] = mode_i[ 2] == 0 ? 'w' : '-';
  if (mode_i[ 3] < 2) mod_str[3] = mode_i[ 3] == 0 ? 'x' : '-';
  if (mode_i[ 4] < 2) mod_str[4] = mode_i[ 4] == 0 ? 'r' : '-';
  if (mode_i[ 5] < 2) mod_str[5] = mode_i[ 5] == 0 ? 'w' : '-';
  if (mode_i[ 6] < 2) mod_str[6] = mode_i[ 6] == 0 ? 'x' : '-';
  if (mode_i[ 7] < 2) mod_str[7] = mode_i[ 7] == 0 ? 'r' : '-';
  if (mode_i[ 8] < 2) mod_str[8] = mode_i[ 8] == 0 ? 'w' : '-';
  if (mode_i[ 9] < 2) mod_str[9] = mode_i[ 9] == 0 ? 'x' : '-';

  if (mode_i[10] < 2) if (mode_i[10] == 0) mod_str[3] = (mode_i[ 3] == 0) ? 's' : 'S';
  if (mode_i[11] < 2) if (mode_i[11] == 0) mod_str[6] = (mode_i[ 6] == 0) ? 's' : 'S';
  if (mode_i[12] < 2) if (mode_i[12] == 0) mod_str[9] = (mode_i[ 9] == 0) ? 't' : 'T';

  return 1;
}

/*---------------------------------------------------------------------------*/

#ifdef _TARGET_GO32_
#include <dpmi.h>
#include <go32.h>

int file_get_sfn( const char *in, char *out )
{
  fname_t src;
  fname_t dst;

  strcpy( src, in );

  __dpmi_regs r;
  dosmemput(src, strlen (src)+1, __tb);
  r.x.ax = 0x7160;    /* Truename */
  r.x.cx = 1;     /* Get short name */
  r.x.ds = r.x.es = __tb / 16;
  r.x.si = r.x.di = __tb & 15;
  __dpmi_int(0x21, &r);
  if (r.x.flags & 1 || r.x.ax == 0x7100)
    {
    strcpy( out, in );
    return -1;
    }
  dosmemget (__tb, MAX_PATH, dst);
  strcpy( out, dst );
  return 0;
}

int file_get_lfn( const char *in, char *out )
{
  fname_t src;
  fname_t dst;

  strcpy( src, in );

  __dpmi_regs r;
  dosmemput(src, strlen (src)+1, __tb);
  r.x.ax = 0x7160;    /* Truename */
  r.x.cx = 2;     /* Get long name */
  r.x.ds = r.x.es = __tb / 16;
  r.x.si = r.x.di = __tb & 15;
  __dpmi_int(0x21, &r);
  if (r.x.flags & 1 || r.x.ax == 0x7100)
    {
    strcpy( out, in );
    return -1;
    }
  dosmemget (__tb, MAX_PATH, dst);
  strcpy( out, dst );
  return 0;
}

#endif /* _TARGET_GO32_ */

/*###########################################################################*/

/* eof vfusys.cpp */

