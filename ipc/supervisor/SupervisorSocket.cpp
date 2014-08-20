/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <sys/socket.h>
#include <sys/un.h>

#include <string.h>

#include "SupervisorSocket.h"
#include "SupervisorObserver.h"

#include "mozilla/ipc/SupervisorChild.h"

/* from b2g/supervisor/include */
#include <utils/Common.h>
#include <ipc/Message.h>

using namespace mozilla::ipc;

SupervisorConnector::SupervisorConnector(const char* aSockPath)
  : mSockPath(aSockPath)
{}

int32_t
SupervisorConnector::Create()
{
  MOZ_ASSERT(!NS_IsMainThread());

  return socket(PF_UNIX, SOCK_STREAM, 0);
}

bool
SupervisorConnector::CreateAddr(bool aIsServer,
                                socklen_t& aAddrSize,
                                sockaddr_any& aAddr,
                                const char* aAddress)
{
  if (aIsServer)
    return false;

  memset(&aAddr, 0, sizeof(aAddr));

  aAddrSize = sizeof(struct sockaddr_un);
  aAddr.un.sun_family = AF_UNIX;
  strncpy(aAddr.un.sun_path, mSockPath, UNIX_PATH_MAX-1);

  return true;
}

bool
SupervisorConnector::SetUp(int32_t aFd)
{
  return true;
}

bool
SupervisorConnector::SetUpListenSocket(int32_t aFd)
{
  return false;
}

void
SupervisorConnector::GetSocketAddr(const sockaddr_any& aAddr,
                                   nsAString& aAddrStr)
{
  aAddrStr.AssignASCII(mSockPath);
}

SupervisorSocket::SupervisorSocket(SupervisorObserver* aObserver)
  : mObserver(aObserver)
{}

bool
SupervisorSocket::Connect(const char* aSockPath)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoPtr<SupervisorConnector> c(new SupervisorConnector(aSockPath));

  if (!ConnectSocket(c.forget(), "", 0, HandleSocketRead, HandleSocketWrite)) {
    // TODO: log
    return false;
  }

  return true;
}

void
SupervisorSocket::Disconnect()
{
  CloseSocket();
}

ssize_t
SupervisorSocket::HandleSocketRead(ssize_t aFd, void* aOutBuf, size_t aSize)
{
  SupervisorChild* child = SupervisorChild::Instance();

  const int32_t* fds = NULL;
  uint32_t nfds = 0;
  uint32_t i = 0;

  int32_t bytes_read = 0;

  struct msghdr hdr = {0};
  struct iovec data = {0};

  struct SvMessage* p = (struct SvMessage*)aOutBuf;

  char ctrl[sizeof(struct cmsghdr)*sizeof(int32_t)*SV_MESSAGE_MAX_FDS] = {0};

  data.iov_base = aOutBuf;
  data.iov_len = aSize;

  hdr.msg_iov = &data;
  hdr.msg_iovlen = 1;
  hdr.msg_control = ctrl;
  hdr.msg_controllen = sizeof(ctrl);

  bytes_read = HANDLE_EINTR(recvmsg(aFd, &hdr, MSG_DONTWAIT));

  /* correct message header when receiving file descriptors */
  if (hdr.msg_controllen > 0) {
    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&hdr);
    for (; cmsg; cmsg = CMSG_NXTHDR(&hdr, cmsg)) {
      if (cmsg->cmsg_level == SOL_SOCKET &&
          cmsg->cmsg_type == SCM_RIGHTS) {
        const uint32_t len = cmsg->cmsg_len - CMSG_LEN(0);
        // TODO: check mod 4?
        fds = (int32_t*)(CMSG_DATA(cmsg));
        nfds = len / 4;

        if (hdr.msg_flags & MSG_CTRUNC) {
          goto error_close;
        }
        break;
      }
    }
  }

  if (nfds > SV_MESSAGE_MAX_FDS) {
    goto error_close;
  }

  for(i=0; (i<nfds) && (i<SV_MESSAGE_MAX_FDS); i++) {
    p->fds[i] = fds[i];
  }

  p->header.nfds = nfds;

  if (ValidateMsgHeader(&p->header, bytes_read) < 0) {
    goto error_close;
  }

  pthread_mutex_lock(&child->mRespMutex);

  if (child->StoreResponse(p)) {
    pthread_cond_signal(&child->mRespCond);
    errno = EAGAIN;
    bytes_read = -1;
  }

  pthread_mutex_unlock(&child->mRespMutex);

  return bytes_read;

error_close:
  for (i=0; (i<nfds) && fds; ++i) {
    HANDLE_EINTR(close(fds[i]));
  }
  return bytes_read; // TODO: maybe different
}

ssize_t
SupervisorSocket::HandleSocketWrite(ssize_t aFd, void* aInBuf, size_t aSize)
{
  struct msghdr msgh = {0};

  struct SvMessage* msg = (struct SvMessage*)aInBuf;

  static const int32_t tmp = CMSG_SPACE(sizeof(int32_t[SV_MESSAGE_MAX_FDS]));
  char buf[tmp] = {0};

  if (msg->header.nfds > 0) {
    struct cmsghdr* cmsg = {0};
    const uint32_t nfds = msg->header.nfds;

    if (nfds > SV_MESSAGE_MAX_FDS) {
      return 0;
    }

    msgh.msg_control = buf;
    msgh.msg_controllen = CMSG_SPACE(sizeof(int32_t)*nfds);
    cmsg = CMSG_FIRSTHDR(&msgh);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int32_t)*nfds);

    int32_t* buffer = (int32_t*)CMSG_DATA(cmsg);
    int32_t i = 0;
    for (; i<nfds; i++) {
      *(buffer++) = msg->fds[i];
    }

    msg->header.nfds = nfds;
    msgh.msg_controllen = cmsg->cmsg_len;
  }

  struct iovec iov = {aInBuf, aSize};

  msgh.msg_iov = &iov;
  msgh.msg_iovlen = 1;

  return HANDLE_EINTR(sendmsg(aFd, &msgh, MSG_DONTWAIT));
}

void
SupervisorSocket::ReceiveSocketData(nsAutoPtr<UnixSocketRawData>& aMessage)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mObserver);
  mObserver->ReceiveSocketData(this, aMessage);
}

void
SupervisorSocket::OnConnectSuccess()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mObserver);
  mObserver->OnSocketConnectSuccess(this);
}

void
SupervisorSocket::OnConnectError()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mObserver);
  mObserver->OnSocketConnectError(this);
}

void
SupervisorSocket::OnDisconnect()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mObserver);
  mObserver->OnSocketDisconnect(this);
}
