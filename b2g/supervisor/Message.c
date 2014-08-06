/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* from b2g/supervisor/include */
#include <ipc/Message.h>

/**
 *
 * @param aHdr message header from child
 * @param aSize number of bytes from socket
 *
 */
int32_t
ValidateMsgHeader(struct SvMessageHeader* aHdr, uint32_t aSize)
{
  if (!aHdr || aSize == 0) {
    return -1;
  }

  if (aHdr->magic != SV_MESSAGE_MAGIC) {
    return -1;
  }

  if (aSize < sizeof(struct SvMessage)) {
    return -1;
  }

  if (aHdr->size > (aSize-sizeof(struct SvMessage)) ||
      (int32_t)aHdr->size < 0) {
    return -1;
  }

  return 0;
}
