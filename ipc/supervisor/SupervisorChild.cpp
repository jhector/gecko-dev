/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <stdio.h>

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
}

SupervisorChild::~SupervisorChild()
{
  if (mSocket)
    delete mSocket;

  mInstance = NULL;
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

  *((int32_t*)msg->data) = aCmd;
  msg->header.size = sizeof(aCmd);

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
  free(msg);
  return res;
}

bool
SupervisorChild::SendCmdWifi(const char* aCmd)
{
  bool res = false;
  struct SvMessage* msg = NULL;

  int32_t length = strlen(aCmd);
  int32_t size = length + sizeof(struct SvMessage) + 1;

  if (length <= 0) {
    return false;
  }

  msg = (struct SvMessage*)calloc(1, size);
  if (!msg) {
    return false;
  }

  msg->header.type = SV_TYPE_CMD;
  msg->header.opt = SV_CMD_WIFI;

  strncpy(&msg->data[0], aCmd, length);

  msg->header.size = length + 1;

  // TODO: send and wait for response
  if (SendRaw(msg)) {
    res = true;
  } else {
    res = false;
  }
#if DEBUG
  printf("Send wifi command...\n");
  sleep(20);
#endif
  free(msg);
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
SupervisorChild::Connect(const char* aSockPath)
{
#ifdef DEBUG
  printf("Trying to connect...\n");
#endif

  return mSocket->Connect(aSockPath);
}

void
SupervisorChild::Disconnect()
{
  mSocket->Disconnect();
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
