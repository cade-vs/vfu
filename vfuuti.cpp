/****************************************************************************
 *
 * (c) Vladi Belperchinov-Shabanski "Cade" 1996-2018
 * http://cade.datamax.bg/  <cade@biscom.net> <cade@bis.bg> <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 ****************************************************************************/

#include "vfu.h"
#include "vfuuti.h"
#include "vfufiles.h"
#include "vfumenu.h"
#include "vfusys.h"
#include "vfudir.h"
#include "vfuopt.h"
#include "vfuview.h"

/*---------------------------------------------------------------------------*/

fsize_t file_st_size( struct stat* st )
{
  return st->st_size + ( st->st_size < 0 ) * ((uintmax_t)TYPE_MAXIMUM(off_t) - TYPE_MINIMUM(off_t) + 1);
}

VString vfu_readlink( const char* fname )
{
  char t[MAX_PATH+1];
  t[0] = 0;
  int l = readlink( fname, t, MAX_PATH );
  if (l != -1) t[l] = 0;
  VString res = t;
  return res;
}

/*---------------------------------------------------------------------------*/

int vfu_update_shell_line( VString &a_line, VString &a_options )
{
VString out;
VString s;

int i = 0;
int one = 0;
int full = 0;

// toggles
#ifdef _TARGET_GO32_
  int use_SFN = 0;
#endif

while( a_line[i] )
  if ( a_line[i] == '%' )
    {
    switch( a_line[i+1] )
      {
      #ifdef _TARGET_GO32_
      case '_' : use_SFN = 1; break;
      #endif

      case 'r' : /* rescan files after */
      case 'R' : a_options += "r"; break; // refresh all files after (readdir)
      case 'f' : /* file name */
      case 'F' :
      case 'g' :
      case 'G' : one = 0;
                 if ( a_line[i+1] == 'f' ) one = 1;
                 if ( a_line[i+1] == 'F' ) one = 1;
                 full = 0;
                 if ( a_line[i+1] == 'F' ) full = 1;
                 if ( a_line[i+1] == 'G' ) full = 1;
                 // FIXME: 'one' is used to merge F and G options in the future
                 int z;
                 for( z = 0; z < files_list_count(); z++ )
                   {
                   TF *fi = files_list_get(z);
                   if (  one && z != FLI ) continue; /* if one and not current -- skip */
                   if ( !one && !fi->sel ) continue; /* if not one and not selected -- skip */
                   if( work_mode == WM_ARCHIVE )
                     {
                     s = files_list_get(z)->name();
                     }
                   else if( full )
                     {
                     files_list_get(z)->full_name();
                     }
                   else
                     {
                     s = files_list_get(z)->name();
                     }
                   if( !one )
                     {
                     // need to list files, quote them...
                     // FIXME: this should be optional for all one and !one
                     if ( str_find( s, "'" ) == -1 )
                       {
                       s = "'" + s +"' ";
                       }
                     else if ( str_find( s, "\"" ) == -1 )
                       {
                       s = "\"" + s +"\" ";
                       }
                     else
                       {
                       s = "";
                       }
                     }
                   out += s;
                   }
                 break;

      case 'e' : /* name only */
      case 'E' : /* extension only */
                 if ( a_line[i+1] == 'e' )
                   s = str_file_name( s );
                 else
                   s = files_list_get(FLI)->ext();
                 out += s;
                 break;
      case 's' : /* current file size */
                 sprintf( s, "%.0f", files_list_get(FLI)->size() );
                 out += s;
                 break;
      case '?' : /* prompt user for argument */
                 if (vfu_get_str( "Enter parameter:", s, HID_SHELL_PAR ))
                   out += s;
                 else
                   return 3;
                 break;
      case 'd' : /* prompt user for directory */
                 s = "";
                 if (vfu_get_dir_name( "Enter directory:", s, 0 ))
                   out += s;
                 else
                   return 3;
                 break;
      case 'c' : /* current path */
                 s = work_path;
                 #ifdef _TARGET_GO32_
                  str_tr( s, "/", "\\" );
                 #endif
                 out += s;
                 break;

      case 'C' : /* startup dir */
                 s = startup_path;
                 #ifdef _TARGET_GO32_
                  str_tr( s, "/", "\\" );
                 #endif
                 out += s;
                 break;

      case 'a' : /* Archive name */
                 out += archive_name;
                 break;
      case 'A' : /* Archive path */
                 s = archive_path;
                 #ifdef _TARGET_GO32_
                  str_tr( s, "/", "\\" );
                 #endif
                 out += s;
                 break;

      case 'w' :
      case 'W' : a_options += "w"; break;
      case 'i' : a_options += "i"; break;
      case 'n' : a_options += "n"; break;
      case '!' : a_options += "!"; break;
      default  : /* chars not recognized are accepted "as is" */
                 str_add_ch( out, a_line[i+1] );
                 break;
      }
    i += 2;
    }
  else
    {
    str_add_ch( out, a_line[i] );
    i++;
    }

  a_line = out;
  return 0;
}

