/*
 *    chkservice is a tool for managing systemd units.
 *    more infomration at https://github.com/linuxenko/chkservice
 *
 *    Copyright (C) 2017 Svetlana Linuxenko
 *
 *    chkservice program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    chkservice program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "chk-ui.h"
#include "chk.h"

#include <string.h>
#include <stdlib.h>

#include <assert.h>

void startCurses() {
  initscr();
  start_color();
  noecho();
  cbreak();
  curs_set(false);

  init_pair(1, COLOR_RED, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
  init_pair(3, COLOR_BLUE, COLOR_BLACK);
  init_pair(4, COLOR_CYAN, COLOR_BLACK);
  init_pair(5, COLOR_MAGENTA, COLOR_BLACK);

  keypad(stdscr, true);
}

void stopCurses() {
  endwin();
}

void printInMiddle(WINDOW *win, int starty, int startx, int width,
    char *string, chtype color, char *sp) {

  int length, x, y;
  float temp;

  if(win == NULL)
    win = stdscr;
  getyx(win, y, x);
  if(startx != 0)
    x = startx;
  if(starty != 0)
    y = starty;
  if(width == 0)
    width = 80;

  length = strlen(string);
  temp = (width - length)/ 2;
  x = startx + (int)temp;
  wattron(win, color);

  if (sp) {
    for (int i = startx; i < width; i++) {
      mvwprintw(win, y, i, "%c", sp);
    }
  }

  mvwprintw(win, y, x, "%s", string);
  wattroff(win, color);
}

void aboutWindow(RECTANGLE *parent) {
  const int winH = 22;
  const int winW = 60;

  WINDOW *aboutwin = newwin(winH, winW,
      (parent->h / 2) - (winH / 2),
      (parent->w / 2) - (winW / 2)
      );

  wattron(aboutwin, COLOR_PAIR(2));
  mvwprintw(aboutwin, 0, 0, ABOUT_INFO, VERSION);
  wattroff(aboutwin, COLOR_PAIR(2));
  refresh();
  wrefresh(aboutwin);
  getch();
  delwin(aboutwin);
  clear();
  refresh();
}

// Following function was retrieved from:
// https://alan-mushi.github.io/2014/11/30/ncurses-forms.html

/*
 * This is useful because ncurses fill fields blanks with spaces.
 */
static char* trim_whitespaces(char *str)
{
  char *end;

  // trim leading space
  while(isspace(*str))
    str++;

  if(*str == 0) // all spaces?
    return str;

  // trim trailing space
  end = str + strnlen(str, 128) - 1;

  while(end > str && isspace(*end))
    end--;

  // write new null terminator
  *(end+1) = '\0';

  return str;
}

#define KEY_RETURN  10
#define KEY_ESCAPE  27
void searchWindow(RECTANGLE *parent, char **searchText) {
  FORM *searchForm;
  FIELD *searchField[2];
  WINDOW *searchWinBody, *searchWin;
  int done = 0;
  int ch;

  const int winH = 10;
  const int winW = 60;

  const int searchWinH = (parent->h / 2) - (winH / 2);
  const int searchWinW = (parent->w / 2) - (winW / 2);

  searchWinBody = newwin(winH, winW,
      searchWinH,
      searchWinW
      );
  assert(searchWinBody != NULL);

  const int winFormNLines = winH - 4;
  const int winFormNCols = winW - 2;
  const int winFormY = 3;
  const int winFormX = 1;

  searchWin = derwin(searchWinBody,
        winFormNLines, winFormNCols,
        winFormY,
        winFormX);
  assert(searchWin != NULL);

  wattron(searchWinBody, COLOR_PAIR(2));
  mvwprintw(searchWinBody, 1, 2, SEARCH_FORM_INFO);
  wattroff(searchWinBody, COLOR_PAIR(2));

  const int searchFieldHeight = 1;
  const int searchFieldWidth = winFormNCols - 2;
  searchField[0] = new_field(searchFieldHeight, searchFieldWidth, 0, 0, 0, 0);
  searchField[1] = NULL;
  assert(searchField[0] != NULL);

  set_field_fore(searchField[0], COLOR_PAIR(2));
  set_field_back(searchField[0], A_UNDERLINE);

  searchForm = new_form(searchField);
  assert(searchForm != NULL);
  set_form_win(searchForm, searchWin);
  set_form_sub(searchForm, derwin(searchWin,
        winFormNLines - 2, winFormNCols - 2,
        1, 1));
  post_form(searchForm);

  /* show cursor */
  curs_set(true);

  refresh();
  wrefresh(searchWinBody);
  wrefresh(searchWin);

  while(!done)
  {
    ch = getch();
    switch(ch) {
      case KEY_RETURN:
        form_driver(searchForm, REQ_VALIDATION);
        *searchText = strdup(trim_whitespaces(field_buffer(searchField[0], 0)));
        done = 1;
        break;

      case KEY_ESCAPE:
      case KEY_F(10):
        done = 1;
        break;

      case KEY_BACKSPACE:
        form_driver(searchForm, REQ_DEL_PREV);
        break;

      case KEY_HOME:
        form_driver(searchForm, REQ_BEG_LINE);
        break;

      case KEY_END:
        form_driver(searchForm, REQ_END_LINE);
        break;

      case KEY_LEFT:
        form_driver(searchForm, REQ_PREV_CHAR);
        break;

      case KEY_RIGHT:
        form_driver(searchForm, REQ_NEXT_CHAR);
        break;

      default:
        form_driver(searchForm, ch);
        break;
    }

    wrefresh(searchWin);
  }


  unpost_form(searchForm);
  free_form(searchForm);
  free_field(searchField[0]);
  delwin(searchWin);
  delwin(searchWinBody);

  curs_set(false);
  clear();
  refresh();
}
