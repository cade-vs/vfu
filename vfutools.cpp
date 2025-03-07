/****************************************************************************
 #
 # Copyright (c) 1996-2023 Vladi Belperchinov-Shabanski "Cade" 
 # https://cade.noxrun.com/  <cade@noxrun.com> <cade@bis.bg>
 # https://cade.noxrun.com/projects/vfu     https://github.com/cade-vs/vfu
 #
 # SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 #
 ****************************************************************************/

#include "vfumenu.h"
#include "vfucopy.h"
#include "vfuview.h"
#include "vfufiles.h"
#include "vfutools.h"

/*------------------------------------------------------------------------*/

void __get_classify_str( const char *fname, wchar_t wch, char *tmp )
{
  tmp[0] = 0;
  strcpy( tmp, str_file_path( fname ) );
  if ( wch == L'N')
    {
    strcat( tmp, str_file_name( fname ) );
    if (strlen(fname) == strlen(tmp)) strcat( tmp, ".---" );
    }
  else if (wch == L'E')
    {
    strcat( tmp, str_file_ext( fname ) );
    if (strlen(tmp) == 0) strcat( tmp, "---" );
    }
  else
    {
    strcat( tmp, str_file_name( fname ) );
    str_sleft( tmp, wch - L'0' );
    if (strlen(fname) == strlen(tmp)) strcat( tmp, ".---" );
    }
}

void vfu_tool_classify()
{
  fname_t tmp;
  if ( sel_count == 0 )
    {
    say1( "Classify function works only on already selected files..." );
    return;
    }

  mb.undef();
  mb.push( L"N Name           ");
  mb.push( L"E Ext"             );
  mb.push( L"1 First 1 letter "  );
  mb.push( L"2 First 2 letters" );
  mb.push( L"3 First 3 letters" );
  mb.push( L"4 First 4 letters" );
  mb.push( L"5 First 5 letters" );
  mb.push( L"6 First 6 letters" );
  mb.push( L"7 First 7 letters" );
  mb.push( L"8 First 8 letters" );
  mb.push( L"9 First 9 letter"  );
  if ( vfu_menu_box( 32, 6, L"Classify files by") == -1 ) return;
  wchar_t wch = menu_box_info.ec;

  VArray fa; // files array
  int z;
  int i;
  for ( z = 0; z < files_list_count(); z++ )
    {
    TF *fi = files_list_get(z);
    if (   fi->is_dir() ) continue;
    if ( ! fi->sel      ) continue;
    __get_classify_str( fi->name(), wch, tmp );
    int found = 0;
    for ( i = 0; i < fa.count(); i++ )
      if ( pathcmp( tmp, fa[i] ) == 0 ) { found = 1; break; }
    if ( ! found ) fa.push( tmp );
    }
  for ( i = 0; i < fa.count(); i++ )
    {
    if( dir_exist( fa[i] ) ) continue;
    int res = make_path( fa[i] );
    if ( res )
      {
      fname_t t;
      sprintf( t, "Cannot create directory: %s, (press a key for cancel)", fa.get(i) );
      say1( t );
      say2errno();
      con_getwch();
      return;
      }
    }
  CopyInfo copy_info;
  copy_info.files_size  = sel_size;
  copy_info.files_count = sel_count;
  for ( z = 0; z < files_list_count(); z++ )
    {
    TF *fi = files_list_get(z);
    if ( fi->is_dir()  ) continue;
    if ( fi->is_link() ) continue;
    if ( !fi->sel      ) continue;
    __get_classify_str( fi->name(), wch, tmp );
    strcat( tmp, "/" );
    ASSERT( dir_exist( tmp ) );
    strcat( tmp, fi->name_ext());
    if ( __vfu_file_move( fi->name(), tmp, &copy_info ) == 255 ) break;
    }
  vfu_read_files( 0 );
}

/*------------------------------------------------------------------------*/

