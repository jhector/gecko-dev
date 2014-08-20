/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>

#include "mozilla/ipc/SupervisorChild.h"

// from b2g/supervisor/include
#include <ipc/Message.h>

using namespace mozilla::ipc;

SupervisorChild* SupervisorChild::mInstance = NULL;

SupervisorChild* SupervisorChild::Instance()
{
  if (!mInstance) {
    mInstance = new SupervisorChild();
  }

  return mInstance;
}

SupervisorChild::SupervisorChild()
{
  mSocket = new SupervisorSocket(this);
  mResponse = NULL;
  mWaitOn = -1;

  mRespCond = PTHREAD_COND_INITIALIZER;
  mRespMutex = PTHREAD_MUTEX_INITIALIZER;
  mUseMutex = PTHREAD_MUTEX_INITIALIZER;

  mRandFd = -1;
}

SupervisorChild::~SupervisorChild()
{
  if (mSocket)
    delete mSocket;

  mInstance = NULL;

  pthread_cond_destroy(&mRespCond);
  pthread_mutex_destroy(&mRespMutex);
}

bool
SupervisorChild::Connect(const char* aSockPath)
{
#ifdef DEBUG
  printf("Trying to connect...\n");
#endif
  if (mRandFd < 0) {
    mRandFd = open("/dev/urandom", O_RDONLY);

    if (mRandFd < 0) {
      return false;
    }
  }

  return mSocket->Connect(aSockPath);
}

void
SupervisorChild::Disconnect()
{
  mSocket->Disconnect();
}

bool
SupervisorChild::SendCmdReboot(int32_t aCmd)
{
  bool res = false;
  struct SvMessage* msg = NULL;
  int32_t size = sizeof(struct SvMessage) + sizeof(aCmd);

  msg = (struct SvMessage*)calloc(1, size);
  if (!msg) {
    return false;
  }

  msg->header.type = SV_TYPE_CMD;
  msg->header.opt = SV_CMD_REBOOT;

  void* iter = (void*)msg->data;
  if ((size = WriteInt(&iter, aCmd, 4)) < 0) {
    res = false;
    goto exit_free;
  }

  msg->header.size = size;

#ifdef DEBUG
  printf("Send reboot command..\n");
#endif
  if (SendRaw(msg)) {
    res = true;
  } else {
    res = false;
  }

#ifdef DEBUG
  printf("SendRaw() response: %d\n", res);
#endif
exit_free:
  free(msg);
  return res;
}

void
SupervisorChild::HandleWifiResponse(const char* aCmd,
                                    struct SvMessage* aResp,
                                    struct WifiOutput* aOut)
{
  if (!aResp || !aCmd || !aOut) {
    return;
  }

  if (!strcmp(aCmd, "wifi_command")) {
    if (!aOut->buffer || !aOut->length || !(*(aOut->length))) {
      return;
    }

    uint32_t size = 0;
    int32_t len = -1;
    void* data = NULL;

    void *iter = (void*)aResp->data;
    if (ReadInt(&iter, (uint32_t*)&len, &size) < 0 ||
        ReadRaw(&iter, &data, (uint32_t*)&len) < 0) {
      return;
    }

    if (len < 0 || len > *(aOut->length)) {
      return;
    }

    memcpy(aOut->buffer, data, len);
    *(aOut->length) = len;
#ifdef DEBUG
    uint32_t* ptr = (uint32_t*)aOut->buffer;
    printf("WIFI response, length: %d\nString: %s\nHex:\n%08x %08x %08x %08x\n",
        len, aOut->buffer, *ptr++, *ptr++, *ptr++, *ptr++);
#endif
  }
}

int32_t
SupervisorChild::SendCmdWifi(const char* aCmd,
                             struct WifiInput* aIn,
                             struct WifiOutput* aOut)
{
  void* iter = NULL;

  int32_t res = -1;
  struct SvMessage* msg = NULL;

  int32_t ret = -1;
  int32_t tmp = -1;
  int32_t length = strlen(aCmd)+1;
  int32_t size = length + sizeof(struct SvMessage);

  if (length <= 0) {
    return -1;
  }

