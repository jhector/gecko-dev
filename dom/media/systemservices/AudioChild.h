/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_AudioChild_h
#define mozilla_AudioChild_h

#include "mozilla/audio/PAudioChild.h"
#include "mozilla/audio/PAudioParent.h"

#include "cubeb/cubeb.h"

// The datatype |cubeb| is an opaque handle (see cubeb.h) of type struct cubeb.
// On the child side the cubeb context is represented by an ID which
// is used by the parent to access the real cubeb object.
// Therefore struct cubeb is defined here as a simple int.
struct cubeb {
  uint32_t id;
};

namespace mozilla {
namespace audio {

int Init(cubeb** aContext, char const* aContextName);
const char* GetBackendId(cubeb* aContext);
void Destroy(cubeb* aContext);

class AudioChild : public PAudioChild
{
public:
  AudioChild();
  virtual ~AudioChild();
};

PAudioChild* CreateAudioChild();

} // namespace audio
} // namespace mozilla

#endif  // mozilla_AudioChild_h
