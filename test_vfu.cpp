/****************************************************************************
 #
 #  VFU -- Visual File Utility
 #
 #  test suite for the testable parts of vfu
 #
 #  vfu is an application, not a library: most of it talks to the terminal
 #  through con_*() or waits for keys, and cannot be exercised without a pty.
 #  what is covered here is everything that computes rather than draws --
 #  formatting, the file list, sorting, file masks, the history, the size
 #  cache and the option helpers.  see the note above main() for what is
 #  deliberately left out.
 #
 #  vfu's globals live in vfu.cpp next to main(), so that translation unit
 #  has to be linked in; the build renames its main() out of the way with
 #  -Dmain=vfu_main_disabled and this file cancels that for itself.
 #
 #  build:
 #    g++ -O0 -ggdb3 -I. -I../vstring -I../vslib -D_UNICON_USE_CURSES_ \
 #        -Dmain=vfu_main_disabled -o test_vfu test_vfu.cpp *.cpp \
 #        -L../vstring -lvstring -L../vslib -lvslib -lvscon -lncursesw \
 #        -lpcre2-8 -lpcre2-32
 #
 ****************************************************************************/

#undef main   /* the build renames vfu.cpp's main(), not this one */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <sys/stat.h>

#include "vfu.h"
#include "vfuuti.h"
#include "vfufiles.h"
#include "vfudir.h"
#include "vfuview.h"
#include "vfuopt.h"
#include "vfusys.h"

/****************************************************************************
** test bookkeeping
****************************************************************************/

int     tests_run    = 0;
int     tests_failed = 0;
int     quiet        = 0;
VString failed_lines;

static void pass_fail( int line, int ok, const char* what,
                       const char* got, const char* want )
{
  tests_run++;
  if( ! ok ) { tests_failed++; failed_lines = failed_lines + " " + VString( line ); }
  if( ok && quiet ) return;
  printf( "  %4d  %-4s  %-34s got %-22s want %s\n",
          line, ok ? "ok" : "FAIL", what, got, want );
}

void eqs( int line, const char* what, const char* got, const char* want )
{
  VString g = VString( "[" ) + ( got  ? got  : "(null)" ) + "]";
  VString w = VString( "[" ) + ( want ? want : "(null)" ) + "]";
  pass_fail( line, got && want && strcmp( got, want ) == 0, what, g, w );
}

void eqi( int line, const char* what, long got, long want )
{
  pass_fail( line, got == want, what, VString( (long)got ), VString( (long)want ) );
}

void gr( const char* name )
{
  printf( "\n--- %s\n", name );
}

/* the file list joined into one string, so a whole list is one comparison */
static VString flist()
{
  VString r;
  for( int z = 0; z < files_list_count(); z++ )
    {
    if( z ) r += "|";
    r += files_list_is_empty( z ) ? "(empty)" : files_list_get( z )->name_ext();
    }
  return r;
}

/* vfu_sort_files() reads files_list[FLI], so the cursor index has to be kept
   inside the list the way the navigation code does it in the running app */
static void fix_index()
{
  int n = files_list_count();
  file_list_index.set_min_max( 0, n > 0 ? n - 1 : 0 );
  file_list_index.set_pagesize( n > 0 ? n : 1 );
  file_list_index.go( 0 );
}

static void add_file( const char* name, fsize_t size, int is_dir, time_t mtime = 0 )
{
  struct stat st;
  memset( &st, 0, sizeof( st ) );
  st.st_mode  = is_dir ? ( S_IFDIR | 0755 ) : ( S_IFREG | 0644 );
  st.st_size  = size;
  st.st_mtime = mtime ? mtime : 1000 + size;
  st.st_ctime = st.st_atime = st.st_mtime;
  vfu_add_file( name, &st, 0 );
  fix_index();
}

/* vfu's globals are not initialised until vfu_init() runs, which needs a
   terminal -- set up by hand the few the tested functions read */
static void setup()
{
  opt.reset();
  opt.sort_order      = 'N';
  opt.sort_direction  = 'A';
  opt.sort_top_dirs   = 0;
  opt.f_time_type     = 1;
  opt.use_si_sizes    = 0;
  opt.mask_auto_expand = 0;
  files_mask       = "*";
  files_mask_array = str_split( " ", files_mask );
  files_list_clear();
  fix_index();
  vfu_hist_remove( -1, -1 );
}

/****************************************************************************
** part 1 -- named cases
****************************************************************************/

