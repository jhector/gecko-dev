/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef mozilla_AudioServiceIPC_h
#define mozilla_AudioServiceIPC_h

#include "mozilla/audio/PAudio.h"
#include "mozilla/audio/PAudioParent.h"
#include "mozilla/audio/PAudioChild.h"

namespace mozilla {
namespace audio {

PAudioParent*
CreateAudioParent(mozilla::ipc::Transport* aTransport,
                  base::ProcessId aOtherProcess);

PAudioChild*
CreateAudioChild(mozilla::ipc::Transport* aTransport,
                 base::ProcessId aOtherProcess);

} // namespace audio
} // namespace mozilla

#endif // mozilla_AudioServiceIPC_h
