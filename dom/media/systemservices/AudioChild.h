/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef mozilla_AudioChild_h
#define mozilla_AudioChild_h

#include "mozilla/audio/AudioContextChild.h"
#include "mozilla/audio/AudioService.h"
#include "mozilla/audio/PAudioChild.h"
#include "mozilla/layers/SynchronousTask.h"

class MessageLoop;
class cubeb;

namespace mozilla {
namespace audio {

class AudioChild
  : public PAudioChild
{
public:
  explicit AudioChild(AudioService* aAudioService);
  ~AudioChild() override;

  void Open(Transport* aTransport, ProcessId aOtherPid, MessageLoop* aIOLoop);

  void ActorDestroy(ActorDestroyReason aWhy) override;

  int InitializeContext(cubeb**, char const*);

  MessageLoop* ServiceLoop();

  static AudioChild* Get() { return sInstance; }

private:
  virtual PAudioContextChild*
  AllocPAudioContextChild(const nsCString &aName, int *aRet) override;

  virtual bool
  DeallocPAudioContextChild(PAudioContextChild* aActor) override;

  void
  InitializeContextSync(layers::SynchronousTask* aTask, cubeb** aContext,
                                  char const* aName, int* aRet);

  static Atomic<AudioChild*> sInstance;
  const RefPtr<AudioService> mAudioService;
};

Atomic<AudioChild*> AudioChild::sInstance;

} // namespace audio
} // namespace mozilla

#endif // mozilla_AudioChild_h
