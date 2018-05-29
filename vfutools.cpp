/****************************************************************************
 *
 * (c) Vladi Belperchinov-Shabanski "Cade" 1996-2015
 * http://cade.datamax.bg/  <cade@biscom.net> <cade@bis.bg> <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 ****************************************************************************/

#include "vfumenu.h"
#include "vfucopy.h"
#include "vfuview.h"
#include "vfufiles.h"
#include "vfutools.h"

/*------------------------------------------------------------------------*/

void __get_classify_str( const char *fname, char ch, char *tmp )
{
  tmp[0] = 0;
  strcpy( tmp, str_file_path( fname ) );
  if (ch == 'N')
    {
    strcat( tmp, str_file_name( fname ) );
    if (strlen(fname) == strlen(tmp)) strcat( tmp, ".---" );
    }
  else if (ch == 'E')
    {
    strcat( tmp, str_file_ext( fname ) );
    if (strlen(tmp) == 0) strcat( tmp, "---" );
    }
  else
    {
    strcat( tmp, str_file_name( fname ) );
    str_sleft( tmp, ch - '0' );
    if (strlen(fname) == strlen(tmp)) strcat( tmp, ".---" );
    }
}

void vfu_tool_classify()
{
  /* FIXME: how this will handle files with path in the list? */
  char tmp[MAX_PATH];
  if ( sel_count == 0 )
    {
    say1( "Classify function works only on already selected files..." );
    return;
    }

  mb.undef();
  mb.push( "N Name"            );
  mb.push( "E Ext"             );
  mb.push( "1 First 1 letter"  );
  mb.push( "2 First 2 letters" );
  mb.push( "3 First 3 letters" );
  mb.push( "4 First 4 letters" );
  mb.push( "5 First 5 letters" );
  mb.push( "6 First 6 letters" );
  mb.push( "7 First 7 letters" );
  mb.push( "8 First 8 letters" );
  mb.push( "9 First 9 letter"  );
  if ( vfu_menu_box( 50, 5, "Classify files by") == -1 ) return;
  char ch = menu_box_info.ec;

  mb.undef();
  int z;
  int i;
  for ( z = 0; z < files_count; z++ )
    {
    TF *fi = files_list[z];
    if (   fi->is_dir() ) continue;
    if ( ! fi->sel      ) continue;
    __get_classify_str( fi->name(), ch, tmp );
    int found = 0;
    for ( i = 0; i < mb.count(); i++ )
      if ( pathcmp( tmp, mb[i] ) == 0 ) { found = 1; break; }
    if ( ! found ) mb.push( tmp );
    }
  for ( i = 0; i < mb.count(); i++ )
    {
    if( dir_exist( mb[i] ) ) continue;
    int res = make_path( mb[i] );
    if ( res )
      {
      char t[MAX_PATH];
      sprintf( t, "Cannot create directory: %s, (press a key for cancel)", mb.get(i) );
      say1( t );
      say2errno();
      con_getch();
      return;
      }
    }
  CopyInfo copy_info;
  copy_info.files_size  = sel_size;
  copy_info.files_count = sel_count;
  for ( z = 0; z < files_count; z++ )
    {
    TF *fi = files_list[z];
    if ( fi->is_dir()  ) continue;
    if ( fi->is_link() ) continue;
    if ( !fi->sel      ) continue;
    __get_classify_str( fi->name(), ch, tmp );
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

  if ( files_count < 1 )
    { say1( "No files to rename... (Empty directory)" ); return; };
  if ( sel_count < 1 )
    { say1( "No files to rename... (You have to select required files)" ); return; };

  mb.undef();
  mb.push( "1 README.TXT => readme.txt"  );
  mb.push( "2 README.TXT => readme.TXT"  );
  mb.push( "3 README.TXT => README.txt"  );
  mb.push( "4 readme.txt => README.TXT"  );
  mb.push( "5 readme.txt => README.txt"  );
  mb.push( "6 readme.txt => readme.TXT"  );
  mb.push( "_ Replace spaces with _"     );
  mb.push( "Y Simplify name (RTFM)"      );
  mb.push( "S Sequential rename"         );
  mb.push( "W Swap SymLink w.Original"   );
  mb.push( "R Replace S.Link w.Original" );
  if (vfu_menu_box( 50, 5, "Rename Tools" ) == -1) return;
  switch( menu_box_info.ec )
    {
    case 'S' : vfu_tool_seq_rename(); break;
    case 'R' : vfu_tool_replace_sym_org(0); break;
    case 'W' : vfu_tool_replace_sym_org(1); break;
    case '1' :
    case '2' :
    case '3' :
    case '4' :
    case '5' :
    case '6' :
    case '_' :
    case 'Y' :
               err = 0;
               for ( z = 0; z < files_count; z++ )
                 {
                 TF* fi = files_list[z];

                 // if ( fi->is_dir() ) continue; // why not? ;)
                 if ( !fi->sel ) continue;
                 path = str_file_path( fi->name() );
                 new_name = "";

                 t = str_file_name( fi->name() );
                 if (menu_box_info.ec == '1' || menu_box_info.ec == '2')
                   str_low( t );
                 if (menu_box_info.ec == '4' || menu_box_info.ec == '5')
                   str_up( t );
                 new_name += t;

                 t = str_file_ext( fi->name() );
                 if (menu_box_info.ec == '1' || menu_box_info.ec == '3')
                   str_low( t );
                 if (menu_box_info.ec == '4' || menu_box_info.ec == '6')
                   str_up( t );

                 if (strlen(t) > 0)
                   {
                   new_name += ".";
                   new_name += t;
                   }

                 if (menu_box_info.ec == '_')
                   str_tr( new_name, " ", "_" );

                 if (menu_box_info.ec == 'Y')
                   {
                   str_replace( new_name, "%20", "_" );
                   str_tr( new_name, " `'&\"\\/,()!", "___________" );
                   str_tr( new_name, "âáëéèêàíîïùúóô", "aaeeeeaiiiuuoo" );
                   str_squeeze( new_name, "_" );
                   str_replace( new_name, "_-_", "-" );
                   }

                 new_name = path + new_name;

                 if ( !file_exist( new_name) )
                   {
                   if (rename( fi->name(), new_name ) == 0) /* FIXME: full name ? */
                     {
                     fi->set_name( new_name );
                     do_draw = 2; /* FIXME: this should be optimized? */
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
  for ( z = 0; z < files_count; z++ )
    {
    TF* fi = files_list[z];

    if ( fi->is_dir() ) continue;
    if ( !fi->sel ) continue;

    sprintf( new_name, fmt, prefix.data(), start, suffix.data() );

    t = str_file_path( fi->name() ); /* FIXME: full name? */
    new_name = t + new_name;

    if (access( new_name, F_OK ) == 0) { err++; continue; }
    if (rename( fi->name(), new_name )) { err++; continue; }
    fi->set_name( new_name );
    do_draw = 2; /* FIXME: this should be optimized? */
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
  for ( z = 0; z < files_count; z++ )
    {
    TF* fi = files_list[z];

    if ( fi->is_dir() ) continue; // FIXME: dali?
    if ( !fi->sel ) continue;
    if ( !fi->is_link() ) continue;

    VString sym = fi->full_name();
    VString org = vfu_readlink( sym );

    if (access( org, F_OK )) { err++; continue; }
    if (unlink( sym ))       { err++; continue; }
    if (rename( org, sym ))  { err++; continue; }
    if (swap)
      {
      if (symlink( sym, org ));
      }
    fi->update_stat();
    do_draw = 2; /* FIXME: this should be optimized? */
    }
  char t[256];
  sprintf( t, "Replace complete (errors: %d)", err );
  say1( t );
}

/*------------------------------------------------------------------------*/


/* eof vfutools.cpp */
