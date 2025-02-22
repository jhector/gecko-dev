// Copyright (c) 2006-2011 The Chromium Authors. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
//  * Neither the name of Google, Inc. nor the names of its contributors
//    may be used to endorse or promote products derived from this
//    software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
// AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

// This file is used for both Linux and Android.

/*
# vim: sw=2
*/
#include <stdio.h>
#include <math.h>

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/prctl.h> // set name
#include <stdlib.h>
#include <sched.h>
#include <ucontext.h>
// Ubuntu Dapper requires memory pages to be marked as
// executable. Otherwise, OS raises an exception when executing code
// in that page.
#include <sys/types.h>  // mmap & munmap
#include <sys/mman.h>   // mmap & munmap
#include <sys/stat.h>   // open
#include <fcntl.h>      // open
#include <unistd.h>     // sysconf
#include <semaphore.h>
#ifdef __GLIBC__
#include <execinfo.h>   // backtrace, backtrace_symbols
#endif  // def __GLIBC__
#include <strings.h>    // index
#include <errno.h>
#include <stdarg.h>

#include "prenv.h"
#include "mozilla/Atomics.h"
#include "mozilla/LinuxSignal.h"
#include "mozilla/DebugOnly.h"
#include "ProfileEntry.h"

// Memory profile
#include "nsMemoryReporterManager.h"

#include <string.h>
#include <list>

using namespace mozilla;

// All accesses to these two variables are on the main thread, so no locking is
// needed.
static bool gIsSigprofHandlerInstalled;
static struct sigaction gOldSigprofHandler;

// All accesses to these two variables are on the main thread, so no locking is
// needed.
static bool gHasSigprofSenderLaunched;
static pthread_t gSigprofSenderThread;

#if defined(USE_LUL_STACKWALK)
// A singleton instance of the library.  It is initialised at first
// use.  Currently only the main thread can call PlatformStart(), so
// there is no need for a mechanism to ensure that it is only
// created once in a multi-thread-use situation.
lul::LUL* gLUL = nullptr;

// This is the gLUL initialization routine.
static void gLUL_initialization_routine(void)
{
  MOZ_ASSERT(!gLUL);
  MOZ_ASSERT(gettid() == getpid()); /* "this is the main thread" */
  gLUL = new lul::LUL(logging_sink_for_LUL);
  // Read all the unwind info currently available.
  read_procmaps(gLUL);
}
#endif

/* static */ Thread::tid_t
Thread::GetCurrentId()
{
  return gettid();
}

#if !defined(GP_OS_android)
// Keep track of when any of our threads calls fork(), so we can
// temporarily disable signal delivery during the fork() call.  Not
// doing so appears to cause a kind of race, in which signals keep
// getting delivered to the thread doing fork(), which keeps causing
// it to fail and be restarted; hence forward progress is delayed a
// great deal.  A side effect of this is to permanently disable
// sampling in the child process.  See bug 837390.

// Unfortunately this is only doable on non-Android, since Bionic
// doesn't have pthread_atfork.

// This records the current state at the time we paused it.
static bool gWasPaused = false;

// In the parent, just before the fork, record the pausedness state,
// and then pause.
static void paf_prepare(void) {
  // This function can run off the main thread.
  gWasPaused = gIsPaused;
  gIsPaused = true;
}

// In the parent, just after the fork, return pausedness to the
// pre-fork state.
static void paf_parent(void) {
  // This function can run off the main thread.
  gIsPaused = gWasPaused;
}

// Set up the fork handlers.
static void* setup_atfork() {
  pthread_atfork(paf_prepare, paf_parent, NULL);
  return NULL;
}
#endif /* !defined(GP_OS_android) */

static mozilla::Atomic<ThreadInfo*> gCurrentThreadInfo;
static sem_t gSignalHandlingDone;

static void SetSampleContext(TickSample* sample, void* context)
{
  // Extracting the sample from the context is extremely machine dependent.
  ucontext_t* ucontext = reinterpret_cast<ucontext_t*>(context);
  mcontext_t& mcontext = ucontext->uc_mcontext;
#if defined(GP_ARCH_x86)
  sample->pc = reinterpret_cast<Address>(mcontext.gregs[REG_EIP]);
  sample->sp = reinterpret_cast<Address>(mcontext.gregs[REG_ESP]);
  sample->fp = reinterpret_cast<Address>(mcontext.gregs[REG_EBP]);
#elif defined(GP_ARCH_amd64)
  sample->pc = reinterpret_cast<Address>(mcontext.gregs[REG_RIP]);
  sample->sp = reinterpret_cast<Address>(mcontext.gregs[REG_RSP]);
  sample->fp = reinterpret_cast<Address>(mcontext.gregs[REG_RBP]);
#elif defined(GP_ARCH_arm)
  sample->pc = reinterpret_cast<Address>(mcontext.arm_pc);
  sample->sp = reinterpret_cast<Address>(mcontext.arm_sp);
  sample->fp = reinterpret_cast<Address>(mcontext.arm_fp);
  sample->lr = reinterpret_cast<Address>(mcontext.arm_lr);
#else
# error "bad platform"
#endif
}

