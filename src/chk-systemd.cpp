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

#include <iostream>
#include <vector>
#include <cassert>

#include "chk-systemd.h"

ChkBus::ChkBus() {

}

ChkBus::~ChkBus() {
  disconnect();
}

bool ChkBus::connect() {
  int status;
  errorMessage.clear();

  if (isConnected()) {
    disconnect();
  }

  status = sd_bus_open_system(&bus);

  if (status < 0) {
    setErrorMessage(status);
    disconnect();

    throw std::string(errorMessage);
  }

  return isConnected();
}

void ChkBus::disconnect() {
  if (bus != NULL) {
    sd_bus_unref(bus);
    bus = NULL;
  }
}

bool ChkBus::isConnected() {
  return bus == NULL ? false : true;
}

void ChkBus::setErrorMessage(int status) {
  errorMessage = (char *)ERR_PREFIX;

  if (status < 0) {
    status = -status;
  }

  errorMessage += strerror(status);
}

void ChkBus::setErrorMessage(const char *message) {
  errorMessage = (char *)ERR_PREFIX;
  errorMessage += message;
}

const char *ChkBus::getState(const char *name) {
  int status;
  const char *state;

  errorMessage.clear();

  if (!isConnected()) {
    connect();
  }

  sd_bus_message *busMessage = NULL;
  sd_bus_error error = SD_BUS_ERROR_NULL;

  status = sd_bus_call_method(
    bus,
    "org.freedesktop.systemd1",
    "/org/freedesktop/systemd1",
    "org.freedesktop.systemd1.Manager",
    "GetUnitFileState",
    &error,
    &busMessage,
    "s",
    name);

  if (status < 0) {
    setErrorMessage(error.message);
    goto finish;
  }

  status = sd_bus_message_read(busMessage, "s", &state);

  if (status < 0) {
    setErrorMessage(status);
    goto finish;
  }

  finish:
    sd_bus_error_free(&error);
    sd_bus_message_unref(busMessage);
    if (state < 0) {
      throw std::string(errorMessage);
    }

  return status < 0 ? NULL : strdup(state);
}

std::vector<UnitInfo *> ChkBus::getUnitFiles() {
  int status;
  const char *state;
  char *path;
  std::vector<UnitInfo *> units;

  sd_bus_message* busMessage = NULL;
  sd_bus_message* reply = NULL;
  sd_bus_error error = SD_BUS_ERROR_NULL;

  errorMessage.clear();

  if (!isConnected()) {
    connect();
  }

  status = sd_bus_message_new_method_call(
    bus,
    &busMessage,
    "org.freedesktop.systemd1",
    "/org/freedesktop/systemd1",
    "org.freedesktop.systemd1.Manager",
    "ListUnitFiles");

  if (status < 0) {
    setErrorMessage(status);
    goto finish;
  }

  status = sd_bus_call(bus, busMessage, 0, &error, &reply);

  if (status < 0) {
    setErrorMessage(error.message);
    goto finish;
  }

  status = sd_bus_message_enter_container(reply, SD_BUS_TYPE_ARRAY, "(ss)");

  if (status < 0) {
    setErrorMessage(status);
    goto finish;
  }

  while ((status = sd_bus_message_read(reply, "(ss)", &path, &state)) > 0) {
    UnitInfo *unit = new UnitInfo();

    unit->unitPath = strdup(path);
    unit->state = strdup(state);

    std::string p = std::string(unit->unitPath);
    unit->id = strdup(p.substr(p.find_last_of("/") + 1, p.length()).c_str());

    units.push_back(unit);

    path = NULL;
    state = NULL;
  }

  status = sd_bus_message_exit_container(reply);

  if (status < 0) {
    setErrorMessage(status);
    goto finish;
  }

  finish:
    sd_bus_error_free(&error);
    sd_bus_message_unref(busMessage);
    sd_bus_message_unref(reply);
    disconnect();

    if (status < 0) {
      throw std::string(errorMessage);
    }

  return units;
}

