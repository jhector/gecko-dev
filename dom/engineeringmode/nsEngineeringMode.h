/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/ */

#ifndef mozilla_dom_engineeringmode_EngineeringMode_h
#define mozilla_dom_engineeringmode_EngineeringMode_h

#include "nsIEngineeringMode.h"

#include "prlink.h"

#include "nsTArray.h"
#include "nsRefPtr.h"
#include "nsDataHashtable.h"

#include "nsIObserver.h"
#include "nsIObserverService.h"

#include "nsEngineeringPlugin.h"

#define ENGINEERING_MODE_CID                            \
  { 0x68c5e530, 0x0c8a, 0x40c3,                         \
    { 0xa3, 0xc9, 0x80, 0x6a, 0xa7, 0xef, 0x03, 0x43 } }

#define ENGINEERING_MODE_CONTRACTID "@mozilla.org/b2g/engineering-mode-impl;1"

namespace mozilla {
namespace dom {

struct PluginAPI;
class MessageHandlerArray;

typedef nsDataHashtable<nsPtrHashKey<PRLibrary>, nsRefPtr<struct PluginAPI> > PluginTable;
typedef nsDataHashtable<nsCStringHashKey, PluginHandlerFn> NamespaceTable;
typedef nsDataHashtable<nsCStringHashKey, nsRefPtr<MessageHandlerArray> > MessageHandlerTable;

typedef struct PluginAPI
{
public:
  PluginAPI();

  void Release();
  void AddRef();

  PluginInitFn init;
  PluginDestroyFn destroy;

private:
  int32_t mRefCount;
} PluginAPI;

class MessageHandlerArray : public nsTArray<PluginRecvMessageFn>
{
public:
  MessageHandlerArray();

  void Release();
  void AddRef();

private:
  int32_t mRefCount;
};

class nsEngineeringMode : public nsIEngineeringMode,
                          public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIENGINEERINGMODE
  NS_DECL_NSIOBSERVER

  static already_AddRefed<nsEngineeringMode> FactoryCreate();

  int RegisterNamespaceImpl(const char* aNs, PluginHandlerFn aHandler);
  int RegisterMessageListenerImpl(const char* aTopic, PluginRecvMessageFn aHandler);

private:
  nsEngineeringMode();
  virtual ~nsEngineeringMode();

  void LoadPlugins();
  void UnloadPlugins();

  bool LoadPluginAPI(PRLibrary *aPlugin, struct PluginAPI *aApi);

  NamespaceTable mNamespaces;
  MessageHandlerTable mMessageHandlers;
  PluginTable mLoadedPlugins;
};

} // dom
} // mozilla

#endif
