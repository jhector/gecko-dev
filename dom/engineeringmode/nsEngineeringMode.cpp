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
RegisterNamespaceCallback(const char* aNs,
                          struct MozEngPluginInterface* aInterface)
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
  return engmodeimpl->RegisterNamespaceImpl(aNs, aInterface);
}

static int
RegisterMessageListenerCallback(const char* aTopic,
                                struct MozEngPluginInterface *aInterface)
{
  return PLUGIN_ERROR;
}

PluginInterfaceArray::PluginInterfaceArray()
  : mRefCount(0)
{}

void
PluginInterfaceArray::Release()
{
  NS_PRECONDITION(int32_t(mRefCount) != 0, "dup release");
  int32_t count = --mRefCount;
  NS_LOG_RELEASE(this, count, "EngineeringMode::PluginInterfaceArray");

  if (count == 0) {
    delete this;
  }
}

void
PluginInterfaceArray::AddRef()
{
  NS_PRECONDITION(int32_t(mRefCount) >= 0, "illegal refcount");
  NS_LOG_ADDREF(this, mRefCount++, "EngineeringMode::PluginInterfaceArray", sizeof(*this));
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
  int rv = -1;
  const char* msg = nullptr;
  struct MozEngPluginInterface* interface;

  if (!aCallback || aName.FindChar(':') < 0) {
    return NS_ERROR_INVALID_ARG;
  }

  nsCString ns = NS_ConvertUTF16toUTF8(Substring(aName, 0, aName.FindChar(':')));

  // TODO: msg should be owned by us and allocated by the plugin
  if (!mNamespaces.Get(ns, &interface)) {
    aCallback->Onerror(rv);
    return NS_OK;
  }

#if 1
  printf("Got interface at %p and handle at %p\n", interface, interface->handle);
#endif

  if ((rv = interface->handle(NS_LossyConvertUTF16toASCII(aName).get(),
                              nullptr,
                              &msg,
                              PLUGIN_HANDLE_GET)) != PLUGIN_OK) {
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
  struct MozEngPluginInterface* interface;

  if (!aCallback || aName.FindChar(':') < 0) {
    return NS_ERROR_INVALID_ARG;
  }

  nsCString ns = NS_ConvertUTF16toUTF8(Substring(aName, 0, aName.FindChar(':')));

  if (!mNamespaces.Get(ns, &interface) ||
      (rv = interface->handle(NS_LossyConvertUTF16toASCII(aName).get(),
                              NS_LossyConvertUTF16toASCII(aValue).get(),
                              nullptr,
                              PLUGIN_HANDLE_SET)) != PLUGIN_OK) {
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

    // we assume that every file in this directory is a loadable shared library
    PRLibrary *plugin;
    rv = entry->Load(&plugin);
    if (NS_FAILED(rv))
      continue;

    /*
     * |pluginInterface| is allocated by the plugin and should be free()ed
     * by the plugin when |interface->destroy| is called
     * no refcounting needed in Gecko
     */
    struct MozEngPluginInterface* pluginInterface;

    if (!LoadPluginInterface(plugin, &pluginInterface)) {
      PR_UnloadLibrary(plugin);
      continue;
    }

    HostInterface hostInterface = {
      .register_namespace_callback = &RegisterNamespaceCallback,
      .register_message_listener_callback = &RegisterMessageListenerCallback
    };

    if (pluginInterface->init(pluginInterface, &hostInterface) == PLUGIN_ERROR) {
      PR_UnloadLibrary(plugin);
      continue;
    }

    /* plugin initializaiton completed and successful */
    mLoadedPlugins.Put(plugin, pluginInterface);
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
nsEngineeringMode::LoadPluginInterface(
    PRLibrary *aPlugin,
    struct MozEngPluginInterface** aInterface)
{
  PluginGetInterface getInterface;
  getInterface = (PluginGetInterface)PR_FindSymbol(
      aPlugin,
      "moz_get_engineering_mode_interface");

  if (!getInterface) {
    NS_WARNING("plugin is missing 'moz_get_engineering_mode_interface' export");
    return false;
  }

  uint32_t major = 0, minor = 0;
  *aInterface = getInterface(&major, &minor);

  // TODO: version check

  if (!*aInterface) {
    NS_WARNING("plugin didn't return a interface");
    return false;
  }

  return true;
}

/* Functions exposed to the plugins */
int
nsEngineeringMode::RegisterNamespaceImpl(
    const char *aNs,
    struct MozEngPluginInterface* aInterface)
{
#if 1
  printf("Inside RegisterNamespaceImpl\n");
#endif
  struct MozEngPluginInterface* interface;
  if (mNamespaces.Get(nsCString(aNs), &interface))
    return PLUGIN_ERROR;

  mNamespaces.Put(nsCString(aNs), aInterface);
  return PLUGIN_OK;
}

int
nsEngineeringMode::RegisterMessageListenerImpl(
    const char *aTopic,
    struct MozEngPluginInterface* aInterface)
{
  /* TODO: register for the message */

  nsRefPtr<PluginInterfaceArray> interfaces;
  if (mMessageHandlers.Get(nsCString(aTopic), &interfaces)) {
    if (interfaces->Contains(aInterface)) {
      return PLUGIN_OK;
    }

    interfaces->AppendElement(aInterface);
    return PLUGIN_OK;
  }

  interfaces = new PluginInterfaceArray;
  interfaces->AppendElement(aInterface);
  mMessageHandlers.Put(nsCString(aTopic), interfaces);
  return PLUGIN_OK;
}

} // dom
} // mozilla