static void
SigprofHandler(int signal, siginfo_t* info, void* context)
{
  // Avoid TSan warning about clobbering errno.
  int savedErrno = errno;

  // XXX: this is an off-main-thread(?) use of gSampler
  if (!gSampler) {
    sem_post(&gSignalHandlingDone);
    errno = savedErrno;
    return;
  }

  TickSample sample_obj;
  TickSample* sample = &sample_obj;
  sample->context = context;

  // Extract the current pc and sp.
  SetSampleContext(sample, context);
  sample->threadInfo = gCurrentThreadInfo;
  sample->timestamp = mozilla::TimeStamp::Now();
  sample->rssMemory = sample->threadInfo->mRssMemory;
  sample->ussMemory = sample->threadInfo->mUssMemory;

  Tick(sample);

  gCurrentThreadInfo = NULL;
  sem_post(&gSignalHandlingDone);
  errno = savedErrno;
}

#if defined(GP_OS_android)
#define SYS_tgkill __NR_tgkill
#endif

int tgkill(pid_t tgid, pid_t tid, int signalno) {
  return syscall(SYS_tgkill, tgid, tid, signalno);
}

class PlatformData {
 public:
  PlatformData()
  {
    MOZ_COUNT_CTOR(PlatformData);
  }

  ~PlatformData()
  {
    MOZ_COUNT_DTOR(PlatformData);
  }
};

UniquePlatformData
AllocPlatformData(int aThreadId)
{
  return UniquePlatformData(new PlatformData);
}

void
PlatformDataDestructor::operator()(PlatformData* aData)
{
  delete aData;
}

static void
SleepMicro(int aMicroseconds)
{
  if (MOZ_UNLIKELY(aMicroseconds >= 1000000)) {
    // Use usleep for larger intervals, because the nanosleep
    // code below only supports intervals < 1 second.
    MOZ_ALWAYS_TRUE(!::usleep(aMicroseconds));
    return;
  }

  struct timespec ts;
  ts.tv_sec  = 0;
  ts.tv_nsec = aMicroseconds * 1000UL;

  int rv = ::nanosleep(&ts, &ts);

  while (rv != 0 && errno == EINTR) {
    // Keep waiting in case of interrupt.
    // nanosleep puts the remaining time back into ts.
    rv = ::nanosleep(&ts, &ts);
  }

  MOZ_ASSERT(!rv, "nanosleep call failed");
}

