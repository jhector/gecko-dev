/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_AudioContextChild_h
#define mozilla_AudioContextChild_h

#include "mozilla/audio/PAudioContextChild.h"

#include "mozilla/audio/AudioChild.h"
#include "mozilla/layers/SynchronousTask.h"
#include "cubeb/cubeb.h"

class MessageLoop;

namespace mozilla {
namespace audio {

class AudioChild;

class AudioContextChild
  : public PAudioContextChild
{
public:
  AudioContextChild(AudioChild* aAudioChild);
  virtual ~AudioContextChild() {};

  int InitializeStream(cubeb* aContext,
                       cubeb_stream** aStream,
                       char const* aName,
                       cubeb_stream_params* aInputStreamParams,
                       cubeb_stream_params* aOutputStreamParams,
                       const int& aLatencyFrames,
                       cubeb_data_callback data_callback,
                       cubeb_state_callback state_callback,
                       void* aUserPtr);

  MessageLoop* ServiceLoop();

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

private:
  virtual PAudioStreamChild*
  AllocPAudioStreamChild(const nsCString &aName,
                         const cubeb_stream_params& aInputStreamParams,
                         const cubeb_stream_params& aOutputStreamParams,
                         const int& aLatencyFrames,
                         int *aRet) override;

  virtual bool
  DeallocPAudioStreamChild(PAudioStreamChild* aActor) override;

  void
  InitializeStreamSync(layers::SynchronousTask* aTask,
                       cubeb_stream** aStream,
                       char const* aName,
                       cubeb_stream_params* aInputStreamParams,
                       cubeb_stream_params* aOutputStreamParams,
                       const int& aLatencyFrames,
                       int* aRet);

 AudioChild* mAudioChild;
};

} // namespace audio
} // namespace mozilla

#endif // mozilla_AudioContextChild_h
