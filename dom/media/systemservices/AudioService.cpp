/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/AudioService.h"
#include "mozilla/AudioServiceIPC.h"

#include "base/thread.h"

namespace mozilla {
namespace audio {

/*
 * Basic architecutre:
 * TODO: describe architecture
 */

namespace {

/* Child process objects */

class AudioChild
  : public PAudioChild
{
public:
  explicit AudioChild(AudioService* aAudioService);
  ~AudioChild() override;

  void Open(Transport* aTransport, ProcessId aOtherPid, MessageLoop* aIOLoop);

  void ActorDestroy(ActorDestroyReason aWhy) override;

  static AudioChild* Get() { return sInstance; }

private:
  static Atomic<AudioChild*> sInstance;

  const RefPtr<AudioService> mAudioService;
};

Atomic<AudioChild*> AudioChild::sInstance;

/* Parent process objects */

class AudioParent
  : public PAudioParent
{
public:
  explicit AudioParent(AudioService* aAudioService);
  ~AudioParent() override;

  void Open(Transport* aTransport, ProcessId aPid, MessageLoop* aIOLoop);

  virtual ipc::IPCResult RecvHello() override;

  void ActorDestroy(ActorDestroyReason aWhy) override;

private:
  const RefPtr<AudioService> mAudioService;
};

} // namespace

/* AudioChild implementation */

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
  // TODO: loop assertion
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

/* AudioParent implementation */

AudioParent::AudioParent(AudioService* aAudioService)
  : mAudioService(aAudioService)
{
}

AudioParent::~AudioParent()
{
}

void
AudioParent::Open(Transport* aTransport, ProcessId aPid, MessageLoop* aIOLoop)
{
  // TODO: loop assertion
  DebugOnly<bool> ok = PAudioParent::Open(aTransport, aPid, aIOLoop);
  MOZ_ASSERT(ok);
}

mozilla::ipc::IPCResult
AudioParent::RecvHello()
{
  return IPC_OK();
}

void
AudioParent::ActorDestroy(ActorDestroyReason aWhy)
{
  // TODO: loop assertion
  // TODO: shutdown function
}

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

PAudioParent*
CreateAudioParent(mozilla::ipc::Transport* aTransport,
                  base::ProcessId aOtherPid)
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());

  AudioService* service = AudioService::GetOrCreate();
  auto* parent = new AudioParent(service);

  return parent;
}

PAudioChild*
CreateAudioChild(mozilla::ipc::Transport* aTransport,
                 base::ProcessId aOtherPid)
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());

  AudioService* service = AudioService::GetOrCreate();
  auto* child = new AudioChild(service);

  return child;
}

} // namespace audio
} // namespace mozilla
