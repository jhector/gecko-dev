/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/audio/AudioContextParent.h"

#include "cubeb/cubeb.h"

namespace mozilla {
namespace audio {

AudioContextParent::AudioContextParent()
  : mContext(nullptr)
{
}

int
AudioContextParent::Initialize(const nsCString& aName)
{
  return cubeb_init(&mContext, aName.get());
}

void
AudioContextParent::ActorDestroy(ActorDestroyReason aWhy)
{
// TODO: implement
}

} // namespace audio
} // namespace mozilla
