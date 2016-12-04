/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/audio/AudioContextParent.h"

#include "mozilla/audio/AudioStreamParent.h"
#include "cubeb/cubeb.h"

namespace mozilla {
namespace audio {

AudioContextParent::AudioContextParent()
  : mContext(nullptr)
{
}

int
AudioContextParent::Initialize(const nsCString& aName)
{
  return cubeb_init(&mContext, aName.get());
}

void
AudioContextParent::ActorDestroy(ActorDestroyReason aWhy)
{
// TODO: implement
}

PAudioStreamParent*
AudioContextParent::AllocPAudioStreamParent(const nsCString& aName, int *aRet)
{
  return new AudioStreamParent();
}

bool
AudioContextParent::DeallocPAudioStreamParent(PAudioStreamParent* aActor)
{
  delete aActor;
  return true;
}

mozilla::ipc::IPCResult
AudioContextParent::RecvPAudioStreamConstructor(PAudioStreamParent* aActor,
                                                const nsCString& aName, int* aRet)
{
  auto actor = static_cast<AudioStreamParent*>(aActor);
  /*
  *aRet = actor->Initialize(aName);

  // In case of failure, we don't need the actor pair anymore,
  // delete them in a clean fashion
  if (*aRet != CUBEB_OK) {
    Unused << AudioContextParent::Send__delete__(aActor);
  }
  */

  return IPC_OK();
}

} // namespace audio
} // namespace mozilla