std::vector<UnitInfo *> ChkBus::getUnits() {
  int status;
  UnitInfo unit;
  std::vector<UnitInfo *> units;

  sd_bus_message* busMessage = NULL;
  sd_bus_message* reply = NULL;
  sd_bus_error error = SD_BUS_ERROR_NULL;

  errorMessage.clear();

  if (!isConnected()) {
    connect();
  }

  status = sd_bus_message_new_method_call(
    bus,
    &busMessage,
    "org.freedesktop.systemd1",
    "/org/freedesktop/systemd1",
    "org.freedesktop.systemd1.Manager",
    "ListUnits"
  );

  if (status < 0) {
    setErrorMessage(status);
    goto finish;
  }

  status = sd_bus_call(bus, busMessage, 0, &error, &reply);

  if (status < 0) {
    setErrorMessage(error.message);
    goto finish;
  }

  status = sd_bus_message_enter_container(reply, SD_BUS_TYPE_ARRAY, "(ssssssouso)");

  if (status < 0) {
    setErrorMessage(status);
    goto finish;
  }

  while ((status = busParseUnit(reply, &unit)) > 0) {
    UnitInfo *u = new UnitInfo();

    u->id = strdup(unit.id);
    u->description = strdup(unit.description);
    u->loadState = strdup(unit.loadState);
    u->subState = strdup(unit.subState);
    u->unitPath = strdup(unit.unitPath);

    try {
      u->state = getState(unit.id);
    } catch (std::string &err) {
      break;
    }

    units.push_back(u);
  }

  status = sd_bus_message_exit_container(reply);

  if (status < 0) {
    setErrorMessage(status);
    goto finish;
  }

  finish:
    sd_bus_error_free(&error);
    sd_bus_message_unref(busMessage);
    sd_bus_message_unref(reply);
    disconnect();

    if (status < 0) {
      throw std::string(errorMessage);
    }

  return units;
}

void ChkBus::freeUnitInfo(UnitInfo *unit) {
  free((void *)unit->id);
  free((void *)unit->unitPath);
  free((void *)unit->description);
  free((void *)unit->loadState);
  free((void *)unit->subState);
}

std::vector<UnitInfo *> ChkBus::getAllUnits() {
  std::vector<UnitInfo *> files;
  std::vector<UnitInfo *> units;

  try {
    files = getUnitFiles();
    units = getUnits();
  } catch(std::string &err) {
    throw err;
  }

  for (auto unit : units) {
    int idx = 0;
    bool found = false;
    for (auto file : files) {
      std::string uid(unit->id);
      std::string fid(file->id);

      if (uid.find(fid) == 0) {
        freeUnitInfo(files[idx]);

        files[idx]->id = unit->id;
        files[idx]->unitPath = unit->unitPath;
        files[idx]->description = unit->description;
        files[idx]->loadState = unit->loadState;
        files[idx]->subState = unit->subState;

        found = true;
        break;
      }
      idx++;
    }

    if (!found) {
      files.push_back(unit);
    } else {
      delete unit;
    }
  }

  units.clear();
  units.shrink_to_fit();
  return files;
}

void ChkBus::reloadDaemon() {
  int status;

  sd_bus_message* busMessage = NULL;
  sd_bus_error error = SD_BUS_ERROR_NULL;

  if (!isConnected()) {
    connect();
  }

  status  = sd_bus_message_new_method_call(
    bus,
    &busMessage,
    "org.freedesktop.systemd1",
    "/org/freedesktop/systemd1",
    "org.freedesktop.systemd1.Manager",
    "Reload"
  );

  if (status < 0) {
     setErrorMessage(status);
     goto finish;
  }

  sd_bus_call(bus, busMessage, 1000, &error, NULL);

  finish:
    sd_bus_error_free(&error);
    sd_bus_message_unref(busMessage);
    disconnect();

    if (status < 0) {
      throw std::string(errorMessage);
    }
}

void ChkBus::applyUnitState(const char *method, char **names, int flags) {
  int status;
  bool checkState = false;

  sd_bus_error error = SD_BUS_ERROR_NULL;
  sd_bus_message *busMessage = NULL;

  if (!isConnected()) {
    connect();
  }

  status = sd_bus_message_new_method_call(
    bus,
    &busMessage,
    "org.freedesktop.systemd1",
    "/org/freedesktop/systemd1",
    "org.freedesktop.systemd1.Manager",
    method);

  if (status < 0) {
    setErrorMessage(status);
    goto finish;
  }

  status = sd_bus_message_append_strv(busMessage, names);

  if (status < 0) {
    setErrorMessage(status);
    goto finish;
  }

  switch(flags) {
    case STATE_FLAGS_ENABLE:
      status = sd_bus_message_append(busMessage, "bb", true, true);
      break;
    case STATE_FLAGS_DISABLE:
      status = sd_bus_message_append(busMessage, "b", true);
      checkState = true;
      break;
    case STATE_FLAGS_DISABLE_ISO:
      status = sd_bus_message_append(busMessage, "b", false);
      checkState = true;
      break;
    default:
      break;
  }

  if (status < 0) {
    setErrorMessage(status);
    goto finish;
  }

  status = sd_bus_call(bus, busMessage, 0, &error, NULL);

  if (status < 0) {
    setErrorMessage(error.message);
    goto finish;
  }

  if (flags == STATE_FLAGS_ENABLE) {
    applySYSv("enable", (const char **)names);
  } else if (flags == STATE_FLAGS_DISABLE) {
    applySYSv("disable", (const char **)names);
  }

  finish:
    sd_bus_error_free(&error);
    sd_bus_message_unref(busMessage);
    disconnect();

    if (status < 0) {
      throw std::string(errorMessage);
    }

  if (checkState) {
    checkDisabledStatus(names);
  }
}

