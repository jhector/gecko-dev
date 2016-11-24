/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_AudioContext_h
#define mozilla_AudioContext_h

#include "mozilla/audio/PAudioContextChild.h"
#include "mozilla/audio/PAudioContextParent.h"

#include "cubeb/cubeb.h"

namespace mozilla {
namespace audio {

class AudioContextChild
  : public PAudioContextChild
{
public:
  AudioContextChild() {};
  virtual ~AudioContextChild() {};

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;
};

class AudioContextParent
  : public PAudioContextParent
{
public:
  AudioContextParent() {};
  virtual ~AudioContextParent() {};

  bool Init(const nsCString& aName);

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

private:
  cubeb* mContext;  

  // TODO: store instance of AudioParent??
};

} // namespace audio
} // namespace mozilla

#endif // mozilla_AudioContext_h
