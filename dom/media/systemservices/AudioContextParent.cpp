/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/audio/AudioContextParent.h"

#include "mozilla/audio/AudioStreamParent.h"
#include "mozilla/Unused.h"

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
AudioContextParent::AllocPAudioStreamParent(const nsCString& aName,
                                            const cubeb_stream_params& aInputParams,
                                            const cubeb_stream_params& aOutputParams,
                                            const int& aLatencyFrames,
                                            int *aRet)
{
  return new AudioStreamParent(this);
}

bool
AudioContextParent::DeallocPAudioStreamParent(PAudioStreamParent* aActor)
{
  delete aActor;
  return true;
}

mozilla::ipc::IPCResult
AudioContextParent::RecvPAudioStreamConstructor(PAudioStreamParent* aActor,
                                                const nsCString& aName,
                                                const cubeb_stream_params& aInputParams,
                                                const cubeb_stream_params& aOutputParams,
                                                const int& aLatencyFrames,
                                                int* aRet)
{
  // TODO: aInputParams/aOutputParams can be null to indicate
  // if the stream is input or output, we need to handle that case here somehow

  auto actor = static_cast<AudioStreamParent*>(aActor);
  *aRet = actor->Initialize(aName.get(), aInputParams, aOutputParams, aLatencyFrames);

  // In case of failure, we don't need the actor pair anymore,
  // delete them in a clean fashion
  if (*aRet != CUBEB_OK) {
    Unused << AudioStreamParent::Send__delete__(aActor);
  }

  return IPC_OK();
}

mozilla::ipc::IPCResult
AudioContextParent::RecvGetBackendId(nsCString* aRet)
{
  if (!mContext) {
    return IPC_OK(); // TODO: failure here?
  }

  const char* backend = cubeb_get_backend_id(mContext);
  // TODO: better way to do this??
  nsAutoCString name(backend);
  *aRet = name;

  return IPC_OK();
}

mozilla::ipc::IPCResult
AudioContextParent::RecvGetMaxChannelCount(uint32_t* aMaxChannel, int* aRet)
{
  if (!mContext) {
    return IPC_OK(); // TODO: failure here?
  }

  *aRet = cubeb_get_max_channel_count(mContext, aMaxChannel);
  return IPC_OK();
}

} // namespace audio
} // namespace mozilla
