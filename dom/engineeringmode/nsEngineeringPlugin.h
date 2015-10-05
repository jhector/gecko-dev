/*
 * Copyright (C) 2014-2015  Mozilla Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef mozilla_dom_engineeringmode_EngineeringPlugin_h
#define mozilla_dom_engineeringmode_EngineeringPlugin_h

__BEGIN_DECLS

#define PLUGIN_PATH "/system/b2g/plugins/"

#define PLUGIN_ERROR (0)
#define PLUGIN_OK (1)

#define PLUGIN_HANDLE_GET (0)
#define PLUGIN_HANDLE_SET (1)

struct MozEngPluginInterface;

typedef struct {
  int (*register_namespace_callback)(const char *, struct MozEngPluginInterface*);
  int (*register_message_listener_callback)(const char*, struct MozEngPluginInterface*);
} HostInterface;

struct MozEngPluginInterface {
  int (*init)(struct MozEngPluginInterface* aInterface, HostInterface *aCallbacks);
  void (*destroy)(struct MozEngPluginInterface* aInterface, HostInterface *aCallbacks);
  int (*handle)(const char*, const char*, const char**, int);
  int (*receive)(const char*, const char*);
};

typedef struct MozEngPluginInterface* (*PluginGetInterface)(uint32_t* aMajor,
                                                           uint32_t* aMinor);

__END_DECLS

#endif
