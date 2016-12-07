/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/audio/AudioParent.h"

#include "mozilla/audio/AudioContextParent.h"
#include "mozilla/audio/AudioService.h"
#include "mozilla/Unused.h"

// TODO: validate pointer owner shipt etc... RefPtr usage etc

namespace mozilla {
namespace audio {

AudioParent::AudioParent(AudioService* aAudioService)
  : mAudioService(aAudioService)
{}

AudioParent::~AudioParent()
{}

void
AudioParent::Open(Transport* aTransport, ProcessId aPid, MessageLoop* aIOLoop)
{
  MOZ_RELEASE_ASSERT(MessageLoop::current() == ServiceLoop());

  DebugOnly<bool> ok = PAudioParent::Open(aTransport, aPid, aIOLoop);
  MOZ_ASSERT(ok);
}

void
AudioParent::ActorDestroy(ActorDestroyReason aWhy)
{
  // TODO: loop assertion
  // TODO: shutdown function
}

MessageLoop*
AudioParent::ServiceLoop()
{
  return mAudioService->ServiceLoop();
}

PAudioContextParent*
AudioParent::AllocPAudioContextParent(const nsCString& aName, int *aRet)
{
  return new AudioContextParent();
}

bool
AudioParent::DeallocPAudioContextParent(PAudioContextParent* aActor)
{
  delete aActor;
  return true;
}

mozilla::ipc::IPCResult
AudioParent::RecvPAudioContextConstructor(PAudioContextParent* aActor,
                                          const nsCString& aName, int* aRet)
{
  auto actor = static_cast<AudioContextParent*>(aActor);
  *aRet = actor->Initialize(aName);

  // In case of failure, we don't need the actor pair anymore,
  // delete them in a clean fashion
  if (*aRet != CUBEB_OK) {
    Unused << AudioContextParent::Send__delete__(aActor);
  }

  return IPC_OK();
}

} // namespace audio
} // namespace mozilla
