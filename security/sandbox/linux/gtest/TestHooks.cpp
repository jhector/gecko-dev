#include "gtest/gtest.h"

#include <signal.h>

namespace mozilla {

TEST(SandboxHook, Sigprocmask)
{
  sigset_t oldset;
  sigset_t newset;

  EXPECT_EQ(0, sigfillset(&newset)) << "Cannot fill new signal mask";
  EXPECT_EQ(0, sigprocmask(SIG_SETMASK, &newset, &oldset))
    << "Cannot set new signal mask";

  EXPECT_EQ(0, sigemptyset(&oldset)) << "Cannot empty old signal mask";
  EXPECT_EQ(0, sigemptyset(&newset)) << "Cannot empty new signal mask";

  EXPECT_EQ(0, sigprocmask(SIG_UNBLOCK, &newset, &oldset))
    << "Cannot retrieve old signal set";

  EXPECT_NE(1, sigismember(&oldset, SIGSYS)) << "SIGSYS is masked";
}

}
