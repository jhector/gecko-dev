/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/audio/AudioContextChild.h"

#include "mozilla/audio/AudioStreamChild.h"

namespace mozilla {
namespace audio {

AudioContextChild::AudioContextChild(AudioChild* aAudioChild)
  : mAudioChild(aAudioChild)
{
}

int
AudioContextChild::InitializeStream(cubeb* aContext,
                                    cubeb_stream** aStream,
                                    char const* aName,
                                    const int& aLatencyFrames,
                                    cubeb_data_callback aDataCallback,
                                    cubeb_state_callback aStateCallback,
                                    void* aUserPtr)
{
  nsAutoCString name(aName);
  int ret;

  layers::SynchronousTask task("InitializeStream Lock");
  ServiceLoop()->PostTask(NewNonOwningRunnableMethod
                          <layers::SynchronousTask*,
                           cubeb_stream**, char const*, const int&,
                           int*>(this,
                                 &AudioContextChild::InitializeStreamSync,
                                 &task, aStream, aName, aLatencyFrames, &ret));

  task.Wait();

  // Finish child side initialization, i.e. callbacks registration etc.
  // when object creation succeeded
  if (ret == CUBEB_OK) {
    (*aStream)->actor->Initialize(aDataCallback, aStateCallback, aUserPtr);
  }

  return ret;
}

MessageLoop*
AudioContextChild::ServiceLoop()
{
  return mAudioChild->ServiceLoop();
}


void
AudioContextChild::ActorDestroy(ActorDestroyReason aWhy)
{
// TODO: implement
}

PAudioStreamChild*
AudioContextChild::AllocPAudioStreamChild(const nsCString& aName,
                                          const int& aLatencyFrames, int* aRet)
{
  return new AudioStreamChild(this);
}

bool
AudioContextChild::DeallocPAudioStreamChild(PAudioStreamChild* aActor)
{
  delete aActor;
  return true;
}

void
AudioContextChild::InitializeStreamSync(layers::SynchronousTask* aTask,
                                        cubeb_stream** aStream,
                                        char const* aName,
                                        const int& aLatencyFrames,
                                        int* aRet)
{
  MOZ_RELEASE_ASSERT(MessageLoop::current() == ServiceLoop());

  layers::AutoCompleteTask complete(aTask);
  nsAutoCString name(aName);

  *aStream = new cubeb_stream(); // TODO: need to make sure the memory is properly free()ed, RefPtr?
  (*aStream)->actor = static_cast<AudioStreamChild*>
                         (SendPAudioStreamConstructor(name,
                                                      aLatencyFrames,
                                                      aRet));

  if (*aRet != CUBEB_OK) {
    delete *aStream;
    *aStream= nullptr;
  }
}

} // namespace audio
} // namespace mozilla
