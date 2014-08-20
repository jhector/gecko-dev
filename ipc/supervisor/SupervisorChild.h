/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef SUPERVISOR_CHILD_H
#define SUPERVISOR_CHILD_H

#include <stdint.h>
#include <pthread.h>

#include "mozilla/ipc/UnixSocket.h"
#include "mozilla/ipc/SupervisorObserver.h"
#include "mozilla/ipc/SupervisorSocket.h"

#define MAX_WAIT_TIME 400000000 // ns

struct SvMessage;

struct WifiInput {
  int32_t enable;
  const char* interface;
  const char* request;
};

struct WifiOutput {
  char* buffer;
  int32_t* length;
};

namespace mozilla {
namespace ipc {

class SupervisorChild : public mozilla::ipc::SupervisorObserver
{
public:
  NS_EXPORT static SupervisorChild* Instance();

  bool Connect(const char *aSockPath);
  void Disconnect();

  bool SendCmdReboot(int32_t aCmd);
  void HandleWifiResponse(const char* aCmd,
                          struct SvMessage* aResp,
                          struct WifiOutput* aOut);
  int32_t SendCmdWifi(const char* aCmd,
                      struct WifiInput* aIn,
                      struct WifiOutput* aOut);
  int32_t SendCmdSetprio(int32_t pid, int32_t nice);

  bool SendAndWait(struct SvMessage* aMsg);
  bool SendRaw(struct SvMessage* aMsg);

  bool StoreResponse(struct SvMessage* aMsg);

  void ReceiveSocketData(
      mozilla::ipc::SupervisorSocket* aSocket,
      nsAutoPtr<mozilla::ipc::UnixSocketRawData>& aMessage);

  void OnSocketConnectSuccess(
      mozilla::ipc::SupervisorSocket* aSocket);
  void OnSocketConnectError(
      mozilla::ipc::SupervisorSocket* aSocket);
  void OnSocketDisconnect(
      mozilla::ipc::SupervisorSocket* aSocket);

public:
  pthread_cond_t mRespCond;
  pthread_mutex_t mRespMutex;

private:
  static SupervisorChild* mInstance;

  pthread_mutex_t mUseMutex;

  mozilla::ipc::SupervisorSocket* mSocket;

  int32_t mRandFd;
  uint32_t mWaitOn;
  struct SvMessage* mResponse;

private:
  SupervisorChild();
  ~SupervisorChild();
};

} // namespace ipc
} // namespace mozilla

#endif // SUPERVISOR_CHILD_H
