/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "AudioParent.h"

namespace mozilla {
namespace audio {

void
AudioParent::ActorDestroy(ActorDestroyReason aWhy)
{
}

AudioParent::AudioParent()
{
  MOZ_COUNT_CTOR(AudioParent);
}

AudioParent::~AudioParent()
{
  MOZ_COUNT_DTOR(AudioParent);
}

PAudioParent* CreateAudioParent()
{
  return new AudioParent();
}

} // namespace audio
} // namespace mozilla
