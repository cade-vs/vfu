/*
 *
 * (c) Vladi Belperchinov-Shabanski "Cade" 1996-2003
 * http://soul.datamax.bg/~cade  <cade@biscom.net>  <cade@datamax.bg>
 *
 * SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
 *
 * $Id: see.cpp,v 1.17 2003/01/29 22:59:16 cade Exp $
 *
 */

#include <string.h>
#include <assert.h>

#include "see.h"

#ifndef ASSERT
#define ASSERT assert
#endif

  #define CHKPOS ASSERT( fpos >= 0 ); ASSERT( fpos <= fsize )

  char HEXCHARS[] = "0123456789ABCDEF";

  char bg_xlat_table[2][64] =
  {
  " ¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º¼¾¿€‚ƒ„…†‡ˆ‰Š‹ŒŽ‘’“”•–—˜™šœžŸ",
  "abwgdevzijklmnoprstufhc`[]yxuqABWGDEVZIJKLMNOPRSTUFHC`[]YXUQ"
  };

  char bgw_xlat_table[2][64] =
  {
  "àáâãäåæçèéêëìíîïðñòóôõö÷øùúüþÿÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÜÞß",
  "abwgdevzijklmnoprstufhc`[]yxuqABWGDEVZIJKLMNOPRSTUFHC`[]YXUQ"
  };

  #define MAXCOLS 1024
  
/*--------------------------------------------------------------------*/
  
  SeeViewer::SeeViewer( SeeViewerOptions *a_opt )
  {
  opt = a_opt;
  memset( &escape_keys, 0, sizeof(escape_keys));
  f = NULL;
  line = last_line = 1;
  col = 0;
  end_reached = 0;
  fpos = fsize = 0;
  fname = "";
  freezed = 0;
  do_draw = 0;
  
  if ( opt->auto_size )
    {
    opt->xmin = 1;
    opt->ymin = 1;
    opt->xmax = con_max_x();
    opt->ymax = con_max_y();
    }
  rows = opt->ymax - opt->ymin - (opt->status != 0) + 1;
  cols = opt->xmax - opt->xmin + 1;
  
  buff = (char*)malloc( opt->bsize + 32 ); /* +32 for tab expansion */
  
  help_str = 
  "+-----------------------------------------------------------------------------+\n"
  "| SeeViewer v" SEE_VERSION " (c) Vladi Belperchinov-Shabanski <cade@biscom.net>          |\n"
  "|                                                                             |\n"
  "| Key       TextMode             HexMode            Compatibility             |\n"
  "| --------+--------------------+--------------------+------------------------ |\n"
  "| UpArrow | one line back      | 16 bytes back      | P     = Home            |\n"
  "| DnArrow | one line forward   | 16 bytes forward   | B     = PgUp            |\n"
  "| LtArrow | col -8 ( `.' `>' ) |  1 byte  back      | SPC   = PgDn            |\n"
  "| RtArrow | col +8 ( `,' `<' ) |  1 byte  forward   | ENTER = DnArrow         |\n"
  "| Home    | go to line 1       | go to byte 0       |                         |\n"
  "| End     | go to last line    | go to last byte    |                         |\n"
  "| Ctrl+E  | -'- (no line info) | go to last byte    | l -- BG DOS xlate (slow)|\n"
  "| PgUp/Dn | one page back/forw | one page back/forw | L -- BG WIN xlate (slow)|\n"
  "| --------+--------------------+--------------------+------------------------ |\n"
  "| TAB  -- switch between Text and Hex mode          | ESC   -- exit           |\n"
  "| 1..0 -- switch to slot 1 .. slot 10               | Alt+X -- exit           |\n"
  "| W w  -- wrap to screen width                      | -     -- exit           |\n"
  "| +    -- goto line/pos (+line/pos, -line/pos)      | d -- show dec.pos (HEX) |\n"
  "| I    -- edit! (only for HEX mode)                 | o -- show EOL's (TEXT)  |\n"
  "| F S  -- find string F=no-case, S=case-sense       | r -- show ruler (TEXT)  |\n"
  "|         ~pattern  is regexp pattern search        | a -- filter backspaces  |\n"
  "|         $pattern  is hex pattern search           | t -- tab expansion      |\n"
  "| N F3 -- find next                                 | g G -- grid (HEX)       |\n"
  "+-----------------------------------------------------------------------------+";
  };
  
/*--------------------------------------------------------------------*/
  
  SeeViewer::~SeeViewer()  
  {
  close();
  if ( buff ) free( buff );
  buff = NULL;
  };
  
/*--------------------------------------------------------------------*/
  
  /* add escape key which will cause run() exit */
  void SeeViewer::escape_on( int key )  
  {
  int z = 0;
  while( z < MAX_ESCAPE_KEYS-1 )
    {
    if (!escape_keys[z])
      {
      escape_keys[z] = key;
      return;
      }
    z++;
    }
  };

/*--------------------------------------------------------------------*/
  
  int SeeViewer::open( const char* a_fname )
  {
  if (!buff) return 1;
  if (f) fclose( f );
  xlat = 0;
  f = NULL;
  line = 0;
  col = 0;
  last_line = 0;
  end_reached = 0;
  fpos = fsize = 0;
  fname = a_fname;
  freezed = 0;
  do_draw = 0;
  f = fopen( fname, "r" );
  if (!f) return 2;
  fsize = file_size( f );
  return 0;
  };

/*--------------------------------------------------------------------*/
  
  void SeeViewer::close()  
  {
  if (f) fclose( f );
  f = NULL;
  };

/*--------------------------------------------------------------------*/
  
  void SeeViewer::status( const char* s, int color )  
  {
  VString sss;
  sss = "| ";
  sss += s;
  if (str_len(sss) >= cols)
    str_sleft( sss, (cols-2) );
  else
    str_pad( sss, -(cols-2) );
  sss += "|";
  con_out( opt->xmin, opt->ymax, sss, color != -1 ? color : opt->cs );
  };

/*--------------------------------------------------------------------*/
  
  void SeeViewer::filter( char *s, int size )  
  {
  int z;
  for ( z = 0; z < size; z++ ) if ( (unsigned char)s[z] < 32 ) s[z] = '.';
  if (xlat == 1) str_tr( s,  bg_xlat_table[0],  bg_xlat_table[1] ); else
  if (xlat == 2) str_tr( s, bgw_xlat_table[0], bgw_xlat_table[1] );
  };

