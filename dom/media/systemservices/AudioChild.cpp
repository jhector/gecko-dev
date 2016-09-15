/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "AudioChild.h"

#include "mozilla/ipc/BackgroundChild.h"
#include "mozilla/ipc/PBackgroundChild.h"

namespace mozilla {
namespace audio {

static PAudioChild* sAudio;

static AudioChild*
Audio()
{
  if (!sAudio) {
    // Try to get PBackground handle
    ipc::PBackgroundChild* backgroundChild =
      ipc::BackgroundChild::GetForCurrentThread();

    // If it doesn't exist yet, wait for it
    if (!backgroundChild) {
      backgroundChild =
        ipc::BackgroundChild::SynchronouslyCreateForCurrentThread();
    }

    // By now, we should have a PBackground
    MOZ_RELEASE_ASSERT(backgroundChild);
    sAudio = backgroundChild->SendPAudioConstructor();
  }

  MOZ_ASSERT(sAudio);
  return static_cast<AudioChild*>(sAudio);
}

int CubebInit(cubeb** aContext, char const* aContextName)
{
  nsAutoCString name(aContextName);

  // Object is allocated here and free()ed in CubebDestroy()
  *aContext = (cubeb*)malloc(sizeof(cubeb));
  if (*aContext == nullptr) {
    return CUBEB_ERROR;
  }

  if (!Audio()->SendCubebInit(name, &((*aContext)->id))) {
    return CUBEB_ERROR;
  }

  return CUBEB_OK;
}

const char* CubebGetBackendId(cubeb* aContext)
{
  nsCString backend;
  if (!Audio()->SendCubebGetBackendId(aContext->id, &backend)) {
    return nullptr;
  }

  return ToNewCString(backend);
}

void CubebDestroy(cubeb* aContext)
{
  if (aContext) {
    Audio()->SendCubebDestroy(aContext->id);
    free(aContext);
  }
}

AudioChild::AudioChild()
{
  MOZ_COUNT_CTOR(AudioChild);
}

AudioChild::~AudioChild()
{
  MOZ_COUNT_DTOR(AudioChild);
}

PAudioChild* CreateAudioChild()
{
  return new AudioChild();
}

} // namespace audio
} // namespace mozilla
