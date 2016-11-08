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

typedef struct stream_info {
  uint32_t id;
  cubeb_stream* stream;
} StreamInfo;

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
  virtual bool RecvGetMinLatency(const uint32_t& aCtxId,
                                 const cubeb_stream_params& aParams,
                                 uint32_t* aLatencyFrame,
                                 int* aRet) override;
  virtual bool RecvGetPreferredSampleRate(const uint32_t& aCtxId,
                                          uint32_t* aRate,
                                          int* aRet) override;
  virtual bool RecvDestroy(const uint32_t& aCtxId) override;

  virtual bool RecvStreamInit(const uint32_t& aCtxId,
                              const nsCString& aName,
                              const cubeb_stream_params& aInputParams,
                              const cubeb_stream_params& aOutputParams,
                              const int& aLatencyFrames,
                              uint32_t *aId) override;

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

private:
  long DataCallback(cubeb_stream *aStream, const void* aInputBuffer,
                    void* aOutputBuffer, long aFrames);
  void StateCallback(cubeb_stream *aStream, cubeb_state aState);

  // cubeb callbacks
  static long DataCallback_S(cubeb_stream* aStream, void* aThis,
                             const void* aInputBuffer, void* aOutputBuffer,
                             long aFrames)
  {
    return static_cast<AudioParent*>(aThis)->DataCallback(aStream, aInputBuffer,
      aOutputBuffer, aFrames);
  }

  static void StateCallback_S(cubeb_stream* aStream, void* aThis, cubeb_state aState)
  {
    static_cast<AudioParent*>(aThis)->StateCallback(aStream, aState);
  }

protected:
  uint32_t mContextIdCounter;
  uint32_t mStreamIdCounter;

  nsDataHashtable<nsUint32HashKey, cubeb*> mCubebContexts;
  nsDataHashtable<nsUint32HashKey, StreamInfo*> mCubebStreams;
};

PAudioParent* CreateAudioParent();

} // namespace audio
} // namespace mozilla

#endif  // mozilla_AudioParent_h
