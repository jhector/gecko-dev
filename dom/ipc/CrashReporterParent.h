/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_CrashReporterParent_h
#define mozilla_dom_CrashReporterParent_h

#include "mozilla/dom/PCrashReporterParent.h"
#include "mozilla/dom/TabMessageUtils.h"
#include "nsIFile.h"
#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#include "nsDataHashtable.h"
#endif

namespace mozilla {
namespace dom {

class CrashReporterParent : public PCrashReporterParent
{
#ifdef MOZ_CRASHREPORTER
  typedef CrashReporter::AnnotationTable AnnotationTable;
#endif
public:
  CrashReporterParent();
  virtual ~CrashReporterParent();

#ifdef MOZ_CRASHREPORTER

  /*
   * Attempt to create a bare-bones crash report, along with extra process-
   * specific annotations present in the given AnnotationTable. Calls
   * GenerateChildData and FinalizeChildData.
   *
   * @returns true if successful, false otherwise.
   */
  template<class Toplevel>
  bool
  GenerateCrashReport(Toplevel* t, const AnnotationTable* processNotes);

  /**
   * Apply child process annotations to an existing paired mindump generated
   * with GeneratePairedMinidump.
   *
   * Be careful about calling generate apis immediately after this call,
   * see FinalizeChildData.
   *
   * @param processNotes (optional) - Additional notes to append. Annotations
   *   stored in mNotes will also be applied. processNotes can be null.
   * @returns true if successful, false otherwise.
   */
  bool
  GenerateChildData(const AnnotationTable* processNotes);

  /**
   * Handles main thread finalization tasks after a report has been
   * generated. Does the following:
   *  - register the finished report with the crash service manager
   *  - records telemetry related data about crashes
   *
   * Be careful about calling generate apis immediately after this call,
   * if this api is called on a non-main thread it will fire off a runnable
   * to complete its work async.
   */
  void
  FinalizeChildData();

  /*
   * Attempt to generate a full paired dump complete with any child
   * annoations, and finalizes the report. Note this call is only valid
   * on the main thread. Calling on a background thread will fail.
   *
   * @returns true if successful, false otherwise.
   */
  template<class Toplevel>
  bool
  GenerateCompleteMinidump(Toplevel* t);

  /**
   * Submits a raw minidump handed in, calls GenerateChildData and
   * FinalizeChildData. Used by content plugins and gmp.
   *
   * @returns true if successful, false otherwise.
   */
  bool
  GenerateCrashReportForMinidump(nsIFile* minidump,
                                 const AnnotationTable* processNotes);
#endif // MOZ_CRASHREPORTER

  /*
   * Initialize this reporter with data from the child process.
   */
  void
  SetChildData(const NativeThreadId& id, const uint32_t& processType);

  /*
   * Returns the ID of the child minidump.
   * GeneratePairedMinidump or GenerateCrashReport must be called first.
   */
  const nsString& ChildDumpID() const {
    return mChildDumpID;
  }

  /*
   * Add an annotation to our internally tracked list of annotations.
   * Callers must apply these notes using GenerateChildData otherwise
   * the notes will get dropped.
   */
  void
  AnnotateCrashReport(const nsCString& aKey, const nsCString& aData);

 protected:
  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  virtual mozilla::ipc::IPCResult RecvAnnotateCrashReport(const nsCString& aKey,
                                                          const nsCString& aData) override
  {
    AnnotateCrashReport(aKey, aData);
    return IPC_OK();
  }

  virtual mozilla::ipc::IPCResult RecvAppendAppNotes(const nsCString& aData) override;

#ifdef MOZ_CRASHREPORTER
  void
  NotifyCrashService();
#endif

#ifdef MOZ_CRASHREPORTER
  AnnotationTable mNotes;
#endif
  nsCString mAppNotes;
  nsString mChildDumpID;
  // stores the child main thread id
  NativeThreadId mMainThread;
  time_t mStartTime;
  // stores the child process type
  GeckoProcessType mProcessType;
  bool mInitialized;
};

#ifdef MOZ_CRASHREPORTER
template<class Toplevel>
inline bool
CrashReporterParent::GenerateCrashReport(Toplevel* t,
                                         const AnnotationTable* processNotes)
{
  nsCOMPtr<nsIFile> crashDump;
  if (t->TakeMinidump(getter_AddRefs(crashDump), nullptr) &&
      CrashReporter::GetIDFromMinidump(crashDump, mChildDumpID)) {
    bool result = GenerateChildData(processNotes);
    FinalizeChildData();
    return result;
  }
  return false;
}

template<class Toplevel>
inline bool
CrashReporterParent::GenerateCompleteMinidump(Toplevel* t)
{
  mozilla::ipc::ScopedProcessHandle child;
  if (!NS_IsMainThread()) {
    NS_WARNING("GenerateCompleteMinidump can't be called on non-main thread.");
    return false;
  }

#ifdef XP_MACOSX
  child = t->Process()->GetChildTask();
#else
  if (!base::OpenPrivilegedProcessHandle(t->OtherPid(), &child.rwget())) {
    NS_WARNING("Failed to open child process handle.");
    return false;
  }
#endif
  nsCOMPtr<nsIFile> childDump;
  if (CrashReporter::CreateMinidumpsAndPair(child,
                                            mMainThread,
                                            NS_LITERAL_CSTRING("browser"),
                                            nullptr, // pair with a dump of this process and thread
                                            getter_AddRefs(childDump)) &&
      CrashReporter::GetIDFromMinidump(childDump, mChildDumpID)) {
    bool result = GenerateChildData(nullptr);
    FinalizeChildData();
    return result;
  }
  return false;
}

#endif

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_CrashReporterParent_h
