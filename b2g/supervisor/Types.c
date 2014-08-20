/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <sys/resource.h>
#include <sys/reboot.h>
#include <dlfcn.h>
#include <string.h>

/* from b2g/supervisor/include */
#include <ipc/Types.h>
#include <ipc/Message.h>
#include <ipc/Channel.h>

#ifdef DEBUG
#include <stdio.h>
#endif

#define BUFFER_SIZE 2048

int32_t
SendErrorResponse(uint32_t aId, enum SvTypeError aErr, const char* aStr)
{
  struct SvMessage* msg = NULL;
  int32_t ret = -1;
  uint32_t size = 0;
  uint32_t length = 0;
  size += sizeof(struct SvMessage);

  if (aStr) {
    length = strlen(aStr) + 1;
    size += length;
  }

  msg = (struct SvMessage*)calloc(1, size);
  if (!msg) {
    return -1;
  }

  msg->header.id = aId;
  msg->header.type = SV_TYPE_ERROR;
  msg->header.opt = aErr;
#ifdef DEBUG
  printf("Error Response for: 0x%08x, opt: %d, str: %s\n", aId, aErr, aStr ? aStr : "");
#endif

  if (!aStr || length < 2) {
    ret = ChannelWrite(msg);
    goto exit_free;
  }

  void* iter = (void*)msg->data;
  if (WriteString(&iter, aStr, length) < 0) {
    ret = -1;
    goto exit_free;
  }

  msg->header.size = length;
  ret = ChannelWrite(msg);

exit_free:
  free(msg);
  return ret;
}

int32_t
SendResponse(uint32_t aId, uint32_t aOpt, void* aData, uint32_t aSize)
{
  int32_t ret = -1;
  struct SvMessage* msg = NULL;
  uint32_t size = sizeof(struct SvMessage) + aSize;

  msg = (struct SvMessage*)calloc(1, size);
  if (!msg) {
    return -1;
  }
#ifdef DEBUG
  printf("Response for: 0x%08x, opt: %d\n", aId, aOpt);
#endif

  msg->header.id = aId;
  msg->header.type = SV_TYPE_RES;
  msg->header.opt = aOpt;

  if (aData || aSize) {
    memcpy(msg->data, aData, aSize);
    msg->header.size = aSize;
  }
#ifdef DEBUG
  printf("Sending message, id: 0x%08x, size: %d\n", msg->header.id, msg->header.size);
#endif

  ret = ChannelWrite(msg);

exit_free:
  free(msg);
  return ret;
}

void
HandleCmdReboot(struct SvMessage* aMsg)
{
#ifdef DEBUG
  printf("Got reboot command\n");
#endif
  int32_t cmd = 0;
  uint32_t len = 0;
  char* err_msg = NULL;
  void* iter = (void*)aMsg->data;

  enum SvTypeError err = SV_ERROR_OK;

  if (ReadInt(&iter, (uint32_t*)&cmd, &len) < 0) {
    err = SV_ERROR_FAILED;
    err_msg = "SV_CMD_REBOOT: failed to read message body";
    goto respond;
  }

  if (len != sizeof(uint32_t)) {
    err = SV_ERROR_INVALID;
    err_msg = "SV_CMD_REBOOT: argument is invalid";
    goto respond;
  }

#ifdef DEBUG
  printf("Command: %d\n", cmd);
#endif

  if (cmd != RB_AUTOBOOT &&
      cmd != RB_POWER_OFF) {
    err = SV_ERROR_DENIED;
    err_msg = "SV_CMD_REBOOT: argument not in whitelist";
    goto respond;
  }

  // TODO: maybe some cleanup function?
  reboot(cmd);
  return;

respond:
  SendErrorResponse(aMsg->header.id, err, err_msg);
  return;
}