/*--------------------------------------------------------------------*/
  
  void SeeViewer::draw_hex()  
  {
  CHKPOS;
  if ( cols < 79 || cols >= MAXCOLS )
    {
    status( "HEX mode only supported on 80-1024 columns!" );
    return;
    }
  int z = 0;
  int i = 0;
  char lin[MAXCOLS];
  char str[32];
  int buffsize = rows * 16; // all size :)
  fseek( f, fpos, SEEK_SET );
  buffsize = fread( buff, 1, buffsize, f );
  for( z = 0; z < rows; z++ )
    {
    sprintf( lin, opt->dec_pos ? "%8d : ":"%08X : ", fpos + z * 16 );
    for ( i = 0; i < 16; i++ )
      {
      if ( z*16+i < buffsize )
        {
        sprintf( lin+11+i*3+(i>7)*2, (i==7)?"%02X - ":"%02X ", (unsigned char)buff[z*16+i]);
        str[i] = buff[z*16+i];
        }
      else
        {
        sprintf( lin+11+i*3+(i>7)*2, (i==7)?"     ":"   " );
        str[i] = ' ';
        }
      }
    str[16] = 0;
    filter( str, 16 );
    if ((z-1)*16+i < buffsize)
      strcat( lin, ": " );
    else
      strcat( lin, "  " );
    if (lin[11] == ' ') lin[11] = '~'; // hack
    strcat( lin, str );
    str_pad( lin, -cols );
    con_out( 1, z+1, lin, (opt->grid && (fpos/16+z)%2==0) ? opt->ch : opt->cn );
    }
  sprintf( buff, "SeeViewer v" SEE_VERSION " | %3.0f%% | Pos. %4d of %4d | H,Alt+H Help ", (100.0*fpos)/(fsize?fsize:1), fpos, fsize );
  status( buff );
  };

/*--------------------------------------------------------------------*/
  
  void SeeViewer::draw_txt()  
  {
  CHKPOS;
  if ( line == -1 ) last_line = -1;
  int cpos = fpos;
  int z = 0;
  int y = 0;
  fseek( f, cpos, SEEK_SET );
  VString sss;
  while( y < rows )
    {              
    if ( cpos >= fsize )
      {
      sss = "~";
      str_pad( sss, -cols );
      while( y < rows )
        {
        con_out( 1, y+1,sss, opt->cn);
        y++;
        }
      break;
      }

    z = read_text( cpos );
    while ( z > 0 && ( buff[z-1] == '\r' || buff[z-1] == '\n' ) ) z--;
    buff[z] = 0;
    filter( buff, z );
    
    int show_lmark = 0;
    int show_rmark = 0;
    int show_eol   = -1;
    if (col)
      {
      if (col >= z)
        {
        buff[0] = 0;
        show_lmark = 1;
        z = 0;
        }
      else
        {
        str_trim_left( buff, col );
        z -= col;
        }
      }
    if ( z > cols )
      {
      buff[cols] = 0;
      show_rmark = 1;
      }
    else
      {
      if ( opt->show_eol && !show_lmark ) show_eol = z+1;
      }
    str_pad( buff, -cols );
    con_out( 1, opt->ymin+y, buff, (opt->grid && y%2==0) ? opt->ch : opt->cn);
    if (show_lmark) con_out(1,opt->ymin+y,"<",chRED);
    if (show_rmark) con_out( opt->xmax, opt->ymin+y, ">", chRED );
    if (show_eol != -1) con_out( show_eol, opt->ymin+y, "$", chGREEN );
    y++;
    }
  sprintf( buff, "SeeViewer v" SEE_VERSION " | %3.0f%% | Line %4d of %4d%c|%4d+ | H,Alt+H Help ", (100.0*fpos)/(fsize?fsize:1), line, last_line, end_reached?' ':'?', col+1 );
  status( buff );
  };

/*--------------------------------------------------------------------*/
  
  void SeeViewer::draw() 
  { 
  (opt->hex_mode) ? draw_hex() : draw_txt(); 
  if ( xlat == 1 ) con_out( opt->xmax -  7, opt->ymin, "BG XLAT", chRED );
  if ( xlat == 2 ) con_out( opt->xmax - 10, opt->ymin, "BGWIN XLAT", chRED );
  }

/*--------------------------------------------------------------------*/
  
  void SeeViewer::up()  
  {
  CHKPOS;
  if (opt->hex_mode)
    {
    fpos -= 16;
    if (fpos < 0) fpos = 0;
    return;
    }
  int cpos = fpos;
  if ( cpos == 0 ) return;

  int i = opt->wrap;
  if ( cpos - i < 0 ) i = cpos;
  cpos -= i;
  fseek( f, cpos, SEEK_SET );
  int res = fread( buff, 1, i, f );
  ASSERT( res == i );
  int z = 0;
  if ( buff[i-1] == '\n' )
    {
    i--;
    z++;
    }
  while( i > 0 && buff[i-1] != '\n' )
    {
    i--;
    z++;
    }
  fpos -= z;
  if ( fpos <  0 ) fpos = 0;
  if ( fpos == 0 ) line = 1;
  if ( line >  1 ) line--;
  };

/*--------------------------------------------------------------------*/
  
  void SeeViewer::down()  
  {
  CHKPOS;
  if (opt->hex_mode)
    {
    fpos += 16;
    if ( fpos > fsize ) fpos = fsize;
    return;
    }
  int z = 0;
  if ( fpos == fsize ) return;
  if ( fseek( f, fpos, SEEK_SET ) ) return;
  int res = fread( buff, 1, opt->wrap, f );
  z = 0;
  while( z < res && buff[z] != '\n' ) z++;
  if (buff[z] == '\n') z++;
  buff[z] = 0; // need by SeeFindNext()
//    strcpy( cline, buff ); // need by SeeFindNext()
  fpos += z;
  if ( line >= 0 ) line++;
  if ( line > last_line ) last_line = line;
  if ( fpos >  fsize ) fpos = fsize;
  if ( fpos == fsize && last_line != -1 ) end_reached = 1;
  };

/*--------------------------------------------------------------------*/
  
  void SeeViewer::home()  
  {
  fpos = 0;
  line = 1;
  };