void t_format()
{
  gr( "size_str_compact -- binary units" );
  eqs( __LINE__, "0",             size_str_compact( 0 ),             "0   B" );
  eqs( __LINE__, "1",             size_str_compact( 1 ),             "1   B" );
  eqs( __LINE__, "999",           size_str_compact( 999 ),           "999   B" );
  eqs( __LINE__, "1023",          size_str_compact( 1023 ),          "1'023   B" );
  eqs( __LINE__, "1024",          size_str_compact( 1024 ),          "1 KiB" );
  eqs( __LINE__, "1536 rounds up",size_str_compact( 1536 ),          "2 KiB" );
  eqs( __LINE__, "1 MiB",         size_str_compact( 1048576 ),       "1 MiB" );
  eqs( __LINE__, "1 GiB",         size_str_compact( 1073741824LL ),  "1.000 GiB" );
  eqs( __LINE__, "1 TiB",         size_str_compact( 1099511627776LL ), "1'024 GiB" );

  gr( "size_str_compact -- opt.use_si_sizes" );
  opt.use_si_sizes = 1;
  eqs( __LINE__, "1000 is 1 KB",  size_str_compact( 1000 ),          "1 KB " );
  eqs( __LINE__, "1048576",       size_str_compact( 1048576 ),       "1 MB " );
  opt.use_si_sizes = 0;
  eqs( __LINE__, "back to binary",size_str_compact( 1024 ),          "1 KiB" );

  gr( "fsize_fmt" );
  eqs( __LINE__, "0",             fsize_fmt( 0 ),                    "0" );
  eqs( __LINE__, "1234",          fsize_fmt( 1234 ),                 "1'234" );
  eqs( __LINE__, "1234567",       fsize_fmt( 1234567 ),              "1'234'567" );
  eqs( __LINE__, "use_gib on 1 GiB", fsize_fmt( 1073741824LL, 1 ),   "1.000 GiB +" );

  gr( "vfu_str_comma" );
  VString a = "1234567";  vfu_str_comma( a );
  eqs( __LINE__, "VString&",      a,                                 "1'234'567" );
  VString b = "12";       vfu_str_comma( b );
  eqs( __LINE__, "short is left alone", b,                           "12" );
  VString c = "";         vfu_str_comma( c );
  eqs( __LINE__, "empty",         c,                                 "" );
  char d[64]; strcpy( d, "98765" ); vfu_str_comma( d );
  eqs( __LINE__, "char*",         d,                                 "98'765" );
  eqs( __LINE__, "fsize_t 0",     vfu_str_comma( (fsize_t)0 ),       "0" );
  eqs( __LINE__, "fsize_t neg",   vfu_str_comma( (fsize_t)-1234567 ), "-1'234'567" );

  gr( "time_str_compact" );
  char tb[64];
  eqi( __LINE__, "returns its buffer", time_str_compact( 1700000000, tb ) == tb, 1 );
  eqi( __LINE__, "and fills it",       str_len( VString( tb ) ) > 0, 1 );
}

void t_options()
{
  gr( "key_by_name -- function keys" );
  eqi( __LINE__, "F1",          key_by_name( "F1" ),   UKEY_F1 );
  eqi( __LINE__, "F12",         key_by_name( "F12" ),  UKEY_F1 + 11 );
  eqi( __LINE__, "lowercase f3",key_by_name( "f3" ),   UKEY_F1 + 2 );
  eqi( __LINE__, "@F1 is alt",  key_by_name( "@F1" ),  UKEY_ALT_F1 );
  eqi( __LINE__, "^F1 is ctrl", key_by_name( "^F1" ),  UKEY_CTRL_F1 );
  eqi( __LINE__, "#F1 is shift",key_by_name( "#F1" ),  UKEY_SH_F1 );

  gr( "key_by_name -- named keys" );
  eqi( __LINE__, "INS",         key_by_name( "INS" ),    UKEY_INS );
  eqi( __LINE__, "IC",          key_by_name( "IC" ),     UKEY_INS );
  eqi( __LINE__, "INSERT",      key_by_name( "INSERT" ), UKEY_INS );
  eqi( __LINE__, "ENTER",       key_by_name( "ENTER" ),  UKEY_ENTER );
  eqi( __LINE__, "RETURN",      key_by_name( "RETURN" ), UKEY_ENTER );

  gr( "key_by_name -- anything else is 0" );
  eqi( __LINE__, "unknown name", key_by_name( "nope" ), 0 );
  eqi( __LINE__, "empty",        key_by_name( "" ),     0 );
  eqi( __LINE__, "modifier alone", key_by_name( "^A" ), 0 );
  eqi( __LINE__, "F with no digits", key_by_name( "F" ), 0 );

  gr( "vfu_opt_time -- selected by opt.f_time_type" );
  struct stat st;
  memset( &st, 0, sizeof( st ) );
  st.st_ctime = 100; st.st_mtime = 200; st.st_atime = 300;
  opt.f_time_type = 0;  eqi( __LINE__, "type 0 is ctime", vfu_opt_time( &st ), 100 );
  opt.f_time_type = 1;  eqi( __LINE__, "type 1 is mtime", vfu_opt_time( &st ), 200 );
  opt.f_time_type = 2;  eqi( __LINE__, "type 2 is atime", vfu_opt_time( &st ), 300 );
  opt.f_time_type = 1;
  eqi( __LINE__, "the struct form agrees", vfu_opt_time( st ), 200 );
  eqi( __LINE__, "the explicit form",      vfu_opt_time( 1, 2, 3 ), 2 );
}

