/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_Audio_h
#define mozilla_Audio_h

#include "cubeb/cubeb.h"

namespace mozilla {
namespace audio {

int Init(cubeb** aContext, char const* aContextName);
const char* GetBackendId(cubeb* aContext);
int GetMaxChannelCount(cubeb* aContext, uint32_t* aMaxChannels, int aRet);
/*
int GetMinLatency(cubeb* aContext,
                  cubeb_stream_params aParams,
                  uint32_t* aLatencyFrame);
int GetPreferredSampleRate(cubeb* aContext, uint32_t* aRate);
*/
void Destroy(cubeb* aContext);

/*
int StreamInit(cubeb* aContext,
               cubeb_stream** aStream,
               char const* aStreamName,
               cubeb_devid input_device,
               cubeb_stream_params* aInputStreamParams,
               cubeb_devid output_device,
               cubeb_stream_params* aOutputStreamParams,
               unsigned int aLatencyFrames,
               cubeb_data_callback aDataCallback,
               cubeb_state_callback aStateCallback,
               void* aUserPtr);
void StreamDestroy(cubeb_stream* aStream);
int StreamStart(cubeb_stream* aStream);
int StreamStop(cubeb_stream* aStream);
int StreamGetPosition(cubeb_stream* aStream, uint64_t* aPosition);
int StreamGetLatency(cubeb_stream* aStream, uint32_t* aLatency);
int StreamSetVolume(cubeb_stream* aStream, float aVolume);
int StreamSetPanning(cubeb_stream* aStream, int aPanning);
int StreamGetCurrentDevice(cubeb_stream* aStream,
                           cubeb_device** const aDevice);
int StreamDeviceDestroy(cubeb_stream* aStream, cubeb_device* aDevices);
int StreamRegisterDeviceChangedCallback(
  cubeb_stream* aStream,
  cubeb_device_changed_callback aDeviceChangedCallback);
int EnumerateDevices(cubeb* aContext,
                     cubeb_device_type aDevtype,
                     cubeb_device_collection** aCollection);
int DeviceCollectionDestroy(cubeb_device_collection* aCollection);
int DeviceInfoDestroy(cubeb_device_info* aInfo);
int RegisterDeviceCollectionChanged(
  cubeb* aContext,
  cubeb_device_type aDevtype,
  cubeb_device_collection_changed_callback aCallback,
  void* aUserPtr);
*/

} // namespace audio
} // namespace mozilla

#endif  // mozilla_Audio_h
