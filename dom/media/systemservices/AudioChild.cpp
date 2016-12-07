/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "mozilla/audio/AudioChild.h"

#include "mozilla/audio/AudioContextChild.h"
#include "mozilla/audio/AudioStreamChild.h"
#include "mozilla/audio/AudioService.h"
#include "mozilla/layers/SynchronousTask.h"

struct cubeb {
  mozilla::audio::AudioContextChild* actor;
};

struct cubeb_stream {
  mozilla::audio::AudioStreamChild* actor;
};

namespace mozilla {
namespace audio {

Atomic<AudioChild*> AudioChild::sInstance;

AudioChild::AudioChild(AudioService* aAudioService)
  : mAudioService(aAudioService)
{
}

AudioChild::~AudioChild()
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(sInstance == this);

  sInstance = nullptr;
}

void
AudioChild::Open(Transport* aTransport, ProcessId aPid, MessageLoop* aIOLoop)
{
  MOZ_RELEASE_ASSERT(MessageLoop::current() == ServiceLoop());

  MOZ_ASSERT(!sInstance);
  sInstance = this;

  DebugOnly<bool> ok = PAudioChild::Open(aTransport, aPid, aIOLoop);
  MOZ_ASSERT(ok);
}

void
AudioChild::ActorDestroy(ActorDestroyReason aWhy)
{
  // TODO: loop assertion
  // TODO: shutdown function
}

MessageLoop*
AudioChild::ServiceLoop()
{
  return mAudioService->ServiceLoop();
}

int
AudioChild::InitializeContext(cubeb** aContext, char const* aName)
{
  nsAutoCString name(aName);
  int ret;

  layers::SynchronousTask task("InitializeContext Lock");
  ServiceLoop()->PostTask(NewNonOwningRunnableMethod
                          <layers::SynchronousTask*,
                           cubeb**, char const*,
                           int*>(this,
                                 &AudioChild::InitializeContextSync,
                                 &task, aContext, aName, &ret));

  task.Wait();

  return ret;
}

char const*
AudioChild::GetBackendId(cubeb* aContext)
{
  // TODO: pointer check?
  return aContext->actor->GetBackendId();
}

int
AudioChild::InitializeStream(cubeb* aContext,
                             cubeb_stream** aStream,
                             char const* aName,
                             cubeb_stream_params* aInputParams,
                             cubeb_stream_params* aOutputParams,
                             int aLatencyFrames,
                             cubeb_data_callback aDataCallback,
                             cubeb_state_callback aStateCallback,
                             void* aUserPtr)
{
  // TODO: pointer check?
  return aContext->actor->InitializeStream(aContext,
                                           aStream,
                                           aName,
                                           aInputParams,
                                           aOutputParams,
                                           aLatencyFrames,
                                           aDataCallback,
                                           aStateCallback,
                                           aUserPtr);
}

PAudioContextChild*
AudioChild::AllocPAudioContextChild(const nsCString& aName, int* aRet)
{
  return new AudioContextChild(this);
}

bool
AudioChild::DeallocPAudioContextChild(PAudioContextChild* aActor)
{
  delete aActor;
  return true;
}

void
AudioChild::InitializeContextSync(layers::SynchronousTask* aTask,
                                  cubeb** aContext, char const* aName, int* aRet)
{
  MOZ_RELEASE_ASSERT(MessageLoop::current() == ServiceLoop());

  layers::AutoCompleteTask complete(aTask);
  nsAutoCString name(aName);

  *aContext = new cubeb(); // TODO: need to make sure the memory is properly free()ed
  (*aContext)->actor = static_cast<AudioContextChild*>
                         (SendPAudioContextConstructor(name, aRet));

  if (*aRet != CUBEB_OK) {
    delete *aContext;
    *aContext = nullptr;
  }
}

} // namespace audio
} // namespace mozilla
