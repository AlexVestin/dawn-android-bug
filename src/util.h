

/*
 * Vulkan Samples
 *
 * Copyright (C) 2015-2016 Valve Corporation
 * Copyright (C) 2015-2016 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __DAWN_ANDROID_UTIL_H
#define __DAWN_ANDROID_UTIL_H

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

// Include files for Android
#include <unistd.h>
#include <android/log.h>
#include <android/native_activity.h>

/* Amount of time, in nanoseconds, to wait for a command buffer to complete */
#define FENCE_TIMEOUT 100000000

#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif

typedef unsigned long long timestamp_t;

// Android specific definitions & helpers.
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "DAWN-ANDROID", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "DAWN-ANDROID", __VA_ARGS__))
// Replace printf to logcat output.
#define printf(...) __android_log_print(ANDROID_LOG_DEBUG, "DAWN-ANDROID", __VA_ARGS__);

bool Android_process_command();
ANativeWindow* AndroidGetApplicationWindow();

// #ifdef __ANDROID__
// #ifndef VK_API_VERSION_1_0
// // On Android, NDK would include slightly older version of headers that is missing the definition.
// #define VK_API_VERSION_1_0 VK_API_VERSION
// #endif
// #endif

#endif // #ifndef __DAWN_ANDROID_UTIL_H
