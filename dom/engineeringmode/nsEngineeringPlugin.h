/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/ */

#ifndef mozilla_dom_engineeringmode_EngineeringPlugin_h
#define mozilla_dom_engineeringmode_EngineeringPlugin_h

#define PLUGIN_PATH "/system/b2g/plugins/"

#define PLUGIN_ERROR (0)
#define PLUGIN_OK (1)

#define PLUGIN_HANDLE_GET (0)
#define PLUGIN_HANDLE_SET (1)

typedef int (*PluginHandlerFn)(const char*, const char*, const char**, int);
typedef int (*PluginRecvMessageFn)(const char*, const char*);

typedef int (*RegisterNamespaceFn)(const char*, PluginHandlerFn);
typedef int (*RegisterMessageListenerFn)(const char*, PluginRecvMessageFn);

typedef struct {
  RegisterNamespaceFn mRegisterNamespace;
  RegisterMessageListenerFn mRegisterMessageListener;
} HostInterface;

typedef int (*PluginInitFn)(HostInterface*);
typedef void (*PluginDestroyFn)();

#endif