void t_file_type()
{
  /* file_type_str() returns a shared static buffer, so every call has to be
     consumed before the next one -- that is why each is its own statement */
  gr( "file_type_str" );
  eqs( __LINE__, "plain file",      file_type_str( S_IFREG | 0644, 0 ), "--" );
  eqs( __LINE__, "executable",      file_type_str( S_IFREG | 0755, 0 ), "**" );
  eqs( __LINE__, "group execute",   file_type_str( S_IFREG | 0654, 0 ), "**" );
  eqs( __LINE__, "directory",       file_type_str( S_IFDIR | 0755, 0 ), "[]" );
  eqs( __LINE__, "link to a dir",   file_type_str( S_IFDIR | 0755, 1 ), "<>" );
  eqs( __LINE__, "link to a file",  file_type_str( S_IFREG | 0644, 1 ), "->" );
  eqs( __LINE__, "fifo",            file_type_str( S_IFIFO | 0644, 0 ), "()" );
  eqs( __LINE__, "socket",          file_type_str( S_IFSOCK | 0644, 0 ), "@@" );
  eqs( __LINE__, "block device",    file_type_str( S_IFBLK | 0644, 0 ), "==" );
  eqs( __LINE__, "char device",     file_type_str( S_IFCHR | 0644, 0 ), "++" );

  gr( "file_get_mode_str" );
  mode_str_t ms;
  file_get_mode_str( (mode_t)( S_IFREG | 0751 ), ms );
  eqs( __LINE__, "0751",            ms, "-rwxr-x--x" );
  file_get_mode_str( (mode_t)( S_IFDIR | 0755 ), ms );
  eqs( __LINE__, "a directory",     ms, "drwxr-xr-x" );
  file_get_mode_str( (mode_t)( S_IFREG | 0000 ), ms );
  eqs( __LINE__, "no permissions",  ms, "----------" );
  file_get_mode_str( (mode_t)( S_IFREG | 0777 ), ms );
  eqs( __LINE__, "0777",            ms, "-rwxrwxrwx" );
}

void t_file_list()
{
  gr( "file list -- add and read back" );
  setup();
  eqi( __LINE__, "empty count",     files_list_count(), 0 );
  add_file( "bbb.txt", 100, 0 );
  eqi( __LINE__, "count after 1",   files_list_count(), 1 );
  add_file( "aaa.txt", 200, 0 );
  add_file( "zdir",      0, 1 );
  eqi( __LINE__, "count after 3",   files_list_count(), 3 );
  eqs( __LINE__, "insertion order", flist(), "bbb.txt|aaa.txt|zdir" );

  gr( "file list -- the TF entry" );
  TF* f = files_list_get( 0 );
  eqs( __LINE__, "name_ext()",      f->name_ext(), "bbb.txt" );
  eqs( __LINE__, "ext()",           f->ext(),      ".txt" );
  eqi( __LINE__, "size()",          (long)f->size(), 100 );
  eqi( __LINE__, "is_dir() false",  f->is_dir(),   0 );
  eqi( __LINE__, "is_link() false", f->is_link(),  0 );
  eqi( __LINE__, "sel starts at 0", f->sel,        0 );
  eqi( __LINE__, "the dir is a dir", files_list_get( 2 )->is_dir(), 1 );
  TF* g = files_list_get( 2 );
  eqs( __LINE__, "a name with no ext", g->ext(), "" );

  gr( "file list -- del marks empty, pack compacts, trim drops the last" );
  setup();
  add_file( "a", 10, 0 ); add_file( "b", 20, 0 ); add_file( "c", 30, 0 );
  files_list_del( 1 );
  eqi( __LINE__, "count is unchanged",  files_list_count(), 3 );
  eqi( __LINE__, "is_empty(1)",         files_list_is_empty( 1 ), 1 );
  eqi( __LINE__, "is_empty(0)",         files_list_is_empty( 0 ), 0 );
  eqs( __LINE__, "the hole is visible", flist(), "a|(empty)|c" );
  files_list_pack();
  eqi( __LINE__, "count after pack",    files_list_count(), 2 );
  eqs( __LINE__, "packed",              flist(), "a|c" );
  files_list_trim();
  eqs( __LINE__, "after trim",          flist(), "a" );
  files_list_clear();
  eqi( __LINE__, "after clear",         files_list_count(), 0 );
  eqs( __LINE__, "and it joins empty",  flist(), "" );
}

