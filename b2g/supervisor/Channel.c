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
#include <ipc/Channel.h>

static int32_t sSockListen = -1;
static int32_t sSockChannel = -1;

enum ioresult EpollCbSockListen(int32_t aFd, uint32_t aEvent, void* aData);
enum ioresult EpollCbSockChannel(int32_t aFd, uint32_t aEvent, void* aData);

void ChannelOpen(struct ChannelDataCb* aCbData);

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
  return IO_OK;
}

// TODO: docu
void ChannelOpen(struct ChannelDataCb* aCbData)
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
