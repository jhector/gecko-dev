/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef SUPERVISOR_IPC_CHANNEL_H
#define SUPERVISOR_IPC_CHANNEL_H

#include <unistd.h>

/* forward declarations */
struct SvMessage;
enum ioresult;

struct ChannelDataCb {
  enum ioresult (*OnMessageReceived)(struct SvMessage* msg);
  void (*OnChannelClosed)();
  void (*OnChannelOpened)();
};

struct ChannelDataInfo {
  char *path;
  uid_t owner;
  uid_t group;
};

struct ChannelData {
  struct ChannelDataInfo* channelInfo;
  struct ChannelDataCb* channelCb;
};

enum ioresult ChannelInit(void* aData);

#endif // SUPERVISOR_IPC_CHANNEL_H

