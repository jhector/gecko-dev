/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "WidevineDecryptor.h"

#include "WidevineAdapter.h"
#include "WidevineUtils.h"
#include "WidevineFileIO.h"
#include "GMPPlatform.h"
#include <mozilla/SizePrintfMacros.h>
#include <stdarg.h>
#include "TimeUnits.h"

using namespace cdm;
using namespace std;

namespace mozilla {

static map<uint32_t, RefPtr<CDMWrapper>> sDecryptors;

/* static */
RefPtr<CDMWrapper>
WidevineDecryptor::GetInstance(uint32_t aInstanceId)
{
  auto itr = sDecryptors.find(aInstanceId);
  if (itr != sDecryptors.end()) {
    return itr->second;
  }
  return nullptr;
}


WidevineDecryptor::WidevineDecryptor()
  : mCallback(nullptr)
{
  CDM_LOG("WidevineDecryptor created this=%p, instanceId=%u", this, mInstanceId);
  AddRef(); // Released in DecryptingComplete().
}

WidevineDecryptor::~WidevineDecryptor()
{
  CDM_LOG("WidevineDecryptor destroyed this=%p, instanceId=%u", this, mInstanceId);
}

void
WidevineDecryptor::SetCDM(RefPtr<CDMWrapper> aCDM, uint32_t aInstanceId)
{
  mCDM = aCDM;
  mInstanceId = aInstanceId;
  sDecryptors[mInstanceId] = aCDM.forget();
}

void
WidevineDecryptor::Init(GMPDecryptorCallback* aCallback,
                        bool aDistinctiveIdentifierRequired,
                        bool aPersistentStateRequired)
{
  CDM_LOG("WidevineDecryptor::Init() this=%p distinctiveId=%d persistentState=%d",
          this, aDistinctiveIdentifierRequired, aPersistentStateRequired);
  MOZ_ASSERT(aCallback);
  mCallback = aCallback;
  MOZ_ASSERT(mCDM);
  mDistinctiveIdentifierRequired = aDistinctiveIdentifierRequired;
  mPersistentStateRequired = aPersistentStateRequired;
  if (CDM()) {
    CDM()->Initialize(aDistinctiveIdentifierRequired,
                      aPersistentStateRequired);
  }
}

static SessionType
ToCDMSessionType(GMPSessionType aSessionType)
{
  switch (aSessionType) {
    case kGMPTemporySession: return kTemporary;
    case kGMPPersistentSession: return kPersistentLicense;
    case kGMPSessionInvalid: return kTemporary;
    // TODO: kPersistentKeyRelease
  }
  MOZ_ASSERT(false); // Not supposed to get here.
  return kTemporary;
}

void
WidevineDecryptor::CreateSession(uint32_t aCreateSessionToken,
                                 uint32_t aPromiseId,
                                 const char* aInitDataType,
                                 uint32_t aInitDataTypeSize,
                                 const uint8_t* aInitData,
                                 uint32_t aInitDataSize,
                                 GMPSessionType aSessionType)
{
  CDM_LOG("Decryptor::CreateSession(token=%d, pid=%d)", aCreateSessionToken, aPromiseId);
  InitDataType initDataType;
  if (!strcmp(aInitDataType, "cenc")) {
    initDataType = kCenc;
  } else if (!strcmp(aInitDataType, "webm")) {
    initDataType = kWebM;
  } else if (!strcmp(aInitDataType, "keyids")) {
    initDataType = kKeyIds;
  } else {
    // Invalid init data type
    const char* errorMsg = "Invalid init data type when creating session.";
    OnRejectPromise(aPromiseId, kNotSupportedError, 0, errorMsg, sizeof(errorMsg));
    return;
  }
  mPromiseIdToNewSessionTokens[aPromiseId] = aCreateSessionToken;
  CDM()->CreateSessionAndGenerateRequest(aPromiseId,
                                         ToCDMSessionType(aSessionType),
                                         initDataType,
                                         aInitData, aInitDataSize);
}

void
WidevineDecryptor::LoadSession(uint32_t aPromiseId,
                               const char* aSessionId,
                               uint32_t aSessionIdLength)
{
  CDM_LOG("Decryptor::LoadSession(pid=%d, %s)", aPromiseId, aSessionId);
  // TODO: session type??
  CDM()->LoadSession(aPromiseId, kPersistentLicense, aSessionId, aSessionIdLength);
}

void
WidevineDecryptor::UpdateSession(uint32_t aPromiseId,
                                 const char* aSessionId,
                                 uint32_t aSessionIdLength,
                                 const uint8_t* aResponse,
                                 uint32_t aResponseSize)
{
  CDM_LOG("Decryptor::UpdateSession(pid=%d, session=%s)", aPromiseId, aSessionId);
  CDM()->UpdateSession(aPromiseId, aSessionId, aSessionIdLength, aResponse, aResponseSize);
}

void
WidevineDecryptor::CloseSession(uint32_t aPromiseId,
                                const char* aSessionId,
                                uint32_t aSessionIdLength)
{
  CDM_LOG("Decryptor::CloseSession(pid=%d, session=%s)", aPromiseId, aSessionId);
  CDM()->CloseSession(aPromiseId, aSessionId, aSessionIdLength);
}

void
WidevineDecryptor::RemoveSession(uint32_t aPromiseId,
                                 const char* aSessionId,
                                 uint32_t aSessionIdLength)
{
  CDM_LOG("Decryptor::RemoveSession(%s)", aSessionId);
  CDM()->RemoveSession(aPromiseId, aSessionId, aSessionIdLength);
}

void
WidevineDecryptor::SetServerCertificate(uint32_t aPromiseId,
                                        const uint8_t* aServerCert,
                                        uint32_t aServerCertSize)
{
  CDM_LOG("Decryptor::SetServerCertificate()");
  CDM()->SetServerCertificate(aPromiseId, aServerCert, aServerCertSize);
}

class WidevineDecryptedBlock : public cdm::DecryptedBlock {
public:

