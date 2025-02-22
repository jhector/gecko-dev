/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WidevineUtils_h_
#define WidevineUtils_h_

#include "stddef.h"
#include "content_decryption_module.h"
#include "gmp-api/gmp-decryption.h"
#include "gmp-api/gmp-platform.h"
#include "nsISupportsImpl.h"
#include "nsTArray.h"
#include "mozilla/Logging.h"

namespace mozilla {

namespace detail {
LogModule* GetCDMLog();
} // namespace detail

#define CDM_LOG(...) MOZ_LOG(detail::GetCDMLog(), mozilla::LogLevel::Debug, (__VA_ARGS__))

#define ENSURE_TRUE(condition, rv) { \
  if (!(condition)) {\
    CDM_LOG("ENSURE_TRUE FAILED %s:%d", __FILE__, __LINE__); \
    return rv; \
  } \
} \

#define ENSURE_GMP_SUCCESS(err, rv) { \
  if (GMP_FAILED(err)) {\
    CDM_LOG("ENSURE_GMP_SUCCESS FAILED %s:%d", __FILE__, __LINE__); \
    return rv; \
    } \
} \

GMPErr
ToGMPErr(cdm::Status aStatus);

class WidevineDecryptor;

class CDMWrapper {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CDMWrapper)

  explicit CDMWrapper(cdm::ContentDecryptionModule_8* aCDM,
                      WidevineDecryptor* aDecryptor);
  cdm::ContentDecryptionModule_8* GetCDM() const { return mCDM; }
private:
  ~CDMWrapper();
  cdm::ContentDecryptionModule_8* mCDM;
  RefPtr<WidevineDecryptor> mDecryptor;
};

void InitInputBuffer(const GMPEncryptedBufferMetadata* aCrypto,
                     int64_t aTimestamp,
                     const uint8_t* aData,
                     size_t aDataSize,
                     cdm::InputBuffer &aInputBuffer,
                     nsTArray<cdm::SubsampleEntry> &aSubsamples);

} // namespace mozilla

#endif // WidevineUtils_h_
