/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef SUPERVISOR_SOCKET_H
#define SUPERVISOR_SOCKET_H

#include <stdint.h>

#include "mozilla/ipc/UnixSocket.h"

namespace mozilla {
namespace ipc {

class SupervisorObserver;

class SupervisorConnector : public UnixSocketConnector
{
public:
  SupervisorConnector(const char* aSockPath);

  virtual int32_t Create() MOZ_OVERRIDE;
  virtual bool CreateAddr(bool aIsServer,
                          socklen_t& aAddrSize,
                          sockaddr_any& aAddr,
                          const char* aAddress);
  virtual bool SetUp(int32_t aFd);
  virtual bool SetUpListenSocket(int32_t aFd);
  virtual void GetSocketAddr(const sockaddr_any& aAddr, nsAString& aAddrStr);

private:
  const char* mSockPath;
};

class SupervisorSocket : public UnixSocketConsumer
{
public:
  SupervisorSocket(SupervisorObserver* aObserver);

  bool Connect(const char* aSockPath);
  void Disconnect();

  virtual void OnConnectSuccess();
  virtual void OnConnectError();
  virtual void OnDisconnect();
  virtual void ReceiveSocketData(nsAutoPtr<UnixSocketRawData>& aMessage);

  static int32_t HandleSocketRead(int32_t aFd, void* aOutBuf, size_t aSize);
  static int32_t HandleSocketWrite(int32_t aFd, void* aInBuf, size_t aSize);



private:
  SupervisorObserver* mObserver;
};

} // namespace ipc
} // namespace mozilla

#endif // SUPERVISOR_SOCKET_H