  WidevineDecryptedBlock()
    : mBuffer(nullptr)
    , mTimestamp(0)
  {
  }

  ~WidevineDecryptedBlock() override {
    if (mBuffer) {
      mBuffer->Destroy();
      mBuffer = nullptr;
    }
  }

  void SetDecryptedBuffer(cdm::Buffer* aBuffer) override {
    mBuffer = aBuffer;
  }

  cdm::Buffer* DecryptedBuffer() override {
    return mBuffer;
  }

  void SetTimestamp(int64_t aTimestamp) override {
    mTimestamp = aTimestamp;
  }

  int64_t Timestamp() const override {
    return mTimestamp;
  }

private:
  cdm::Buffer* mBuffer;
  int64_t mTimestamp;
};

cdm::Time
WidevineDecryptor::ThrottleDecrypt(cdm::Time aWallTime, cdm::Time aSampleDuration)
{
  const cdm::Time WindowSize = 1.0;
  const cdm::Time MaxThroughput = 2.0;

  // Forget decrypts that happened before the start of our window.
  while (!mDecrypts.empty() && mDecrypts.front().mWallTime < aWallTime - WindowSize) {
    mDecrypts.pop_front();
  }

  // How much time duration of the media would we have decrypted inside the
  // time window if we did decrypt this block?
  cdm::Time durationDecrypted = aSampleDuration;
  for (const DecryptJob& job : mDecrypts) {
    durationDecrypted += job.mSampleDuration;
  }

  if (durationDecrypted > MaxThroughput) {
    // If we decrypted a sample of this duration, we would have decrypted more than
    // our threshold for max throughput, over the preceding wall time window.
    return durationDecrypted - MaxThroughput;
  }

  return 0.0;
}

void
WidevineDecryptor::Decrypt(GMPBuffer* aBuffer,
                           GMPEncryptedBufferMetadata* aMetadata,
                           uint64_t aDurationUsecs)
{
  if (!mCallback) {
    CDM_LOG("WidevineDecryptor::Decrypt() this=%p FAIL; !mCallback", this);
    return;
  }

  cdm::Time duration = double(aDurationUsecs) / USECS_PER_S;
  mPendingDecrypts.push({aBuffer, aMetadata, duration});
  ProcessDecrypts();
}

void
WidevineDecryptor::ProcessDecryptsFromTimer()
{
  MOZ_ASSERT(mPendingDecryptTimerSet);
  mPendingDecryptTimerSet = false;
  ProcessDecrypts();
}

void
WidevineDecryptor::ProcessDecrypts()
{
  while (!mPendingDecrypts.empty()) {
    PendingDecrypt job = mPendingDecrypts.front();

    // We throttle our decrypt so that we don't decrypt more than a certain
    // duration of samples per second. This is to work around bugs in the
    // Widevine CDM. See bug 1338924.
    cdm::Time now = GetCurrentWallTime();
    cdm::Time delay = ThrottleDecrypt(now, job.mSampleDuration);

    if (delay > 0.0) {
      // If we decrypted this sample now, we'd decrypt more than our threshold
      // per second of samples. Enqueue the sample, and wait until we'd be able
      // to decrypt it without breaking our throughput threshold.
      if (!mPendingDecryptTimerSet) {
        mPendingDecryptTimerSet = true;
        RefPtr<WidevineDecryptor> self = this;
        GMPTask* task = gmp::NewGMPTask(
          [self]() {
            self->ProcessDecryptsFromTimer();
          });
        gmp::SetTimerOnMainThread(task, delay * 1000);
      }
      return;
    }
    DecryptBuffer(job);
    mDecrypts.push_back(DecryptJob(now, job.mSampleDuration));
    mPendingDecrypts.pop();
  }
}

void
WidevineDecryptor::DecryptBuffer(const PendingDecrypt& aJob)
{
  GMPBuffer* buffer = aJob.mBuffer;
  const GMPEncryptedBufferMetadata* crypto = aJob.mMetadata;
  InputBuffer sample;
  nsTArray<SubsampleEntry> subsamples;
  InitInputBuffer(crypto, buffer->Id(), buffer->Data(), buffer->Size(), sample, subsamples);
  WidevineDecryptedBlock decrypted;
  Status rv = CDM()->Decrypt(sample, &decrypted);
  CDM_LOG("Decryptor::Decrypt(timestamp=%" PRId64 ") rv=%d sz=%d",
          sample.timestamp, rv, decrypted.DecryptedBuffer()->Size());
  if (rv == kSuccess) {
    buffer->Resize(decrypted.DecryptedBuffer()->Size());
    memcpy(buffer->Data(),
           decrypted.DecryptedBuffer()->Data(),
           decrypted.DecryptedBuffer()->Size());
  }
  mCallback->Decrypted(buffer, ToGMPErr(rv));
}

void
WidevineDecryptor::DecryptingComplete()
{
  CDM_LOG("WidevineDecryptor::DecryptingComplete() this=%p, instanceId=%u",
          this, mInstanceId);

  // Ensure buffers are freed.
  while (!mPendingDecrypts.empty()) {
    PendingDecrypt& job = mPendingDecrypts.front();
    if (mCallback) {
      mCallback->Decrypted(job.mBuffer, GMPAbortedErr);
    }
    mPendingDecrypts.pop();
  }

  // Drop our references to the CDMWrapper. When any other references
  // held elsewhere are dropped (for example references held by a
  // WidevineVideoDecoder, or a runnable), the CDMWrapper destroys
  // the CDM.
  mCDM = nullptr;
  sDecryptors.erase(mInstanceId);
  mCallback = nullptr;
  Release();
}

class WidevineBuffer : public cdm::Buffer {
public:
  explicit WidevineBuffer(size_t aSize) {
    CDM_LOG("WidevineBuffer(size=%" PRIuSIZE ") created", aSize);
    mBuffer.SetLength(aSize);
  }
  ~WidevineBuffer() override {
    CDM_LOG("WidevineBuffer(size=%" PRIu32 ") destroyed", Size());
  }
  void Destroy() override { delete this; }
  uint32_t Capacity() const override { return mBuffer.Length(); };
  uint8_t* Data() override { return mBuffer.Elements(); }
  void SetSize(uint32_t aSize) override { mBuffer.SetLength(aSize); }
  uint32_t Size() const override { return mBuffer.Length(); }

private:
  WidevineBuffer(const WidevineBuffer&);
  void operator=(const WidevineBuffer&);