/*--------------------------------------------------------------------*/
  
  void SeeViewer::end()  
  {
  if (end_reached)
    {
    end2();
    return;
    }
  while ( fpos < fsize )
    {
    if ( con_kbhit() && con_getch() == 27 ) return;
    down();
    if (line % 768 == 0)
      {
      char tmp[128];
      sprintf(tmp, " Going down.... line: %6d (%3.0f%%) press ESCAPE to cancel ", line, (100.0*fpos)/(fsize?fsize:1) );
      status(tmp);
      }
    }
  end2();
  };

/*--------------------------------------------------------------------*/

  void SeeViewer::end2()  
  {
  int z = 0;
  if (!end_reached)
    line = -1;
  else
    line = last_line;
  fpos = fsize;
  for ( z = 0; z < rows / 2; z++ ) up();
  };
  
/*--------------------------------------------------------------------*/
  
  void SeeViewer::go_to()  
  {
  VString sss;
  if(opt->hex_mode)
    {
    sprintf( sss, "x%X", fpos );
    status( " Goto pos: " );
    if (!TextInput( 15, opt->ymax, "", 20, 20, &sss ))
      {
      draw();
      return;
      }
    int new_pos = fpos;
    str_cut_spc( sss );
    str_up( sss );
    if ( sss[0] == '-' )
      new_pos = (sss[1] == 'X') ? hex2long( (const char*)sss+2 ) : atol( (const char*)sss+1 );
    else
    if ( sss[0] == '+' )
      new_pos += (sss[1] == 'X') ? hex2long( (const char*)sss+2 ) : atol( (const char*)sss+1 );
    else
      new_pos  = (sss[0] == 'X') ? hex2long( (const char*)sss+1 ) : atol( (const char*)sss );
    if ( new_pos >= 0 && new_pos < fsize ) fpos = new_pos;
    draw();
    }
  else
    {
    if ( last_line == -1 )
      {
      status( "Cannot determine line number..." );
      return;
      }
    sprintf( sss, "%d", line);
    status( " Goto line: " );
    if (!TextInput( 15, opt->ymax, "", 20, 20, &sss ))
      {
      draw();
      return;
      }
    int new_line = line;
    str_cut_spc( sss );
    str_up( sss );
    if ( sss[0] == '-' )
      new_line -= atol( (const char*)sss+1 );
    else
    if ( sss[0] == '+' )
      new_line += atol( (const char*)sss+1 );
    else
      new_line  = atol( (const char*)sss );
    if (new_line < 0) new_line = 0;
    if (last_line != -1 && end_reached && new_line > last_line) new_line = last_line;
    if (new_line == line)
      {
      draw();
      return;
      }
    if (new_line > line)
      while( new_line != line && fpos < fsize )
        {
        if ( con_kbhit() && con_getch() == 27 ) return;
        down();
        if ( line % 768 == 0)
          {
          sprintf( sss, " Going down.... line: %6d -- %3.0f%% (press ESCAPE to cancel) ", line, (100.0*fpos)/(fsize?fsize:1) );
          status( sss);
          }
        }
    else
      while( new_line != line && fpos >  0 )
        {
        if ( con_kbhit() && con_getch() == 27 ) return;
        up();
        if ( line % 768 == 0)
          {
          sprintf( sss, " Going up.... line: %6d -- %3.0f%% (press ESCAPE to cancel) ", line, (100.0*fpos)/fsize );
          status( sss);
          }
        }
    draw();
    }
  };

/*--------------------------------------------------------------------*/
  
  int SeeViewer::find_next()  
  {
  VString sss;
  if ( !opt->last_search[0] )
    {
    status( "No search pattern..." );
    return 1;
    }
  sprintf( sss, "Searching for `%s'...", opt->last_search );
  status( sss );
  if ( opt->hex_mode )
    fpos++;
  else
    down(); /* start search from the next line -- avoid blocking */
  int new_pos = file_find_string( opt->last_search, f, opt->no_case, fpos+1 );
  if ( new_pos >= 0 )
    {
    fpos = new_pos;
    if (!opt->hex_mode)
      {
      up(); /* back to the start of the line */
      line += file_grep_lines_read;
      int mpos = new_pos - fpos; // marker pos
      if ( mpos > opt->xmax ) col = ( mpos / 8 - 1 )*8;
      mpos -= col;
      draw();
      if (mpos > 0)
        con_out( mpos, 1, ">", chMAGENTA );
      }
    else
      draw();
    sprintf( sss, "Pattern `%s' found at pos: %d (0x%X)", opt->last_search, new_pos, new_pos );
    status( sss );
    }
  else
    {
    sprintf( sss, "Pattern `%s' not found...", opt->last_search );
    status( sss );
    }
  return 0;
  };

/*--------------------------------------------------------------------*/
  
  int SeeViewer::find( int no_case )  
  {
  VString sss;
  sprintf( sss, "Find %s: ", no_case?"(no case)":"(with case)");
  status( sss );
  int ii = str_len(sss)+2;
  sss = opt->last_search;
  if(!TextInput( opt->xmin+ii, opt->ymax, "", opt->xmax-ii-4, opt->xmax-ii-4, &sss ))
    {
    draw();
    return 1;
    }
  str_sleft( sss, MAX_SEARCH_LEN );
  strcpy( opt->last_search, sss );
  opt->no_case = no_case;
  return find_next();
  };
  
