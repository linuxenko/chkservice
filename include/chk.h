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

#ifndef _CHK_H
#define _CHK_H

#define VERSION "0.3"

#define ABOUT_INFO "\n\
                    chkservice %s  \n\
\n\
  Units list information:\n\
\n\
    [x] - enabled unit.  [ ] - disabled unit \n\
    [s] - static unit.   -m- - masked unit \n\
\n\
  Navigation keys:\n\
\n\
    Up/k   - move cursor up. Down/j   - move cursor down.\n\
    PgUp/b - move page up.   PgDown/f - move page down.\n\
    / - for search.\n\
\n\
  Action keys:\n\
\n\
    r     - reload/update.   q - exit.\n\
    Space - enable/disable.  s - start/stop unit.\n\
    e - edit unit\n\
\n\
  License:\n\
    GPLv3 (c) Svetlana Linuxenko"

#endif
