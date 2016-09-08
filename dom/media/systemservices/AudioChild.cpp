/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "AudioChild.h"

#include "mozilla/ipc/BackgroundChild.h"
#include "mozilla/ipc/PBackgroundChild.h"

namespace mozilla {
namespace audio {

static PAudioChild* sAudio;

static AudioChild*
Audio()
{
  if (!sAudio) {
    // Try to get PBackground handle
    ipc::PBackgroundChild backgroundChild =
      ipc::BackgroundChild::GetForCurrentThread();

    // If it doesn't exist yet, wait for it
    if (!backgroundChild) {
      backgroundChild =
        ipc::BackgroundChild::SynchronouslyCreateForCurrentThread();
    }

    // By now, we should have a PBackground
    MOZ_RELEASE_ASSERT(backgroundChild);
    sAudio = backgroundChild->SendPAudioConstructor();
  }

  MOZ_ASSERT(sAudio);
  return static_cast<AudioChild*>(sAudio);
}

AudioChild::AudioChild()
{
  MOZ_COUNT_CTOR(AudioChild);
}

AudioChild::~AudioChild()
{
  MOZ_COUNT_DTOR(AudioChild);
}

PAudioChild* CreateAudioChild()
{
  return new AudioChild();
}

} // namespace audio
} // namespace mozilla