/*--------------------------------------------------------------------*/
  
  void SeeViewer::hex_edit()  
  {
  if (!opt->hex_mode)
    {
    status( "HexEdit is available only in HEX mode :)" );
    return;
    }

  int in_text = 0; // if text is edited
  int editbs = rows * 16;
  unsigned char *editb = (unsigned char*)malloc( editbs );
  fseek( f, fpos, SEEK_SET );
  editbs = fread( editb, 1, editbs, f );
  if ( editbs == 0 )
    {
    free( editb );
    status( "Nothing to edit or read error..." );
    return;
    };
  int epos = 0;
  int bytepos = 0; /* first or second byte part? :) */

  /* 79 is warning (bright white on red) hard coded color*/
  status( "WARNING: HEX EDITING MODE! ENTER = SAVE, ESC = CANCEL, TAB = TOGGLE EDIT MODE !", 79 );
  con_cshow();
  int key = 0;
  while(4)
    {
    if (in_text)
      con_xy( 64 + epos % 16, 1 + epos / 16 );
    else
      con_xy( 12 + (epos%16)*3 + 2*(epos%16 > 7) + bytepos, 1 + epos / 16 );
    if ( key == 0  ) key = con_getch();
    if ( key == 27 ) break;
    if ( key == 13 )
      {
      /* will commit changes -- file should be reopened for RW */
      fclose( f );
      f = fopen( fname, "r+b" );
      fseek( f, fpos, SEEK_SET );
      int r = fwrite( editb, 1, editbs, f );
      fclose( f );
      if (  r != editbs )
        {
        status( "Write error (press a key)" );
        con_beep();
        con_getch();
        }
      f = fopen( fname, "rb" );
      break;
      }
    switch( key )
      {
      case 9         : in_text = !in_text; break;
      case KEY_RIGHT : if (bytepos == 0 && !in_text)
                         bytepos = 1;
                       else
                         if (epos < editbs - 1)
                           {
                           epos++;
                           if (!in_text) bytepos = 0;
                           }
                       break;
      case KEY_LEFT  : if (bytepos == 1 && !in_text )
                         bytepos = 0;
                       else
                         if (epos > 0)
                           {
                           epos--;
                           if (!in_text) bytepos = 1;
                           }
                       break;
      case KEY_DOWN  : if ( epos + 16 <  editbs ) epos += 16; break;
      case KEY_UP    : if ( epos - 16 >= 0      ) epos -= 16; break;
      
      case KEY_HOME  : epos = epos - epos % 16; bytepos = 0; break;
      case KEY_END   : epos = epos + (16 - epos%16 - 1);
                       if (epos >= editbs) epos = editbs - 1;
                       bytepos = 0;  break;
      }
    if ( !in_text && key > 0 && key < 255 && strchr( HEXCHARS, toupper(key) ) )
      {
      int n = str_find( HEXCHARS, toupper(key) );
      char tmp[2];
      tmp[0] = HEXCHARS[n]; tmp[1] = 0;
      con_puts( tmp, chRED );
      if (bytepos == 0)
        {
        editb[epos] = (n << 4) + ( editb[epos] & 0x0F );
        }
      else
        {
        editb[epos] = (n     ) + ( editb[epos] & 0xF0 );
        }
      tmp[0] = editb[epos];
      filter( tmp, 1 );
      con_xy( 64 + epos % 16, 1 + epos / 16);
      con_puts( tmp, chRED );
      key = KEY_RIGHT;
      }
    else
    if ( in_text && key >= 32 && key < 255 )
      {
      char tmp[3];
      tmp[0] = key; tmp[1] = 0;
      con_puts( tmp, chRED );
      con_xy( 12 + (epos % 16)*3 + 2*(epos % 16 > 7), 1 + epos / 16 );
      sprintf( tmp, "%02X", key );
      tmp[2] = 0;
      con_puts( tmp, chRED );
      editb[epos] = key;
      key = KEY_RIGHT;
      }
    else
      key = 0;
    }
  con_chide();
  free(editb);
  draw();
  };

/*--------------------------------------------------------------------*/
  
  void SeeViewer::help()  
  {
  con_out( 1, 1, help_str );
  do_draw = 1;
  con_getch();
  };
  
/*--------------------------------------------------------------------*/
  
  int SeeViewer::run()  
  {
  CHKPOS;
  if (!f) return 27;
  int ch = 0;
  draw();
  while(ch != 27)
    {
    if ( do_draw )
      {
      draw();
      do_draw = 0;
      }
    ch = con_getch();
    if (  ch == 27        || ch == '-'           || ch == 'q' ||
          ch == KEY_ALT_X || ch == KEY_BACKSPACE ) return ch;
    int z = 0;
    while( escape_keys[z] )
      if ( escape_keys[z++] == ch )
        return ch;      
    switch(ch)
      {
      case KEY_F1     :
      case KEY_ALT_H  :
      case 'h'        :
      case 'H'        : help(); break;
      case KEY_UP     : up(); draw(); break;
      case 13         :
      case KEY_DOWN   : down(); draw(); break;
      case 'b'        :
      case 'B'        :
      case KEY_PPAGE  : for ( z = 0; z < rows; z++ ) up(); draw(); break;
      case ' '        :
      case KEY_NPAGE  : for ( z = 0; z < rows; z++ ) down(); draw(); break;
      case 'p'        :
      case 'P'        :
      case KEY_HOME   : if (fpos == 0) col = 0; else home(); draw(); break;
      case KEY_END    : end();  draw(); break;
      case KEY_CTRL_E : end2(); draw(); break;

      case KEY_CTRL_L : if ( opt->auto_size )
                          {
                          opt->xmin = 1;
                          opt->ymin = 1;
                          opt->xmax = con_max_x();
                          opt->ymax = con_max_y();
                          }
                        rows = opt->ymax - opt->ymin - (opt->status != 0) + 1;
                        cols = opt->xmax - opt->xmin + 1;
                        con_cs();
                        draw();
                        break;

      case '>'        :
      case '.'        :
      case KEY_RIGHT  : if (opt->hex_mode)
                          {
                          if (fpos < fsize) fpos++;
                          draw();
                          }
                        else
                          {
                          if (col < opt->wrap-10)
                            {
                            col += (ch == '>') ? 1 : 8;
                            draw();
                            }
                          };
                        break;
      case '<'        :
      case ','        :
      case KEY_LEFT   :
                        if (opt->hex_mode)
                          {
                          if (fpos > 0) fpos--;
                          draw();
                          }
                        else
                          {
                          if (col > 0)
                            {
                            col -= (ch=='<')?1:8;
                            if (col < 0) col = 0;
                            draw();
                            }
                          };
                        break;
      case 9          : opt->hex_mode = !opt->hex_mode;
                        if (!opt->hex_mode) 
                          {
                          fpos++;
                          if ( fpos > fsize ) fpos = fsize;
                          up();
                          }
                        draw(); 
                        break;
      case 'g'        : 
      case 'G'        : opt->grid = !opt->grid; draw(); break;
      case 'W'        :
      case 'w'        : opt->wrap = (opt->wrap < opt->bsize)? opt->bsize : cols;
                        draw();
                        status( (opt->wrap == cols)? " Wrap ON" : " Wrap OFF" );
                        break;
      
      case 'l'        : xlat = (xlat == 1) ? 0 : 1; draw(); break;
      case 'L'        : xlat = (xlat == 2) ? 0 : 2; draw(); break;
      
      case 'f'        :
      case 'F'        : find( 1 ); break;
      case 's'        :
      case 'S'        : find( 0 ); break;
      case KEY_F(3)   :
      case 'n'        :
      case 'N'        : find_next(); break;
      case '+'        : go_to(); break;
      case 'd'        :
      case 'D'        : if (opt->hex_mode) 
                          { 
                          opt->dec_pos = !opt->dec_pos; 
                          draw(); 
                          } 
                        break;
      case 'o'        :
      case 'O'        : if (!opt->hex_mode) 
                          { 
                          opt->show_eol = !opt->show_eol; 
                          draw(); 
                          } 
                        break;
      case 'a'        :
      case 'A'        : if (!opt->hex_mode)
                          {
                          opt->handle_bs = !opt->handle_bs; 
                          draw(); 
                          status( opt->handle_bs? " BackSpace handling ON" : " BackSpace handling OFF" );
                          }
                        break;
      case 't'        :
      case 'T'        : if (!opt->hex_mode)
                          {
                          opt->handle_tab = !opt->handle_tab; 
                          draw(); 
                          status( opt->handle_tab? " TAB expansion ON" : " TAB expansion OFF" );
                          }
                        break;
      case 'r'        :
      case 'R'        : if (!opt->hex_mode)
                          {
                          int z = 0;
                          VString ruler;
                          while ( str_len(ruler) < opt->xmax ) 
                            {
                            ruler += "|0-------";
                            z++;
                            ruler += z % 10;
                            }
                          str_sleft( ruler, opt->xmax );
                          con_out( 1, 1, ruler, opt->ch );
                          }
                        break;
      case 'i'        :
      case 'I'        : hex_edit(); break;
      }
    }
  return ch; /* 27 */
  };