void vfu_tool_rename()
{
  int z;
  int err;
  VString path;
  VString new_name;
  VString t;

  if ( files_list_count() < 1 )
    { say1( "No files to rename... (Empty directory)" ); return; };
  if ( sel_count < 1 )
    { say1( "No files to rename... (You have to select required files)" ); return; };

  mb.undef();
  mb.push( L"1 README.TXT => readme.txt"    );
  mb.push( L"2 README.TXT => readme.TXT"    );
  mb.push( L"3 README.TXT => README.txt"    );
  mb.push( L"4 readme.txt => README.TXT"    );
  mb.push( L"5 readme.txt => README.txt"    );
  mb.push( L"6 readme.txt => readme.TXT"    );
  mb.push( L"_ Replace spaces with _"       );
  mb.push( L"Y Simplify name (RTFM)"        );
  mb.push( L"S Sequential rename"           );
  mb.push( L"T Prefix with current date+time" );
  mb.push( L"D Prefix with current date"      );
  mb.push( L"I Add suffix with current date+time" );
  mb.push( L"E Add suffix with current date"      );
  mb.push( L"W Swap SymLink with Original"     );
  mb.push( L"R Replace SymLink with Original"   );
  if (vfu_menu_box( 32, 6, L"Rename Tools" ) == -1) return;
  switch( menu_box_info.ec )
    {
    case L'S' : vfu_tool_seq_rename(); break;
    case L'R' : vfu_tool_replace_sym_org(0); break;
    case L'W' : vfu_tool_replace_sym_org(1); break;
    case L'1' :
    case L'2' :
    case L'3' :
    case L'4' :
    case L'5' :
    case L'6' :
    case L'_' :
    case L'Y' :
               err = 0;
               for ( z = 0; z < files_list_count(); z++ )
                 {
                 TF* fi = files_list_get(z);

                 // if ( fi->is_dir() ) continue; // why not? ;)
                 if ( !fi->sel ) continue;
                 path = str_file_path( fi->name() );
                 new_name = "";

                 t = str_file_name( fi->name() );
                 if (menu_box_info.ec == L'1' || menu_box_info.ec == L'2')
                   str_low( t );
                 if (menu_box_info.ec == L'4' || menu_box_info.ec == L'5')
                   str_up( t );
                 new_name += t;

                 t = str_file_ext( fi->name() );
                 if (menu_box_info.ec == L'1' || menu_box_info.ec == L'3')
                   str_low( t );
                 if (menu_box_info.ec == L'4' || menu_box_info.ec == L'6')
                   str_up( t );

                 if (strlen(t) > 0)
                   {
                   new_name += ".";
                   new_name += t;
                   }

                 if (menu_box_info.ec == L'_')
                   str_tr( new_name, " ", "_" );

                 if (menu_box_info.ec == L'Y')
                   {
                   str_replace( new_name, "%20", "_" );
                   str_tr( new_name, " `'&\"\\/,()!", "___________" );
                   str_tr( new_name, "âáëéèêàíîïùúóô", "aaeeeeaiiiuuoo" );
                   str_squeeze( new_name, "_" );
                   str_replace( new_name, "_-_", "-" );
                   if( new_name[0] == '-' ) new_name[0] = '_';
                   }

                 new_name = path + new_name;

                 if ( !file_exist( new_name) )
                   {
                   if (rename( fi->name(), new_name ) == 0)
                     {
                     fi->set_name( new_name );
                     do_draw = 2;
                     }
                   else
                     err++;
                   }
                 else
                   err++;
                 }
               sprintf( t, "Rename complete (errors: %d)", err );
               say1( t );
               break;

    case L'T' :
    case L'D' :
    case L'I' :
    case L'E' :
               int ps_time = menu_box_info.ec == L'T' || menu_box_info.ec == L'I';
               int ps_suff = menu_box_info.ec == L'I' || menu_box_info.ec == L'E';
               char   time_str[32];
               time_t timenow = time( NULL );
               tm     tmnow;
               localtime_r( &timenow, &tmnow );
               if( strftime( time_str, sizeof( time_str ) - 1, ps_time ? "%Y%m%d_%H%M%S" : "%Y%m%d", &tmnow ) <= 0 )
                 {
                 t = "Rename error! Cannot constrict date/time string: ";
                 t += strerror( errno );
                 say1( t );
                 break;
                 }

               err = 0;
               for ( z = 0; z < files_list_count(); z++ )
                 {
                 TF* fi = files_list_get(z);

                 if ( !fi->sel ) continue;
                 path = str_file_path( fi->name() );
                 
                 if( ps_suff )
                   new_name = path + str_file_name( fi->name() ) + "." + time_str + "." + str_file_ext( fi->name() );
                 else  
                   new_name = path + time_str + "_" + str_file_name_ext( fi->name() );

                 if ( ! file_exist( new_name) )
                   {
                   if ( rename( fi->name(), new_name ) == 0)
                     {
                     fi->set_name( new_name );
                     do_draw = 2;
                     }
                   else
                     err++;
                   }
                 else
                   err++;
                 }
               sprintf( t, "Rename complete (errors: %d)", err );
               say1( t );
               break;

    }
}

