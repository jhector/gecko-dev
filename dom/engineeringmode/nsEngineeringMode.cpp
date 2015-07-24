/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/ */

#include "nsEngineeringMode.h"

#if 1
#include <unistd.h>
#endif

#include "mozilla/Services.h"

#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "nsLiteralString.h"            // for NS_LITERAL_STRING

namespace mozilla {
namespace dom {

MessageHandlerArray::MessageHandlerArray()
  : mRefCount(0)
{}

void
MessageHandlerArray::Release()
{
  NS_PRECONDITION(int32_t(mRefCount) != 0, "dup release");
  int32_t count = --mRefCount;
  NS_LOG_RELEASE(this, count, "MessageHandlerArray");

  if (count == 0)
    delete this;
}

void
MessageHandlerArray::AddRef()
{
  NS_PRECONDITION(int32_t(mRefCount) >= 0, "illegal refcount");
  NS_LOG_ADDREF(this, mRefCount++, "MessageHandlerArray", sizeof(*this));
}


NS_IMPL_ISUPPORTS(nsEngineeringMode, nsIEngineeringMode, nsIObserver)

nsEngineeringMode::nsEngineeringMode()
{
#if 1
  printf("[%d] nsEngineeringMode::nsEngineeringMode()\n", getpid());
#endif
}

nsEngineeringMode::~nsEngineeringMode()
{
}

NS_IMETHODIMP
nsEngineeringMode::GetValue(const nsAString& aName,
                            nsIEngineeringModeCallback* aCallback)
{
  if (!aCallback) {
    return NS_ERROR_INVALID_ARG;
  }
  /* Write your corresponding implementation here,
   * then invoke |aCallback->Onsuccess| to return value.
   * Or |aCallback->Onerror| to indicate an error happened.
   *
   * e.g. aCallback->Onsuccess(NS_LITERAL_STRING("bar"));
   * e.g. aCallback->Onerror(-1);
   *
   * The returned value should be a DOMstring,
   * while the returned error is a int32 error code.
   */
  if (aName.IsEmpty()) {
    aCallback->Onerror(-1);
  } else {
    aCallback->Onsuccess(NS_LITERAL_STRING("bar"));
  }
  return NS_OK;
}

NS_IMETHODIMP
nsEngineeringMode::SetValue(const nsAString& aName, const nsAString& aValue,
                            nsIEngineeringModeCallback* aCallback)
{
  if (!aCallback) {
    return NS_ERROR_INVALID_ARG;
  }
  /* Write your corresponding implementation here,
   * then invoke |aCallback->Onsuccess| to indicate successful.
   * Or |aCallback->Onerror| to indicate an error happened.
   *
   * e.g. aCallback->Onsuccess();
   * e.g. aCallback->Onerror(-1);
   *
   * The returned value should be an empty DOMstring (just for passing
   * compiling, won't be caught by the caller),
   * while the returned error is a int32 error code.
   */
  if (aName.IsEmpty() || aValue.IsEmpty()) {
    aCallback->Onerror(-1);
  } else {
    aCallback->Onsuccess(NS_LITERAL_STRING(""));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEngineeringMode::SetMessageHandler(nsIEngineeringModeMessageHandler* aHandler)
{
  /* You should keep aHandler somewhere, and invoke it when you have something
   * to notify applications who are listening to EngineeringMode message event.
   */

  return NS_OK;
}

NS_IMETHODIMP
nsEngineeringMode::Observe(nsISupports *aSubject, const char *aTopic, const
                           char16_t *aData)
{
#if 1
  printf("nsEngineeringMode::Observe(): %s\n", aTopic);
#endif
  if (nsCRT::strcmp(aTopic, "profile-after-change") == 0) {
    nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();

    observerService->AddObserver(this, "profile-before-change", false);

    LoadPlugins();
  }

  if (nsCRT::strcmp(aTopic, "profile-before-change") == 0) {
    nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();

    observerService->RemoveObserver(this, "profile-before-change");

    UnloadPlugins();
  }

  return NS_OK;
}

void
nsEngineeringMode::LoadPlugins()
{
  /* dlopen()/dlsym() everything in a specific directory */
#if 1
  printf("[%d] nsEngineeringMode::LoadPlugins()\n", getpid());
#endif
}

void
nsEngineeringMode::UnloadPlugins()
{
  /* unload all loaded plugins */
#if 1
  printf("nsEngineeringMode::UnloadPlugins()\n");
#endif
}

/* Functions exposed to the plugins */
int
nsEngineeringMode::RegisterNamespace(const char *aNs, PluginHandler aHandler)
{
  PluginHandler handler;
  if (mNamespaces.Get(nsCString(aNs), &handler))
    return PLUGIN_ERROR;

  mNamespaces.Put(nsCString(aNs), aHandler);
  return PLUGIN_OK;
}

int
nsEngineeringMode::RegisterMessageListener(const char *aTopic,
                                           PluginRecvMessage aHandler)
{
  /* TODO: register for the message */

  nsRefPtr<MessageHandlerArray> handlers;
  if (mMessageHandlers.Get(nsCString(aTopic), &handlers)) {
    if (handlers->Contains(aHandler)) {
      return PLUGIN_OK;
    }

    handlers->AppendElement(aHandler);
    return PLUGIN_OK;
  }

  handlers = new MessageHandlerArray;
  handlers->AppendElement(aHandler);
  mMessageHandlers.Put(nsCString(aTopic), handlers);
  return PLUGIN_OK;
}

} // dom
} // mozilla
