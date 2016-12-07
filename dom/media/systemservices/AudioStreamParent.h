/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef mozilla_AudioStreamParent_h
#define mozilla_AudioStreamParent_h

#include "mozilla/audio/PAudioStreamParent.h"

#include "mozilla/audio/PAudioContext.h"
#include "cubeb/cubeb.h"

namespace mozilla {
namespace audio {

class AudioContextParent;

class AudioStreamParent
  : public PAudioStreamParent
{
public:
  AudioStreamParent(AudioContextParent* aContextParent);
  virtual ~AudioStreamParent();

  void ActorDestroy(ActorDestroyReason aWhy) override;

  int Initialize(char const* aName,
                 const cubeb_stream_params& aInputParams,
                 const cubeb_stream_params& aOutputParams,
                 const int& aLatencyFrames);

  long DataCallback();
  void StateCallback();

private:
  // Static callbacks which will inform the child
  static long
  DataCallback_S(cubeb_stream * aStream, void* aUserPtr,
                 void const* aInputBuffer, void* aOutputBuffer,
                 long aNframes) {
    return static_cast<AudioStreamParent*>(aUserPtr)->DataCallback();
  }

  static void
  StateCallback_S(cubeb_stream* aStream, void* aUserPtr, cubeb_state aState) {
    static_cast<AudioStreamParent*>(aUserPtr)->StateCallback();
  }

  cubeb_stream* mStream;
  AudioContextParent* mContextParent;
};

} // namespace audio
} // namespace mozilla

#endif // mozilla_AudioStreamParent_h