/*------------------------------------------------------------------------*/

void vfu_tool_seq_rename()
{
  VString prefix;
  VString suffix;
  VString s_digpos;
  VString s_start;

  if(!vfu_get_str( "Enter filename prefix: ", prefix, HID_SEQ_PREFIX )) return;
  if(!vfu_get_str( "Enter filename suffix: ", suffix, HID_SEQ_SUFFIX )) return;
  if(!vfu_get_str( "Enter digit places: (digits only) ", s_digpos, HID_SEQ_DIGPOS )) return;
  if(!vfu_get_str( "Enter start number: (digits only) ", s_start, HID_SEQ_START )) return;

  int digpos = atoi( s_digpos );
  int start = atoi( s_start );

  if (digpos < 1 || digpos > 20) digpos = 1;
  if (start < 0) start = 0;

  VString new_name;
  VString t;
  VString fmt;

  sprintf( fmt, "%%s%%0%dd%%s", digpos );

  int err = 0;
  int z;
  for ( z = 0; z < files_list_count(); z++ )
    {
    TF* fi = files_list_get(z);

    if ( fi->is_dir() ) continue;
    if ( !fi->sel ) continue;

    sprintf( new_name, fmt, prefix.data(), start, suffix.data() );

    t = str_file_path( fi->name() ); /* FIXME: full name? */
    new_name = t + new_name;

    if (access( new_name, F_OK ) == 0) { err++; continue; }
    if (rename( fi->name(), new_name )) { err++; continue; }
    fi->set_name( new_name );
    do_draw = 2;
    start++;

    }
  sprintf( t, "Rename complete (errors: %d)", err );
  say1( t );
}

/*------------------------------------------------------------------------*/

// replaces symlink entry with original one:
// rm symlink
// mv original symlink

void vfu_tool_replace_sym_org( int swap )
{
  int err = 0;
  int z;
  for ( z = 0; z < files_list_count(); z++ )
    {
    TF* fi = files_list_get(z);

    if ( fi->is_dir()   ) continue;
    if ( !fi->sel       ) continue;
    if ( !fi->is_link() ) continue;

    VString sym = fi->full_name();
    VString org = vfu_readlink( sym );

    if (access( org, F_OK )) { err++; continue; }
    if (unlink( sym ))       { err++; continue; } // FIXME: TODO: correct?
    if (rename( org, sym ))  { err++; continue; }
    if (swap)
      {
      if(symlink( sym, org ))
        {
        say1( "error: cannot symlink " + sym + " -> " + org );
        say2errno();
        return;
        }
      }
    fi->update_stat();
    do_draw = 2;
    }
  char t[256];
  sprintf( t, "Replace complete (errors: %d)", err );
  say1( t );
}

/*------------------------------------------------------------------------*/


/* eof vfutools.cpp */
