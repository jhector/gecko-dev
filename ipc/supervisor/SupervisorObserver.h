/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef SUPERVISOR_OBSERVER_H
#define SUPERVISOR_OBSERVER_H

#include "mozilla/ipc/UnixSocket.h"

namespace mozilla {
namespace ipc {

class SupervisorSocket;

class SupervisorObserver
{
public:
  virtual void ReceiveSocketData(
      SupervisorSocket* aSocket,
      nsAutoPtr<mozilla::ipc::UnixSocketRawData>& aMessage) = 0;

  /**
   * A callback function which would be called when a socket connection
   * is established successfully. To be more specific, this would be called
   * when socket state changes from CONNECTING/LISTENING to CONNECTED.
   */
  virtual void OnSocketConnectSuccess(SupervisorSocket* aSocket) = 0;

  /**
   * A callback function which would be called when SupervisorSocket::Connect()
   * fails.
   */
  virtual void OnSocketConnectError(SupervisorSocket* aSocket) = 0;

  /**
   * A callback function which would be called when a socket connection
   * is dropped. To be more specific, this would be called when socket state
   * changes from CONNECTED/LISTENING to DISCONNECTED.
   */
  virtual void OnSocketDisconnect(SupervisorSocket* aSocket) = 0;
};

} // namespace ipc
} // namespace mozilla

#endif // SUPERVISOR_OBSERVER_H
