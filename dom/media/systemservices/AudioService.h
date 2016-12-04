/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_AudioService_h
#define mozilla_AudioService_h

#include "nsISupportsImpl.h"

class MessageLoop;
class cubeb;

namespace base {
class Thread;
} // namespace base

namespace mozilla {
namespace dom {
class ContentParent;
} // namespace dom

namespace audio {

class AudioService
{
private:
  AudioService();
  virtual ~AudioService();

public:
  NS_INLINE_DECL_REFCOUNTING(AudioService)

  static AudioService* Get() { return sInstance; }
  static AudioService* GetOrCreate();

  MessageLoop* ServiceLoop();

  static void Start(dom::ContentParent*);

private:
  static AudioService* sInstance;

  base::Thread* mThread;
};

} // namespace audio
} // namespace mozilla

#endif // mozilla_AudioService_h
