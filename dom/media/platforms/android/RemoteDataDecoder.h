/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef RemoteDataDecoder_h_
#define RemoteDataDecoder_h_

#include "AndroidDecoderModule.h"
#include "FennecJNIWrappers.h"
#include "SurfaceTexture.h"
#include "TimeUnits.h"
#include "mozilla/Maybe.h"
#include "mozilla/Monitor.h"

namespace mozilla {

class RemoteDataDecoder : public MediaDataDecoder
{
public:
  static already_AddRefed<MediaDataDecoder>
  CreateAudioDecoder(const CreateDecoderParams& aParams,
                     const nsString& aDrmStubId,
                     CDMProxy* aProxy);

  static already_AddRefed<MediaDataDecoder>
  CreateVideoDecoder(const CreateDecoderParams& aParams,
                     const nsString& aDrmStubId,
                     CDMProxy* aProxy);

  RefPtr<DecodePromise> Decode(MediaRawData* aSample) override;
  RefPtr<DecodePromise> Drain() override;
  RefPtr<FlushPromise> Flush() override;
  RefPtr<ShutdownPromise> Shutdown() override;
  const char* GetDescriptionName() const override
  {
    return "android remote decoder";
  }

protected:
  virtual ~RemoteDataDecoder() { }
  RemoteDataDecoder(MediaData::Type aType,
                    const nsACString& aMimeType,
                    java::sdk::MediaFormat::Param aFormat,
                    const nsString& aDrmStubId, TaskQueue* aTaskQueue);

  // Methods only called on mTaskQueue.
  RefPtr<ShutdownPromise> ProcessShutdown();
  void Output(MediaData* aSample);
  void ReturnDecodedData();
  void DrainComplete();
  void Error(const MediaResult& aError);
  void AssertOnTaskQueue()
  {
    MOZ_ASSERT(mTaskQueue->IsCurrentThreadIn());
  }

  MediaData::Type mType;

  nsAutoCString mMimeType;
  java::sdk::MediaFormat::GlobalRef mFormat;

  java::CodecProxy::GlobalRef mJavaDecoder;
  java::CodecProxy::NativeCallbacks::GlobalRef mJavaCallbacks;
  nsString mDrmStubId;

  RefPtr<TaskQueue> mTaskQueue;
  // Only ever accessed on mTaskqueue.
  bool mShutdown = false;
  MozPromiseHolder<DecodePromise> mDecodePromise;
  MozPromiseHolder<DecodePromise> mDrainPromise;
  enum class DrainStatus
  {
    DRAINED,
    DRAINABLE,
    DRAINING,
  };
  DrainStatus mDrainStatus = DrainStatus::DRAINED;
  DecodedData mDecodedData;
};

} // namespace mozilla

#endif
