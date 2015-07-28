/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/ */

#include "nsEngineeringMode.h"

#if 1
#include <unistd.h>
#endif

#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/RefPtr.h"

#include "nsIFile.h"
#include "nsISimpleEnumerator.h"
#include "nsXULAppAPI.h"
#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "nsLiteralString.h"            // for NS_LITERAL_STRING

namespace mozilla {
namespace dom {

mozilla::StaticRefPtr<nsEngineeringMode> gEngineeringMode;

static int
RegisterNamespace(const char* aNs, PluginHandlerFn aFn)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIEngineeringMode> engmode =
    do_GetService(ENGINEERING_MODE_CONTRACTID, &rv);

  if (NS_FAILED(rv) || !engmode) {
    NS_ERROR("Failed to get EngineeringMode service");
    return PLUGIN_ERROR;
  }

#if 1
  printf("Got EngineeringMode service\n");
#endif

  // XXX: is this cast always possible??
  nsEngineeringMode* engmodeimpl =
    reinterpret_cast<nsEngineeringMode*>(engmode.get());
  return engmodeimpl->RegisterNamespaceImpl(aNs, aFn);
}

static int
RegisterMessageListener(const char* aTopic, PluginRecvMessageFn aFn)
{
  return PLUGIN_ERROR;
}

PluginAPI::PluginAPI()
  : mRefCount(0)
{}

void
PluginAPI::Release()
{
  NS_PRECONDITION(int32_t(mRefCount) != 0, "dup release");
  int32_t count = --mRefCount;
  NS_LOG_RELEASE(this, count, "EngineeringMode::PluginAPI");

  if (count == 0) {
    delete this;
  }
}

void
PluginAPI::AddRef()
{
  NS_PRECONDITION(int32_t(mRefCount) >= 0, "illegal refcount");
  NS_LOG_ADDREF(this, mRefCount++, "EngineeringMode::PluginAPI", sizeof(*this));
}

MessageHandlerArray::MessageHandlerArray()
  : mRefCount(0)
{}

void
MessageHandlerArray::Release()
{
  NS_PRECONDITION(int32_t(mRefCount) != 0, "dup release");
  int32_t count = --mRefCount;
  NS_LOG_RELEASE(this, count, "EngineeringMode::MessageHandlerArray");

  if (count == 0) {
    delete this;
  }
}

void
MessageHandlerArray::AddRef()
{
  NS_PRECONDITION(int32_t(mRefCount) >= 0, "illegal refcount");
  NS_LOG_ADDREF(this, mRefCount++, "EngineeringMode::MessageHandlerArray", sizeof(*this));
}


NS_IMPL_ISUPPORTS(nsEngineeringMode, nsIEngineeringMode, nsIObserver)

/* static */ already_AddRefed<nsEngineeringMode>
nsEngineeringMode::FactoryCreate()
{
  if (!XRE_IsParentProcess()) {
    return nullptr;
  }

  MOZ_ASSERT(NS_IsMainThread());

  if (!gEngineeringMode) {
    gEngineeringMode = new nsEngineeringMode();
  }

  nsRefPtr<nsEngineeringMode> service = gEngineeringMode.get();
  return service.forget();
}

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
#if 0
  printf("this ptr: %p\n", this);
#endif
  int rv = -1;
  const char *msg = nullptr;
  PluginHandlerFn handler;

  if (!aCallback || aName.FindChar(':') < 0) {
    return NS_ERROR_INVALID_ARG;
  }

  nsCString ns = NS_ConvertUTF16toUTF8(Substring(aName, 0, aName.FindChar(':')));