/*---------------------------------------------------------------------------*/

int vfu_break_op()
{
  if (con_kbhit())
    if (con_getch() == 27)
      {
      say2( "Press ENTER to cancel or other key to continue..." );
      int key = con_getch();
      say2( "" );
      if ( key == 13 )
        return 1;
      }
  return 0;
}

/*---------------------------------------------------------------------------*/

fsize_t vfu_update_sel_size( int one ) // used before copy/move to calc estimated size
{
  fsize_t size = 0;
  int z;
  int need_size_cache_sort = 0;
  for( z = 0; z < files_list_count(); z++ )
    {
    TF *fi = files_list_get(z);

    if ( one && z != FLI ) continue; /* if one and not current -- skip */
    if ( !one && !fi->sel ) continue; /* if not one and not selected -- skip */
    if ( fi->is_link() ) continue; /* links does not have own size -- skip */

    if ( fi->is_dir() )
      { /* this is directory */
      fsize_t dir_size = vfu_dir_size( fi->name(), 0 );
      need_size_cache_sort = 1;
      if ( dir_size == -1 )
        { /* dir size calculation has been canceled */
        size = -1;
        break;
        }
      fi->set_size( dir_size );
      size += dir_size;
      }
    else
      { /* this is regular file */
      size += fi->size();
      }
    } /* for */
  if( need_size_cache_sort ) size_cache_sort();
  update_status(); /*  */
  vfu_redraw(); /* just to redraw before copy/move/etc... */
  vfu_redraw_status(); /* just to redraw before copy/move/etc... */
  return size;
}

/*---------------------------------------------------------------------------*/

int vfu_ask( const char *prompt, const char *allowed )
{
  int ch = 1;
  say1( prompt );
  while ( strchr( allowed, ch ) == NULL ) ch = con_getch();
  return ch;
}

/*---------------------------------------------------------------------------*/

VString& vfu_expand_mask( VString& mask )
{
  if ( str_count( mask, "*?" ) > 0 ) return mask;
  mask += "*";
  if ( mask[0] == '.' ) str_ins( mask, 0, "*" );
  str_replace( mask, "**", "*" );
  return mask;
}

/*---------------------------------------------------------------------------*/

char* time_str_compact( const time_t tim, char* buf )
{
  ASSERT( buf );
  time_t timenow = time( NULL );
  strcpy(buf, ctime(&tim));
  if (timenow > tim + 6L * 30L * 24L * 60L * 60L /* old */
      ||
      timenow < tim - 60L * 60L) /* in the future */
      strcpy (buf + 11, buf + 19);
  buf[16] = 0;
  strcpy(buf, buf+4);
  if (buf[4] == ' ') buf[4] = '0';
  return buf;
}

/*---------------------------------------------------------------------------*/

VString size_str_compact( const fsize_t siz )
{
  char buf[32];
  const char* size_str;
  int units_size = opt.use_si_sizes ? 1000 : 1024;
  
  if ( siz < units_size )
    {
    sprintf( buf, "%.0f", siz );
    size_str = " B  ";
    }
  else if ( siz < units_size * units_size )
    {
    sprintf( buf, "%.0f", siz/units_size );
    size_str = opt.use_si_sizes ? " KB " : " KiB";
    }
  else if ( siz < units_size * units_size * units_size )
    {
    sprintf( buf, "%.0f", siz/( units_size * units_size ) );
    size_str = opt.use_si_sizes ? " MB " : " MiB";
    }
  else
    {
    sprintf( buf, "%.0f", siz/( units_size * units_size * units_size ) );
    size_str = opt.use_si_sizes ? " GB " : " GiB";
    }
  vfu_str_comma( buf );  
  strcat( buf, size_str );
  return VString( buf );
}

/*---------------------------------------------------------------------------*/

void vfu_beep()
{
  if ( opt.allow_beep ) { con_beep(); }
}

/*###########################################################################*/

static char hist_menu_hotkeys[] = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";
#define MAXHIST         128      // max history items per id
#define HISTIDPAD       8

void vfu_hist_add( int hist_id, const char* str )
{
  VString hstr = hist_id;
  str_pad( hstr, HISTIDPAD );
  hstr += ",";
  hstr += str;
  int z;
  z = vfu_hist_index( hist_id, str );
  if ( z != -1 ) vfu_hist_remove( hist_id, z );
  z = vfu_hist_count( hist_id );
  while (z >= MAXHIST)
    {
    z--;
    vfu_hist_remove( hist_id, z );
    }
  if (z) z++;
  history.ins( 0, hstr );
}

