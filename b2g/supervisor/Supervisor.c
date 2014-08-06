/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <fdio/loop.h>

/* from b2g/supervisor/include */
#include <ipc/Channel.h>
#include <ipc/Message.h>

static const struct ChannelDataCb gChannelDataCb = {
  .OnMessageReceived = NULL,
  .OnChannelOpened = NULL,
  .OnChannelClosed = NULL
};

static struct ChannelDataInfo gChannelDataInfo;

static const struct ChannelData gChannelData = {
  .channelInfo = &gChannelDataInfo,
  .channelCb = &gChannelDataCb
};

int 
main(int argc, char* argv[])
{
  gChannelData.channelInfo->path = "/dev/socket/supervisord";
  gChannelData.channelInfo->owner = 1000;
  gChannelData.channelInfo->group = 1000;

  epoll_loop(ChannelInit, NULL, &gChannelData);

  return 0;
}
