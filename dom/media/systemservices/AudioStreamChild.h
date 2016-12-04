/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef mozilla_AudioStreamChild_h
#define mozilla_AudioStreamChild_h

#include "mozilla/audio/PAudioStreamChild.h"

#include "mozilla/audio/AudioContextChild.h"
#include "cubeb/cubeb.h"

class MessageLoop;

namespace mozilla {
namespace audio {

class AudioContextChild;

class AudioStreamChild
  : public PAudioStreamChild
{
public:
  AudioStreamChild(AudioContextChild* aContextChild);
  virtual ~AudioStreamChild();

  void Initialize(cubeb_data_callback aDataCallback,
                  cubeb_state_callback aStateCallback,
		  void* aUserPtr);

  MessageLoop* ServiceLoop();

  void ActorDestroy(ActorDestroyReason aWhy) override;

private:
  AudioContextChild* mContextChild;

  cubeb_data_callback mDataCallback;
  cubeb_state_callback mStateCallback;
  void* mUserPtr;
};

} // namespace audio
} // namespace mozilla

#endif // mozilla_AudioStreamChild_h
