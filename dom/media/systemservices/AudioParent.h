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

  virtual bool RecvCubebInit(const nsCString& aName, int* aId) override;

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

protected:
  nsTArray<cubeb*> mCubebContexts;
};

PAudioParent* CreateAudioParent();

} // namespace audio
} // namespace mozilla

#endif  // mozilla_AudioParent_h
