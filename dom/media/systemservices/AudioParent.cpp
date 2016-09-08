/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "AudioParent.h"

#include <cstdio>

namespace mozilla {
namespace audio {

bool
AudioParent::RecvCubebInit(const nsCString& aName, int* aId)
{
  cubeb* ctx;

  int rv = cubeb_init(&ctx, aName.get());
  if (rv != CUBEB_OK) {
    return false;
  }

  *aId = mCubebContexts.Length();
  mCubebContexts.AppendElement(ctx);

  return true;
}

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

  for (unsigned int i = 0; i < mCubebContexts.Length(); i++ ) {
    cubeb_destroy(mCubebContexts[i]);
  }
}

PAudioParent* CreateAudioParent()
{
  return new AudioParent();
}

} // namespace audio
} // namespace mozilla
