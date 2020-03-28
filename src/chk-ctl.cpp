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

#include <algorithm>

#include "chk-ctl.h"
#include "chk-systemd.h"

ChkCTL::ChkCTL() {
  bus = new ChkBus();
  items.clear();
}

ChkCTL::~ChkCTL() {
  delete bus;
  items.clear();
}

std::vector<UnitItem *> ChkCTL::getItems() {
  sortByName(&items);
  return items;
}

std::vector<UnitItem *> ChkCTL::getByTarget(const char *target) {
  std::vector<UnitItem *> found;
  std::string pattern = target == NULL ? "" : target;

  for (UnitItem *item : items) {
    if (item->target.find(pattern) == 0) {
      found.push_back(item);
    }
  }

  return found;
}

void ChkCTL::fetch() {
  std::vector<UnitInfo *> sysUnits;

  for (auto item : items) {
    delete item;
  }

  items.clear();
  items.shrink_to_fit();

  try {
    sysUnits = bus->getAllUnits();
  } catch (std::string &err) {
    throw err;
  }

  for (auto unit : sysUnits) {
    if (unit->id) {
      pushItem(unit);
    }
    delete unit;
  }

  sysUnits.clear();
  sysUnits.shrink_to_fit();
}

void ChkCTL::sortByName(std::vector<UnitItem *> *sortable) {
  std::sort(sortable->begin(), sortable->end(), [](const UnitItem *a, const UnitItem *b) {
    const char* s1 = a->id.c_str();
    const char* s2 = b->id.c_str();
    while(true) {
       if ( std::toupper(*s1) < std::toupper(*s2) ) return true;
       if ( std::toupper(*s1) > std::toupper(*s2) ) return false;
       if ( *s1 == 0 && *s2 == 0 ) return false;
       if ( *s1 > *s2) return false;
       if ( *s1 < *s2) return true;
       ++s1; ++s2;
    }
  });
}

void ChkCTL::pushItem(UnitInfo *unit) {
  UnitItem *item = new UnitItem();

  std::string id(unit->id);

  item->id = id;
  item->target = id.substr(id.find_last_of('.') + 1, id.length());
  item->location = std::string(unit->unitPath);

  if (unit->description != NULL) {
    item->description = unit->description;
    item->editable = false;
  } else {
    item->description = unit->unitPath;
    item->editable = true;
  }

  if (unit->state != NULL) {
    std::string state(unit->state);
    std::string sub(unit->subState == NULL ? "" : unit->subState);

    if (state.find("enabled") == 0) {
      item->state = UNIT_STATE_ENABLED;
    } else if (state.find("mask") == 0) {
      item->state = UNIT_STATE_MASKED;
    } else if (state.find("static") == 0) {
      item->state = UNIT_STATE_STATIC;
    } else if (state.find("bad") == 0 || state.find("removed") == 0) {
      item->state = UNIT_STATE_BAD;
    } else {
      item->state = UNIT_STATE_DISABLED;
    }

    if (!sub.empty()) {
      if (sub.find("running") == 0) {
        item->sub = UNIT_SUBSTATE_RUNNING;
      } else {
        item->sub = UNIT_SUBSTATE_CONNECTED;
      }
    } else {
        item->sub = UNIT_SUBSTATE_INVALID;
    }

  } else {
    item->state = UNIT_STATE_MASKED;
  }

  bus->freeUnitInfo(unit);

  items.push_back(item);
};

std::vector<UnitItem *> ChkCTL::getItemsSorted() {
  std::vector<std::string> orderedTargets;
  std::vector<UnitItem *> sunits;
  bool alreadyHasTarget = false;

  orderedTargets.push_back("service");
  orderedTargets.push_back("timer");
  orderedTargets.push_back("socket");

  for (const auto unit : items) {
    alreadyHasTarget = false;
    for (auto t : orderedTargets) {
      if (t.find(unit->target) == 0) {
        alreadyHasTarget = true;
        break;
      }
    }
    if (!alreadyHasTarget) {
      orderedTargets.push_back(unit->target);
    }
  }

  bool isFirst = false;

  for (std::string target : orderedTargets) {
    auto targetedUnits = getByTarget(target.c_str());
    sortByName(&targetedUnits);

    if (isFirst) {
      UnitItem *separatorTitle = new UnitItem();
      UnitItem *separator = new UnitItem();
      separatorTitle->target = target;

      sunits.push_back(separator);
      sunits.push_back(separatorTitle);
      sunits.push_back(separator);
    }

    sunits.insert(sunits.end(), targetedUnits.begin(), targetedUnits.end());
    isFirst = true;
  }

  return sunits;
}

void ChkCTL::toggleUnitState(UnitItem *item) {
  try {
    if (item->state == UNIT_STATE_ENABLED || item->state == UNIT_STATE_STATIC) {

      if (item->sub == UNIT_SUBSTATE_RUNNING || item->sub == UNIT_SUBSTATE_CONNECTED) {
        bus->stopUnit(item->id.c_str());
      }
      bus->disableUnit(item->id.c_str());

    } else if (item->state == UNIT_STATE_DISABLED) {
      bus->enableUnit(item->id.c_str());
    }

    item->state = UNIT_STATE_TMP;
  } catch (std::string &err) {
    throw err;
  }
}

void ChkCTL::toggleUnitSubState(UnitItem *item) {
  try {
    if (item->sub != UNIT_SUBSTATE_RUNNING) {
      bus->startUnit(item->id.c_str());
    } else {
      bus->stopUnit(item->id.c_str());
    }

    item->sub = UNIT_SUBSTATE_TMP;
  } catch (std::string &err) {
    throw err;
  }
}
