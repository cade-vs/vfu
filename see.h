/*
 *
 * (c) Vladi Belperchinov-Shabanski "Cade" 1996-2003
 * http://soul.datamax.bg/~cade <cade@biscom.net>  <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 * $Id: see.h,v 1.8 2003/01/26 21:48:42 cade Exp $
 *
 */

#ifndef _SEE_H_
#define _SEE_H_

#include <vslib.h>

#define SEE_VERSION "4.00"

#define MAX_SEARCH_LEN  128
#define MAX_PIPE_LEN  128
#define MAX_ESCAPE_KEYS  64

struct SeeViewerOptions
{
  SeeViewerOptions()
    { reset(); }
  void reset()  
    {
    auto_size = 1;
    xmin = xmax = ymin = ymax = -1; /* auto */
    cn = CONCOLOR( cWHITE, cBLACK ); 
    ch = CONCOLOR( cWHITE, cBLUE  ); 
    cs = CONCOLOR( cBLACK, cWHITE ); 
    status = 1;
    bsize = 4096;
    tabsize = 8;
    wrap = bsize;
    handle_bs = 1;
    handle_tab = 1;
    hex_mode = 0;
    dec_pos = 0;
    grid = 0;
    show_eol = 0;
    last_search[0] = 0;
    no_case = 1;
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

  char last_search[MAX_SEARCH_LEN+1];
  int no_case;
};

class SeeViewer
{
  SeeViewerOptions* opt;
  int escape_keys[MAX_ESCAPE_KEYS];
  VString help_str;

  FILE* f;
  VString fname;
  int fpos;
  int fsize;
  int line;
  int last_line;
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

  void status( const char* s, int color = -1 );
  
  void filter( char *s, int size );
  
  void draw_hex();
  void draw_txt();
  void draw();
  
  void up();
  void down();
  
  void home();
  void end();
  void end2();
  
  void go_to();
  
  int find_next();
  int find( int no_case );
  
  void hex_edit();
  void help();
  
  int run();
  
  protected:
  
  int read_text( int &cpos );
  
};

/**********************************************************************/

struct SeeEditorOptions
{
  SeeEditorOptions()
    { reset(); }
  void reset()  
    {
    auto_size = 1;
    xmin = xmax = ymin = ymax = -1; /* auto */
    cn = CONCOLOR( cWHITE, cBLACK ); 
    ch = CONCOLOR( cWHITE, cBLUE  ); 
    cs = CONCOLOR( cBLACK, cWHITE ); 
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
  
  char last_search[MAX_SEARCH_LEN+1];
  int no_case;
  char last_pipe_cmd[MAX_PIPE_LEN];
};

class SeeEditor
{
  SeeEditorOptions* opt;
  int escape_keys[MAX_ESCAPE_KEYS];
  VString help_str;

  VString fname;
  
  int col;
  int colpage;
  TScrollPos sv; /* vertical scroller */
  VArray va; /* string/text cluster */
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

  void status( const char* s, int color = -1 );
  
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
  void kinsert( int ch );
  
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