static void*
SigprofSender(void* aArg)
{
  // This function runs on its own thread.

  // Taken from platform_thread_posix.cc
  prctl(PR_SET_NAME, "SamplerThread", 0, 0, 0);

  int vm_tgid_ = getpid();
  DebugOnly<int> my_tid = gettid();

  TimeDuration lastSleepOverhead = 0;
  TimeStamp sampleStart = TimeStamp::Now();
  while (gIsActive) {
    gBuffer->deleteExpiredStoredMarkers();

    if (!gIsPaused) {
      StaticMutexAutoLock lock(gRegisteredThreadsMutex);

      bool isFirstProfiledThread = true;
      for (uint32_t i = 0; i < gRegisteredThreads->size(); i++) {
        ThreadInfo* info = (*gRegisteredThreads)[i];

        // This will be null if we're not interested in profiling this thread.
        if (!info->hasProfile() || info->IsPendingDelete()) {
          continue;
        }

        if (info->Stack()->CanDuplicateLastSampleDueToSleep()) {
          info->DuplicateLastSample();
          continue;
        }

        info->UpdateThreadResponsiveness();

        // We use gCurrentThreadInfo to pass the ThreadInfo for the
        // thread we're profiling to the signal handler.
        gCurrentThreadInfo = info;

        int threadId = info->ThreadId();
        MOZ_ASSERT(threadId != my_tid);

        // Profile from the signal sender for information which is not signal
        // safe, and will have low variation between the emission of the signal
        // and the signal handler catch.
        if (isFirstProfiledThread && gProfileMemory) {
          info->mRssMemory = nsMemoryReporterManager::ResidentFast();
          info->mUssMemory = nsMemoryReporterManager::ResidentUnique();
        } else {
          info->mRssMemory = 0;
          info->mUssMemory = 0;
        }

        // Profile from the signal handler for information which is signal safe
        // and needs to be precise too, such as the stack of the interrupted
        // thread.
        if (tgkill(vm_tgid_, threadId, SIGPROF) != 0) {
          printf_stderr("profiler failed to signal tid=%d\n", threadId);
#ifdef DEBUG
          abort();
#else
          continue;
#endif
        }

        // Wait for the signal handler to run before moving on to the next one
        sem_wait(&gSignalHandlingDone);
        isFirstProfiledThread = false;
      }
#if defined(USE_LUL_STACKWALK)
      // The LUL unwind object accumulates frame statistics. Periodically we
      // should poke it to give it a chance to print those statistics. This
      // involves doing I/O (fprintf, __android_log_print, etc.) and so can't
      // safely be done from the unwinder threads, which is why it is done
      // here.
      gLUL->MaybeShowStats();
#endif
    }

    // This off-main-thread use of gInterval is safe due to implicit
    // synchronization -- this function cannot run at the same time as
    // profiler_{start,stop}(), where gInterval is set.
    TimeStamp targetSleepEndTime =
      sampleStart + TimeDuration::FromMicroseconds(gInterval * 1000);
    TimeStamp beforeSleep = TimeStamp::Now();
    TimeDuration targetSleepDuration = targetSleepEndTime - beforeSleep;
    double sleepTime = std::max(0.0, (targetSleepDuration - lastSleepOverhead).ToMicroseconds());
    SleepMicro(sleepTime);
    sampleStart = TimeStamp::Now();
    lastSleepOverhead = sampleStart - (beforeSleep + TimeDuration::FromMicroseconds(sleepTime));
  }
  return 0;
}

static void
PlatformStart()
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());

#if defined(USE_EHABI_STACKWALK)
  mozilla::EHABIStackWalkInit();
#elif defined(USE_LUL_STACKWALK)
  // NOTE: this isn't thread-safe.  But we expect PlatformStart() to be
  // called only from the main thread, so this is OK in general.
  if (!gLUL) {
     gLUL_initialization_routine();
  }
#endif

  // Initialize signal handler communication
  gCurrentThreadInfo = nullptr;
  if (sem_init(&gSignalHandlingDone, /* pshared: */ 0, /* value: */ 0) != 0) {
    LOG("Error initializing semaphore");
    return;
  }

  // Request profiling signals.
  LOG("Request signal");
  struct sigaction sa;
  sa.sa_sigaction = MOZ_SIGNAL_TRAMPOLINE(SigprofHandler);
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_SIGINFO;
  if (sigaction(SIGPROF, &sa, &gOldSigprofHandler) != 0) {
    LOG("Error installing signal");
    return;
  }
  LOG("Signal installed");
  gIsSigprofHandlerInstalled = true;

#if defined(USE_LUL_STACKWALK)
  // Switch into unwind mode.  After this point, we can't add or
  // remove any unwind info to/from this LUL instance.  The only thing
  // we can do with it is Unwind() calls.
  gLUL->EnableUnwinding();

  // Has a test been requested?
  if (PR_GetEnv("MOZ_PROFILER_LUL_TEST")) {
     int nTests = 0, nTestsPassed = 0;
     RunLulUnitTests(&nTests, &nTestsPassed, gLUL);
  }
#endif

  // Start a thread that sends SIGPROF signal to VM thread.
  // Sending the signal ourselves instead of relying on itimer provides
  // much better accuracy.
  MOZ_ASSERT(!gIsActive);
  gIsActive = true;
  if (pthread_create(&gSigprofSenderThread, NULL, SigprofSender, NULL) == 0) {
    gHasSigprofSenderLaunched = true;
  }
  LOG("Profiler thread started");
}

static void
PlatformStop()
{
  MOZ_RELEASE_ASSERT(NS_IsMainThread());

  MOZ_ASSERT(gIsActive);
  gIsActive = false;

  // Wait for signal sender termination (it will exit after setting
  // active_ to false).
  if (gHasSigprofSenderLaunched) {
    pthread_join(gSigprofSenderThread, NULL);
    gHasSigprofSenderLaunched = false;
  }

  // Restore old signal handler
  if (gIsSigprofHandlerInstalled) {
    sigaction(SIGPROF, &gOldSigprofHandler, 0);
    gIsSigprofHandlerInstalled = false;
  }
}

