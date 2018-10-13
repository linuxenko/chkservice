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

#ifndef _CHK_UI_H
#define _CHKUI_H

#include <curses.h>
#include <form.h>
#include "chk-ctl.h"

typedef struct RECTANGLE {
  int x;
  int y;
  int w;
  int h;
} RECTANGLE;

class MainWindow {
  public:
    WINDOW *win;
    MainWindow();
    ~MainWindow();
    void createMenu();
  private:
    RECTANGLE *screenSize = new RECTANGLE();
    RECTANGLE *winSize = new RECTANGLE();
    RECTANGLE *padding = new RECTANGLE();
    ChkCTL *ctl = new ChkCTL;
    std::vector<UnitItem *> units;

    char *searchPattern;
    // Defines whether instructions on how to go back to full services list
    // must be displayed
    bool showBackInstructions;

    int selected = 0;
    int start = 0;
    void createWindow();
    void resize();
    void setSize();
    void moveUp();
    void moveDown();
    void movePageUp();
    void movePageDown();
    void drawUnits();

    void filterUnits();
    void clearFilter();

    void drawItem(UnitItem *unit, int y);
    void drawInfo();
    void toggleUnitState();
    void toggleUnitSubState();
    void updateUnits();
    void error(char *err);
    void reloadAll();
};

void startCurses();
void stopCurses();
void printInMiddle(WINDOW *win, int starty, int startx, int width,
    char *string, chtype color, char *sp);
void aboutWindow(RECTANGLE *parent);
void searchWindow(RECTANGLE *parent, char **searchText);

#endif
