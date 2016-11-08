/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "AudioParent.h"

#include <cstdio>

namespace mozilla {
namespace audio {

bool
AudioParent::RecvInit(const nsCString& aName, uint32_t* aId)
{
  cubeb* ctx;

  int rv = cubeb_init(&ctx, aName.get());
  if (rv != CUBEB_OK) {
    return false;
  }

  *aId = mContextIdCounter++;
  mCubebContexts.Put(*aId, ctx);

  return true;
}

bool
AudioParent::RecvGetBackendId(const uint32_t& aCtxId, nsCString* aName)
{
  cubeb* ctx = mCubebContexts.Get(aCtxId);

  if (ctx) {
    const char* backend = cubeb_get_backend_id(ctx);
    // TODO: better way to do this??
    nsAutoCString name(backend);
    *aName = name;
    return true;
  }

  return false;
}

bool
AudioParent::RecvGetMaxChannelCount(const uint32_t& aCtxId,
                                    uint32_t* aChannels,
                                    int* aRet)
{
  cubeb* ctx = mCubebContexts.Get(aCtxId);
  if (ctx) {
    *aRet = cubeb_get_max_channel_count(ctx, aChannels);
    return true;
  }

  return false;
}

bool
AudioParent::RecvGetMinLatency(const uint32_t& aCtxId,
                               const cubeb_stream_params& aParams,
                               uint32_t* aLatencyFrame,
                               int* aRet)
{
  cubeb* ctx = mCubebContexts.Get(aCtxId);
  if (ctx) {
    *aRet = cubeb_get_min_latency(ctx, aParams, aLatencyFrame);
  }

  return false;
}

bool
AudioParent::RecvGetPreferredSampleRate(const uint32_t& aCtxId,
                                        uint32_t* aRate,
                                        int* aRet)
{
  cubeb* ctx = mCubebContexts.Get(aCtxId);
  if (ctx) {
    *aRet = cubeb_get_preferred_sample_rate(ctx, aRate);
    return true;
  }

  return false;
}

bool
AudioParent::RecvDestroy(const uint32_t& aCtxId)
{
  cubeb* ctx = mCubebContexts.Get(aCtxId);
  if (ctx) {
    cubeb_destroy(ctx);
    mCubebContexts.Remove(aCtxId);
    return true;
  }

  return false;
}

bool
AudioParent::RecvStreamInit(const uint32_t& aCtxId,
                            const nsCString& aName,
                            const cubeb_stream_params& aInputParams,
                            const cubeb_stream_params& aOutputParams,
                            const int& aLatencyFrames,
                            uint32_t* aId)
{
  cubeb* ctx = mCubebContexts.Get(aCtxId);
  if (!ctx) {
    return false;
  }

  cubeb_stream_params inputParams = aInputParams;
  cubeb_stream_params outputParams = aOutputParams;

  StreamInfo* stream = new StreamInfo();
  int ret = cubeb_stream_init(ctx,
                              &stream->stream,
                              aName.get(),
                              nullptr, // TODO: cubeb_devid opaque handle
                              &inputParams,
                              nullptr, // TODO: cubeb_devid opaque handle
                              &outputParams,
                              (unsigned int) aLatencyFrames,
                              DataCallback_S,
                              StateCallback_S,
                              this);
  if (ret != CUBEB_OK) {
    return false;
  }

  *aId = mStreamIdCounter++;
  stream->id = *aId;
  mCubebStreams.Put(*aId, stream);

  return true;
}

void
AudioParent::ActorDestroy(ActorDestroyReason aWhy)
{
}

AudioParent::AudioParent()
  : mContextIdCounter(0)
  , mStreamIdCounter(0)
{
  MOZ_COUNT_CTOR(AudioParent);
}

AudioParent::~AudioParent()
{
  MOZ_COUNT_DTOR(AudioParent);

  for (auto iter = mCubebContexts.Iter(); !iter.Done(); iter.Next()) {
    cubeb_destroy(iter.Data());
    iter.Remove();
  }

  for (auto iter = mCubebStreams.Iter(); !iter.Done(); iter.Next()) {
    cubeb_stream_stop(iter.Data()->stream);
    cubeb_stream_destroy(iter.Data()->stream);
    delete iter.Data();
    iter.Remove();
  }
}

long
AudioParent::DataCallback(cubeb_stream *aStream, const void* aInputBuffer,
                          void* aOutputBuffer, long aFrames)
{
  // XXX
  return 0;
}

void
AudioParent::StateCallback(cubeb_stream *aStream, cubeb_state aState)
{
  // XXX
}

PAudioParent* CreateAudioParent()
{
  return new AudioParent();
}

} // namespace audio
} // namespace mozilla
