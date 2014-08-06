/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

/* from b2g/supervisor/include */
#include <utils/Common.h>

int32_t
DropFilePriv(const char* aPath, uid_t aOwner, uid_t aGroup, mode_t aMode)
{
  if (chown(aPath, aOwner, aGroup) != 0 ||
      chmod(aPath, aMode) != 0) {
    return -1;
  }

  return 0;
}

int32_t
SetFdFlag(int32_t aFd, uint32_t aFlag)
{
  int32_t flags = 0;

  if ((flags = fcntl(aFd, F_GETFL, 0)) == -1) {
    return -1;
  }

  flags |= aFlag;
  if (fcntl(aFd, F_SETFL, flags) == -1) {
    return -1;
  }

  return 0;
}