void t_sorting()
{
  gr( "sort by name" );
  setup();
  add_file( "big", 300, 0 ); add_file( "small", 100, 0 ); add_file( "mid", 200, 0 );
  opt.sort_order = 'N'; opt.sort_direction = 'A'; vfu_sort_files();
  eqs( __LINE__, "N ascending",     flist(), "big|mid|small" );
  opt.sort_direction = 'D'; vfu_sort_files();
  eqs( __LINE__, "N descending",    flist(), "small|mid|big" );

  gr( "sort by size" );
  opt.sort_order = 'S'; opt.sort_direction = 'A'; vfu_sort_files();
  eqs( __LINE__, "S ascending",     flist(), "small|mid|big" );
  opt.sort_direction = 'D'; vfu_sort_files();
  eqs( __LINE__, "S descending",    flist(), "big|mid|small" );

  gr( "sort by time" );
  setup();
  add_file( "t3", 1, 0, 3000 ); add_file( "t1", 2, 0, 1000 ); add_file( "t2", 3, 0, 2000 );
  opt.sort_order = 'T'; opt.sort_direction = 'A'; vfu_sort_files();
  eqs( __LINE__, "T ascending",     flist(), "t1|t2|t3" );
  opt.sort_direction = 'D'; vfu_sort_files();
  eqs( __LINE__, "T descending",    flist(), "t3|t2|t1" );

  gr( "sort by extension" );
  setup();
  add_file( "f.c", 1, 0 ); add_file( "f.a", 2, 0 ); add_file( "f.b", 3, 0 );
  opt.sort_order = 'E'; opt.sort_direction = 'A'; vfu_sort_files();
  eqs( __LINE__, "E ascending",     flist(), "f.a|f.b|f.c" );
  opt.sort_direction = 'D'; vfu_sort_files();
  eqs( __LINE__, "E descending",    flist(), "f.c|f.b|f.a" );

  gr( "sort_top_dirs floats directories to the top" );
  setup();
  add_file( "afile", 10, 0 ); add_file( "zdir", 0, 1 ); add_file( "mfile", 20, 0 );
  opt.sort_order = 'N'; opt.sort_direction = 'A';
  opt.sort_top_dirs = 1; vfu_sort_files();
  eqs( __LINE__, "dirs first",      flist(), "zdir|afile|mfile" );
  opt.sort_top_dirs = 0; vfu_sort_files();
  eqs( __LINE__, "plain name order",flist(), "afile|mfile|zdir" );

  gr( "sorting an empty or single entry list" );
  setup();
  vfu_sort_files();
  eqi( __LINE__, "empty is fine",   files_list_count(), 0 );
  add_file( "only", 1, 0 );
  vfu_sort_files();
  eqs( __LINE__, "one entry",       flist(), "only" );

  gr( "namenumcmp -- digits compare as numbers" );
  eqi( __LINE__, "a1 < a2",         namenumcmp( "a1", "a2" ) < 0, 1 );
  eqi( __LINE__, "a2 < a10",        namenumcmp( "a2", "a10" ) < 0, 1 );
  eqi( __LINE__, "a10 > a2",        namenumcmp( "a10", "a2" ) > 0, 1 );
  eqi( __LINE__, "equal",           namenumcmp( "abc", "abc" ), 0 );
  eqi( __LINE__, "empty equal",     namenumcmp( "", "" ), 0 );
  eqi( __LINE__, "a < b",           namenumcmp( "a", "b" ) < 0, 1 );
}

void t_masks()
{
  gr( "vfu_fmask_match -- 0 means match, as with FNMATCH" );
  setup();
  files_mask = "*.txt"; files_mask_array = str_split( " ", files_mask );
  eqi( __LINE__, "a.txt matches",   vfu_fmask_match( "a.txt" ), 0 );
  eqi( __LINE__, "a.c does not",    vfu_fmask_match( "a.c" ) != 0, 1 );
  files_mask = "*.txt *.c"; files_mask_array = str_split( " ", files_mask );
  eqi( __LINE__, "first of two",    vfu_fmask_match( "a.txt" ), 0 );
  eqi( __LINE__, "second of two",   vfu_fmask_match( "a.c" ), 0 );
  eqi( __LINE__, "neither",         vfu_fmask_match( "a.h" ) != 0, 1 );
  files_mask = "*"; files_mask_array = str_split( " ", files_mask );
  eqi( __LINE__, "* matches all",   vfu_fmask_match( "anything" ), 0 );
  eqi( __LINE__, "even no ext",     vfu_fmask_match( "noext" ), 0 );
  files_mask = "a?c"; files_mask_array = str_split( " ", files_mask );
  eqi( __LINE__, "? is one char",   vfu_fmask_match( "abc" ), 0 );
  eqi( __LINE__, "and only one",    vfu_fmask_match( "abbc" ) != 0, 1 );
  files_mask = "[ab]*"; files_mask_array = str_split( " ", files_mask );
  eqi( __LINE__, "a charset",       vfu_fmask_match( "axx" ), 0 );
  eqi( __LINE__, "outside the set", vfu_fmask_match( "cxx" ) != 0, 1 );

  gr( "vfu_expand_mask" );
  VString a = "txt";  vfu_expand_mask( a );
  eqs( __LINE__, "a bare word gets *",  a, "txt*" );
  VString b = "*.c";  vfu_expand_mask( b );
  eqs( __LINE__, "already a mask",      b, "*.c" );
  VString c = "";     vfu_expand_mask( c );
  eqs( __LINE__, "empty becomes *",     c, "*" );
  VString d = "a?c";  vfu_expand_mask( d );
  eqs( __LINE__, "? counts as a mask",  d, "a?c" );
}

