/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>

#include <fdio/loop.h>

/* from b2g/supervisor/include */
#include <utils/Common.h>
#include <ipc/Message.h>
#include <ipc/Channel.h>

static int32_t sSockListen = -1;
static int32_t sSockChannel = -1;

enum ioresult EpollCbSockListen(int32_t aFd, uint32_t aEvent, void* aData);
enum ioresult EpollCbSockChannel(int32_t aFd, uint32_t aEvent, void* aData);

enum ioresult ChannelRead(struct ChannelDataCb* aCbData);
void ChannelOpen(struct ChannelDataCb* aCbData);
void ChannelClose(struct ChannelDataCb* aCbData);

// TODO: docu
enum ioresult
EpollCbSockListen(int32_t aFd, uint32_t aEvent, void* aData)
{
  if (aFd != sSockListen) {
    return IO_ABORT;
  }

  struct ChannelDataCb* aCbData = (struct ChannelDataCb*)aData;

  if (aEvent & EPOLLERR) {
    // TODO: close channel and die
  } else if (aEvent & EPOLLIN) {
    ChannelOpen(aCbData);
  }

  return IO_OK;
}

// TODO: docu
enum ioresult
EpollCbSockChannel(int32_t aFd, uint32_t aEvent, void* aData)
{
  if (aFd != sSockChannel) {
    return IO_ABORT;
  }

  struct ChannelDataCb* cbData = (struct ChannelDataCb*)aData;

  enum ioresult res = IO_OK;
  if (aEvent & EPOLLHUP) {
    ChannelClose(cbData);
  } else if (aEvent & EPOLLIN) {
    res = ChannelRead(cbData);
  }

  return res;
}

// TODO: docu
enum ioresult
ChannelRead(struct ChannelDataCb *aCbData)
{
  if (sSockChannel < 0) {
    return IO_ABORT;
  }

  // TODO: license, this code is more or less from chromium
  int32_t bytes_read = 0;
  int32_t i = 0;

  struct msghdr hdr = {0};
  struct iovec data = {0};

  char ctrl[sizeof(struct cmsghdr)*sizeof(int32_t)*SV_MESSAGE_MAX_FDS] = {0};
  char input[SV_MESSAGE_MAX_RAW] = {0};

  data.iov_base = input;
  data.iov_len = sizeof(input);

  hdr.msg_iov = &data;
  hdr.msg_iovlen = 1;
  hdr.msg_control = ctrl;
  hdr.msg_controllen = sizeof(ctrl);

  const int32_t* fds = NULL;
  uint32_t nfds = 0;

  bytes_read = HANDLE_EINTR(recvmsg(sSockChannel, &hdr, MSG_DONTWAIT));

  if (bytes_read < 0) {
    if (errno == EAGAIN) {
      return IO_OK;
    } else {
      return IO_ABORT;
    }
  } else if (bytes_read == 0) {
    // TODO: close channel
    return IO_OK;
  }

  /* see if we have a control message and extract file descriptors */
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
          goto exit_close;
        }
        break;
      }
    }
  }

  if (nfds > SV_MESSAGE_MAX_FDS) {
    goto exit_close;
  }

  /* we assume that the start of the buffer is the start of the message */
  struct SvMessage* msg = (struct SvMessage*)input;

  if (ValidateMsgHeader(&msg->header, bytes_read) < 0) {
    goto exit_close;
  }

  /* replace header information regarding fds */
  for (i=0; (i<nfds) && (i<SV_MESSAGE_MAX_FDS); i++) {
    msg->fds[i] = fds[i];
  }

  msg->header.nfds = nfds;

  if (aCbData && aCbData->OnMessageReceived) {
    return aCbData->OnMessageReceived(msg);
  }

  return IO_OK;

exit_close:
  for (i=0; i<nfds; i++) {
    HANDLE_EINTR(close(fds[i]));
  }

  return IO_OK; // TODO: check this
}

