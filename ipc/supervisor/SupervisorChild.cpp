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
