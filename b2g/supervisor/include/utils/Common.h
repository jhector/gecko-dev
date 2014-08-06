/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef SUPERVISOR_UTILS_COMMON_H
#define SUPERVISOR_UTILS_COMMON_H

#include <unistd.h>
#include <stdint.h>
#include <errno.h>

/* TODO: from ipc/chromium/src/base/eintr_wrapper.h */
#define HANDLE_EINTR(x) ({ \
    typeof(x) __eintr_result__; \
    do { \
    __eintr_result__ = x; \
    } while (__eintr_result__ == -1 && errno == EINTR); \
    __eintr_result__;\
    })

int32_t SetFdFlag(int32_t aFd, uint32_t aFlag);
int32_t DropFilePriv(const char* aPath, 
                     uid_t aOwner, 
                     uid_t aGroup, 
                     mode_t aMode);

#endif // SUPERVISOR_UTILS_COMMON_H
