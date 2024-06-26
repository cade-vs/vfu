/****************************************************************************
 #
 # Copyright (c) 1996-2023 Vladi Belperchinov-Shabanski "Cade" 
 # https://cade.noxrun.com/  <cade@noxrun.com> <cade@bis.bg>
 # https://cade.noxrun.com/projects/vfu     https://github.com/cade-vs/vfu
 #
 # SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 #
 ****************************************************************************/

#ifndef _VFU_H_
#define _VFU_H_

/*############################################ INCLUDE's ###############*/

  #include <dirent.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <sys/wait.h>
  #include <sys/param.h>
  #include <fcntl.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <stdint.h>
  #include <unistd.h>
  #include <string.h>
  #include <time.h>
  #include <utime.h>
  #include <assert.h>
  #include <ctype.h>
  #include <grp.h>
  #include <pwd.h>
  #include <errno.h>
  #include <signal.h>
  #include <math.h>

  #include <vstruti.h>
  #include <vslib.h>

  #ifdef HAVE_CONFIG_H
  #  include "config.h"
  #endif

  #if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    #if defined(__OpenBSD__)
      #include <sys/types.h>
    #else
      #include <sys/param.h>
    #endif
    #include <sys/mount.h>
  #else
    #if defined(__APPLE__)
      #include <sys/param.h>
      #include <sys/mount.h>
    #else
      #include <sys/vfs.h>
    #endif  
  #endif

  #include "vfusetup.h"

  #include "vfusys.h" // for attrs_t

/*############################################ COMPATIBILITY DEF's #####*/

#define FNMATCH_FLAGS 0
#define FNCASE        1
#define PATH_DELIMITER   ":"

#define FNMATCH(p,s)      sfn_match((p),(s),FNMATCH_FLAGS)
#define FNMATCH_NC(p,s)   sfn_match((p),(s),SFN_CASEFOLD)
#define FNMATCH_OC(p,s,n) sfn_match((p),(s),(n)?SFN_CASEFOLD:FNMATCH_FLAGS)

#define pathcmp strcmp
#define pathncmp strncmp

  typedef double fsize_t; /* used as big integer */
  typedef char  fname_t[MAX_PATH]; /* */

/*############################################ GLOBAL DEFINES  #########*/

  /* history id's */
  #define HID_GREP        10
  #define HID_GS_MASK     20
  #define HID_GS_GREP     30
  #define HID_MKPATH      40
  #define HID_FFMASK      50
  #define HID_FFPATH      60
  #define HID_FFGREP      70
  #define HID_EE_TIME     80 // entry edit
  #define HID_EE_OWNER    90 // entry edit
  #define HID_SHELL_PAR  100
  #define HID_FMASK      110
  #define HID_COMMANDS   120
  #define HID_GETDIR     130
  #define HID_CHDIR      140
  #define HID_SEQ_PREFIX 150
  #define HID_SEQ_SUFFIX 160
  #define HID_SEQ_DIGPOS 170
  #define HID_SEQ_START  180
  #define HID_OMODE      190 // octal mode

/*######################################################################*/

  /* file class type */
  class TF
    {
      VString       _name;  /* name with extension ( ref. _fname ) */
      VString       _name_ext;   /* extension ( ref. _fname ) */
      VString       _ext;   /* extension ( ref. _fname ) */
      struct stat   _st;
      char          _type_str[3];
      int           _is_link;
      int           _is_dir;
      mode_str_t    _mode_str;
      fsize_t       _size;

      VString       _view; /* up to screen width */
      int           _color; /* view colors */

      /* !!! this is used when full name required */
      /* and this is not thread-safe :) */
      VString       _full_name;

      void          reset(); /* reset -- NULL all fields */
      void          refresh_view(); /* this is called by view() only! */

    public:
      TF();
      TF( const char* a_name, const struct stat* a_stat, int a_is_link );
      ~TF();

      const char*   full_name( int fix = 0 );
      const char*   name()     { return (const char*)_name; }
      const char*   name_ext() { return (const char*)_name_ext; }
      const char*   ext()      { return (const char*)_ext; }

      void          set_name( const char* a_new_name );
      const char*   view();
      void          drop_view();

      void          update_stat( const struct stat* a_new_stat = NULL, int a_is_link = -1 );

      const char*   type_str() { return (const char*)_type_str;   }
      const char*   mode_str() { return (const char*)_mode_str;   }
      const struct  stat* st() { return (const struct stat*)&_st; }

      void          set_size( fsize_t a_new_size );
      fsize_t       size() { if ( _is_dir && _size == -1 ) return 0; else return _size; }

      int           is_link() { return _is_link; }
      int           is_dir()  { return _is_dir;  }

      int           color();

      /* public member variables */
      int           sel; /* this saves set/get_sel() functions :) */
      int           x; /* misc used extra field */
    };