/*--------------------------------------------------------------------*/

  /* read ahead with tab and backspace expansion */
  /* result goes into `buff', the margin is `wrap' */
  int SeeViewer::read_text( int &cpos )
  {
  buff[0] = 0;
  int z = 0;
  unsigned char ch;
  while( z < opt->wrap )
    {
    if ( cpos >= fsize ) break;
    ch = fgetc( f );
    cpos++;
    if (opt->handle_bs && ch == 8)
      {
      if (z > 0) z--;
      continue;
      }
    if (opt->handle_tab && ch == 9)
      {
      ASSERT( opt->tabsize > 0 );
      int i = ((z/opt->tabsize)+1) * opt->tabsize - z;
      while( z < opt->wrap && i > 0 )
        {
        buff[z] = ' ';
        z++;
        i--;
        }
      continue;
      }
    buff[z] = ch;
    z++;
    if ( ch == '\n' ) break;
    }
  return z;
  };

/**********************************************************************/
/**********************************************************************/

  #define SEEDCOL  (col      - colpage + 1) /* screen column  */
  #define SEEDROW  (sv.pos() - sv.page() + 1) /* screen row     */

  SeeEditor::SeeEditor( SeeEditorOptions *a_opt )
  {
  opt = a_opt;
  memset( &escape_keys, 0, sizeof(escape_keys));
  fname = "";
  col = 0;
  colpage = 0;
  
  mod = 0;
  freezed = 0;
  
  if ( opt->auto_size )
    {
    opt->xmin = 1;
    opt->ymin = 1;
    opt->xmax = con_max_x();
    opt->ymax = con_max_y();
    }
  rows = opt->ymax - opt->ymin - (opt->status != 0) + 1;
  cols = opt->xmax - opt->xmin + 1;

  sv.set_min_max( 0, va.count() - 1 );
  sv.set_pagesize( rows );
  sv.go( 0 );
  
  help_str = 
  "+-----------------------------------------------------------------------------+\n"
  "| SeeEditor v" SEE_VERSION " (c) Vladi Belperchinov-Shabanski <cade@biscom.net>          |\n"
  "| ^ is Ctrl+key, @ is Alt+key, # is Shift+key                                 |\n"
  "|                                                                             |\n"
  "| Up_Arrow    or ^P -- one line up           ESC  -- request exit             |\n"
  "| Down_Arrow  or ^N -- one line down                 (will prompt for save)   |\n"
  "| Left_Arrow  or ^B -- one char left         ^W   -- pipe cmd input as text   |\n"
  "| Right_Arrow or ^F -- one char right          @F -- find string (no case)    |\n"
  "| Page_Up     or ^U -- one page up             @S -- find next (with case)    |\n"
  "| Page_Down   or ^V -- one page down           @G -- find next                |\n"
  "| Home        or ^A -- goto beg. of line       F3 -- find next                |\n"
  "| End         or ^E -- goto end  of line       ^L -- redraw screen            |\n"
  "| Del         or ^D -- del. char under cursor  ~pattern is regexp search      |\n"
  "| Backspace   or ^H -- del. char to the left   \\pattern is normal search      |\n"
  "| ^K^U              -- goto beg. of file        pattern is same as \\pattern   |\n"
  "| ^K^V              -- goto end  of file                                      |\n"
  "| ^Y                -- delete current line   ^T   -- toggle auto indent       |\n"
  "| F1 or @H          -- this help screen      ^C   -- quit without save NOW!   |\n"
  "| F2 or ^K^D or ^S  -- save file             ^X   -- Save All and Quit Now    |\n"
  "|                                                                             |\n"
  "| No UNDO! If you make a mistake -- quit the file without saving it!          |\n"
  "| --------------------------------------------------------------------------- |\n"
  "| You can replace this editor with external one -- see VFU docs for details!  |\n"
  "+-----------------------------------------------------------------------------+";
  };

/*--------------------------------------------------------------------*/
  
  SeeEditor::~SeeEditor()
  {
  };

/*--------------------------------------------------------------------*/

  /* add escape key which will cause run() exit */
  void SeeEditor::escape_on( int key )
  {
  int z = 0;
  while( z < MAX_ESCAPE_KEYS-1 )
    {
    if (!escape_keys[z])
      {
      escape_keys[z] = key;
      return;
      }
    z++;
    }
  };

