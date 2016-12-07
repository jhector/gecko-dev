/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_AudioContextParent_h
#define mozilla_AudioContextParent_h

#include "mozilla/audio/PAudioContextParent.h"

#include "mozilla/audio/PAudioStreamParent.h"
#include "cubeb/cubeb.h"

namespace mozilla {
namespace audio {

class AudioContextParent
  : public PAudioContextParent
{
public:
  AudioContextParent();
  virtual ~AudioContextParent() {};

  int Initialize(const nsCString& aName);

  cubeb* GetContext() { return mContext; }

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

private:
  virtual PAudioStreamParent*
  AllocPAudioStreamParent(const nsCString &aName,
                          const cubeb_stream_params& aInputParams,
                          const cubeb_stream_params& aOutputParams,
                          const int& aLatencyFrames,
                          int *aRet) override;

  virtual bool
  DeallocPAudioStreamParent(PAudioStreamParent* aActor) override;

  virtual mozilla::ipc::IPCResult
  RecvPAudioStreamConstructor(PAudioStreamParent* aActor,
                              const nsCString& aName,
                              const cubeb_stream_params& aInputParams,
                              const cubeb_stream_params& aOutputParams,
                              const int& aLatencyFrames,
                              int *aRet) override;

  cubeb* mContext;

  // TODO: store instance of AudioParent??
};

} // namespace audio
} // namespace mozilla

#endif // mozilla_AudioContextParent_h
