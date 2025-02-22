/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GMPDecryptorChild_h_
#define GMPDecryptorChild_h_

#include "mozilla/gmp/PGMPDecryptorChild.h"
#include "gmp-decryption.h"
#include "mozilla/gmp/GMPTypes.h"
#include "GMPEncryptedBufferDataImpl.h"
#include <string>

namespace mozilla {
namespace gmp {

class GMPContentChild;

class GMPDecryptorChild : public GMPDecryptorCallback
                        , public PGMPDecryptorChild
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(GMPDecryptorChild);

  explicit GMPDecryptorChild(GMPContentChild* aPlugin);

  void Init(GMPDecryptor* aSession);

  // GMPDecryptorCallback
  void SetSessionId(uint32_t aCreateSessionToken,
                    const char* aSessionId,
                    uint32_t aSessionIdLength) override;
  void ResolveLoadSessionPromise(uint32_t aPromiseId,
                                 bool aSuccess) override;
  void ResolvePromise(uint32_t aPromiseId) override;

  void RejectPromise(uint32_t aPromiseId,
                     GMPDOMException aException,
                     const char* aMessage,
                     uint32_t aMessageLength) override;

  void SessionMessage(const char* aSessionId,
                      uint32_t aSessionIdLength,
                      GMPSessionMessageType aMessageType,
                      const uint8_t* aMessage,
                      uint32_t aMessageLength) override;

  void ExpirationChange(const char* aSessionId,
                        uint32_t aSessionIdLength,
                        GMPTimestamp aExpiryTime) override;

  void SessionClosed(const char* aSessionId,
                     uint32_t aSessionIdLength) override;

  void SessionError(const char* aSessionId,
                    uint32_t aSessionIdLength,
                    GMPDOMException aException,
                    uint32_t aSystemCode,
                    const char* aMessage,
                    uint32_t aMessageLength) override;

  void KeyStatusChanged(const char* aSessionId,
                        uint32_t aSessionIdLength,
                        const uint8_t* aKeyId,
                        uint32_t aKeyIdLength,
                        GMPMediaKeyStatus aStatus) override;

  void SetCapabilities(uint64_t aCaps) override;

  void Decrypted(GMPBuffer* aBuffer, GMPErr aResult) override;

  void BatchedKeyStatusChanged(const char* aSessionId,
                               uint32_t aSessionIdLength,
                               const GMPMediaKeyInfo* aKeyInfos,
                               uint32_t aKeyInfosLength) override;

private:
  ~GMPDecryptorChild();

  // GMPDecryptorChild
  mozilla::ipc::IPCResult RecvInit(const bool& aDistinctiveIdentifierRequired,
                                   const bool& aPersistentStateRequired) override;

  mozilla::ipc::IPCResult RecvCreateSession(const uint32_t& aCreateSessionToken,
                                            const uint32_t& aPromiseId,
                                            const nsCString& aInitDataType,
                                            InfallibleTArray<uint8_t>&& aInitData,
                                            const GMPSessionType& aSessionType) override;

  mozilla::ipc::IPCResult RecvLoadSession(const uint32_t& aPromiseId,
                                          const nsCString& aSessionId) override;

  mozilla::ipc::IPCResult RecvUpdateSession(const uint32_t& aPromiseId,
                                            const nsCString& aSessionId,
                                            InfallibleTArray<uint8_t>&& aResponse) override;

  mozilla::ipc::IPCResult RecvCloseSession(const uint32_t& aPromiseId,
                                           const nsCString& aSessionId) override;

  mozilla::ipc::IPCResult RecvRemoveSession(const uint32_t& aPromiseId,
                                            const nsCString& aSessionId) override;

  mozilla::ipc::IPCResult RecvDecrypt(const uint32_t& aId,
                                      InfallibleTArray<uint8_t>&& aBuffer,
                                      const GMPDecryptionData& aMetadata,
                                      const uint64_t& aDurationUsecs) override;

  // Resolve/reject promise on completion.
  mozilla::ipc::IPCResult RecvSetServerCertificate(const uint32_t& aPromiseId,
                                                   InfallibleTArray<uint8_t>&& aServerCert) override;

  mozilla::ipc::IPCResult RecvDecryptingComplete() override;

  template <typename MethodType, typename... ParamType>
  void CallMethod(MethodType, ParamType&&...);

  template<typename MethodType, typename... ParamType>
  void CallOnGMPThread(MethodType, ParamType&&...);

  // GMP's GMPDecryptor implementation.
  // Only call into this on the (GMP process) main thread.
  GMPDecryptor* mSession;
  GMPContentChild* mPlugin;
};

} // namespace gmp
} // namespace mozilla

#endif // GMPDecryptorChild_h_