  if (!strcmp(aCmd, "wifi_load_driver") ||
      !strcmp(aCmd, "wifi_unload_driver")) {
    msg = (struct SvMessage*)calloc(1, size);
    if (!msg) {
      return -1;
    }

    iter = (void*)msg->data;
    if ((ret = WriteString(&iter, aCmd, length)) < 0) {
      res = -1;
      goto exit_free;
    }

    size = ret;

  } else if (
      !strcmp(aCmd, "wifi_start_supplicant") ||
      !strcmp(aCmd, "wifi_stop_supplicant")) {
    size += sizeof(int32_t);

    msg = (struct SvMessage*)calloc(1, size);
    if (!msg) {
      return -1;
    }

    iter = (void*)msg->data;
    if ((ret = WriteString(&iter, aCmd, length)) < 0) {
      res = -1;
      goto exit_free;
    }

    size = ret;

    if ((ret = WriteInt(&iter, aIn->enable, 4)) < 0) {
      res = -1;
      goto exit_free;
    }

    size += ret;
  } else if (!strcmp(aCmd, "wifi_connect_to_supplicant")) {
    tmp = strlen(aIn->interface) + 1;
    size += tmp;

    msg = (struct SvMessage*)calloc(1, size);
    if (!msg) {
      return -1;
    }

    iter = (void*)msg->data;
    if ((ret = WriteString(&iter, aCmd, length)) < 0) {
      res = -1;
      goto exit_free;
    }

    size = ret;

    if ((ret = WriteString(&iter, aIn->interface, tmp)) < 0) {
      res = -1;
      goto exit_free;
    }

    size += ret;
  } else if (!strcmp(aCmd, "wifi_command")) {
    size += strlen(aIn->interface) + 1;
    size += strlen(aIn->request) + 1;

    msg = (struct SvMessage*)calloc(1, size);
    if (!msg) {
      return -1;
    }

#ifdef DEBUG
    printf("interface: %s\nrequest: %s\n", aIn->interface, aIn->request);
#endif
    iter = (void*)msg->data;
    if ((ret = WriteString(&iter, aCmd, length)) < 0) {
      res = -1;
      goto exit_free;
    }

    size = ret;
    tmp = strlen(aIn->interface) + 1;
    if ((ret = WriteString(&iter, aIn->interface, tmp)) < 0) {
      res = -1;
      goto exit_free;
    }

    size += ret;
    tmp = strlen(aIn->request) + 1;
    if ((ret = WriteString(&iter, aIn->request, tmp)) < 0) {
      res = -1;
      goto exit_free;
    }

    size += ret;
  } else {
    res = -1;
    goto exit_free;
  }

  msg->header.type = SV_TYPE_CMD;
  msg->header.opt = SV_CMD_WIFI;
  msg->header.size = size;

  if (SendAndWait(msg)) {
    /* mResponse contains the response from supervisor */
    pthread_mutex_lock(&mUseMutex);
#ifdef DEBUG
    printf("Message opt: %d, size: %d\n", mResponse->header.opt, mResponse->header.size);
#endif
    if (mResponse->header.type == SV_TYPE_ERROR &&
        mResponse->header.opt != SV_ERROR_OK) {
      // TODO: print error message?
      res = -1;
    } else {
      res = (int32_t)mResponse->header.opt;
    }

    HandleWifiResponse(aCmd, mResponse, aOut);

    if (mResponse) {
      free(mResponse);
      mResponse = NULL;
    }

    pthread_mutex_unlock(&mUseMutex);
  } else {
    res = -1;
  }

exit_free:
  free(msg);
  return res;
}

int32_t
SupervisorChild::SendCmdSetprio(int32_t pid, int32_t nice)
{
  int32_t res = -1;
  struct SvMessage* msg = NULL;

  int32_t size = sizeof(struct SvMessage);
  size += sizeof(pid);
  size += sizeof(nice);

  msg = (struct SvMessage*)calloc(1, size);
  if (!msg) {
    return -1;
  }

  msg->header.type = SV_TYPE_CMD;
  msg->header.opt = SV_CMD_SETPRIO;

  void* iter = (void*)msg->data;
  int32_t len = -1;
  if ((len = WriteInt(&iter, pid, 4)) < 0) {
    res = -1;
    goto exit_free;
  }

  size = len;

  if ((len = WriteInt(&iter, nice, 4)) < 0) {
    res = -1;
    goto exit_free;
  }

  size += len;

  msg->header.size = size;

  if (SendAndWait(msg)) {
    /* mResponse contains the response from supervisor */
    pthread_mutex_lock(&mUseMutex);
    if (mResponse->header.type == SV_TYPE_ERROR &&
        mResponse->header.opt != SV_ERROR_OK) {
      // TODO: print error message?
      res = -1;
    } else {
      res = mResponse->header.opt;
    }

    if (mResponse) {
      free(mResponse);
      mResponse = NULL;
    }

    pthread_mutex_unlock(&mUseMutex);
  } else {
    res = -1;
  }

exit_free:
  free(msg);
  return res;
}