#if defined(GP_OS_android)
static struct sigaction gOldSigstartHandler;
const int SIGSTART = SIGUSR2;

static void freeArray(const char** array, int size) {
  for (int i = 0; i < size; i++) {
    free((void*) array[i]);
  }
}

static uint32_t readCSVArray(char* csvList, const char** buffer) {
  uint32_t count;
  char* savePtr;
  int newlinePos = strlen(csvList) - 1;
  if (csvList[newlinePos] == '\n') {
    csvList[newlinePos] = '\0';
  }

  char* item = strtok_r(csvList, ",", &savePtr);
  for (count = 0; item; item = strtok_r(NULL, ",", &savePtr)) {
    int length = strlen(item) + 1;  // Include \0
    char* newBuf = (char*) malloc(sizeof(char) * length);
    buffer[count] = newBuf;
    strncpy(newBuf, item, length);
    count++;
  }

  return count;
}

// Currently support only the env variables
// reported in read_profiler_env
static void ReadProfilerVars(const char* fileName, const char** features,
                            uint32_t* featureCount, const char** threadNames, uint32_t* threadCount) {
  FILE* file = fopen(fileName, "r");
  const int bufferSize = 1024;
  char line[bufferSize];
  char* feature;
  char* value;
  char* savePtr;

  if (file) {
    while (fgets(line, bufferSize, file) != NULL) {
      feature = strtok_r(line, "=", &savePtr);
      value = strtok_r(NULL, "", &savePtr);

      if (strncmp(feature, PROFILER_INTERVAL, bufferSize) == 0) {
        set_profiler_interval(value);
      } else if (strncmp(feature, PROFILER_ENTRIES, bufferSize) == 0) {
        set_profiler_entries(value);
      } else if (strncmp(feature, PROFILER_STACK, bufferSize) == 0) {
        set_profiler_scan(value);
      } else if (strncmp(feature, PROFILER_FEATURES, bufferSize) == 0) {
        *featureCount = readCSVArray(value, features);
      } else if (strncmp(feature, "threads", bufferSize) == 0) {
        *threadCount = readCSVArray(value, threadNames);
      }
    }

    fclose(file);
  }
}

static void DoStartTask() {
  uint32_t featureCount = 0;
  uint32_t threadCount = 0;

  // Just allocate 10 features for now
  // FIXME: these don't really point to const chars*
  // So we free them later, but we don't want to change the const char**
  // declaration in profiler_start. Annoying but ok for now.
  const char* threadNames[10];
  const char* features[10];
  const char* profilerConfigFile = "/data/local/tmp/profiler.options";

  ReadProfilerVars(profilerConfigFile, features, &featureCount, threadNames, &threadCount);
  MOZ_ASSERT(featureCount < 10);
  MOZ_ASSERT(threadCount < 10);

  profiler_start(PROFILE_DEFAULT_ENTRY, 1,
      features, featureCount,
      threadNames, threadCount);

  freeArray(threadNames, threadCount);
  freeArray(features, featureCount);
}

static void StartSignalHandler(int signal, siginfo_t* info, void* context) {
  class StartTask : public Runnable {
  public:
    NS_IMETHOD Run() override {
      DoStartTask();
      return NS_OK;
    }
  };
  // XXX: technically NS_DispatchToMainThread is NOT async signal safe. We risk
  // nasty things like deadlocks, but the probability is very low and we
  // typically only do this once so it tends to be ok. See bug 909403.
  NS_DispatchToMainThread(new StartTask());
}

static void
PlatformInit()
{
  LOG("Registering start signal");
  struct sigaction sa;
  sa.sa_sigaction = StartSignalHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_SIGINFO;
  if (sigaction(SIGSTART, &sa, &gOldSigstartHandler) != 0) {
    LOG("Error installing signal");
  }
}

#else

static void
PlatformInit()
{
  // Set up the fork handlers.
  setup_atfork();
}

#endif

void TickSample::PopulateContext(void* aContext)
{
  MOZ_ASSERT(aContext);
  ucontext_t* pContext = reinterpret_cast<ucontext_t*>(aContext);
  if (!getcontext(pContext)) {
    context = pContext;
    SetSampleContext(this, aContext);
  }
}