void
HandleCmdWifi(struct SvMessage* aMsg)
{
#ifdef DEBUG
  printf("Got wifi command\n");
#endif
  static void* wifilib = NULL;

  int32_t buffer_len = BUFFER_SIZE - 1;
  char buffer[BUFFER_SIZE] = {0};

  int32_t data_left = aMsg->header.size;
  uint32_t length = 0;
  char* cmd = NULL;
  char* err_msg = NULL;
  char* resp_data = NULL;

  enum SvTypeError err = SV_ERROR_OK;

  void* iter = (void*)aMsg->data;

  if (ReadString(&iter, &cmd, &length) < 0) {
    err = SV_ERROR_FAILED;
    err_msg = "SV_CMD_WIFI: failed to read message";

    SendErrorResponse(aMsg->header.id, err, err_msg);
    return;
  }

  data_left -= length;

#ifdef DEBUG
  printf("Command: %s\n", cmd);
#endif
  if (length == 0) {
    err = SV_ERROR_MISSING;
    err_msg = "SV_CMD_WIFI: missing wifi command";

    SendErrorResponse(aMsg->header.id, err, err_msg);
    return;
  } else if (data_left < 0) {
    err = SV_ERROR_INVALID;
    err_msg = "SV_CMD_WIFI: wifi command is invalid";

    SendErrorResponse(aMsg->header.id, err, err_msg);
    return;
  }

  if (!wifilib) {
    wifilib = dlopen("/system/lib/libhardware_legacy.so", RTLD_LAZY);
    if (!wifilib) {
      err = SV_ERROR_FAILED;
      err_msg = "SV_CMD_WIFI: failed to load library";

      SendErrorResponse(aMsg->header.id, err, err_msg);
      return;
    }
  }

  int32_t func_ret = -1;
  /* whitelist on commands  */
  if (!strcmp(cmd, "wifi_load_driver") ||
      !strcmp(cmd, "wifi_unload_driver")) {
    int32_t (*func)();
    func = (int32_t (*)())dlsym(wifilib, cmd);

    if (dlerror() != NULL) {
      err = SV_ERROR_FAILED;
      err_msg = "SV_CMD_WIFI: failed to load function";

      SendErrorResponse(aMsg->header.id, err, err_msg);
      return;
    }

    func_ret = (*func)();
    SendResponse(aMsg->header.id, func_ret, NULL, 0);
    return;
  } else if (
      !strcmp(cmd, "wifi_start_supplicant") ||
      !strcmp(cmd, "wifi_stop_supplicant")) {
    int32_t enable = -1;
    if (ReadInt(&iter, &enable, &length) < 0) {
      err = SV_ERROR_FAILED;
      err_msg = "SV_CMD_WIFI: failed to read int32_t argument";

      SendErrorResponse(aMsg->header.id, err, err_msg);
      return;
    }

    data_left -= length;

    if (data_left < 0) {
      err = SV_ERROR_INVALID;
      err_msg = "SV_CMD_WIFI: invalid data supplied";

      SendErrorResponse(aMsg->header.id, err, err_msg);
      return;
    }

    int32_t (*func)(int32_t);
    func = (int32_t (*)(int32_t))dlsym(wifilib, cmd);

    if (dlerror() != NULL) {
      err = SV_ERROR_FAILED;
      err_msg = "SV_CMD_WIFI: failed to load function";

      SendErrorResponse(aMsg->header.id, err, err_msg);
      return;
    }
    /* FIXME: can't let b2g know it succeeded, it won't boot if
       wifi_start_supplicant succeeded, probably tries to execute command
       which needs to be remoted */

    func_ret = (*func)(enable);
#ifndef DEBUG
    func_ret = -1;
#endif
    SendResponse(aMsg->header.id, func_ret, NULL, 0);
  } else if (!strcmp(cmd, "wifi_connect_to_supplicant")) {
      char* interface = NULL;
      if (ReadString(&iter, &interface, &length) < 0) {
        err = SV_ERROR_FAILED;
        err_msg = "SV_CMD_WIFI: failed to read string argument";

        SendErrorResponse(aMsg->header.id, err, err_msg);
        return;
      }

      data_left -= length;
      if (data_left < 0) {
        err = SV_ERROR_INVALID;
        err_msg = "SV_CMD_WIFI: invalid data supplied";

        SendErrorResponse(aMsg->header.id, err, err_msg);
        return;
      }

#ifdef DEBUG
      printf("Interface name: %s\n", interface);
#endif
      // TODO: whitelist on interface name?

      int32_t (*func)(const char*);
      func = (int32_t (*)(const char*))dlsym(wifilib, cmd);

      if (dlerror() != NULL) {
        err = SV_ERROR_FAILED;
        err_msg = "SV_CMD_WIFI: failed to load function";

        SendErrorResponse(aMsg->header.id, err, err_msg);
        return;
      }

      func_ret = (*func)(interface);
      SendResponse(aMsg->header.id, func_ret, NULL, 0);
  } else if (!strcmp(cmd, "wifi_command")) {
    char* interface = NULL;
    char* request = NULL;

    if (ReadString(&iter, &interface, &length) < 0) {
      err = SV_ERROR_FAILED;
      err_msg = "SV_CMD_WIFI: failed to read interface name";

      SendErrorResponse(aMsg->header.id, err, err_msg);
      return;
    }
#ifdef DEBUG
    printf("Interface: %s\nLength: %d\n", interface, length);
#endif

    data_left -= length;
    if (data_left < 0 || length <= 1) {
      err = SV_ERROR_INVALID;
      err_msg = "SV_CMD_WIFI: invalid string";

      SendErrorResponse(aMsg->header.id, err, err_msg);
      return;
    }

    if (ReadString(&iter, &request, &length) < 0) {
      err = SV_ERROR_FAILED;
      err_msg = "SV_CMD_WIFI: failed to read request string";

      SendErrorResponse(aMsg->header.id, err, err_msg);
      return;
    }
#ifdef DEBUG
    printf("Request: %s\nlength: %d\n", request, length);
#endif

    data_left -= length;
    if (data_left < 0 || length <= 1) {
      err = SV_ERROR_INVALID;
      err_msg = "SV_CMD_WIFI: invalid string";

      SendErrorResponse(aMsg->header.id, err, err_msg);
      return;
    }

    // TODO: whitelist interface/request?

    int32_t (*func)(const char*, const char*, char*, int32_t*);
    func = (int32_t (*)(const char*, const char*, char*, int32_t*))
      dlsym(wifilib, cmd);

    if (dlerror() != NULL) {
      err = SV_ERROR_FAILED;
      err_msg = "SV_CMD_WIFI: failed to load function";

      SendErrorResponse(aMsg->header.id, err, err_msg);
      return;
    }

    func_ret = (*func)(interface, request, buffer, &buffer_len);

    length = sizeof(buffer_len) + buffer_len + 1;
    resp_data = (char*)calloc(1, length);

    if (!resp_data) {
      err = SV_ERROR_MEMORY;
      err_msg = "SV_CMD_WIFI: failed to allocate memory";

      SendErrorResponse(aMsg->header.id, err, err_msg);
      return;
    }

    /* prepare data to send back */
    iter = (void*)resp_data;
    if (WriteInt(&iter, buffer_len, sizeof(buffer_len)) < 0 ||
        WriteRaw(&iter, buffer, buffer_len) < 0) {
      err = SV_ERROR_FAILED;
      err_msg = "SV_CMD_WIFI: failed to write response data";

      SendErrorResponse(aMsg->header.id, err, err_msg);
      return;
    }

    SendResponse(aMsg->header.id, func_ret, resp_data, length);

    free(resp_data);
  } else {
    err = SV_ERROR_DENIED;
    err_msg = "SV_CMD_WIFI: command not in whitelist";

    SendErrorResponse(aMsg->header.id, err, err_msg);
    return;
  }
}

void
HandleCmdSetprio(struct SvMessage* aMsg)
{
  char* err_msg = NULL;
  uint32_t size = 0;
  enum SvTypeError err = SV_ERROR_OK;

  int32_t ret = -1;
  int32_t pid = 0;
  int32_t nice = 0;

  void* iter = (void*)aMsg->data;

  if (ReadInt(&iter, &pid, &size) < 0 ||
      ReadInt(&iter, &nice, &size) < 0) {
    err = SV_ERROR_FAILED;
    err_msg = "SV_CMD_SETPRIO: failed to read message";
    goto respond;
  }

  if (pid == 0 || pid == getpid()) {
    err = SV_ERROR_DENIED;
    err_msg = "SV_CMD_SETPRIO: given pid is blacklisted";
    goto respond;
  }

  ret = setpriority(PRIO_PROCESS, pid, nice);
  SendResponse(aMsg->header.id, ret, NULL, 0);

  return;

respond:
  SendErrorResponse(aMsg->header.id, err, err_msg);
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
    case SV_CMD_SETPRIO: HandleCmdSetprio(aMsg); break;
  }
}