/*######################################################################*/

  #define WM_NORMAL    0
  #define WM_ARCHIVE   1

  /* work context */
  extern int    work_mode;
  extern VString work_path;
  /* archive context */
  extern VString archive_name;
  extern VString archive_path;
  extern VArray archive_extensions;

  extern VString external_panelizer;
  extern VArray list_panelizer;

  /* file list statistics */
  extern fsize_t    files_size;
  extern int        sel_count;
  extern fsize_t    sel_size;
  /* file system statistics */
  extern fsize_t    fs_free;
  extern fsize_t    fs_total;
  extern fsize_t    fs_block_size;
  /* index in the files list */
  /* NOTE: following defines are kept for historical reasons :) */
  extern  ScrollPos file_list_index;
  #define FLI       (file_list_index.pos())
  #define FLP       (file_list_index.page())
  #define FLPS      (file_list_index.pagesize())
  #define FLMIN     (file_list_index.min())
  #define FLMAX     (file_list_index.max())
  #define FLGO(n)   (file_list_index.go(n))
  #define FLGET(n)  (files_list_get(n))
  #define FLCUR     (files_list_get(FLI))

  /* some world wide variables */
  extern VString startup_path;
  extern VString home_path;
  extern VString tmp_path;
  extern VString rc_path;

  /* files masks */
  extern VString         files_mask;
  extern VArray          files_mask_array;

  /* misc */
  extern int print_help_on_exit;

  extern VString last_inc_search;

/*############################################ GLOBAL STRUCTS  #########*/

  extern VArray dir_tree;
  extern int    dir_tree_changed;

  extern WArray file_find_results; // filefind results

  extern VArray path_bookmarks;


/*######################################################################*/

  extern VArray user_externals;
  extern VArray history;
  extern VArray see_filters;
  extern VArray panelizers;

  extern WArray mb; /* menu boxes */

  extern VArray trim_tree;

  extern VArray view_profiles;
  extern VString view_profile;

/*############################################ CONFIG SETTINGS #########*/

  extern VString ext_colors[16];

  extern VString shell_browser;
  extern VString shell_editor;
  extern VString shell_diff;
  extern VString shell_prog;

  extern VString user_id_str;
  extern VString group_id_str;
  extern VString host_name_str;

  extern VString filename_opt;
  extern VString filename_conf;
  extern VString filename_history;
  extern VString filename_tree;
  extern VString filename_size_cache;
  extern VString filename_ffr; /* file find results */

/*######################################################################*/

  extern int do_draw;
  extern int do_draw_status;

/*######################################################################*/

/*
  Message issues
*/
void say( int line, int attr, const char* format, ... );

void say1(const char *a_str, int attr = cMESSAGE );
void say2(const char *a_str, int attr = cMESSAGE );
void say2errno();

void saycenter( int line, int attr, const char *a_str );

void say1center(const char *a_str, int attr = cMESSAGE );
void say2center(const char *a_str, int attr = cMESSAGE );

void log_debug( const char* format, ... );

/*
  Main things
*/

void vfu_help();
void vfu_help_cli();

void vfu_init();
void vfu_run();
void vfu_cli();
void vfu_done();
void vfu_reset_screen();
void vfu_signal( int sig );
void vfu_exit_path( const char *a_path );
int vfu_exit( const char* a_path );

void vfu_options();

void vfu_toggle_view_fields( wchar_t wch );
/*
  Support op's
*/

void vfu_shell( const char* a_command, const char* a_options );

void vfu_tools();
void vfu_command();
void vfu_file_find( int menu );
void vfu_file_find_results();
void vfu_directory_sizes( wchar_t wch );

void vfu_change_file_mask( const char* a_new_mask );

void bookmark_goto( wchar_t wch );
void bookmark_set( int a_n, const char* a_path );
const char* bookmark_get( int a_n );
void bookmark_hookup();

void update_status();

int vfu_user_external_find( wchar_t key, const char* ext, const char* type, VString *shell_line );
void vfu_user_external_exec( wchar_t key );
void vfu_user_menu();

void vfu_inc_search( int use_last_one = 0 );
void vfu_goto_filename( const char* fname );

/*
  Main files op's
*/

int  vfu_edit_attr( char *attrs );
void vfu_edit_entry( );
void vfu_rename_file_in_place();

void vfu_browse( const char* a_fname, int no_filters = 0 );
void vfu_browse_selected_files();
void vfu_edit( const char* a_fname );

void vfu_action_plus( wchar_t );
void vfu_action_minus( int mode = 0 );
void vfu_global_select();
void vfu_sort_menu();

void vfu_read_files_menu();

void vfu_clipboard( int act );

void vfu_jump_to_mountpoint( int all );

#endif//_VFU_H_
