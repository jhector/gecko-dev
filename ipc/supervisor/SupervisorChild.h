/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef SUPERVISOR_CHILD_H
#define SUPERVISOR_CHILD_H

#include "mozilla/ipc/UnixSocket.h"
#include "mozilla/ipc/SupervisorObserver.h"
#include "mozilla/ipc/SupervisorSocket.h"

struct SvMessage;

namespace mozilla {
namespace ipc {

class SupervisorChild : public mozilla::ipc::SupervisorObserver
{
public:
  NS_EXPORT static SupervisorChild* Instance();

  bool Connect(const char *aSockPath);
  void Disconnect();

  bool SendCmdWifi(const char* aCmd);

  bool SendRaw(struct SvMessage* aMsg);

  void ReceiveSocketData(
      mozilla::ipc::SupervisorSocket* aSocket,
      nsAutoPtr<mozilla::ipc::UnixSocketRawData>& aMessage);


  void OnSocketConnectSuccess(
      mozilla::ipc::SupervisorSocket* aSocket);
  void OnSocketConnectError(
      mozilla::ipc::SupervisorSocket* aSocket);
  void OnSocketDisconnect(
      mozilla::ipc::SupervisorSocket* aSocket);

private:
  static SupervisorChild* mInstance;

  mozilla::ipc::SupervisorSocket* mSocket;

private:
  SupervisorChild();
  ~SupervisorChild();
};

} // namespace ipc
} // namespace mozilla

#endif // SUPERVISOR_CHILD_H
