/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "Audio.h"

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

int
Init(cubeb** aContext, char const* aContextName)
{
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    nsAutoCString name(aContextName);

    // Object is allocated here and free()ed in CubebDestroy()
    *aContext = (cubeb*)malloc(sizeof(cubeb));
    if (*aContext == nullptr) {
      return CUBEB_ERROR;
    }

    if (!Audio()->SendInit(name, &((*aContext)->id))) {
      return CUBEB_ERROR;
    }

    return CUBEB_OK;
  }

  return cubeb_init(aContext, aContextName);
}

const char*
GetBackendId(cubeb* aContext)
{
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    nsCString backend;
    if (!Audio()->SendGetBackendId(aContext->id, &backend)) {
      return nullptr;
    }

    return ToNewCString(backend);
  }

  return cubeb_get_backend_id(aContext);
}

int
GetMaxChannelCount(cubeb* aContext, uint32_t* aMaxChannels)
{
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    int ret = CUBEB_ERROR;
    if (!Audio()->SendGetMaxChannelCount(aContext->id, aMaxChannels, &ret)) {
      *aMaxChannels = 0;
      return CUBEB_ERROR;
    }

    return ret;
  }

  return cubeb_get_max_channel_count(aContext, aMaxChannels);
}

int
GetMinLatency(cubeb* aContext,
              cubeb_stream_params aParams,
              uint32_t* aLatencyFrame)
{
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    int ret = CUBEB_ERROR;
    if (!Audio()->SendGetMinLatency(aContext->id,
        aParams, aLatencyFrame, &ret)) {
      *aLatencyFrame = 0;
      return CUBEB_ERROR;
    }

    return ret;
  }

  return cubeb_get_min_latency(aContext, aParams, aLatencyFrame);
}

int
GetPreferredSampleRate(cubeb* aContext, uint32_t* aRate)
{
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    int ret = CUBEB_ERROR;
    if (!Audio()->SendGetPreferredSampleRate(aContext->id, aRate, &ret)) {
      *aRate = 0;
      return CUBEB_ERROR;
    }

    return ret;
  }

  return cubeb_get_preferred_sample_rate(aContext, aRate);
}

void
Destroy(cubeb* aContext)
{
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    Audio()->SendDestroy(aContext->id);
    free(aContext);

    return;
  }

  cubeb_destroy(aContext);
}

} // namespace audio
} // namespace mozilla