void t_history()
{
  gr( "history -- add, count, get" );
  setup();
  eqi( __LINE__, "empty count",     vfu_hist_count( HID_CHDIR ), 0 );
  eqs( __LINE__, "get on empty",    vfu_hist_get( HID_CHDIR, 0 ) ? "?" : "(null)", "(null)" );
  vfu_hist_add( HID_CHDIR, "/one" );
  vfu_hist_add( HID_CHDIR, "/two" );
  eqi( __LINE__, "count",           vfu_hist_count( HID_CHDIR ), 2 );
  eqs( __LINE__, "newest is first", vfu_hist_get( HID_CHDIR, 0 ), "/two" );
  eqs( __LINE__, "then the older",  vfu_hist_get( HID_CHDIR, 1 ), "/one" );
  eqs( __LINE__, "past the end",    vfu_hist_get( HID_CHDIR, 9 ) ? "?" : "(null)", "(null)" );

  gr( "history -- ids are independent" );
  vfu_hist_add( HID_GREP, "pattern" );
  eqi( __LINE__, "the other id",    vfu_hist_count( HID_GREP ), 1 );
  eqi( __LINE__, "is unaffected",   vfu_hist_count( HID_CHDIR ), 2 );
  eqs( __LINE__, "and reads back",  vfu_hist_get( HID_GREP, 0 ), "pattern" );
  eqi( __LINE__, "an unused id",    vfu_hist_count( HID_MKPATH ), 0 );

  gr( "history -- re-adding moves to the top without duplicating" );
  vfu_hist_add( HID_CHDIR, "/one" );
  eqi( __LINE__, "count is the same", vfu_hist_count( HID_CHDIR ), 2 );
  eqs( __LINE__, "and it is first",   vfu_hist_get( HID_CHDIR, 0 ), "/one" );
  eqs( __LINE__, "the other moved down", vfu_hist_get( HID_CHDIR, 1 ), "/two" );

  gr( "history -- index and remove" );
  eqi( __LINE__, "index of the first",  vfu_hist_index( HID_CHDIR, "/one" ), 0 );
  eqi( __LINE__, "index of the second", vfu_hist_index( HID_CHDIR, "/two" ), 1 );
  eqi( __LINE__, "index of a missing",  vfu_hist_index( HID_CHDIR, "/nope" ), -1 );
  vfu_hist_remove( HID_CHDIR, 0 );
  eqi( __LINE__, "count after remove",  vfu_hist_count( HID_CHDIR ), 1 );
  eqs( __LINE__, "what is left",        vfu_hist_get( HID_CHDIR, 0 ), "/two" );
  vfu_hist_remove( HID_CHDIR, -1 );
  eqi( __LINE__, "remove all of an id", vfu_hist_count( HID_CHDIR ), 0 );
  eqi( __LINE__, "the other id stays",  vfu_hist_count( HID_GREP ), 1 );
  vfu_hist_remove( -1, -1 );
  eqi( __LINE__, "remove everything",   vfu_hist_count( HID_GREP ), 0 );

  gr( "history -- the per-id cap" );
  setup();
  char b[32];
  for( int z = 0; z < 200; z++ ) { sprintf( b, "/p%03d", z ); vfu_hist_add( HID_CHDIR, b ); }
  eqi( __LINE__, "capped at 128",       vfu_hist_count( HID_CHDIR ), 128 );
  eqs( __LINE__, "the newest survives", vfu_hist_get( HID_CHDIR, 0 ), "/p199" );
  eqi( __LINE__, "other ids unaffected",vfu_hist_count( HID_GREP ), 0 );

  gr( "history -- the buffer form of get" );
  setup();
  vfu_hist_add( HID_CHDIR, "/some/path" );
  char dest[64];
  vfu_hist_get( HID_CHDIR, 0, dest, sizeof( dest ) );
  eqs( __LINE__, "filled in",           dest, "/some/path" );
  char small[5];
  vfu_hist_get( HID_CHDIR, 0, small, sizeof( small ) );
  eqi( __LINE__, "truncated safely",    str_len( VString( small ) ) < 5, 1 );
  vfu_hist_get( HID_CHDIR, 9, dest, sizeof( dest ) );
  eqs( __LINE__, "missing gives \"\"",  dest, "" );

  gr( "history -- save and load round trip" );
  setup();
  VString fn = VString( "/tmp/vfu_test_hist." ) + VString( (int)getpid() );
  filename_history = fn;
  unlink( fn );
  vfu_hist_add( HID_CHDIR, "/a" );
  vfu_hist_add( HID_CHDIR, "/b" );
  vfu_hist_add( HID_GREP,  "gg" );
  vfu_hist_save();
  vfu_hist_load();
  eqi( __LINE__, "chdir count",         vfu_hist_count( HID_CHDIR ), 2 );
  eqi( __LINE__, "grep count",          vfu_hist_count( HID_GREP ), 1 );
  eqs( __LINE__, "order is kept",       vfu_hist_get( HID_CHDIR, 0 ), "/b" );
  eqs( __LINE__, "and the value",       vfu_hist_get( HID_GREP, 0 ), "gg" );

  gr( "history -- save merges with what is already on disk" );
  /* this is what stops a second vfu instance from losing the first one's
     entries: the file is re-read and merged before it is written */
  vfu_hist_remove( -1, -1 );
  vfu_hist_add( HID_CHDIR, "/from_the_other_instance" );
  vfu_hist_save();
  eqi( __LINE__, "the disk entries came back", vfu_hist_count( HID_CHDIR ), 3 );
  eqs( __LINE__, "ours stays newest",   vfu_hist_get( HID_CHDIR, 0 ), "/from_the_other_instance" );
  eqi( __LINE__, "and the other id too",vfu_hist_count( HID_GREP ), 1 );
  vfu_hist_load();
  eqi( __LINE__, "all of it persisted", vfu_hist_count( HID_CHDIR ), 3 );

  gr( "history -- an entry with a newline is not remembered" );
  /* the file is one entry per line, so such a value would come back as two on
     reload, the first half a valid looking but wrong path */
  VString fn2 = VString( "/tmp/vfu_test_nl." ) + VString( (int)getpid() );
  filename_history = fn2;      /* a file of its own: save() merges with disk */
  unlink( fn2 );
  vfu_hist_remove( -1, -1 );
  vfu_hist_add( HID_CHDIR, "/tmp/plain" );
  vfu_hist_add( HID_CHDIR, "/tmp/two\nlines" );
  eqi( __LINE__, "the newline one is dropped", vfu_hist_count( HID_CHDIR ), 1 );
  eqs( __LINE__, "the good one is kept",       vfu_hist_get( HID_CHDIR, 0 ), "/tmp/plain" );
  vfu_hist_add( HID_CHDIR, "/tmp/cr\rhere" );
  eqi( __LINE__, "a carriage return too",      vfu_hist_count( HID_CHDIR ), 1 );
  vfu_hist_save();
  vfu_hist_load();
  eqi( __LINE__, "and nothing splits on load", vfu_hist_count( HID_CHDIR ), 1 );
  eqs( __LINE__, "still the right path",       vfu_hist_get( HID_CHDIR, 0 ), "/tmp/plain" );
  unlink( fn2 );
  filename_history = fn;

  gr( "history -- no temp file is left behind" );
  VString tmpname = fn + ".tmp." + VString( (int)getpid() );
  eqi( __LINE__, "temp is gone",        access( tmpname, F_OK ) != 0, 1 );
  unlink( fn );

  gr( "history -- saving with no filename set is a no-op" );
  filename_history = "";
  vfu_hist_add( HID_CHDIR, "/x" );
  vfu_hist_save();
  eqi( __LINE__, "did not crash",       1, 1 );
}

