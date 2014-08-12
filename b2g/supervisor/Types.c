/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <sys/reboot.h>
#include <dlfcn.h>

/* from b2g/supervisor/include */
#include <ipc/Types.h>
#include <ipc/Message.h>

#ifdef DEBUG
#include <stdio.h>
#endif

void
HandleCmdReboot(struct SvMessage* aMsg)
{
#ifdef DEBUG
  printf("Got reboot command\n");
#endif
  int32_t cmd = 0;
  uint32_t len = 0;
  void* iter = (void*)aMsg->data;

  enum SvTypeError err = SV_ERROR_OK;

  if (ReadInt(iter, (uint32_t*)&cmd, &len) < 0) {
    err = SV_ERROR_FAILED;
    goto respond;
  }

  if (len != sizeof(uint32_t)) {
    err = SV_ERROR_INVALID;
    goto respond;
  }

#ifdef DEBUG
  printf("Command: %d\n", cmd);
#endif

  if (cmd != RB_AUTOBOOT &&
      cmd != RB_POWER_OFF) {
    err = SV_ERROR_DENIED;
    goto respond;
  }

  // TODO: maybe some cleanup function?
  reboot(cmd);

respond:
  // TODO: respond
  return;
}

void
HandleCmdWifi(struct SvMessage* aMsg)
{
#ifdef DEBUG
  printf("Got wifi command\n");
#endif
  static void* wifilib = NULL;

  uint32_t length = 0;
  char* cmd = NULL;

  enum SvTypeError err = SV_ERROR_OK;

  void* iter = (void*)aMsg->data;

  if (ReadString(iter, &cmd, &length) < 0) {
    err = SV_ERROR_FAILED;
    goto respond;
  }

  if (length == 0) {
    err = SV_ERROR_MISSING;
    goto respond;
  } else if (length > aMsg->header.size) {
    err = SV_ERROR_INVALID;
    goto respond;
  }

#ifdef DEBUG
  printf("Command: %s\n", cmd);
#endif

  /* |cmd| contains a valid string */
  if (strcmp(cmd, "wifi_load_driver")) {
    err = SV_ERROR_DENIED;
    goto respond;
  }

  /* |cmd| is in whitelist get function and call it */
  if (!wifilib) {
    wifilib = dlopen("/system/lib/libhardware_legacy.so", RTLD_LAZY);
    if (!wifilib) {
      err = SV_ERROR_FAILED;
      goto respond;
    }
  }

  int (*func)();
  func = (int (*)())dlsym(wifilib, cmd);

  if (dlerror() != NULL) {
    err = SV_ERROR_FAILED;
    goto respond;
  }

  if ((*func)() < 0) {
    err = SV_ERROR_FAILED;
    goto respond;
  }

respond:
  // TODO: send respond message
  return;
}

void
HandleActionCmd(struct SvMessage* aMsg)
{
#ifdef DEBUG
  printf("Got command: %d\n", aMsg->header.opt);
#endif
  switch(aMsg->header.opt) {
    case SV_CMD_REBOOT: HandleCmdReboot(aMsg); break;
    case SV_CMD_WIFI: HandleCmdWifi(aMsg); break;
  }
}
