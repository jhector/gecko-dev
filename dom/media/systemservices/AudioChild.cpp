/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "AudioChild.h"

namespace mozilla {
namespace audio {

AudioChild::AudioChild()
{
  MOZ_COUNT_CTOR(AudioChild);
}

AudioChild::~AudioChild()
{
  MOZ_COUNT_DTOR(AudioChild);

  for (unsigned int i = 0; i < mChildStreams.Length(); i++) {
    delete mChildStreams[i];
    mChildStreams.RemoveElementAt(i);
  }
}

int
AudioChild::StreamInit(cubeb* aContext,
                       cubeb_stream** aStream,
                       char const* aStreamName,
                       cubeb_devid aInputDevice,
                       cubeb_stream_params* aInputStreamParams,
                       cubeb_devid aOutputDevice,
                       cubeb_stream_params* aOutputStreamParams,
                       int aLatencyFrames,
                       cubeb_data_callback aDataCallback,
                       cubeb_state_callback aStateCallback,
                       void* aUserPtr)
{
  nsAutoCString name(aStreamName);

  cubeb_stream* stream = new cubeb_stream();
  if (!stream) {
    return CUBEB_ERROR;
  }

  uint32_t id;
  if (!SendStreamInit(aContext->id, name, *aInputStreamParams,
                      *aOutputStreamParams, aLatencyFrames, &id)) {
    delete stream;
    return CUBEB_ERROR;
  }

  stream->id = id;
  stream->data_callback = aDataCallback;
  stream->data_callback = aDataCallback;
  stream->user_ptr = aUserPtr;

  mChildStreams.AppendElement(stream);

  return CUBEB_OK;
}

PAudioChild* CreateAudioChild()
{
  return new AudioChild();
}

} // namespace audio
} // namespace mozilla
