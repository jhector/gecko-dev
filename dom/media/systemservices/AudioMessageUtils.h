/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_AudioMessageUtils_h
#define mozilla_AudioMessageUtils_h

#include "ipc/IPCMessageUtils.h"

#include "cubeb/cubeb.h"

namespace IPC {

struct CubebSampleFormatValidator {
  static bool IsLegalValue(cubeb_sample_format aValue) {
    switch (aValue) {
      case CUBEB_SAMPLE_S16LE:
      case CUBEB_SAMPLE_S16BE:
      case CUBEB_SAMPLE_FLOAT32LE:
      case CUBEB_SAMPLE_FLOAT32BE:
        return true;
      default:
        return false;
    }
  }
};

// Even though cubeb_sample_format is contiguous, the enum
// itself doesn't meet the requirements for using ContiguousEnumSerializer,
// so we are forced to use EnumSerializer and implement a custom validator
template<>
struct ParamTraits<cubeb_sample_format>
: public EnumSerializer<cubeb_sample_format, CubebSampleFormatValidator>
{ };

#if defined(__ANDROID__)
template<>
struct ParamTraits<cubeb_stream_type> :
  public ContiguousEnumSerializer<
                               cubeb_stream_type,
                               CUBEB_STREAM_TYPE_VOICE_CALL,
                               CUBEB_STREAM_TYPE_MAX>
{ };
#endif

template<>
struct ParamTraits<cubeb_stream_params>
{
  typedef cubeb_stream_params paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, aParam.format);
    WriteParam(aMsg, aParam.rate);
    WriteParam(aMsg, aParam.channels);
#if defined(__ANDROID__)
    WriteParam(aMsg, aParam.stream_type);
#endif
  }

  static bool Read(const Message* aMsg, PickleIterator* aIter, paramType* aResult)
  {
    cubeb_sample_format format;
    unsigned int rate;
    unsigned int channels;
#if defined(__ANDROID__)
    cubeb_stream_type type;
#endif
    if (ReadParam(aMsg, aIter, &format) &&
        ReadParam(aMsg, aIter, &rate) &&
        ReadParam(aMsg, aIter, &channels) &&
#if defined(__ANDROID__)
        ReadParam(aMsg, aIter, &type) &&
#endif
        true) {
      aResult->format = format;
      aResult->rate = rate;
      aResult->channels = channels;
#if defined(__ANDROID__)
      aResult->stream_type = type;
#endif
      return true;
    }
    return false;
  }

  static void Log(const paramType& aParam, std::wstring* aLog)
  {
    aLog->append(StringPrintf(L"[%d %d %d %d]", aParam.format,
                             aParam.rate, aParam.channels,
#if defined(__ANDROID__)
                             aParam.stream_type
#else
                             -1
#endif
                             ));

  }
};

} // namespace IPC

#endif  // mozilla_AudioMessageUtils_h
