/*
 * Vulkan Samples
 *
 * Copyright (C) 2015-2016 Valve Corporation
 * Copyright (C) 2015-2016 LunarG, Inc.
 * Copyright (C) 2015-2016 Google, Inc.
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

/*
VULKAN_SAMPLE_DESCRIPTION
samples utility functions
*/

#include <cstdio>
#include <cstring>
#include <cassert>
#include <cstdlib>

#include <string>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <iostream>

#include "util.h"
#include "lib.h"

// Android specific include files.
#include <unordered_map>

// Header files.
#include "string.h"
#include "errno.h"
#include <native_app_glue/android_native_app_glue.h>
// Static variable that keeps ANativeWindow and asset manager instances.
static android_app *Android_application = nullptr;

// Helpder class to forward the cout/cerr output to logcat derived from:
// http://stackoverflow.com/questions/8870174/is-stdcout-usable-in-android-ndk
class AndroidBuffer : public std::streambuf {
   public:
    AndroidBuffer(android_LogPriority priority) {
        priority_ = priority;
        this->setp(buffer_, buffer_ + kBufferSize - 1);
    }

   private:
    static const int32_t kBufferSize = 128;
    int32_t overflow(int32_t c) {
        if (c == traits_type::eof()) {
            *this->pptr() = traits_type::to_char_type(c);
            this->sbumpc();
        }
        return this->sync() ? traits_type::eof() : traits_type::not_eof(c);
    }

    int32_t sync() {
        int32_t rc = 0;
        if (this->pbase() != this->pptr()) {
            char writebuf[kBufferSize + 1];
            memcpy(writebuf, this->pbase(), this->pptr() - this->pbase());
            writebuf[this->pptr() - this->pbase()] = '\0';

            rc = __android_log_write(priority_, "std", writebuf) > 0;
            this->setp(buffer_, buffer_ + kBufferSize - 1);
        }
        return rc;
    }

    android_LogPriority priority_ = ANDROID_LOG_INFO;
    char buffer_[kBufferSize];
};


int32_t Android_handle_input(struct android_app* app, AInputEvent* event) {
    int32_t eventType = AInputEvent_getType(event);
    switch(eventType){
        case AINPUT_EVENT_TYPE_MOTION:
            switch (AInputEvent_getSource(event)){
                case AINPUT_SOURCE_TOUCHSCREEN:
                    int action = AKeyEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
                    float x  = AMotionEvent_getXOffset(event);
                    float y  = AMotionEvent_getYOffset(event);

                    printf("Action: %d %f %f\n", action, x, y);
                    switch(action){
                        case AMOTION_EVENT_ACTION_DOWN:
                        break;
                        case AMOTION_EVENT_ACTION_UP:
                        break;
                        case AMOTION_EVENT_ACTION_MOVE:
                        break;
                    }
                break;
            } // end switch
        break;
        case AINPUT_EVENT_TYPE_KEY:
            // handle key input...
        break;
    } // end switch
}

void Android_handle_cmd(android_app *app, int32_t cmd) {    
    switch (cmd) {
        case APP_CMD_INIT_WINDOW: {
            // The window is being shown, get it ready.
            int32_t w   = ANativeWindow_getWidth(app->window);
            int32_t h   = ANativeWindow_getHeight(app->window);
            
            DawnAndroid::Init(w, h);
            DawnAndroid::Frame();
            LOGI("\n");
            LOGI("=================================================");
            LOGI("          The sample ran successfully!!");
            LOGI("=================================================");
            LOGI("\n");
            break;
        }
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            break;
        default:
            LOGI("event not handled: %d", cmd);
    }
}

bool Android_process_command() {
    assert(Android_application != nullptr);
    int events;
    android_poll_source *source;
    // Poll all pending events.
    if (ALooper_pollAll(0, NULL, &events, (void **)&source) >= 0) {
        // Process each polled events
        if (source != NULL) source->process(Android_application, source);
    }
    return Android_application->destroyRequested;
}

void android_main(struct android_app *app) {
    // Set static variables.
    Android_application = app;
    // Set the callback to process system events
    app->onAppCmd = Android_handle_cmd;
    app->onInputEvent = Android_handle_input;

    // Forward cout/cerr to logcat.
    std::cout.rdbuf(new AndroidBuffer(ANDROID_LOG_INFO));
    std::cerr.rdbuf(new AndroidBuffer(ANDROID_LOG_ERROR));

    // Main loop
    do {
        Android_process_command();
    }  // Check if system requested to quit the application
    while (app->destroyRequested == 0);

    return;
}

ANativeWindow *AndroidGetApplicationWindow() {
    assert(Android_application != nullptr);
    return Android_application->window;
}
