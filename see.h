/****************************************************************************
 *
 * Copyright (c) 1996-2022 Vladi Belperchinov-Shabanski "Cade" 
 * http://cade.noxrun.com/  <cade@noxrun.com> <cade@bis.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 ****************************************************************************/

#ifndef _SEE_H_
#define _SEE_H_

#include <vstring.h>
#include <wstring.h>
#include <vslib.h>

#define SEE_VERSION "4.10"

#define MAX_SEARCH_LEN  128
#define MAX_PIPE_LEN  128
#define MAX_ESCAPE_KEYS  64

#define SEE_MAX_LINE_LENGTH (32*1024)

struct SeeViewerOptions
{
  SeeViewerOptions() { reset(); }
  void reset()
    {
    auto_size = 1;
    xmin = xmax = ymin = ymax = -1; /* auto */
    cn = CONCOLOR( cWHITE, cBLACK );
    ch = CONCOLOR( cWHITE, cBLUE  );
    cs = CONCOLOR( cBLACK, cWHITE );
    status = 1;
    bsize = SEE_MAX_LINE_LENGTH;
    tabsize = 8;
    wrap = bsize;
    handle_bs = 1;
    handle_tab = 1;
    hex_mode = 0;
    dec_pos = 0;
    grid = 0;
    show_eol = 0;
    last_search[0] = 0;
    last_opt[0] = 0;
    hex_cols = 2;
    }

  int auto_size;

  int xmin, xmax, ymin, ymax;
  int cn; /* color normal */
  int ch; /* color hilite */
  int cs; /* color status line */

  int status; /* show status line */

  int bsize; /* block size (i.e. the longest line ) */
  int tabsize; /*  tab size usually 8 */
  int wrap; /* word wrap */

  int handle_bs; /* backspaces */
  int handle_tab; /* tabs */

  int hex_mode; /* in hex mode */
  int dec_pos; /* show decimal positions in hex mode */
  int grid; /* show hilite grid */
  int show_eol; /* show end of lines with $ */

  int hex_cols; /* 8-bytes columns to show in hex mode */

  char last_search[MAX_SEARCH_LEN+1];
  char last_opt[32];
};

#define FMT_OFF_T "l"
#ifdef _FILE_OFFSET_BITS
#  if _FILE_OFFSET_BITS == 64
#    undef FMT_OFF_T
#    define FMT_OFF_T "ll"
#  elif _FILE_OFFSET_BITS > 64
#    error "cannot represent _FILE_OFFSET_BITS >64"
#  endif
#endif

class SeeViewer
{
  SeeViewerOptions* opt;
  int escape_keys[MAX_ESCAPE_KEYS];
  VString help_str;

  VRegexp re;

  FILE* f;
  VString fname;
  off_t fpos;
  off_t fsize;
  off_t line;
  off_t last_line;
  int end_reached;
  int col;
  int rows;
  int cols;

  int xlat;

  int freezed;
  int do_draw;

  char* buff;

  public:

  SeeViewer( SeeViewerOptions *a_opt );
  ~SeeViewer();

  /* add escape key which will cause run() exit */
  void escape_on( int key );
  /* set help message */
  void set_help_string( const char* s )
    { if ( s ) help_str = s; };

  int open( const char* a_fname );
  void close();

  void status( const char* format, ... );

  void filter( char *s, int size );
  void filter( VString &s, int size );

  void draw_hex();
  void draw_txt();
  void draw();

  void up_txt();
  void up_hex();
  void down_txt();
  void down_hex();

  void up()   { opt->hex_mode ? up_hex() : up_txt(); }
  void down() { opt->hex_mode ? down_hex() : down_txt(); }

  void home();
  void end_hex();
  void end_txt();
  void end() { opt->hex_mode ? end_hex() : end_txt(); }
  void end2();

  void go_to();

  int find_next_txt( int rev  = 0 );
  int find_next_hex( int rev  = 0 );
  int find_next( int rev  = 0 )
      { opt->hex_mode ? find_next_hex(rev) : find_next_txt(rev); return 0; }
  int find( const char* opts );

  void hex_edit();
  void help();

  int run();

  double fpos_percent() const { return 100 * (double(fpos) / (fsize?fsize:1)); }
  protected:

  int read_text( off_t &cpos );

};

/**********************************************************************/

struct SeeEditorOptions
{
  SeeEditorOptions() { reset(); }
  void reset()
    {
    auto_size = 1;
    xmin = xmax = ymin = ymax = -1; /* auto */
    cn = CONCOLOR( cWHITE,  cBLACK );
    ch = CONCOLOR( cWHITE,  cBLUE  );
    cs = CONCOLOR( cBLACK,  cWHITE );
    status = 1;
    tabsize = 8;
    max_line = 4096;
    handle_tab = 1;
    auto_indent = 0;
    insert = 1;
    last_search[0] = 0;
    no_case = 1;
    last_pipe_cmd[0] = 0;
    }

  int auto_size;

  int xmin, xmax, ymin, ymax;
  int cn; /* color normal */
  int ch; /* color hilite */
  int cs; /* color status line */

  int status; /* show status line */

  int tabsize; /*  tab size usually 8 */
  int max_line; /* word wrap */

  int handle_tab; /* tabs */

  int auto_indent;
  int insert; /* if editor is in `insert' mode */

  wchar_t last_search[MAX_SEARCH_LEN+1];
  int no_case;
  char last_pipe_cmd[MAX_PIPE_LEN];
};

class SeeEditor
{
  SeeEditorOptions* opt;
  int escape_keys[MAX_ESCAPE_KEYS];
  VString help_str;

  VString fname;
  VRegexp re;

  int col;
  int colpage;
  ScrollPos sv; /* vertical scroller */
  WArray va; /* string/text cluster */
  int mod; /* modify flag */

  int rows;
  int cols;

  int freezed;
  int do_draw;

  public:

  SeeEditor( SeeEditorOptions *a_opt );
  ~SeeEditor();

  /* add escape key which will cause run() exit */
  void escape_on( int key );
  /* set help message */
  void set_help_string( const char* s )
    { if ( s ) help_str = s; };

  int open( const char* a_fname );
  void close();

  void status( const char* format, ... );

  int expand_tabs( VString &str, VString &map );

  int real_col( int row = -1 );

  void set_cursor();

  void draw_line( int n );
  void draw( int from = 0 );

  int save();
  int request_quit();

  void up() { sv.up(); };
  void down() { sv.down(); };
  void left();
  void right();

  void home();
  void end();

  void go_to();

  void kdel();
  void kbs();
  void kenter();
  void kinsert( wchar_t wch );

  void insert_file( const char* fn );

  void remove_line( int n = -1 );
  void remove_all();

  void remove_trails( int n = -1 ); /* remove trailing spaces/tabs */

  void insert_pipe_cmd();

  int find_next();
  int find( int no_case );

  void help();

  int run();

};

#endif /* _SEE_H_ */

