/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/audio/AudioContextChild.h"

#include "mozilla/audio/AudioStreamChild.h"
#include "mozilla/Unused.h"

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
                                    cubeb_stream_params* aInputParams,
                                    cubeb_stream_params* aOutputParams,
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
                           cubeb_stream**, char const*,
                           cubeb_stream_params*,
                           cubeb_stream_params*,
                           const int&,
                           int*>(this,
                                 &AudioContextChild::InitializeStreamSync,
                                 &task, aStream, aName, aInputParams,
                                 aOutputParams, aLatencyFrames, &ret));

  task.Wait();

  // Finish child side initialization, i.e. callbacks registration etc.
  // when object creation succeeded
  if (ret == CUBEB_OK) {
    (*aStream)->actor->Initialize(aDataCallback, aStateCallback, aUserPtr);
  }

  return ret;
}

char const*
AudioContextChild::GetBackendId()
{
  nsCString backend;

  layers::SynchronousTask task("GetBackendId Lock");
  ServiceLoop()->PostTask(NewNonOwningRunnableMethod
                          <layers::SynchronousTask*,
                           nsCString*>(this,
                                 &AudioContextChild::GetBackendIdSync,
                                 &task, &backend));

  task.Wait();

  return ToNewCString(backend);
}

void
AudioContextChild::GetBackendIdSync(layers::SynchronousTask* aTask,
                                    nsCString* aBackend)
{
  MOZ_RELEASE_ASSERT(MessageLoop::current() == ServiceLoop());

  layers::AutoCompleteTask complete(aTask);
  Unused << SendGetBackendId(aBackend);
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
                                          const cubeb_stream_params& aInputParams,
                                          const cubeb_stream_params& aOutputParams,
                                          const int& aLatencyFrames,
                                          int* aRet)
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
                                        cubeb_stream_params* aInputParams,
                                        cubeb_stream_params* aOutputParams,
                                        const int& aLatencyFrames,
                                        int* aRet)
{
  MOZ_RELEASE_ASSERT(MessageLoop::current() == ServiceLoop());

  layers::AutoCompleteTask complete(aTask);
  nsAutoCString name(aName);

  // TODO: UGLY!!!! can do better??
#if 1
  cubeb_stream_params inputParams;
  cubeb_stream_params outputParams;

  memset(&inputParams, 0x00, sizeof(inputParams));
  memset(&outputParams, 0x00, sizeof(outputParams));

  if (aInputParams) {
    memcpy(&inputParams, aInputParams, sizeof(inputParams));
  }

  if (aOutputParams) {
    memcpy(&outputParams, aOutputParams, sizeof(outputParams));
  }
#endif

  *aStream = new cubeb_stream(); // TODO: need to make sure the memory is properly free()ed, RefPtr?
  (*aStream)->actor = static_cast<AudioStreamChild*>
                         (SendPAudioStreamConstructor(name,
                                                      inputParams,
                                                      outputParams,
                                                      aLatencyFrames,
                                                      aRet));

  if (*aRet != CUBEB_OK) {
    delete *aStream;
    *aStream= nullptr;
  }
}

} // namespace audio
} // namespace mozilla