/*--------------------------------------------------------------------*/
  
  int SeeEditor::open( const char* a_fname )
  {
  if ( va.count() || str_len( fname ) )
    close();
  fname = a_fname;
  remove_all();
  insert_file( fname );
  if (access( fname, F_OK ))
    {
    mod = 1;
    va.push( "" ); /* hack if new file */
    }
  sv.set_min_max( 0, va.count() - 1 );
  sv.go( 0 );
  col = colpage = 0;
  mod = 0;
  return 0;
  };

/*--------------------------------------------------------------------*/

  void SeeEditor::close()
  {
  if ( mod ) /* if modified */
    if ( request_quit() )
      return; /* request denied */
  fname = "";
  col = 0;
  colpage = 0;
  sv.go( 0 );
  va.undef();
  mod = 0;
  con_chide();
  };

/*--------------------------------------------------------------------*/

  void SeeEditor::status( const char* s, int color )
  {
  VString sss;
  sss = "| ";
  sss += s;
  if (str_len(sss) >= cols)
    str_sleft( sss, (cols-2) );
  else
    str_pad( sss, -(cols-2) );
  sss += "|";
  con_out( opt->xmin, opt->ymax, sss, color != -1 ? color : opt->cs );
  set_cursor();
  };

/*--------------------------------------------------------------------*/

  int SeeEditor::expand_tabs( VString &str, VString &map )
  {
  int res = 0;
  int i = 0;
  map = "";
  str_pad( map, str_len( str ) );
  while( ( i = str_find( str, '\t' ) ) > -1 )
    {
    int j;
    ASSERT( opt->tabsize > 0 );
    j = ( i / opt->tabsize + 1 ) * opt->tabsize;
    j = j - i;
    res += (j - 1);
    str_del( str, i, 1 );
    while( j-- )
      {
      str_ins_ch( str, i, ' ' );
      str_ins_ch( map, i, '+' );
      }
    str_del( map, i, 1 );
    str_ins_ch( map, i, '*' );
    };
  return res;
  };

/*--------------------------------------------------------------------*/

  int SeeEditor::real_col( int row )
  {
  int c = col;
  if (row == -1) row = sv.pos();
  VString str = va[row];
  VString map;
  if ( expand_tabs( str, map ) )
    {
    str_sleft( map, col );
    c -= str_count( map, "+" );
    }
  return c;
  };

/*--------------------------------------------------------------------*/
  
  void SeeEditor::set_cursor()
  {
  con_xy( SEEDCOL, SEEDROW );
  };
                 
/*--------------------------------------------------------------------*/

  void SeeEditor::draw_line( int n )
  {
  if ( freezed ) return;
  
  ASSERT( sv.max() == va.count() - 1 );
  if ( n > sv.max() )
    {
    VString sss = "~";
    str_pad( sss, - cols );
    con_out( 1, ( n - sv.page() ) + 1, sss, opt->cn );
    }
  else
    {
    VString map;
    VString str = va[n];
    expand_tabs( str, map );
    str_trim_left( str, colpage );
    str_sleft( str, cols );
    str_pad( str, - cols );
    con_out( 1, ( n - sv.page() ) + 1, str, opt->cn );
    }
  set_cursor();
  };

/*--------------------------------------------------------------------*/

  void SeeEditor::draw( int from )
  {
  if ( freezed ) return;
  
  int z;
  VString str;
  con_chide();
  if ( from > -1 ) /* from == -1 to update status line only */
    for( z = from; z < rows; z++ )
      draw_line( sv.page() + z );
  con_cshow();
  sprintf( str, "SeeEditor v" SEE_VERSION " | %s | %3.0f%% | Line:%5d of%5d |%4d+ %s | F1,Alt+H Help", 
                 mod?"MOD!":"----", 
                 (100.0*sv.pos())/(sv.max()?sv.max():1), sv.pos()+1, sv.max()+1, col+1, opt->insert?"INS":"ovr" );
  status( str );
  set_cursor();
  };

/*--------------------------------------------------------------------*/

  int SeeEditor::save()
  {
  remove_trails();
  if (va.fsave( fname ))
    {
    VString s = "Cannot save file! ";
    s += fname;
    status( s );
    return 0;
    }
  else
    {
    status( "File saved ok" );  
    mod = 0;
    return 1;
    }
  };

/*--------------------------------------------------------------------*/

  int SeeEditor::request_quit()
  {
  if ( mod == 0 ) return 0; /* ok -- not modified */
  while(4)
    {
    con_beep();
    status( "File is modified! Press: <S> Save, <Q> Quit, <ESC> Cancel" );
    
    con_chide();
    int k = con_getch();
    con_cshow();
    
    if ( k == 'S' || k == 's' )
      {
      if(!save())
        {
        status( "Cannot save file! Press: <S> Save, <Q> Quit, <ESC> Cancel" );
        continue; /* error saving file */
        }
      else
        return 0; /* okay */
      }
    if ( k == 'Q' || k == 'q' )
      {
      mod = 0; /* considered unmodified at that point */
      return 0; /* okay */
      }
    if ( k == 27 ) 
      {
      return 1; /* denied */
      }
    }
  };

/*--------------------------------------------------------------------*/

  void SeeEditor::left()
  {
  if (col <= 0) return;
  VString str = va[sv.pos()];
  VString map;
  if ( expand_tabs( str, map ) )
    {
    col--;
    while (col > 0 && map[col] == '+')
      col--;
    }
  else
    col--;
  if (SEEDCOL < 1)
    colpage--;
  };

/*--------------------------------------------------------------------*/

  void SeeEditor::right()
  {
  VString str = va[sv.pos()];
  VString map;
  if ( expand_tabs( str, map ) )
    {
    col++;
    while (map[col] == '+')
      col++;
    }
  else
    col++;
  if (SEEDCOL > cols)
    colpage++;
  };

/*--------------------------------------------------------------------*/

  void SeeEditor::home()
  {
  col = colpage = 0;
  };

/*--------------------------------------------------------------------*/

  void SeeEditor::end()
  {
  remove_trails( sv.pos() );
  VString str = va[sv.pos()];
  VString map;
  expand_tabs( str, map );
  col = str_len( str );
  
  if (SEEDCOL > cols)
    {
    colpage = col - cols/2;
    if (colpage < 0) colpage = 0;
    draw();
    }
  };

