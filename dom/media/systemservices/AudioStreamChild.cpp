/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/audio/AudioStreamChild.h"

namespace mozilla {
namespace audio {

AudioStreamChild::AudioStreamChild(AudioContextChild* aContextChild)
  : mContextChild(aContextChild),
    mDataCallback(nullptr),
    mStateCallback(nullptr)
{}

AudioStreamChild::~AudioStreamChild()
{}

void
AudioStreamChild::Initialize(cubeb_data_callback aDataCallback,
                             cubeb_state_callback aStateCallback,
			     void* aUserPtr)
{
  mDataCallback = aDataCallback;
  mStateCallback = aStateCallback;
  mUserPtr = aUserPtr;
}

MessageLoop*
AudioStreamChild::ServiceLoop()
{
  return mContextChild->ServiceLoop();
}


void
AudioStreamChild::ActorDestroy(ActorDestroyReason aWhy)
{
  // TODO: loop assertion
  // TODO: shutdown function
}

} // namespace audio
} // namespace mozilla