  // TODO: msg should be owned by us and allocated by the plugin
  if (!mNamespaces.Get(ns, &handler) ||
      (rv = handler(NS_LossyConvertUTF16toASCII(aName).get(),
                    nullptr,
                    &msg,
                    PLUGIN_HANDLE_GET))) {
    aCallback->Onerror(rv);
  } else {
    aCallback->Onsuccess(NS_ConvertASCIItoUTF16(msg));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEngineeringMode::SetValue(const nsAString& aName, const nsAString& aValue,
                            nsIEngineeringModeCallback* aCallback)
{
  int rv = -1;
  PluginHandlerFn handler;

  if (!aCallback || aName.FindChar(':') < 0) {
    return NS_ERROR_INVALID_ARG;
  }

  nsCString ns = NS_ConvertUTF16toUTF8(Substring(aName, 0, aName.FindChar(':')));

  if (!mNamespaces.Get(ns, &handler) ||
      (rv = handler(NS_LossyConvertUTF16toASCII(aName).get(),
                    NS_LossyConvertUTF16toASCII(aValue).get(),
                    nullptr,
                    PLUGIN_HANDLE_SET))) {
    aCallback->Onerror(rv);
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
#if 1
  printf("[%d] nsEngineeringMode::LoadPlugins()\n", getpid());
#endif
  nsresult rv;

  nsCOMPtr<nsIFile> pluginDir = do_CreateInstance("@mozilla.org/file/local;1", &rv);
  if (NS_FAILED(rv)) {
    NS_ERROR("Failed to get nsIFile instance");
    return;
  }

  rv = pluginDir->InitWithPath(NS_LITERAL_STRING(PLUGIN_PATH));
  if (NS_FAILED(rv)) {
    NS_ERROR("Failed to open plugin path: " PLUGIN_PATH);
    return;
  }

  nsCOMPtr<nsISimpleEnumerator> iter;
  rv = pluginDir->GetDirectoryEntries(getter_AddRefs(iter));
  if (NS_FAILED(rv)) {
    NS_ERROR("Failed to get directory entries");
    return;
  }

  bool hasMore = false;
  while (NS_SUCCEEDED(iter->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> supports;
    rv = iter->GetNext(getter_AddRefs(supports));
    if (NS_FAILED(rv))
      continue;

    nsCOMPtr<nsIFile> entry(do_QueryInterface(supports, &rv));
    if (NS_FAILED(rv))
      continue;

    /* we assume that every file in this directory is a loadable shared library */
    PRLibrary *plugin;
    rv = entry->Load(&plugin);
    if (NS_FAILED(rv))
      continue;

    nsRefPtr<PluginAPI> api = new PluginAPI();
    if (!LoadPluginAPI(plugin, api)) {
      PR_UnloadLibrary(plugin);
      continue;
    }

    HostInterface interface = {
      .mRegisterNamespace = &RegisterNamespace,
      .mRegisterMessageListener = &RegisterMessageListener
    };

    if (api->init(&interface) == PLUGIN_ERROR) {
      PR_UnloadLibrary(plugin);
      continue;
    }

    /* plugin initializaiton completed and successful */
    mLoadedPlugins.Put(plugin, api);
  }
}

void
nsEngineeringMode::UnloadPlugins()
{
  /* unload all loaded plugins */
#if 1
  printf("nsEngineeringMode::UnloadPlugins()\n");
#endif
}

bool
nsEngineeringMode::LoadPluginAPI(PRLibrary *aPlugin, struct PluginAPI *aApi)
{
  aApi->init = (PluginInitFn) PR_FindSymbol(aPlugin, "PluginInit");
  if (!aApi->init) {
    NS_WARNING("plugin is missing 'PluginInit' export");
    return false;
  }

  aApi->destroy = (PluginDestroyFn) PR_FindSymbol(aPlugin, "PluginDestroy");
  if (!aApi->destroy) {
    NS_WARNING("plugin is missing 'PluginDestroy' export");
    return false;
  }

  return true;
}

/* Functions exposed to the plugins */
int
nsEngineeringMode::RegisterNamespaceImpl(const char *aNs, PluginHandlerFn aHandler)
{
#if 1
  printf("Inside RegisterNamespaceImpl\n");
#endif
  PluginHandlerFn handler;
  if (mNamespaces.Get(nsCString(aNs), &handler))
    return PLUGIN_ERROR;

  mNamespaces.Put(nsCString(aNs), aHandler);
  return PLUGIN_OK;
}

int
nsEngineeringMode::RegisterMessageListenerImpl(const char *aTopic,
                                           PluginRecvMessageFn aHandler)
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
