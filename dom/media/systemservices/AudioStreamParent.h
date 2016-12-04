/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef mozilla_AudioStreamParent_h
#define mozilla_AudioStreamParent_h

#include "mozilla/audio/PAudioStreamParent.h"

#include "mozilla/audio/PAudioContext.h"

namespace mozilla {
namespace audio {

class AudioStreamParent
  : public PAudioStreamParent
{
public:
  AudioStreamParent();
  virtual ~AudioStreamParent();

  void ActorDestroy(ActorDestroyReason aWhy) override;
};

} // namespace audio
} // namespace mozilla

#endif // mozilla_AudioStreamParent_h
