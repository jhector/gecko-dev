/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <string.h>

/* from b2g/supervisor/include */
#include <ipc/Message.h>

/**
 * Validates the received message by checking the magic.
 * It also checks if we received at least a message header
 * along with the file descriptor array (empty or not).
 *
 * |size| field in header is checked whether it reasonable or not,
 * by checking if it exceeds the total amount of bytes received.
 *
 * @param aHdr message header from child
 * @param aSize number of bytes from socket
 *
 * @return 0 when header is valid
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
 * Reads an 32 bit integer at the current |aIter| position.
 * The integer is stored in |aOut|. |aLen| contains the size
 * of the read integer. After the read, |aIter| will be
 * repositioned.
 *
 * Note: |aIter| is expected to be point to a mapped memory
 * region.
 *
 * @param aIter pointer to a data block pointer
 * @param aOut pointer to store read integer
 * @param aLen pointer to store read length
 *
 * @return 0 on successful read
 */
int32_t
ReadInt(void** aIter, uint32_t* aOut, uint32_t* aLen)
{
  if (!aIter || !*aIter || !aOut || !aLen) {
    return -1;
  }

  *aOut = *((uint32_t*)(*aIter));
  *aLen = sizeof(uint32_t);

  *aIter += sizeof(uint32_t);

  return 0;
}

/**
 * Returns a pointer to the beginning of the string. Length
 * of string will be written to |aLen| and |aIter| will be
 * repositioned to pointer after the null termination of the
 * string.
 *
 * Note: caller needs to check if the string length is valid.
 *
 * @param aIter pointer to a data block pointer
 * @param aOut pointer to store the char pointer
 * @param aLen pointer to store the string length
 *
 * @return 0 on successful read
 */
int32_t
ReadString(void** aIter, char** aOut, uint32_t* aLen)
{
  if (!aOut || !aLen || !aIter || !*aIter) {
    return -1;
  }

  *aOut = (char*)(*aIter);
  *aLen = strlen(*aIter);

  /* let the iterator point after the null terminator */
  *aIter += (*aLen)+1;

  return 0;
}

/**
 * Returns a pointer to raw byte data. |aLen| is expected to
 * contain the length of the raw data, so that |aIter| can
 * be repositioned to the end of the raw data.
 *
 * @param aIter pointer to a data block pointer
 * @param aOut pointer to store void pointer
 * @param aLen pointer to legnth of raw data block
 *
 * @return 0 on successful read
 */
int32_t
ReadRaw(void** aIter, void** aOut, uint32_t* aLen)
{
  if (!aOut || !aLen || !aIter || !*aIter || !*aLen) {
    return -1;
  }

  *aOut = (void*)(*aIter);

  *aIter += (*aLen);

  return 0;
}

/**
 * Writes a given 32 bit integer to |aIter| and
 * shifts |aIter| to the end for next write.
 *
 * Note: no check where we write to
 *
 * @param aIter pointer to data block to write to
 * @param aIn integer value to write
 * @param aLen length of the value to write (ignored here)
 *
 * @return amounts of bytes written
 */
int32_t
WriteInt(void** aIter, uint32_t aIn, uint32_t aLen)
{
  if (!aIter || !*aIter) {
    return -1;
  }

  *((uint32_t*)(*aIter)) = aIn;
  *aIter += sizeof(aIn);

  return sizeof(aIn);
}

/**
 * Copies a string with up to |aLen| bytes to given location.
 * |aLen| includes the null terminator. After the copy |aIter|
 * will point to the location after the null-terminator
 *
 * @param aIter pointer to the data block to write to
 * @param aIn pointer to a string that should be copied
 * @param aLen length of the string including null termination
 *
 * @return aLen
 */
int32_t
WriteString(void** aIter, const char* aIn, uint32_t aLen)
{
  if (!aIter || !*aIter || !aIn || !aLen) {
    return -1;
  }

  strncpy((char*)(*aIter), aIn, aLen-1);
  ((char*)(*aIter))[aLen-1] = '\0';

  *aIter += aLen;

  return aLen;
}

/**
 * Copies |aLen| of raw data to given location. After the copy
 * |aIter| will point after the raw data block
 *
 * @param aIter pointer to the data block to write to
 * @param aIn pointer to raw data to copy
 * @param aLen length of raw data
 *
 * @return aLen
 */
int32_t
WriteRaw(void** aIter, const void* aIn, uint32_t aLen)
{
  if (!aIter || !*aIter || !aIn || !aLen) {
    return -1;
  }

  memcpy((char*)(*aIter), aIn, aLen);
  *aIter += aLen;

  return aLen;
}
