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

#include "chk.h"
#include "chk-ui.h"
#include "chk-systemd.h"
#include <iostream>
#include <csignal>
#include <cstring>
#include <sstream>
#include <iomanip>

#include <boost/regex.hpp>

MainWindow::MainWindow() {
  setSize();

  padding->x = 2;
  padding->y = 2;
  searchPattern = NULL;
  showBackInstructions = false;
}

MainWindow::~MainWindow() {
  delwin(win);
}

void MainWindow::resize() {
  // Tear current window down
  endwin();
  wrefresh(stdscr);
  clear();
  delwin(win);

  // Bring up a new one
  setSize();
  createWindow();

  // This case handles resizing to smaller window
  // We need to make sure that currently selected line
  // will appear in newly created window.
  // If selected line number is greater than newly created window height,
  // we make this line the first line at the top of this new window.
  if ((selected + 1 + padding->x) >= screenSize->h) {
    start = start + selected;
    selected = 0;
  }

  drawUnits();
}


void MainWindow::createWindow() {
  win = newwin(screenSize->h, screenSize->w, 0, 0);
}

void MainWindow::createMenu() {
  createWindow();

  while(1) {
    drawUnits();

    int key = wgetch(stdscr);
    error(NULL);

    switch(key) {
      case 'k':
      case KEY_UP:
        moveUp();
        break;
      case 'j':
      case KEY_DOWN:
        moveDown();
        break;
      case 'f':
      case KEY_NPAGE:
        movePageDown();
        break;
      case 'b':
      case KEY_PPAGE:
        movePageUp();
        break;
      case 'q':
        stopCurses();
        delwin(win);
        exit(0);
        break;
      case ' ':
        toggleUnitState();
        break;
      case 's':
        toggleUnitSubState();
        break;
      case 'r':
        clearFilter();
        updateUnits();
        drawUnits();
        error((char *)"Updated..");
        break;
      case '?':
        aboutWindow(screenSize);
        break;
      case KEY_RESIZE:
        resize();
        break;
      case '/':
        searchWindow(screenSize, &searchPattern);
        // After filtering, service units list becomes list of units matching search pattern
        // To go back to full units list, user must press 'r'
        filterUnits();
        break;
      default:
        break;
    }
  }
}

void MainWindow::setSize() {
  getmaxyx(stdscr, screenSize->h, screenSize->w);
}

void MainWindow::moveUp() {
  int ps = winSize->h - (padding->y + 1);

  if (start > 0 && selected < ps / 2) {
    start--;
  } else if (selected > 0) {
    selected--;
  }

  if (units[start + selected]->id.size() == 0) {
    moveUp();
  }
}

#define MIN(a,b) (a<b) ? a : b;

void MainWindow::moveDown() {
  int offset = start + selected;
  int ps = winSize->h - (padding->y + 1);
  int max = units.size() - 1;

  if ((start + ps) < max) {
    if (selected < ps / 2) {
      selected++;
    } else {
      start++;
    }
  } else if (offset < max) {
    selected++;
  }

  if (offset >= max) {
    selected = MIN(max,ps);
  }

  if (units[start + selected]->id.size() == 0) {
    moveDown();
  }
}

void MainWindow::movePageUp() {
  int ps = winSize->h - (padding->y + 1);

  if (start > 0) {
    start -= ps;
  }

  if (start < 0) {
    start = 0;
    selected = 0;
  }

  if (units[start + selected]->id.size() == 0) {
    moveUp();
  }
}

void MainWindow::movePageDown() {
  int ps = winSize->h - 3;
  int max = units.size() - 1;

  if ((start + ps / 2) < max) {
    start += ps;
  }

  if ((start + ps) > max) {
    start = max - ps;
    selected = ps;
  }

  if (units[start + selected]->id.size() == 0) {
    moveDown();
  }
}

void MainWindow::reloadAll() {
  try {
    ctl->bus->reloadDaemon();
    updateUnits();
  } catch (std::string &err) {
    error((char *)err.c_str());
  }
}


void MainWindow::clearFilter()
{
  showBackInstructions = false;
  units.clear();
  units.shrink_to_fit();
}

void MainWindow::filterUnits()
{
  if (!searchPattern) {
    return;
  }

  std::vector<UnitItem *> tmpUnits;

  boost::regex expr(searchPattern);
  boost::smatch what;

  for (auto unit : units) {
    if (boost::regex_search(unit->id, what, expr)) {
      tmpUnits.push_back(unit);
    }
  }

  if (tmpUnits.empty()) {
    error((char *)"No matches found.");
  } else {
    units.clear();
    units.shrink_to_fit();

    /* Make units list the filtered units list */
    units = tmpUnits;

    start = 0;
    selected = 0;
    showBackInstructions = true;
  }

  free(searchPattern);
  searchPattern = NULL;
}

void MainWindow::updateUnits() {
  units.clear();
  units.shrink_to_fit();

  try {
    ctl->fetch();
    units = ctl->getItemsSorted();
  } catch(std::string &err) {
    error((char *)err.c_str());
  }

}

void MainWindow::drawUnits() {
  if (units.empty()) {
    updateUnits();
  }

  getmaxyx(win, winSize->h, winSize->w);
  winSize->h -= padding->y;

  for (int i = 0; i < (winSize->h - padding->y); i++) {
    if ((i + start) > (int)units.size() - 1) {
      break;
    }

    UnitItem *unit = units[start + i];

    if (i == selected) {
      wattron(win, A_REVERSE);
    }

    drawItem(unit, i + padding->y);
    wattroff(win, A_REVERSE);
  }

  drawInfo();

  refresh();
  wrefresh(win);
}