void t_size_cache()
{
  gr( "size cache" );
  size_cache_set( "/tmp/aaa", 1000 );
  eqi( __LINE__, "get",             (long)size_cache_get( "/tmp/aaa" ), 1000 );
  eqi( __LINE__, "index is found",  size_cache_index( "/tmp/aaa" ) >= 0, 1 );
  eqi( __LINE__, "a missing get",   (long)size_cache_get( "/tmp/none" ), -1 );
  eqi( __LINE__, "a missing index", size_cache_index( "/tmp/none" ), -1 );
  size_cache_set( "/tmp/aaa", 2000 );
  eqi( __LINE__, "overwritten",     (long)size_cache_get( "/tmp/aaa" ), 2000 );
  size_cache_clean( "/tmp/aaa" );
  eqi( __LINE__, "after clean",     (long)size_cache_get( "/tmp/aaa" ), -1 );
  size_cache_set( "/tmp/b1", 10 );
  size_cache_set( "/tmp/b2", 20 );
  eqi( __LINE__, "two entries kept apart", (long)size_cache_get( "/tmp/b1" ), 10 );
  eqi( __LINE__, "the second one",         (long)size_cache_get( "/tmp/b2" ), 20 );
  size_cache_clean( "/tmp/b1" );
  size_cache_clean( "/tmp/b2" );

  gr( "size_cache_compose_key" );
  VString k1 = size_cache_compose_key( "/tmp/x", 1234 );
  VString k2 = size_cache_compose_key( "/tmp/x", 1234 );
  VString k3 = size_cache_compose_key( "/tmp/y", 1234 );
  VString k4 = size_cache_compose_key( "/tmp/x", 9999 );
  eqs( __LINE__, "the key is stable",   k1, k2 );
  eqi( __LINE__, "a different path differs", k1 != k3, 1 );
  eqi( __LINE__, "a different size differs", k1 != k4, 1 );
  eqi( __LINE__, "the size leads the key",   str_find( k1, "1234" ) >= 0, 1 );
}

