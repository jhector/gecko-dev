/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef mozilla_AudioParent_h
#define mozilla_AudioParent_h

#include "mozilla/audio/PAudioParent.h"
#include "mozilla/audio/PAudioContext.h"

class MessageLoop;

namespace mozilla {
namespace audio {

class AudioService;

class AudioParent
  : public PAudioParent
{
public:
  explicit AudioParent(AudioService* aAudioService);
  ~AudioParent() override;

  void Open(Transport* aTransport, ProcessId aPid, MessageLoop* aIOLoop);

  void ActorDestroy(ActorDestroyReason aWhy) override;

  MessageLoop* ServiceLoop();

private:
  virtual PAudioContextParent*
  AllocPAudioContextParent(const nsCString &aName, int *aRet) override;

  virtual bool
  DeallocPAudioContextParent(PAudioContextParent* aActor) override;

  virtual mozilla::ipc::IPCResult
  RecvPAudioContextConstructor(PAudioContextParent* aActor,
                               const nsCString& aName, int *aRet) override;

  const RefPtr<AudioService> mAudioService;
};

} // namespace audio
} // namespace mozilla

#endif // mozilla_AudioParent_h
