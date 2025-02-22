/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "FileBlobImpl.h"
#include "mozilla/dom/WorkerPrivate.h"
#include "mozilla/dom/WorkerRunnable.h"
#include "nsCExternalHandlerService.h"
#include "nsIFile.h"
#include "nsIFileStreams.h"
#include "nsIMIMEService.h"
#include "nsNetUtil.h"
#include "nsStreamUtils.h"
#include "SlicedInputStream.h"

namespace mozilla {
namespace dom {

using namespace workers;

NS_IMPL_ISUPPORTS_INHERITED0(FileBlobImpl, BlobImpl)

FileBlobImpl::FileBlobImpl(nsIFile* aFile)
  : BaseBlobImpl(EmptyString(), EmptyString(), UINT64_MAX, INT64_MAX)
  , mFile(aFile)
  , mWholeFile(true)
{
  MOZ_ASSERT(mFile, "must have file");
  // Lazily get the content type and size
  mContentType.SetIsVoid(true);
  mFile->GetLeafName(mName);
}

FileBlobImpl::FileBlobImpl(const nsAString& aName,
                           const nsAString& aContentType,
                           uint64_t aLength, nsIFile* aFile)
  : BaseBlobImpl(aName, aContentType, aLength, UINT64_MAX)
  , mFile(aFile)
  , mWholeFile(true)
{
  MOZ_ASSERT(mFile, "must have file");
}

FileBlobImpl::FileBlobImpl(const nsAString& aName,
                           const nsAString& aContentType,
                           uint64_t aLength, nsIFile* aFile,
                           int64_t aLastModificationDate)
  : BaseBlobImpl(aName, aContentType, aLength, aLastModificationDate)
  , mFile(aFile)
  , mWholeFile(true)
{
  MOZ_ASSERT(mFile, "must have file");
}

FileBlobImpl::FileBlobImpl(nsIFile* aFile, const nsAString& aName,
                           const nsAString& aContentType)
  : BaseBlobImpl(aName, aContentType, UINT64_MAX, INT64_MAX)
  , mFile(aFile)
  , mWholeFile(true)
{
  MOZ_ASSERT(mFile, "must have file");
  if (aContentType.IsEmpty()) {
    // Lazily get the content type and size
    mContentType.SetIsVoid(true);
  }
}

FileBlobImpl::FileBlobImpl(const FileBlobImpl* aOther, uint64_t aStart,
                           uint64_t aLength, const nsAString& aContentType)
  : BaseBlobImpl(aContentType, aOther->mStart + aStart, aLength)
  , mFile(aOther->mFile)
  , mWholeFile(false)
{
  MOZ_ASSERT(mFile, "must have file");
  mImmutable = aOther->mImmutable;
}

already_AddRefed<BlobImpl>
FileBlobImpl::CreateSlice(uint64_t aStart, uint64_t aLength,
                          const nsAString& aContentType,
                          ErrorResult& aRv)
{
  RefPtr<BlobImpl> impl =
    new FileBlobImpl(this, aStart, aLength, aContentType);
  return impl.forget();
}

void
FileBlobImpl::GetMozFullPathInternal(nsAString& aFilename,
                                     ErrorResult& aRv) const
{
  MOZ_ASSERT(mIsFile, "Should only be called on files");
  aRv = mFile->GetPath(aFilename);
}

uint64_t
FileBlobImpl::GetSize(ErrorResult& aRv)
{
  if (BaseBlobImpl::IsSizeUnknown()) {
    MOZ_ASSERT(mWholeFile,
                 "Should only use lazy size when using the whole file");
    int64_t fileSize;
    aRv = mFile->GetFileSize(&fileSize);
    if (NS_WARN_IF(aRv.Failed())) {
      return 0;
    }

    if (fileSize < 0) {
      aRv.Throw(NS_ERROR_FAILURE);
      return 0;
    }

    mLength = fileSize;
  }

  return mLength;
}

namespace {

class GetTypeRunnable final : public WorkerMainThreadRunnable
{
public:
  GetTypeRunnable(WorkerPrivate* aWorkerPrivate,
                  BlobImpl* aBlobImpl)
    : WorkerMainThreadRunnable(aWorkerPrivate,
                               NS_LITERAL_CSTRING("FileBlobImpl :: GetType"))
    , mBlobImpl(aBlobImpl)
  {
    MOZ_ASSERT(aBlobImpl);
    aWorkerPrivate->AssertIsOnWorkerThread();
  }

  bool
  MainThreadRun() override
  {
    MOZ_ASSERT(NS_IsMainThread());

    nsAutoString type;
    mBlobImpl->GetType(type);
    return true;
  }

private:
  ~GetTypeRunnable() = default;

  RefPtr<BlobImpl> mBlobImpl;
};

} // anonymous namespace

void
FileBlobImpl::GetType(nsAString& aType)
{
  aType.Truncate();

  if (mContentType.IsVoid()) {
    MOZ_ASSERT(mWholeFile,
               "Should only use lazy ContentType when using the whole file");

    if (!NS_IsMainThread()) {
      WorkerPrivate* workerPrivate = GetCurrentThreadWorkerPrivate();
      if (!workerPrivate) {
        // I have no idea in which thread this method is called. We cannot
        // return any valid value.
        return;
      }

      RefPtr<GetTypeRunnable> runnable =
        new GetTypeRunnable(workerPrivate, this);

      ErrorResult rv;
      runnable->Dispatch(Terminating, rv);
      if (NS_WARN_IF(rv.Failed())) {
        rv.SuppressException();
      }
      return;
    }

    nsresult rv;
    nsCOMPtr<nsIMIMEService> mimeService =
      do_GetService(NS_MIMESERVICE_CONTRACTID, &rv);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return;
    }

    nsAutoCString mimeType;
    rv = mimeService->GetTypeFromFile(mFile, mimeType);
    if (NS_FAILED(rv)) {
      mimeType.Truncate();
    }

    AppendUTF8toUTF16(mimeType, mContentType);
    mContentType.SetIsVoid(false);
  }

  aType = mContentType;
}

int64_t
FileBlobImpl::GetLastModified(ErrorResult& aRv)
{
  MOZ_ASSERT(mIsFile, "Should only be called on files");
  if (BaseBlobImpl::IsDateUnknown()) {
    PRTime msecs;
    aRv = mFile->GetLastModifiedTime(&msecs);
    if (NS_WARN_IF(aRv.Failed())) {
      return 0;
    }

    mLastModificationDate = msecs;
  }

  return mLastModificationDate;
}

void
FileBlobImpl::SetLastModified(int64_t aLastModified)
{
  MOZ_CRASH("SetLastModified of a real file is not allowed!");
}

const uint32_t sFileStreamFlags =
  nsIFileInputStream::CLOSE_ON_EOF |
  nsIFileInputStream::REOPEN_ON_REWIND |
  nsIFileInputStream::DEFER_OPEN |
  nsIFileInputStream::SHARE_DELETE;

void
FileBlobImpl::GetInternalStream(nsIInputStream** aStream, ErrorResult& aRv)
{
  if (mWholeFile) {
    aRv = NS_NewLocalFileInputStream(aStream, mFile, -1, -1, sFileStreamFlags);
    return;
  }

  aRv = NS_NewPartialLocalFileInputStream(aStream, mFile, mStart, mLength,
                                          -1, -1, sFileStreamFlags);
}

bool
FileBlobImpl::IsDirectory() const
{
  bool isDirectory = false;
  if (mFile) {
    mFile->IsDirectory(&isDirectory);
  }
  return isDirectory;
}

} // namespace dom
} // namespace mozilla