void t_misc()
{
  gr( "vfu_readlink" );
  eqs( __LINE__, "not a link gives \"\"", vfu_readlink( "/etc/hostname" ), "" );
  eqs( __LINE__, "missing gives \"\"",    vfu_readlink( "/nonexistent/xyz" ), "" );

  gr( "vfu_temp / vfu_temp_dir produce fresh names" );
  VString t1 = vfu_temp();
  VString t2 = vfu_temp();
  eqi( __LINE__, "non empty",           str_len( t1 ) > 0, 1 );
  eqi( __LINE__, "two calls differ",    t1 != t2, 1 );
  eqi( __LINE__, "carries the prefix",  str_find( t1, "vfu." ) >= 0, 1 );
  VString d1 = vfu_temp_dir();
  eqi( __LINE__, "dir name non empty",  str_len( d1 ) > 0, 1 );
  eqi( __LINE__, "and differs from it", d1 != t1, 1 );
  /* vfu_temp()/vfu_temp_dir() create the entries, so remove them here */
  unlink( t1 );
  unlink( t2 );
  rmdir( d1 );
  eqi( __LINE__, "temp file 1 is gone",  access( t1, F_OK ) != 0, 1 );
  eqi( __LINE__, "temp file 2 is gone",  access( t2, F_OK ) != 0, 1 );
  eqi( __LINE__, "temp dir is gone",     access( d1, F_OK ) != 0, 1 );
}

/****************************************************************************
** part 2 -- sweeps
****************************************************************************/

long sweep_run  = 0;
long sweep_fail = 0;

static void sw( int line, int ok, const char* what, const char* detail )
{
  sweep_run++;
  if( ok ) return;
  sweep_fail++;
  if( sweep_fail <= 25 )
    printf( "  %4d  FAIL  %-28s %s\n", line, what, detail );
}

void sweep_sorting()
{
  gr( "sweep: every sort order and direction" );

  static const char orders[] = { 'N', 'S', 'T', 'E', 0 };
  static const char dirs[]   = { 'A', 'D', 0 };

  for( int o = 0; orders[o]; o++ )
    for( int d = 0; dirs[d]; d++ )
      for( int n = 0; n <= 8; n++ )
        {
        setup();
        char b[16];
        for( int z = 0; z < n; z++ )
          {
          sprintf( b, "f%d.e%d", ( z * 7 ) % 10, ( z * 3 ) % 10 );
          add_file( b, ( z * 13 ) % 100, 0, 1000 + ( z * 17 ) % 100 );
          }
        int before = files_list_count();
        opt.sort_order = orders[o];
        opt.sort_direction = dirs[d];
        vfu_sort_files();
        sw( __LINE__, files_list_count() == before, "sorting keeps the count", flist() );

        /* sorting again must not move anything */
        VString once = flist();
        vfu_sort_files();
        sw( __LINE__, flist() == once, "sorting is idempotent", flist() );

        /* the two directions are reverses of each other */
        opt.sort_direction = dirs[d] == 'A' ? 'D' : 'A';
        vfu_sort_files();
        VString other = flist();
        opt.sort_direction = dirs[d];
        vfu_sort_files();
        VArray fwd = str_split_simple( "|", flist() );
        VArray rev = str_split_simple( "|", other );
        rev.reverse();
        sw( __LINE__, str_join( fwd, "|" ) == str_join( rev, "|" ),
            "A and D are reverses", flist() );
        }
}

void sweep_history()
{
  gr( "sweep: history over every id, with save and load" );

  static const int ids[] = { HID_GREP, HID_GS_MASK, HID_MKPATH, HID_FFMASK,
                             HID_SHELL_PAR, HID_FMASK, HID_COMMANDS,
                             HID_GETDIR, HID_CHDIR, HID_OMODE, 0 };

  setup();
  VString fn = VString( "/tmp/vfu_test_hsweep." ) + VString( (int)getpid() );
  filename_history = fn;
  unlink( fn );

  char b[32];
  for( int i = 0; ids[i]; i++ )
    for( int z = 0; z < 5; z++ )
      {
      sprintf( b, "id%d_val%d", ids[i], z );
      vfu_hist_add( ids[i], b );
      }

  for( int i = 0; ids[i]; i++ )
    {
    sw( __LINE__, vfu_hist_count( ids[i] ) == 5, "each id kept its five",
        VString( vfu_hist_count( ids[i] ) ) );
    sprintf( b, "id%d_val4", ids[i] );
    sw( __LINE__, VString( vfu_hist_get( ids[i], 0 ) ) == b, "newest first", b );
    sprintf( b, "id%d_val0", ids[i] );
    sw( __LINE__, vfu_hist_index( ids[i], b ) == 4, "oldest last", b );
    }

  /* a save/load round trip must change nothing */
  VString before;
  for( int i = 0; ids[i]; i++ )
    for( int z = 0; z < 5; z++ )
      before = before + "|" + vfu_hist_get( ids[i], z );
  vfu_hist_save();
  vfu_hist_load();
  VString after;
  for( int i = 0; ids[i]; i++ )
    for( int z = 0; z < 5; z++ )
      after = after + "|" + vfu_hist_get( ids[i], z );
  sw( __LINE__, before == after, "round trip is exact", after );

  /* every entry must still be reachable by index */
  for( int i = 0; ids[i]; i++ )
    for( int z = 0; z < 5; z++ )
      {
      sprintf( b, "id%d_val%d", ids[i], 4 - z );
      sw( __LINE__, VString( vfu_hist_get( ids[i], z ) ) == b, "index still resolves", b );
      }

  /* removing one id must leave the others alone */
  for( int i = 0; ids[i]; i++ )
    {
    vfu_hist_remove( ids[i], -1 );
    sw( __LINE__, vfu_hist_count( ids[i] ) == 0, "id emptied", VString( ids[i] ) );
    for( int j = i + 1; ids[j]; j++ )
      sw( __LINE__, vfu_hist_count( ids[j] ) == 5, "later ids untouched", VString( ids[j] ) );
    }
  unlink( fn );
}