/*--------------------------------------------------------------------*/

  void SeeEditor::go_to()
  {
  };

/*--------------------------------------------------------------------*/

  void SeeEditor::kdel()
  {
  VString str = va[sv.pos()];
  int c = real_col();
  if (c >= str_len( str ))
    {
    if ( sv.pos() == sv.max() ) return;
    mod = 1;
    VString nstr = va[sv.pos()+1]; /* next string (below) */
    if ( c > str_len( str ) ) str_pad( str, -c, ' ' ); /* the line is short -- pad with spaces */
    str += nstr;
    va[ sv.pos() ] = str;
    va.del( sv.pos()+1 );
    sv.set_min_max( 0, va.count() - 1 );
    draw(); /* FIXME: from ROW to the end of the page */
    }
  else
    {
    mod = 1;
    str_del( va[ sv.pos() ], c, 1 );
    draw_line( sv.pos() );
    };
  };

/*--------------------------------------------------------------------*/

  void SeeEditor::kbs()
  {
  VString str = va[sv.pos()];
  int c = real_col();
  if ( c > str_len( str ) )
    {
    left();
    return;
    } else
  if (c == 0)
    {
    if (sv.pos() == 0) return;
    up();
    end();
    kdel();
    }
  else
    {
    left();
    kdel();
    };
  };

/*--------------------------------------------------------------------*/

  void SeeEditor::kenter()
  {
  mod = 1;
  if ( va.count() == 0 ) va.push( "" );
  int c = real_col();
  VString str = va[sv.pos()];
  VString nstr = str;
  str_sleft( str, c );
  str_trim_left( nstr, c );
  va.set( sv.pos(), str );
  va.ins( sv.pos()+1, nstr );
  sv.set_min_max( 0, va.count()-1 );
  sv.down();
  col = 0; /* !!! here should go the auto indenting... */
  if ( opt->auto_indent && sv.pos() > 1)
    {
    str = va[sv.pos()-1];
    int z = 0;
    int nc = 0;
    while( z < str_len(str) && (str[z] == ' ' || str[z] == '\t') )
      {
      if ( str[z] == '\t' ) nc += opt->tabsize; else nc++;
      z++;
      }
    str = va[sv.pos()];
    col = nc;
    while( nc-- ) str_ins_ch( str, 0, ' ' );
    va.set( sv.pos(), str );
    };
  if ( SEEDCOL > opt->xmax || SEEDCOL < 1 )
    {
    colpage = col - cols/2;
    if (colpage < 0) colpage = 0;
    draw();
    }
  else
    draw(); /* FIXME: from ROW to the end of the page */
  };

/*--------------------------------------------------------------------*/

  void SeeEditor::kinsert( int ch )
  {
  if ( ch < 0 || ch > 255 ) return;
  if ( ch == 13 || ch == 10 )
    {
    kenter();
    return;
    };
  mod = 1;
  if ( va.count() == 0 ) va.push( "" );
  VString str = va[sv.pos()];

  int c = real_col();

  if (!opt->insert)
    str_del( str, c, 1 );
  if ( str_len(str) < c ) str_pad( str, -c, ' ' );
  str_ins_ch( str, c, ch );
  va.set( sv.pos(), str );
  right();
  
  if ( SEEDCOL > opt->xmax || SEEDCOL < 1 )
    {
    colpage = col - cols/2;
    if (colpage < 0) colpage = 0;
    draw();
    }
  else
    draw(); /* FIXME: from ROW to the end of the page */
  };

/*--------------------------------------------------------------------*/
  
  void SeeEditor::insert_file( const char* fn )
  {
  mod = va.count();
  
  /* FIXME: this should insert file in current position! */
  va.fload( fname );
  remove_trails();
  sv.set_min_max( 0, va.count() - 1 );
  };
  
/*--------------------------------------------------------------------*/

  void SeeEditor::remove_line( int n )
  {
  if ( n == -1 ) n = sv.pos();
  ASSERT( sv.max() == va.count() - 1 );
  if ( n < 0 || n > sv.max() ) return;
  mod = 1;
  if ( n == sv.max() )
    {
    if ( str_len( va[n] ) == 0 ) return;
    va.set( n, "" );
    }
  else
    {  
    va.del( n );
    sv.set_min_max( 0, va.count() - 1 );
    sv.go( sv.pos() );
    }  
  draw();  
  };

/*--------------------------------------------------------------------*/

  void SeeEditor::remove_all()
  {
  while( va.count() )
    {
    remove_line( 0 );
    mod = 1;
    }
  };

/*--------------------------------------------------------------------*/
  
  void SeeEditor::remove_trails( int n ) /* remove trailing spaces/tabs */
  {
  if ( n != -1 )
    {
    ASSERT( sv.max() == va.count() - 1 );
    if ( n < 0 || n > sv.max() ) return;
    VString str = va[n];
    str_cut_right( str, " \t\n\r" );
    va.set( n, str );
    }
  else  
  for ( int z = 0; z < va.count(); z++ )
    {
    VString str = va[z];
    str_cut_right( str, " \t\n\r" );
    va.set( z, str );
    }
  };
  
/*--------------------------------------------------------------------*/

  void SeeEditor::insert_pipe_cmd()
  {
  VString sss = "Command to pipe in: ";
  int ii = str_len( sss )+2;
  status( sss );
  sss = opt->last_pipe_cmd;
  if(!TextInput( opt->xmin+ii, opt->ymax, "", opt->xmax-ii-4, opt->xmax-ii-4, &sss ))
    {
    draw();
    return;
    }
  str_sleft( sss, MAX_SEARCH_LEN );
  strcpy( opt->last_pipe_cmd, sss );
  FILE* f = popen( opt->last_pipe_cmd, "r" );
  if ( !f )
    {
    status( "Command execution failed..." );
    status( opt->last_pipe_cmd );
    return;
    }
  char ch;
  freezed = 1;
  while( (ch = fgetc( f ) ) != EOF )
    kinsert( ch );
  freezed = 0;
  pclose( f );
  draw();
  };