void ChkBus::checkDisabledStatus(char **names) {
  try {
    for (int i = 0; names[i] != NULL; i++) {
      const char *name = names[i];
      const char *state = getState(name);

      if (std::string(state).find("enabled") == 0) {
        applyUnitState("DisableUnitFiles", names, STATE_FLAGS_DISABLE_ISO);
        applyUnitState("DisableUnitFiles", names, STATE_FLAGS_DISABLE);
      }

      if (state != NULL) {
        free((void *)state);
      }
    }
  } catch(std::string &err) {
    throw err;
  }
}

void ChkBus::applyUnitSub(const char *name, const char *method) {
  int status;

  sd_bus_error error = SD_BUS_ERROR_NULL;
  sd_bus_message *busMessage = NULL;
  sd_bus_message *reply = NULL;

  if (!isConnected()) {
    connect();
  }

  status = sd_bus_call_method(
    bus,
    "org.freedesktop.systemd1",
    "/org/freedesktop/systemd1",
    "org.freedesktop.systemd1.Manager",
    method,
    &error,
    &reply,
    "ss",
    name,
    "replace-irreversibly");

  if (status < 0) {
    setErrorMessage(error.message);
    goto finish;
  }

  finish:
    sd_bus_error_free(&error);
    sd_bus_message_unref(busMessage);
    disconnect();

    if (status < 0) {
      throw std::string(errorMessage);
    }
}

void ChkBus::enableUnits(std::set<std::string> *ids) {
  int i = 0;

  if (ids->size() < 1) {
    return;
  }

  char *names[ids->size()];

  for (auto id : (*ids)) {
    const char *name = id.c_str();
    char *copy = new char[strlen(name)+ 1];
    strcpy(copy, name);
    names[i] =copy;
    i++;
  }
  names[i] = NULL;

  try {
    applyUnitState("EnableUnitFiles", names, STATE_FLAGS_ENABLE);
  } catch (std::string &err) {
    for (i; i < 0; --i)
	    delete names[i];
    throw err;
  }
  for (i; i < 0; --i)
	  delete names[i];
}

void ChkBus::disableUnits(std::set<std::string> *ids) {
  int i = 0;

  if (ids->size() < 1) {
    return;
  }

  char *names[ids->size()];

  for (auto id : (*ids)) {
    const char *name = id.c_str();
    char *copy = new char[strlen(name)+ 1];
    strcpy(copy, name);
    names[i] =copy;
    i++;
  }

  names[i] = NULL;

  try {
    applyUnitState("DisableUnitFiles", names, STATE_FLAGS_DISABLE);
  } catch (std::string &err) {
    for (i; i < 0; --i)
	    delete names[i];
    throw err;
  }
  for (i; i < 0; --i)
	  delete names[i];
}

void ChkBus::enableUnit(const char *name) {
  try {
    std::set<std::string> id;
    id.insert(name);
    enableUnits(&id);
  } catch (std::string &err) {
    throw err;
  }
}

void ChkBus::disableUnit(const char *name) {
  try {
    std::set<std::string> id;
    id.insert(name);
    disableUnits(&id);
  } catch (std::string &err) {
    throw err;
  }
}

void ChkBus::stopUnits(std::set<std::string> *ids) {
  try {
    for (auto id : (*ids)) {
      applyUnitSub(id.c_str(), "StopUnit");
    }
  } catch (std::string &err) {
    throw err;
  }
}

void ChkBus::startUnits(std::set<std::string> *ids) {
  try {
    for (auto id : (*ids)) {
      applyUnitSub(id.c_str(), "StartUnit");
    }
  } catch (std::string &err) {
    throw err;
  }
}

void ChkBus::stopUnit(const char *name) {
  try {
    applyUnitSub(name, "StopUnit");
  } catch (std::string &err) {
    throw err;
  }
}

void ChkBus::startUnit(const char *name) {
  try {
    applyUnitSub(name, "StartUnit");
  } catch (std::string &err) {
    throw err;
  }
}