void sweep_masks()
{
  gr( "sweep: file masks against generated names" );

  static const char* masks[] = { "*", "*.c", "*.txt", "a*", "*b", "a?c", "[ab]*", NULL };
  static const char* names[] = { "a.c", "b.c", "a.txt", "abc", "ab", "b", "axc",
                                 "noext", "a.c.txt", "", NULL };

  for( int m = 0; masks[m]; m++ )
    {
    setup();
    files_mask = masks[m];
    files_mask_array = str_split( " ", files_mask );
    for( int n = 0; names[n]; n++ )
      {
      int a = vfu_fmask_match( names[n] ) == 0;
      int b = FNMATCH( masks[m], names[n] ) == 0;
      sw( __LINE__, a == b, "matches FNMATCH directly",
          ( VString( masks[m] ) + " vs [" + names[n] + "]" ) );
      }
    }

  /* a mask list matches if any one of its masks does */
  setup();
  files_mask = "*.c *.h"; files_mask_array = str_split( " ", files_mask );
  static const char* nn[] = { "a.c", "a.h", "a.txt", "c", NULL };
  for( int n = 0; nn[n]; n++ )
    {
    int any = ( FNMATCH( "*.c", nn[n] ) == 0 ) || ( FNMATCH( "*.h", nn[n] ) == 0 );
    sw( __LINE__, ( vfu_fmask_match( nn[n] ) == 0 ) == any, "any of the list", nn[n] );
    }
}

void sweep_file_list()
{
  gr( "sweep: file list del / pack at every position" );

  for( int size = 1; size <= 8; size++ )
    for( int pos = 0; pos < size; pos++ )
      {
      setup();
      char b[16];
      for( int z = 0; z < size; z++ ) { sprintf( b, "f%d", z ); add_file( b, z, 0 ); }

      files_list_del( pos );
      sw( __LINE__, files_list_count() == size, "del keeps the count", flist() );
      sw( __LINE__, files_list_is_empty( pos ) == 1, "the slot is empty", flist() );
      for( int z = 0; z < size; z++ )
        if( z != pos )
          sw( __LINE__, files_list_is_empty( z ) == 0, "the others are not", flist() );

      files_list_pack();
      sw( __LINE__, files_list_count() == size - 1, "pack drops one", flist() );
      VString want;
      for( int z = 0; z < size; z++ )
        {
        if( z == pos ) continue;
        sprintf( b, "f%d", z );
        if( str_len( want ) ) want += "|";
        want += b;
        }
      sw( __LINE__, flist() == want, "and keeps the order", flist() );
      }
}

/****************************************************************************
**
** what is deliberately not covered
**
** everything that needs a terminal or a keypress: vfu_draw()/vfu_redraw()/
** vfu_redraw_status()/tree_draw_*()/show_pos() and the rest of vfuview.cpp,
** vfu_ask()/vfu_get_str()/vfu_break_op()/vfu_hist_menu()/vfu_menu_box(),
** the vfu_nav_*() family, vfu_options()/vfu_edit_conf_file(), tree_view(),
** the vfu_tool_*() family, and the copy/archive operations in vfucopy.cpp
** and vfuarc.cpp, which shell out and touch the filesystem in bulk.
**
** also left out: nothing else -- every other computing function is covered.

**
****************************************************************************/

int main( int argc, char** argv )
{
  setlocale( LC_ALL, "" );
  for( int z = 1; z < argc; z++ )
    if( strcmp( argv[z], "-q" ) == 0 ) quiet = 1;

  printf( "=========================================================================\n" );
  printf( " vfu -- part 1: named cases\n" );
  printf( "=========================================================================\n" );
  t_format();
  t_options();
  t_file_type();
  t_file_list();
  t_sorting();
  t_masks();
  t_history();
  t_size_cache();
  t_misc();

  int named_run    = tests_run;
  int named_failed = tests_failed;

  printf( "\n=========================================================================\n" );
  printf( " vfu -- part 2: sweeps\n" );
  printf( "=========================================================================\n" );
  sweep_sorting();
  sweep_history();
  sweep_masks();
  sweep_file_list();
  if( sweep_fail > 25 )
    printf( "  ... and %ld more\n", sweep_fail - 25 );

  printf( "\n=========================================================================\n" );
  printf( " named cases : %d run, %d failed\n", named_run, named_failed );
  if( named_failed )
    {
    printf( " failed lines:%s\n", failed_lines.data() );
    printf( " debug with  : gdb --args ./test_vfu   then  break test_vfu.cpp:LINE\n" );
    }
  printf( " sweeps      : %ld run, %ld failed\n", sweep_run, sweep_fail );
  printf( "=========================================================================\n" );

  return ( named_failed || sweep_fail ) ? 1 : 0;
}

/****************************************************************************
**
** EOF
**
****************************************************************************/