void MainWindow::drawItem(UnitItem *unit, int y) {
  if (unit->id.size() == 0) {
    if (unit->target.size() == 0) {
      printInMiddle(win, y, 0, winSize->w, (char *)"", COLOR_PAIR(3), (char *)' ');
    } else {
      std::string title(unit->target);
      title += "s";
      title[0] = std::toupper(title[0]);

      printInMiddle(win, y, 0, winSize->w, (char *)"", COLOR_PAIR(3), (char *)' ');
      printInMiddle(win, y, 0, winSize->w / 2, (char *)title.c_str(), COLOR_PAIR(3), (char *)' ');
    }
    return;
  }


//    if (unit->id.find("acpid.service") == 0) {
//      std::cout << unit->id << " " << unit->state << UNIT_STATE_DISABLED <<  std::endl;
//      exit(0);
//    }

  if (unit->state == UNIT_STATE_ENABLED) {
    wattron(win, COLOR_PAIR(2));
    mvwprintw(win, y, padding->x, "[x]");
    wattroff(win, COLOR_PAIR(2));
  } else if (unit->state == UNIT_STATE_DISABLED) {
    wattron(win, COLOR_PAIR(5));
    mvwprintw(win, y, padding->x, "[ ]");
    wattroff(win, COLOR_PAIR(5));
  } else if (unit->state == UNIT_STATE_STATIC) {
    wattron(win, COLOR_PAIR(5));
    mvwprintw(win, y, padding->x, "[s]");
    wattroff(win, COLOR_PAIR(5));
  } else if (unit->state == UNIT_STATE_BAD) {
    wattron(win, COLOR_PAIR(1));
    mvwprintw(win, y, padding->x, "-b-");
    wattroff(win, COLOR_PAIR(1));
  } else if (unit->state == UNIT_STATE_MASKED) {
    wattron(win, COLOR_PAIR(3));
    mvwprintw(win, y, padding->x, "-m-");
    wattroff(win, COLOR_PAIR(3));
  }

  if (unit->sub == UNIT_SUBSTATE_RUNNING) {
    wattron(win, COLOR_PAIR(3));
    mvwprintw(win, y, padding->x + 3, "  >  ");
    wattroff(win, COLOR_PAIR(3));
  } else if (unit->sub == UNIT_SUBSTATE_CONNECTED) {
    wattron(win, COLOR_PAIR(5));
    mvwprintw(win, y, padding->x + 3, "  =  ");
    wattroff(win, COLOR_PAIR(5));
  } else {
    wattron(win, COLOR_PAIR(5));
    mvwprintw(win, y, padding->x + 3, "     ");
    wattroff(win, COLOR_PAIR(5));
  }

  unsigned int leftPad = padding->x + 8;
  unsigned int rightPad = (winSize->w - leftPad);

  if (unit->id.size() > (rightPad - padding->x)) {
    unit->id.resize(rightPad - padding->x);
  }

  std::stringstream sline;

  unit->description.resize(winSize->w / 2, ' ');
  sline << std::string(unit->id.size(), ' ') << " "
    << std::setw(rightPad - unit->id.size())
    << unit->description;

  std::string cline(sline.str());
  std::string name(unit->id);

  name.resize(cline.find_first_of(unit->description[0]), ' ');

  if (cline.size() > rightPad) {
    cline.resize(rightPad - 2 );
  }

  wattron(win, COLOR_PAIR(4));
  mvwprintw(win, y, leftPad, "%s", cline.c_str());
  wattroff(win, COLOR_PAIR(4));
  mvwprintw(win, y, leftPad, "%s", name.c_str());
}

void MainWindow::drawInfo() {
  int count = 0;
  int countUntilNow = start + selected;

  for (auto unit : units) {
    if (unit->id.size() == 0) {
      if (countUntilNow > count) {
        countUntilNow--;
      }
      continue;
    }

    count++;
  }

  std::stringstream position;

  position << countUntilNow + 1 << "/" << count;

  printInMiddle(win, winSize->h + 1, 0, winSize->w, (char *)"", COLOR_PAIR(5), (char *)' ');
  printInMiddle(win, winSize->h + 1, 0, (winSize->w / 2), (char *)position.str().c_str(), COLOR_PAIR(5), (char *)NULL);

  wattron(win, COLOR_PAIR(4));
  mvwprintw(win, winSize->h + 1, winSize->w - 22, "? - help, / - search");
  wattroff(win, COLOR_PAIR(4));

  if (showBackInstructions) {
    // Remind user to press 'r' to return to full services list
    error ((char *)"Press 'r' to return to full services list");
  }
}

void MainWindow::error(char *err) {
  // Make error message a bit more visible
  wattron(win, COLOR_PAIR(2));
  mvwprintw(win, 0, 0, std::string(winSize->w, ' ').c_str());

  if (err) {
    mvwprintw(win, 0, 1, err);
  }
  wattroff(win, COLOR_PAIR(2));
}

void MainWindow::toggleUnitState() {
  try {
    ctl->toggleUnitState(units[start + selected]);
    reloadAll();
  } catch (std::string &err) {
    error((char *)err.c_str());
  }
}

void MainWindow::toggleUnitSubState() {
  try {
    ctl->toggleUnitSubState(units[start + selected]);
    reloadAll();
  } catch (std::string &err) {
    error((char *)err.c_str());
  }
}
