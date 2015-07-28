/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/ */

#ifndef mozilla_dom_engineeringmode_EngineeringPlugin_h
#define mozilla_dom_engineeringmode_EngineeringPlugin_h

#define PLUGIN_PATH "/system/b2g/plugins/"

#define PLUGIN_ERROR (0)
#define PLUGIN_OK (1)

typedef int (*PluginHandler)(const char* aFn, const char* aParam, const char** aRet);
typedef int (*PluginRecvMessage)(const char* aTopic, const char* aParams);

typedef int (*RegisterNamespace)(const char* aNs, PluginHandler aFn);
typedef int (*RegisterMessageListener)(const char* aTopic, PluginRecvMessage aFn);

typedef struct {
  RegisterNamespace mRegisterNamespace;
  RegisterMessageListener mRegisterListener;
} HostInterface;

typedef int (*PluginInit)(HostInterface* aHostInterface);
typedef void (*PluginDestroy)();

#endif