// TODO: docu
int32_t
ChannelWrite(struct SvMessage* aMsg)
{
  // TODO: license, taken more or less from chromium code
  struct msghdr msgh = {0};

  aMsg->header.magic = SV_MESSAGE_MAGIC;

  static const int32_t tmp = CMSG_SPACE(sizeof(int32_t[SV_MESSAGE_MAX_FDS]));
  char buf[tmp];

  if (aMsg->header.nfds > 0) {
    struct cmsghdr* cmsg;
    const int32_t nfds = aMsg->header.nfds;

    if (nfds > SV_MESSAGE_MAX_FDS) {
      return 0;
    }

    msgh.msg_control = buf;
    msgh.msg_controllen = CMSG_SPACE(sizeof(int32_t)*nfds);
    cmsg = CMSG_FIRSTHDR(&msgh);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int32_t)*nfds);

    int32_t* ptr = (int32_t*)CMSG_DATA(cmsg);
    int32_t i = 0;
    for (; i<nfds; i++) {
      *(ptr++) = aMsg->fds[i];
    }

    aMsg->header.nfds = nfds;
    msgh.msg_controllen = cmsg->cmsg_len;
  }

  int32_t size = sizeof(struct SvMessage) + aMsg->header.size;
  struct iovec iov = {(char*)aMsg, size};

  msgh.msg_iov = &iov;
  msgh.msg_iovlen = 1;

  return HANDLE_EINTR(sendmsg(sSockChannel, &msgh, MSG_DONTWAIT));
}

// TODO: docu
void
ChannelOpen(struct ChannelDataCb* aCbData)
{
  char* err = NULL;

  if (sSockChannel > 0) {
    return;
  }

  sSockChannel = HANDLE_EINTR(accept(sSockListen, NULL, 0));
  if (sSockChannel < 0) {
    err = "failed to accept()";
    goto error;
  }

  if (SetFdFlag(sSockChannel, O_NONBLOCK) < 0) {
    err = "failed to set O_NONBLOCK flag";
    goto error_close;
  }

  if (add_fd_to_epoll_loop(sSockChannel, EPOLLIN,
        EpollCbSockChannel, aCbData) < 0) {
    err = "failed to register epoll event for channel";
    goto error_close;
  }

  if (add_fd_to_epoll_loop(sSockListen, EPOLLERR,
        EpollCbSockListen, aCbData) < 0) {
    err = "failed to register epoll event for listen";
    goto error_close;
  }

  if (aCbData && aCbData->OnChannelOpened) {
    aCbData->OnChannelOpened();
  }

  return;

error_close:
  HANDLE_EINTR(close(sSockChannel));
  sSockChannel = -1;
error:
  // TODO: log err
  return;
}

// TODO: docu
void
ChannelClose(struct ChannelDataCb* aCbData)
{
  remove_fd_from_epoll_loop(sSockChannel);

  HANDLE_EINTR(close(sSockChannel));
  sSockChannel = -1;

  if (aCbData && aCbData->OnChannelClosed) {
    aCbData->OnChannelClosed();
  }

  if (add_fd_to_epoll_loop(sSockListen, EPOLLIN,
        EpollCbSockListen, aCbData) < 0) {
    // TODO: handle
  }
}

// TODO: docu
enum ioresult
ChannelInit(void *aData)
{
  struct ChannelData* info = (struct ChannelData*)aData;
  char* err = NULL;

  struct sockaddr_un addr = {0};

  if (!info || !info->channelInfo->path) {
    err = "socket path not given";
    goto error;
  }

  unlink(info->channelInfo->path);

  if ((sSockListen = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
    err = "failed to create UNIX socket";
    goto error;
  }

  if (SetFdFlag(sSockListen, O_NONBLOCK) < 0) {
    err = "failed to set O_NONBLOCK flag";
    goto error_close;
  }

  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, info->channelInfo->path, UNIX_PATH_MAX-1);

  if (bind(sSockListen, (struct sockaddr*)&addr,
        sizeof(struct sockaddr_un)) != 0) {
    err = "failed to bind() on socket";
    goto error_close;
  }

  if (DropFilePriv(info->channelInfo->path, info->channelInfo->owner,
        info->channelInfo->group, S_IRUSR | S_IWUSR) < 0) {
    err = "failed to drop privileges";
    goto error_close;
  }

  if (listen(sSockListen, 5) != 0) {
    err = "failed to listen() on socket";
    goto error_close;
  }

  if (add_fd_to_epoll_loop(sSockListen, EPOLLIN, EpollCbSockListen,
        (void*)(info->channelCb)) < 0) {
    err = "failed to register epoll event";
    goto error_close;
  }

  return IO_OK;

error_close:
  HANDLE_EINTR(close(sSockListen));
  sSockListen = -1;
error:
  // TODO: log err
  return IO_ABORT;
}
