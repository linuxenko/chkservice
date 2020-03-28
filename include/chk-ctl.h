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

#ifndef _CHK_CTL_H
#define _CHK_CTL_H

#include "chk-systemd.h"

typedef struct UnitItem {
  std::string id;
  std::string target;
  std::string description;
  std::string location;
  bool editable;
  int sub;
  int state;
} UnitItem;

enum {
  UNIT_STATE_DISABLED = 0x01,
  UNIT_STATE_ENABLED = 0x02,
  UNIT_STATE_STATIC = 0x03,
  UNIT_STATE_BAD = 0x04,
  UNIT_STATE_MASKED = 0x05,
  UNIT_STATE_TMP = 0x06
};

enum {
  UNIT_SUBSTATE_RUNNING = 0x1a,
  UNIT_SUBSTATE_CONNECTED = 0x2a,
  UNIT_SUBSTATE_INVALID = 0x3a,
  UNIT_SUBSTATE_TMP = 0x4a
};

class ChkCTL {
  public:
    ChkCTL();
    ~ChkCTL();
    ChkBus *bus;
    std::vector<UnitItem *> getItemsSorted();
    std::vector<UnitItem *> getByTarget(const char *target);
    std::vector<UnitItem *> getItems();
    void toggleUnitState(UnitItem *item);
    void toggleUnitSubState(UnitItem *item);
    void fetch();
  private:
    std::vector<UnitItem *> items;
    void pushItem(UnitInfo *unit);
    void sortByName(std::vector<UnitItem *> *sortable);
};

#endif
