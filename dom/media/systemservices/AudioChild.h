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

// The datatype |cubeb_stream| is an opaque handle (see cubeb.h) of
// type struct cubeb_stream.
// On the child side the struct contains an ID which identifies the correct
// stream object on the parent side. It also contains the registered callbacks
// so if a callback is invoked on the parent side, we know which to call on the
// child side.
struct cubeb_stream {
  uint32_t id;
  cubeb_data_callback data_callback;
  cubeb_state_callback state_callback;
  void* user_ptr;
};

namespace mozilla {
namespace audio {

class AudioChild : public PAudioChild
{
public:
  AudioChild();
  virtual ~AudioChild();

  int StreamInit(cubeb* aContext,
                 cubeb_stream** aStream,
                 char const* aStreamName,
                 cubeb_devid aInputDevice,
                 cubeb_stream_params* aInputStreamParams,
                 cubeb_devid aOutputDevice,
                 cubeb_stream_params* aOutputStreamParams,
                 int aLatencyFrames,
                 cubeb_data_callback aDataCallback,
                 cubeb_state_callback aStateCallback,
                 void* aUserPtr);

protected:
  nsTArray<cubeb_stream*> mChildStreams;
};

PAudioChild* CreateAudioChild();

} // namespace audio
} // namespace mozilla

#endif  // mozilla_AudioChild_h