bool
SupervisorChild::SendAndWait(struct SvMessage* aMsg)
{
  bool res = false;

  if (read(mRandFd, (void*)&mWaitOn, sizeof(mWaitOn)) != sizeof(mWaitOn)) {
    return false;
  }
#ifdef DEBUG
  printf("Waiting on: 0x%08x\n", mWaitOn);
#endif
  aMsg->header.id = mWaitOn;

  if (!SendRaw(aMsg)) {
    return false;
  }

  int32_t rc = -1;
  struct timespec ts;

  pthread_mutex_lock(&mRespMutex);

  if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
    res = false;
    goto exit_unlock;
  }

#ifdef DEBUG
  printf("Monotonic time, sec: %u, nsec: %u\n", ts.tv_sec, ts.tv_nsec);
#endif
  ts.tv_nsec += MAX_WAIT_TIME;

#ifdef DEBUG
  printf("Sending requeset and waiting\n");
#endif
  rc = pthread_cond_timedwait(&mRespCond, &mRespMutex, &ts);

  if (rc == ETIMEDOUT) {
#ifdef DEBUG
    printf("Timeout while waiting for response\n");
#endif
    res = false;
    goto exit_unlock;
  } else {
#ifdef DEBUG
    printf("Got response in time\n");
#endif
    res = true;
    goto exit_unlock;
  }

exit_unlock:
  mWaitOn = -1;

  pthread_mutex_unlock(&mRespMutex);
  return res;
}

bool
SupervisorChild::SendRaw(struct SvMessage* aMsg)
{
  aMsg->header.magic = SV_MESSAGE_MAGIC;
  uint32_t size = sizeof(SvMessage) + aMsg->header.size;

  UnixSocketRawData* raw = new UnixSocketRawData((void*)aMsg, size);

  if (!raw)
    return false;

  return mSocket->SendSocketData(raw);
}

bool
SupervisorChild::StoreResponse(struct SvMessage* aMsg)
{
  pthread_mutex_lock(&mUseMutex);
  bool res = false;
  uint32_t size = 0;
#ifdef DEBUG
  printf("StoreResponse(), id: 0x%08x\n", aMsg->header.id);
#endif

  /* check if we are waiting for that response */
  if (mWaitOn != aMsg->header.id) {
    res = false;
    goto unlock_exit;
  }

  /* in case it is not free()ed */
  if (mResponse) {
    free(mResponse);
    mResponse = NULL;
  }

  size = sizeof(struct SvMessage) + aMsg->header.size;
  mResponse = (struct SvMessage*)calloc(1, size);
  if (!mResponse) {
    res = false;
    goto unlock_exit;
  }

  memcpy(mResponse, aMsg, size);
  res = true;

unlock_exit:
  pthread_mutex_unlock(&mUseMutex);
  return res;
}

void
SupervisorChild::ReceiveSocketData(
    SupervisorSocket* aSocket,
    nsAutoPtr<mozilla::ipc::UnixSocketRawData>& aMessage)
{
#ifdef DEBUG
  printf("Data received.\n");
#endif
}

void
SupervisorChild::OnSocketConnectSuccess(SupervisorSocket* aSocket)
{
#ifdef DEBUG
  printf("Connected\n");
#endif
}

void
SupervisorChild::OnSocketConnectError(SupervisorSocket* aSocket)
{
#ifdef DEBUG
  printf("Connect() error\n");
#endif
}

void
SupervisorChild::OnSocketDisconnect(SupervisorSocket* aSocket)
{
#ifdef DEBUG
  printf("Disconnected\n");
#endif
}
