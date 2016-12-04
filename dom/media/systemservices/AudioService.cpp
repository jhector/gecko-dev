/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/audio/AudioService.h"
#include "mozilla/audio/AudioServiceIPC.h"

#include "mozilla/audio/AudioParent.h"
#include "mozilla/audio/AudioChild.h"

#include "mozilla/layers/SynchronousTask.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/Unused.h"

#include "base/thread.h"

namespace mozilla {
namespace audio {

/*
 * Basic architecutre:
 * TODO: describe architecture
 */

AudioService* AudioService::sInstance;

AudioService::AudioService()
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());

  MOZ_COUNT_CTOR(AudioService);

  mThread = new base::Thread("AudioService");
  if (!mThread->Start()) {
    delete mThread;
    mThread = nullptr;
  }
}

AudioService::~AudioService()
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());

  MOZ_COUNT_DTOR(AudioService);

  MOZ_ASSERT(sInstance == this);
  sInstance = nullptr;

  delete mThread;
}

AudioService*
AudioService::GetOrCreate()
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());

  if (!sInstance) {
    sInstance = new AudioService();
  }

  return sInstance;
}

MessageLoop*
AudioService::ServiceLoop()
{
  return mThread->message_loop();
}

/* static */ void
AudioService::Start(dom::ContentParent* aContentParent)
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());

  DebugOnly<bool> opened = PAudio::Open(aContentParent);
  MOZ_ASSERT(opened);
}

PAudioParent*
CreateAudioParent(mozilla::ipc::Transport* aTransport,
                  base::ProcessId aOtherPid)
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());

  AudioService* service = AudioService::GetOrCreate();
  auto* parent = new AudioParent(service);

  service->ServiceLoop()->PostTask(NewNonOwningRunnableMethod
                                   <mozilla::ipc::Transport*,
                                    base::ProcessId,
                                    MessageLoop*>(parent,
                                                  &AudioParent::Open,
                                                  aTransport, aOtherPid,
                                                  XRE_GetIOMessageLoop()));

  return parent;
}

PAudioChild*
CreateAudioChild(mozilla::ipc::Transport* aTransport,
                 base::ProcessId aOtherPid)
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());

  AudioService* service = AudioService::GetOrCreate();
  auto* child = new AudioChild(service);

  service->ServiceLoop()->PostTask(NewNonOwningRunnableMethod
                                   <mozilla::ipc::Transport*,
                                    base::ProcessId,
                                    MessageLoop*>(child,
                                                  &AudioChild::Open,
                                                  aTransport, aOtherPid,
                                                  XRE_GetIOMessageLoop()));

  return child;
}

} // namespace audio
} // namespace mozilla
