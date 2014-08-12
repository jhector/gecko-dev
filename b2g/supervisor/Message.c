/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <string.h>

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

/**
 *
 */
int32_t
ReadInt(void* aIter, uint32_t* aOut, uint32_t* aLen)
{
  if (!aIter || !aOut || !aLen) {
    return -1;
  }

  *aOut = *((uint32_t*)aIter);
  *aLen = sizeof(uint32_t);

  aIter += sizeof(uint32_t);

  return 0;
}

/**
 *
 *
 */
int32_t
ReadString(void* aIter, char** aOut, uint32_t* aLen)
{
  if (!aOut || !aLen || !aIter) {
    return -1;
  }

  *aOut = (char*)aIter;
  *aLen = strlen(aIter);

  /* let the iterator point after the null terminator */
  aIter += (*aLen)+1;

  return 0;
}
