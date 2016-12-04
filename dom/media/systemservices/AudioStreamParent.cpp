/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/audio/AudioStreamParent.h"

#include "mozilla/audio/AudioContextParent.h"

namespace mozilla {
namespace audio {

AudioStreamParent::AudioStreamParent(AudioContextParent* aContextParent)
  : mStream(nullptr),
    mContextParent(aContextParent)
{}

AudioStreamParent::~AudioStreamParent()
{}

void
AudioStreamParent::ActorDestroy(ActorDestroyReason aWhy)
{
  // TODO: loop assertion
  // TODO: shutdown function
}

int
AudioStreamParent::Initialize(char const* aName,
                              const int& aLatencyFrames)
{
  return cubeb_stream_init(mContextParent->GetContext(), &mStream, aName,
                           nullptr, nullptr, nullptr, nullptr, aLatencyFrames,
                           DataCallback_S, StateCallback_S, this);
  // TODO: left off
}

long
AudioStreamParent::DataCallback()
{
  return 0;
}

void
AudioStreamParent::StateCallback()
{

}

} // namespace audio
} // namespace mozilla
