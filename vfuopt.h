/****************************************************************************
 *
 * Copyright (c) 1996-2020 Vladi Belperchinov-Shabanski "Cade" 
 * http://cade.datamax.bg/  <cade@biscom.net> <cade@bis.bg> <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 ****************************************************************************/

#ifndef _VFUOPT_H_
#define _VFUOPT_H_

#include "see.h"
#include "vfuuti.h"

  extern const char *NOYES[];
  extern const char *FTIMETYPE[];
  extern const char *TAGMARKS[];
  extern const char *COMMA_TYPES[];

  struct Options {
    int sort_order;
    int sort_direction;
    int sort_top_dirs;

    max_path_str_t last_copy_path[3];

    max_path_str_t path_bookmarks[10];

    int f_size;
    int f_time;
    int f_mode;
    int f_group;
    int f_owner;
    int f_type;
    int f_time_type;

    int long_name_view;
    int tree_compact;
    int tree_cd;

    int show_hidden_files; /* `dot' files in UNIX, `HS' files in dos */

    int allow_beep;

    int use_colors;
    int use_dir_colors; /* /etc/DIR_COLORS */
    int lower_case_ext_config;

    int copy_free_space_check;
    int copy_calc_totals;
    int copy_keep_mode; /* preserve mode, owner, group on copy ? */

    int tag_mark_type;

    int internal_browser;
    int internal_editor;

    int mask_auto_expand;
    int shell_cls;

    int zap_ro; /* zap/erase read-only files */

    int no_case_glob;

    int show_user_free; /* ...space instead of real fs free */
    int menu_borders;

    int lynx_navigation; /* should <- == - and -> == + */

    int default_copy_to_cwd; /* default copy dir always points to CWD */

    int auto_mount;
    int keep_selection; /* on rescan files */

    int bytes_freed; /* calc/show bytes freed on erase */

    int smart_home_end; /* toggle between first/last entry and first/last file/directory in the list */

    int use_si_sizes;
    int use_gib_usage;

    int comma_type;
    
    SeeViewerOptions svo;
    SeeEditorOptions seo;
  };

  extern Options opt;

  int key_by_name( const char* key_name );

  time_t vfu_opt_time( const struct stat st );
  time_t vfu_opt_time( const struct stat* st );
  time_t vfu_opt_time( time_t ctime, time_t mtime, time_t atime );

  int set_set( const char *line, const char *keyword, char *target );
  int set_set( const char *line, const char *keyword, VString &target );
  int set_set( const char *line, const char *keyword, int &target );
  int set_set( const char *line, const char *keyword, VArray &splitter );

  void vfu_settings_load();
  void vfu_settings_save();

  void vfu_edit_conf_file();
  void vfu_options();

#endif /* _VFUOPT_H_ */