  nsTArray<uint8_t> mBuffer;
};

Buffer*
WidevineDecryptor::Allocate(uint32_t aCapacity)
{
  CDM_LOG("Decryptor::Allocate(capacity=%u)", aCapacity);
  return new WidevineBuffer(aCapacity);
}

class TimerTask : public GMPTask {
public:
  TimerTask(WidevineDecryptor* aDecryptor,
            RefPtr<CDMWrapper> aCDM,
            void* aContext)
    : mDecryptor(aDecryptor)
    , mCDM(aCDM)
    , mContext(aContext)
  {
  }
  ~TimerTask() override = default;
  void Run() override {
    mCDM->GetCDM()->TimerExpired(mContext);
  }
  void Destroy() override { delete this; }
private:
  RefPtr<WidevineDecryptor> mDecryptor;
  RefPtr<CDMWrapper> mCDM;
  void* mContext;
};

void
WidevineDecryptor::SetTimer(int64_t aDelayMs, void* aContext)
{
  CDM_LOG("Decryptor::SetTimer(delay_ms=%" PRId64 ", context=0x%p)", aDelayMs, aContext);
  if (mCDM) {
    GMPSetTimerOnMainThread(new TimerTask(this, mCDM, aContext), aDelayMs);
  }
}

Time
WidevineDecryptor::GetCurrentWallTime()
{
  GMPTimestamp gmpTime = 0;
  GMPGetCurrentTime(&gmpTime);
  double t = (double)gmpTime / 1e3;
  CDM_LOG("Decryptor::GetCurrentWallTime()= %lf", t);
  return t;
}

void
WidevineDecryptor::OnResolveNewSessionPromise(uint32_t aPromiseId,
                                              const char* aSessionId,
                                              uint32_t aSessionIdSize)
{
  if (!mCallback) {
    CDM_LOG("Decryptor::OnResolveNewSessionPromise(aPromiseId=0x%d) FAIL; !mCallback", aPromiseId);
    return;
  }

  // This is laid out in the API. If we fail to load a session we should
  // call OnResolveNewSessionPromise with nullptr as the sessionId.
  // We can safely assume this means that we have failed to load a session
  // as the other methods specify calling 'OnRejectPromise' when they fail.
  if (!aSessionId) {
    CDM_LOG("Decryptor::OnResolveNewSessionPromise(aPromiseId=0x%d) Failed to load session", aPromiseId);
    mCallback->ResolveLoadSessionPromise(aPromiseId, false);
    return;
  }

  CDM_LOG("Decryptor::OnResolveNewSessionPromise(aPromiseId=0x%d)", aPromiseId);
  auto iter = mPromiseIdToNewSessionTokens.find(aPromiseId);
  if (iter == mPromiseIdToNewSessionTokens.end()) {
    CDM_LOG("FAIL: Decryptor::OnResolveNewSessionPromise(aPromiseId=%d) unknown aPromiseId", aPromiseId);
    return;
  }
  mCallback->SetSessionId(iter->second, aSessionId, aSessionIdSize);
  mCallback->ResolvePromise(aPromiseId);
  mPromiseIdToNewSessionTokens.erase(iter);
}

void
WidevineDecryptor::OnResolvePromise(uint32_t aPromiseId)
{
  if (!mCallback) {
    CDM_LOG("Decryptor::OnResolvePromise(aPromiseId=0x%d) FAIL; !mCallback", aPromiseId);
    return;
  }
  CDM_LOG("Decryptor::OnResolvePromise(aPromiseId=%d)", aPromiseId);
  mCallback->ResolvePromise(aPromiseId);
}

static GMPDOMException
ToGMPDOMException(cdm::Error aError)
{
  switch (aError) {
    case kNotSupportedError: return kGMPNotSupportedError;
    case kInvalidStateError: return kGMPInvalidStateError;
    case kInvalidAccessError:
      // Note: Chrome converts kInvalidAccessError to TypeError, since the
      // Chromium CDM API doesn't have a type error enum value. The EME spec
      // requires TypeError in some places, so we do the same conversion.
      // See bug 1313202.
      return kGMPTypeError;
    case kQuotaExceededError: return kGMPQuotaExceededError;
    case kUnknownError: return kGMPInvalidModificationError; // Note: Unique placeholder.
    case kClientError: return kGMPAbortError; // Note: Unique placeholder.
    case kOutputError: return kGMPSecurityError; // Note: Unique placeholder.
  };
  return kGMPTimeoutError; // Note: Unique placeholder.
}

void
WidevineDecryptor::OnRejectPromise(uint32_t aPromiseId,
                                   Error aError,
                                   uint32_t aSystemCode,
                                   const char* aErrorMessage,
                                   uint32_t aErrorMessageSize)
{
  if (!mCallback) {
    CDM_LOG("Decryptor::OnRejectPromise(aPromiseId=%d, err=%d, sysCode=%u, msg=%s) FAIL; !mCallback",
            aPromiseId, (int)aError, aSystemCode, aErrorMessage);
    return;
  }
  CDM_LOG("Decryptor::OnRejectPromise(aPromiseId=%d, err=%d, sysCode=%u, msg=%s)",
          aPromiseId, (int)aError, aSystemCode, aErrorMessage);
  mCallback->RejectPromise(aPromiseId,
                           ToGMPDOMException(aError),
                           !aErrorMessageSize ? "" : aErrorMessage,
                           aErrorMessageSize);
}

static GMPSessionMessageType
ToGMPMessageType(MessageType message_type)
{
  switch (message_type) {
    case kLicenseRequest: return kGMPLicenseRequest;
    case kLicenseRenewal: return kGMPLicenseRenewal;
    case kLicenseRelease: return kGMPLicenseRelease;
  }
  return kGMPMessageInvalid;
}

void
WidevineDecryptor::OnSessionMessage(const char* aSessionId,
                                    uint32_t aSessionIdSize,
                                    MessageType aMessageType,
                                    const char* aMessage,
                                    uint32_t aMessageSize,
                                    const char* aLegacyDestinationUrl,
                                    uint32_t aLegacyDestinationUrlLength)
{
  if (!mCallback) {
    CDM_LOG("Decryptor::OnSessionMessage() FAIL; !mCallback");
    return;
  }
  CDM_LOG("Decryptor::OnSessionMessage()");
  mCallback->SessionMessage(aSessionId,
                            aSessionIdSize,
                            ToGMPMessageType(aMessageType),
                            reinterpret_cast<const uint8_t*>(aMessage),
                            aMessageSize);
}

static GMPMediaKeyStatus
ToGMPKeyStatus(KeyStatus aStatus)
{
  switch (aStatus) {
    case kUsable: return kGMPUsable;
    case kInternalError: return kGMPInternalError;
    case kExpired: return kGMPExpired;
    case kOutputRestricted: return kGMPOutputRestricted;
    case kOutputDownscaled: return kGMPOutputDownscaled;
    case kStatusPending: return kGMPStatusPending;
    case kReleased: return kGMPReleased;
  }
  return kGMPUnknown;
}

void
WidevineDecryptor::OnSessionKeysChange(const char* aSessionId,
                                       uint32_t aSessionIdSize,
                                       bool aHasAdditionalUsableKey,
                                       const KeyInformation* aKeysInfo,
                                       uint32_t aKeysInfoCount)
{
  if (!mCallback) {
    CDM_LOG("Decryptor::OnSessionKeysChange() FAIL; !mCallback");
    return;
  }
  CDM_LOG("Decryptor::OnSessionKeysChange()");

  nsTArray<GMPMediaKeyInfo> key_infos;
  for (uint32_t i = 0; i < aKeysInfoCount; i++) {
    key_infos.AppendElement(GMPMediaKeyInfo(aKeysInfo[i].key_id,
                                            aKeysInfo[i].key_id_size,
                                            ToGMPKeyStatus(aKeysInfo[i].status)));
  }
  mCallback->BatchedKeyStatusChanged(aSessionId, aSessionIdSize,
                                     key_infos.Elements(), key_infos.Length());
}

static GMPTimestamp
ToGMPTime(Time aCDMTime)
{
  return static_cast<GMPTimestamp>(aCDMTime * 1000);
}

void
WidevineDecryptor::OnExpirationChange(const char* aSessionId,
                                      uint32_t aSessionIdSize,
                                      Time aNewExpiryTime)
{
  if (!mCallback) {
    CDM_LOG("Decryptor::OnExpirationChange(sid=%s) t=%lf FAIL; !mCallback",
            aSessionId, aNewExpiryTime);
    return;
  }
  CDM_LOG("Decryptor::OnExpirationChange(sid=%s) t=%lf", aSessionId, aNewExpiryTime);
  GMPTimestamp expiry = ToGMPTime(aNewExpiryTime);
  if (aNewExpiryTime == 0) {
    return;
  }
  mCallback->ExpirationChange(aSessionId, aSessionIdSize, expiry);
}

void
WidevineDecryptor::OnSessionClosed(const char* aSessionId,
                                   uint32_t aSessionIdSize)
{
  if (!mCallback) {
    CDM_LOG("Decryptor::OnSessionClosed(sid=%s) FAIL; !mCallback", aSessionId);
    return;
  }
  CDM_LOG("Decryptor::OnSessionClosed(sid=%s)", aSessionId);
  mCallback->SessionClosed(aSessionId, aSessionIdSize);
}

void
WidevineDecryptor::OnLegacySessionError(const char* aSessionId,
                                        uint32_t aSessionIdLength,
                                        Error aError,
                                        uint32_t aSystemCode,
                                        const char* aErrorMessage,
                                        uint32_t aErrorMessageLength)
{
  if (!mCallback) {
    CDM_LOG("Decryptor::OnLegacySessionError(sid=%s, error=%d) FAIL; !mCallback",
            aSessionId, (int)aError);
    return;
  }
  CDM_LOG("Decryptor::OnLegacySessionError(sid=%s, error=%d)", aSessionId, (int)aError);
  mCallback->SessionError(aSessionId,
                          aSessionIdLength,
                          ToGMPDOMException(aError),
                          aSystemCode,
                          aErrorMessage,
                          aErrorMessageLength);
}

void
WidevineDecryptor::SendPlatformChallenge(const char* aServiceId,
                                         uint32_t aServiceIdSize,
                                         const char* aChallenge,
                                         uint32_t aChallengeSize)
{
  CDM_LOG("Decryptor::SendPlatformChallenge(service_id=%s)", aServiceId);
}

void
WidevineDecryptor::EnableOutputProtection(uint32_t aDesiredProtectionMask)
{
  CDM_LOG("Decryptor::EnableOutputProtection(mask=0x%x)", aDesiredProtectionMask);
}

void
WidevineDecryptor::QueryOutputProtectionStatus()
{
  CDM_LOG("Decryptor::QueryOutputProtectionStatus()");
}

void
WidevineDecryptor::OnDeferredInitializationDone(StreamType aStreamType,
                                                Status aDecoderStatus)
{
  CDM_LOG("Decryptor::OnDeferredInitializationDone()");
}

FileIO*
WidevineDecryptor::CreateFileIO(FileIOClient* aClient)
{
  CDM_LOG("Decryptor::CreateFileIO()");
  if (!mPersistentStateRequired) {
    return nullptr;
  }
  return new WidevineFileIO(aClient);
}

} // namespace mozilla
