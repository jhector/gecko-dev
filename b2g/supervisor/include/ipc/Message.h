/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef SUPERVISOR_IPC_MESSAGE_H
#define SUPERVISOR_IPC_MESSAGE_H

#include <stdint.h>

#define SV_MESSAGE_MAGIC      0xdeadbeef
#define SV_MESSAGE_MAX_RAW    0x00001000
#define SV_MESSAGE_MAX_FDS    0x00000007

enum SvType {
  SV_MESSAGE_HELLO,
  SV_MESSAGE_ERROR,
  SV_MESSAGE_RES,
  SV_MESSAGE_CMD,
  SV_MESSAGE_FOP
};

enum SvTypeError {
  SV_ERROR_OK,
  SV_ERROR_MISSING,
  SV_ERROR_INVALID,
  SV_ERROR_FAILED,
  SV_ERROR_DENIED,
  SV_ERROR_MEMORY
};

struct SvMessageHeader {
  uint32_t magic; 
  uint32_t id;
  uint32_t nfds;
  uint32_t size;
  uint32_t type;
  uint32_t opt;
};

struct SvMessage {
  struct SvMessageHeader header;
  int32_t fds[SV_MESSAGE_MAX_FDS];
  char data[0];
};

int32_t ValidateMsgHeader(struct SvMessageHeader* aHdr, uint32_t aSize);

#endif // SUPERVISOR_IPC_MESSAGE_H