/*--------------------------------------------------------------------*/

  int SeeEditor::find_next()
  {
  if ( opt->last_search[0] == 0 )
    {
    status( "No pattern" );
    return 0;
    }
  
  int z;
  int pos = -1;
  for ( z = sv.pos() + 1; z <= sv.max(); z++ )
    {
    VString str = va[z];
    if ( opt->no_case )
      str_up( str );
    if ( opt->last_search[0] == '~' )  
      pos = str_find_regexp( str, opt->last_search + 1 );
    else if ( str[0] == '\\' )
      pos = str_find( str, opt->last_search + 1 );
    else  
      pos = str_find( str, opt->last_search );
    if ( pos != -1 )  
      break;
    }
  if ( pos != -1 )  
    {
    sv.go( z );
    col = pos;
    if (SEEDCOL > cols)
      {
      colpage = col - cols/2;
      if (colpage < 0) colpage = 0;
      }
    do_draw = 1;
    return 1;
    }
  else
    {
    status( "Pattern not found" );
    return 0;
    }  
  };

/*--------------------------------------------------------------------*/

  int SeeEditor::find( int no_case )
  {
  VString sss;
  sprintf( sss, "Find %s: ", no_case?"(no case)":"(case sense)");
  status( sss );
  int ii = str_len(sss)+2;
  sss = opt->last_search;
  if(!TextInput( opt->xmin+ii, opt->ymax, "", opt->xmax-ii-4, opt->xmax-ii-4, &sss ))
    {
    draw();
    return 1;
    }
  str_sleft( sss, MAX_SEARCH_LEN );
  strcpy( opt->last_search, sss );
  opt->no_case = no_case;
  if ( opt->no_case )
    str_up( opt->last_search );
  return find_next();
  };

/*--------------------------------------------------------------------*/

  void SeeEditor::help()
  {
  con_out( 1, 1, help_str );
  do_draw = 1;
  con_getch();
  };

/*--------------------------------------------------------------------*/

  int SeeEditor::run()
  {
  con_cshow();
  draw();
  set_cursor();

  int key;
  int pend = 0; /* used for double key-strokes as ^K^x command */
  while(4)
    {
    int ox  = SEEDCOL;
    int oy  = SEEDROW;
    int orp = sv.page();
    int ocp = colpage;
    int oi  = opt->insert;

    pend = 0;
    key = con_getch();
    if (key == KEY_CTRL_C)
      {
      mod = 0; /* it is `quit' i.e. no save so this should be ok */
      return key;
      }
    if (key == KEY_CTRL_X)
      {
        save();
        return key;
      } else
    if (key == 27 || key == KEY_ALT_X)
      if ( request_quit() == 0 )
        return key;
      else
        continue;
    if ( key == KEY_CTRL_K )
      {
      pend = key;
      con_out( SEEDCOL, SEEDROW, "^K", opt->cs );
      set_cursor();
      key = con_getch();
      draw_line( sv.pos() );
      }

    switch( key )
      {
      case KEY_CTRL_N :
      case KEY_DOWN   : down(); break;
      case KEY_CTRL_P :
      case KEY_UP     : up(); break;
      case KEY_CTRL_B :
      case KEY_LEFT   : left(); break;
      case KEY_CTRL_F :
      case KEY_RIGHT  : right(); break;
      case KEY_CTRL_U : if ( pend == KEY_CTRL_K )
                          sv.home();
                        else
                          sv.ppage();
                        break;
      case KEY_PPAGE  : sv.ppage(); break;
      case KEY_CTRL_V : if ( pend == KEY_CTRL_K )
                          sv.end();
                        else
                          sv.npage();
                        break;
      case KEY_NPAGE  : sv.npage(); break;
      case KEY_CTRL_A :
      case KEY_HOME   : home(); break;
      case KEY_CTRL_E :
      case KEY_END    : end();  break;
      case KEY_INSERT : opt->insert = !opt->insert; break;

      case KEY_CTRL_Y : remove_line(); break;

      /* SeedKxxx functions are for KEYxxx handles */
      case KEY_ALT_H  :
      case KEY_F1     : help(); break;

      case KEY_CTRL_S :
      case KEY_F2     : save(); break;
      case KEY_CTRL_D : if ( pend == KEY_CTRL_K )
                          save();
                        else
                          kdel();
                        break;

      case KEY_ALT_F     : find( 1 ); break;
      case KEY_ALT_S     : find( 0 ); break;

      case KEY_ALT_G     :
      case KEY_F3        : find_next(); break;

      case KEY_DEL       : kdel(); break;
      
      #ifndef _TARGET_GO32_
      case KEY_BACKSPACE :
      #endif
      case KEY_CTRL_H    : kbs(); break;

      case 10            :
      case 13            : kenter(); break;
      
      case KEY_CTRL_L    : if ( opt->auto_size )
                              {
                              opt->xmin = 1;
                              opt->ymin = 1;
                              opt->xmax = con_max_x();
                              opt->ymax = con_max_y();
                              }
                           rows = opt->ymax - opt->ymin - (opt->status != 0) + 1;
                           cols = opt->xmax - opt->xmin + 1;
                           sv.set_pagesize( rows );
                           con_cs(); 
                           draw(); 
                           break;
      case KEY_CTRL_W    : insert_pipe_cmd(); break;

      case KEY_CTRL_T    : opt->auto_indent = !opt->auto_indent;
                           status( (opt->auto_indent) ? "AutoIndent ON" : "AutoIndent OFF" );
                           break;

      case KEY_ALT_0  :
      case KEY_ALT_1  :
      case KEY_ALT_2  :
      case KEY_ALT_3  :
      case KEY_ALT_4  :
      case KEY_ALT_5  :
      case KEY_ALT_6  :
      case KEY_ALT_7  :
      case KEY_ALT_8  :
      case KEY_ALT_9  : if (key == KEY_ALT_0) key = KEY_ALT_9+1;
                        return key;
      case 27         : return key;
      default         : kinsert( key ); break;

      };

      if ( do_draw || orp != sv.page() || ocp != colpage || oi != opt->insert )
        {
        draw();
        set_cursor();
        do_draw = 0;
        } 
      else if ( ox != SEEDCOL || oy != SEEDROW )
        {
        draw( -1 ); /* just update status line */
        set_cursor();
        }

    };
  };

/***eof****************************************************************/

