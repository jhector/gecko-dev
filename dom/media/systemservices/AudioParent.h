/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_AudioParent_h
#define mozilla_AudioParent_h

#include "mozilla/audio/PAudioParent.h"

#include "cubeb/cubeb.h"

namespace mozilla {
namespace audio {

class AudioParent;

class AudioParent : public PAudioParent
{
public:
  AudioParent();
  virtual ~AudioParent();

  virtual bool RecvInit(const nsCString& aName, uint32_t* aId) override;
  virtual bool RecvGetBackendId(const uint32_t& aCtxId,
                                nsCString* aName) override;
  virtual bool RecvGetMaxChannelCount(const uint32_t& aCtxId,
                                      uint32_t* aChannels,
                                      int* aRet) override;
  virtual bool RecvGetPreferredSampleRate(const uint32_t& aCtxId,
                                          uint32_t* aRate,
                                          int* aRet) override;
  virtual bool RecvDestroy(const uint32_t& aCtxId) override;

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

protected:
  uint32_t mContextIdCounter;

  nsDataHashtable<nsUint32HashKey, cubeb*> mCubebContexts;
};

PAudioParent* CreateAudioParent();

} // namespace audio
} // namespace mozilla

#endif  // mozilla_AudioParent_h