const char* vfu_hist_get( int hist_id, int index )
{
  VString hstr = hist_id;
  str_pad( hstr, HISTIDPAD );
  hstr += ",";
  int i = 0;
  int z;
  for ( z = 0; z < history.count(); z++ )
    if ( strncmp( hstr, history[z], HISTIDPAD+1 ) == 0 )
      {
      if ( index == -1 || index == i )
        return history.get( z ) + HISTIDPAD+1;
      i++;
      }
  return NULL;
}

char* vfu_hist_get( int hist_id, int index, char* str )
{
  str[0] = 0;
  const char* pstr = vfu_hist_get( hist_id, index );
  if ( pstr ) strcpy( str, pstr );
  return str;
}

int vfu_hist_index( int hist_id, const char* value )
{
  int z;
  int cnt = vfu_hist_count( hist_id );
  for ( z = 0; z < cnt; z++ )
    if ( strcmp( value, vfu_hist_get( hist_id, z ) ) == 0 )
      return z;
  return -1;
}

int vfu_hist_count( int hist_id )
{
  VString hstr = hist_id;
  str_pad( hstr, HISTIDPAD );
  hstr += ",";
  int cnt = 0;
  int z;
  for ( z = 0; z < history.count(); z++ )
    cnt += ( strncmp( hstr, history[z], HISTIDPAD+1 ) == 0 );
  return cnt;
}

// use hist_id=-1 and/or index=-1 to remove all
void vfu_hist_remove( int hist_id, int index )
{
  VString hstr = hist_id;
  str_pad( hstr, HISTIDPAD );
  hstr += ",";
  int i = 0;
  int z = 0;
  while( z < history.count() )
    {
    if ( hist_id != -1 && strncmp( hstr, history[z], HISTIDPAD+1 ) != 0 ) { z++; continue; }
    if ( index != -1 && index != i ) { z++; i++; continue; }
    history.del( z );
    if ( index != -1 ) break;
    }
}

int vfu_hist_menu( int x, int y, const char* title, int hist_id )
{
  VString str;

  mb.undef();
  int z;
  int cnt = vfu_hist_count( hist_id );
  if ( cnt < 1 ) return -1;
  if ( cnt > con_max_y() - 9 ) cnt = con_max_y() - 9; // limit to visible space
  for ( z = 0; z < cnt; z++ )
    {
//    ASSERT( z < str_len( hist_menu_hotkeys ) );
    str = "";
    str_add_ch( str, z < str_len( hist_menu_hotkeys ) ? hist_menu_hotkeys[z] : ' ' );
    str = str + " " + vfu_hist_get( hist_id, z );
    mb.push( str );
    }
  return vfu_menu_box( x, y, title );
}

/*---------------------------------------------------------------------------*/

int __vfu_get_str_hist_id; /* used to keep history id passed here... */
void vfu_get_str_history( int key, VString &s, int &pos )
{
  if ( __vfu_get_str_hist_id <= 0 ) return;
  if ( key != KEY_NPAGE && key != KEY_PPAGE ) return;
  con_chide();

  int z = vfu_hist_menu( 5, 5, "Line History", __vfu_get_str_hist_id );

  con_cshow();
  if ( z == -1 ) return;
  s = mb.get(z) + 2;
  str_cut_spc( s );
  pos = str_len( s );
}

int vfu_get_str( const char *prompt, VString& target, int hist_id, int x, int y )
{
  if ( x == -1 ) x = 1;
  if ( y == -1 ) y = con_max_y();
  int len = con_max_x() - 3 - x;

  /* FIXME: this is not correct if x and y are specified */
  if ( prompt && prompt[0] )
    say1( prompt );
  say2( "" );

  __vfu_get_str_hist_id = hist_id;
  if ( strcmp( target, "" ) == 0 && vfu_hist_get( hist_id, 0 ) )
    target = vfu_hist_get( hist_id, 0 );
  char t[1024]; /* FIXME: can be overflowed? */
  strcpy( t, target );
  int r = TextInput( x, y, "", len, len, t, vfu_get_str_history );
  target = t;
  say1( "" );
  say2( "" );
  __vfu_get_str_hist_id = 0;
  if( r )
    vfu_hist_add( hist_id, target );
  return ( r != 0 );
}

/*---------------------------------------------------------------------------*/

/* to fool gcc warning that mktemp() is not safe, I don't really care :/
   still there is no mkstemp for directories :)) */
char vfu_temp_filename[MAX_PATH];
const char* vfu_temp()
{
    strcpy( vfu_temp_filename, tmp_path + "vfu.XXXXXX" );
    if (mkstemp( vfu_temp_filename ));
    unlink( vfu_temp_filename );
    return vfu_temp_filename;
}

/*---------------------------------------------------------------------------*/

char* vfu_str_comma( char* target )
{
  return str_comma( target, COMMA_TYPES[opt.comma_type][0] );
}

VString& vfu_str_comma( VString& target )
{
  return str_comma( target, COMMA_TYPES[opt.comma_type][0] );
}

/*###########################################################################*/

/* eof vfuuti.cpp */
