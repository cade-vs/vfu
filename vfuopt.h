/****************************************************************************
 #
 # Copyright (c) 1996-2023 Vladi Belperchinov-Shabanski "Cade" 
 # https://cade.noxrun.com/  <cade@noxrun.com> <cade@bis.bg>
 # https://cade.noxrun.com/projects/vfu     https://github.com/cade-vs/vfu
 #
 # SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 #
 ****************************************************************************/

#ifndef _VFUOPT_H_
#define _VFUOPT_H_

#include "see.h"
#include "vfuuti.h"

  #define OPT_SCROLL_PAGESTEP(s)  ( (s) == 1 ? 0.3 : (s) == 2 ? 0.5 : 1 )


  extern const wchar_t *NOYES[];
  extern const wchar_t *FTIMETYPE[];
  extern const wchar_t *TAGMARKS[];
  extern const wchar_t *COMMA_TYPES[];

  struct Options {
    wchar_t sort_order;
    wchar_t sort_direction;
    int sort_top_dirs;

    fname_t last_copy_path[3];

    fname_t path_bookmarks[10];

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
    int set_term_title_info;

    int scroll_pagestep;

    int show_symlinks_stat;
    
    SeeViewerOptions svo;
    SeeEditorOptions seo;
    void reset(void)
    {
      sort_order = 0;
      sort_direction = 0;
      sort_top_dirs = 0;
      memset(last_copy_path, 0, sizeof last_copy_path);
      memset(path_bookmarks, 0, sizeof path_bookmarks);
      f_size = 0;
      f_time = 0;
      f_mode = 0;
      f_group = 0;
      f_owner = 0;
      f_type = 0;
      f_time_type = 0;
      long_name_view = 0;
      tree_compact = 0;
      tree_cd = 0;
      show_hidden_files = 0;
      allow_beep = 0;
      use_colors = 0;
      use_dir_colors = 0;
      lower_case_ext_config = 0;
      copy_free_space_check = 0;
      copy_calc_totals = 0;
      copy_keep_mode = 0;
      tag_mark_type = 0;
      internal_browser = 0;
      internal_editor = 0;
      mask_auto_expand = 0;
      shell_cls = 0;
      zap_ro = 0;
      no_case_glob = 0;
      show_user_free = 0;
      menu_borders = 0;
      lynx_navigation = 0;
      default_copy_to_cwd = 0;
      auto_mount = 0;
      keep_selection = 0;
      bytes_freed = 0;
      smart_home_end = 0;
      use_si_sizes = 0;
      use_gib_usage = 0;
      comma_type = 0;
      scroll_pagestep = 0;
      show_symlinks_stat = 0;
      svo.reset();
      seo.reset();
    }
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

  void vfu_settings_load( VArray* data );
  void vfu_settings_save();

  void vfu_edit_conf_file();
  void vfu_options();

#endif /* _VFUOPT_H_ */

