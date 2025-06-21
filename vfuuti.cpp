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
#include "vfuuti.h"
#include "vfufiles.h"
#include "vfumenu.h"
#include "vfusys.h"
#include "vfudir.h"
#include "vfuopt.h"
#include "vfuview.h"

/*---------------------------------------------------------------------------*/

VString vfu_readlink( const char* fname )
{
  fname_t t;
  t[0] = 0;
  int l = readlink( fname, t, MAX_PATH - 1 );
  if ( l != -1 ) t[l] = 0;
  VString res = t;
  return res;
}

/*---------------------------------------------------------------------------*/

int vfu_update_shell_line( VString &a_line, VString &a_options )
{
VString out;
VString s;

int i = 0;
int full = 0;

// place-holders
while( a_line[i] )
  {
  if ( a_line[i] == '%' )
    {
    char ch = a_line[i+1];
    if( ! ch ) break;
    
    if( ch == 'h' || ch == 'H' ) // auto-sensing F or G depending on selection count
      ch = sel_count > 0 ? ch == 'H' ? 'G' : 'g' : ch == 'H' ? 'F' : 'f';
    
    switch( ch )
      {
      case 'r' : /* rescan files after */
      case 'R' : a_options += "r"; 
                 break; 

      case 'f' : /* file name */
      case 'F' : full = ch == 'F';
                 if( work_mode == WM_ARCHIVE )
                   {
                   s = files_list_get(FLI)->name();
                   }
                 else if( full )
                   {
                   s = files_list_get(FLI)->full_name();
                   }
                 else
                   {
                   s = files_list_get(FLI)->name();
                   }
                 out += shell_escape( s );
                 break;

      case 'g' : /* list selected filenames */
      case 'G' : full = ch == 'G';
                 int z;
                 for( z = 0; z < files_list_count(); z++ )
                   {
                   TF *fi = files_list_get(z);
                   if ( ! fi->sel ) continue; /* if not one and not selected -- skip */
                   if( work_mode == WM_ARCHIVE )
                     {
                     s = files_list_get(z)->name();
                     }
                   else if( full )
                     {
                     s = files_list_get(z)->full_name();
                     }
                   else
                     {
                     s = files_list_get(z)->name();
                     }
                   out += shell_escape( s ) + " ";
                   }
                 break;

      case 'e' : /* name only */
      case 'E' : /* extension only */
                 if ( a_line[i+1] == 'e' )
                   s = str_file_name( files_list_get(FLI)->name() );
                 else
                   s = files_list_get(FLI)->ext();
                 out += shell_escape( s );
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
                 out += shell_escape( s );
                 break;

      case 'C' : /* startup dir */
                 s = startup_path;
                 out += shell_escape( s );
                 break;

      case 'a' : /* Archive name */
                 s = archive_name;
                 out += shell_escape( s );
                 break;

      case 'A' : /* Archive path */
                 s = archive_path;
                 out += shell_escape( s );
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
  }  

  a_line = out;
  return 0;
}

/*---------------------------------------------------------------------------*/

int vfu_break_op()
{
  if (! con_kbhit() ) return 0;
  int key = con_getwch();
  if ( key == UKEY_CTRL_C ) return 1;
  if (key == 27)
    {
    say2( "Press ENTER to cancel or other key to continue..." );
    key = con_getwch();
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

wchar_t vfu_ask( const wchar_t *prompt, const wchar_t *allowed )
{
  wchar_t wch = 0;
  say1( VString( prompt ) );
  while(4)
    {
    wch = con_getwch();
    if( wch == 27 ) return wch;
    if( str_find( allowed, wch ) >= 0 ) return wch;
    }
  return 0;
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
  tm     tim_tm;
  localtime_r( &tim, &tim_tm );
  const char* tfm;
  long int tdiff = timenow - tim;
  if ( tdiff > 6L * 30L * 24L * 60L * 60L /* older than 6 months */
       ||
       tdiff < - 23L * 60L * 60L) /* in the future, past next 23 hours */
    {
    tfm = "%b %d  %Y";
    }
  else if( abs( tdiff ) < 23L * 60L * 60L )
    {
    /* in the near 23 hours */
    tfm = "%a %H:%M:%S";
    }
  else
    {
    tfm = "%b %d %H:%M";
    }  
  strftime( buf, 16, tfm, &tim_tm );
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
    size_str = "   B";
    }
  else if ( siz < 1.0 * units_size * units_size )
    {
    sprintf( buf, "%.0f", siz/units_size );
    size_str = opt.use_si_sizes ? " KB " : " KiB";
    }
  else if ( siz < 1.0 * units_size * units_size * units_size )
    {
    sprintf( buf, "%.0f", siz/( units_size * units_size ) );
    size_str = opt.use_si_sizes ? " MB " : " MiB";
    }
  else if ( siz < 100.0 * units_size * units_size * units_size )
    {
    sprintf( buf, "%.3f", siz/( units_size * units_size * units_size ) );
    size_str = opt.use_si_sizes ? " GB " : " GiB";
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

int vfu_hist_menu( int x, int y, const wchar_t* title, int hist_id )
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
    mb.push( WString( str ) );
    }
  return vfu_menu_box( x, y, title );
}

/*---------------------------------------------------------------------------*/

int __vfu_get_str_hist_id; /* used to keep history id passed here... */
void vfu_get_str_history( int key, WString &w, int &pos )
{
  if ( __vfu_get_str_hist_id <= 0 ) return;
  if ( key != UKEY_PGUP && key != UKEY_PGDN ) return;
  con_chide();

  int z = vfu_hist_menu( 5, 5, L"Line History", __vfu_get_str_hist_id );

  con_cshow();
  if ( z == -1 ) return;
  w = WString( mb.get(z) + 2 );
  str_cut_spc( w );
  pos = str_len( w );
}

int vfu_get_str( const char *prompt, VString& target, int hist_id, int x, int y )
{
  if ( x == -1 ) x = 1;
  if ( y == -1 ) y = con_max_y();
  int len = con_max_x() - x;

  /* FIXME: this is not correct if x and y are specified */
  if ( prompt && prompt[0] )
    say1( prompt );
  say2( "" );

  __vfu_get_str_hist_id = hist_id;
  if ( strcmp( target, "" ) == 0 && vfu_hist_get( hist_id, 0 ) )
    target = vfu_hist_get( hist_id, 0 );
  
  WString www = target;
  int r = TextInput( x, y, "", 1024, len, www, vfu_get_str_history );
  target = www;

  say1( "" );
  say2( "" );
  __vfu_get_str_hist_id = 0;
  if( r )
    vfu_hist_add( hist_id, target );
  return ( r != 0 );
}

/*---------------------------------------------------------------------------*/

fname_t vfu_temp_filename;
const char* vfu_temp()
{
    strcpy( vfu_temp_filename, tmp_path + "vfu.XXXXXX" );
    int fd = mkstemp( vfu_temp_filename );
    if( fd >= 0 ) 
      close( fd );
    else
      vfu_temp_filename[0] = 0;  
    return vfu_temp_filename;
}

fname_t vfu_temp_dirname;
const char* vfu_temp_dir()
{
    strcpy( vfu_temp_dirname, tmp_path + "vfu.XXXXXX" );  
    char* r = mkdtemp( vfu_temp_dirname );
    if( ! r ) vfu_temp_dirname[0] = 0;
    return vfu_temp_dirname;
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

VString vfu_str_comma( fsize_t size )
{
  VString str;
  str.fi( size );
  return str_comma( str, COMMA_TYPES[opt.comma_type][0] );
}

/*###########################################################################*/

#define WCHAR_CTRL_LIST L"\u0001\u0002\u0003\u0004\u0005\u0006\u0007\u0008\u0009\u000A\u000B\u000C\u000D\u000E\u000F\u0010\u0011\u0012\u0013\u0014\u0015\u0016\u0017\u0018\u0019\u001A\u001B\u001C\u001D\u001E\u001F"
#define WCHAR_PRNT_LIST L"\u2401\u2402\u2403\u2404\u2405\u2406\u2407\u2408\u2409\u240A\u240B\u240C\u240D\u240E\u240F\u2410\u2411\u2412\u2413\u2414\u2415\u2416\u2417\u2418\u2419\u241A\u241B\u241C\u241D\u241E\u241F"

WString __vfu_translate_controls( const wchar_t *s )
{
  WString str = s;
  // 00..1f --> 2400..241f, replace non-printable chars
  str_tr( str, WCHAR_CTRL_LIST, WCHAR_PRNT_LIST );
  return str;
}

void vfu_con_out( int x, int y, const char    *s )
{
  con_out( x, y, VString( __vfu_translate_controls( WString( s ) ) ) );
}

void vfu_con_out( int x, int y, const char    *s, int attr )
{
  con_out( x, y, VString( __vfu_translate_controls( WString( s ) ) ), attr );
}

void vfu_con_out( int x, int y, const wchar_t *s )
{
  con_out( x, y, VString( __vfu_translate_controls( WString( s ) ) ) );
}

void vfu_con_out( int x, int y, const wchar_t *s, int attr )
{
  con_out( x, y, VString( __vfu_translate_controls( WString( s ) ) ), attr );
}

void vfu_set_title_info( const char* info )
{
  if( ! opt.set_term_title_info      ) return;
  if(   opt.set_term_title_info == 1 ) printf( "\033]0;%s@%s: VFU: %s\007", user_id_str.data(), host_name_str.data(), info );
  if(   opt.set_term_title_info == 2 ) printf( "\033]0;VFU: %s\007", info );
  fflush( stdout );
}

void vfu_set_title_info( const VString info )
{
  return vfu_set_title_info( info.data() );
}

/*###########################################################################*/

/* eof vfuuti.cpp */
