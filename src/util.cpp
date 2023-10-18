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

// Android specific include files.
#include <unordered_map>

// Header files.
#include "string.h"
#include "errno.h"
#include <native_app_glue/android_native_app_glue.h>
// Static variable that keeps ANativeWindow and asset manager instances.
static android_app *Android_application = nullptr;

std::string get_file_name(const std::string &s) {
    char sep = '/';

#ifdef _WIN32
    sep = '\\';
#endif

    // cout << "in get_file_name\n";
    size_t i = s.rfind(sep, s.length());
    if (i != std::string::npos) {
        return (s.substr(i + 1, s.length() - i));
    }

    return ("");
}


std::string get_data_dir(std::string filename) {
    std::string basedir = "";
    // get the base filename
    std::string fnames = get_file_name(filename);

    // get the prefix of the base filename, i.e. the part before the dash
    std::stringstream sstream(fnames);
    std::string prefix;
    std::getline(sstream, prefix, '-');
    std::string ddir = basedir + prefix;
    return ddir;
}


void wait_seconds(int seconds) {
#ifdef WIN32
    Sleep(seconds * 1000);
#elif defined(__ANDROID__)
    sleep(seconds);
#else
    //std::sleep(seconds);
#endif
}

timestamp_t get_milliseconds() {
#ifdef WIN32
    LARGE_INTEGER frequency;
    BOOL useQPC = QueryPerformanceFrequency(&frequency);
    if (useQPC) {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return (1000LL * now.QuadPart) / frequency.QuadPart;
    } else {
        return GetTickCount();
    }
#else
    struct timeval now;
    gettimeofday(&now, NULL);
    return (now.tv_usec / 1000) + (timestamp_t)now.tv_sec;
#endif
}


std::string get_file_directory() {
    assert(Android_application != nullptr);
    return Android_application->activity->externalDataPath;
}

//
// Android specific helper functions.
//

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
            int32_t fmt = ANativeWindow_getFormat(app->window);

            sample_main(app->window, cmd, w, h, fmt);
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


bool AndroidLoadFile(const char *filePath, std::string *data) {
    assert(Android_application != nullptr);
    AAsset *file = AAssetManager_open(Android_application->activity->assetManager, filePath, AASSET_MODE_BUFFER);
    size_t fileLength = AAsset_getLength(file);
    LOGI("Loaded file:%s size:%zu", filePath, fileLength);
    if (fileLength == 0) {
        return false;
    }
    data->resize(fileLength);
    AAsset_read(file, &(*data)[0], fileLength);
    return true;
}


// Android fopen stub described at
// http://www.50ply.com/blog/2013/01/19/loading-compressed-android-assets-with-file-pointer/#comment-1850768990
static int android_read(void *cookie, char *buf, int size) { return AAsset_read((AAsset *)cookie, buf, size); }

static int android_write(void *cookie, const char *buf, int size) {
    return EACCES;  // can't provide write access to the apk
}

static fpos_t android_seek(void *cookie, fpos_t offset, int whence) { return AAsset_seek((AAsset *)cookie, offset, whence); }

static int android_close(void *cookie) {
    AAsset_close((AAsset *)cookie);
    return 0;
}

FILE *AndroidFopen(const char *fname, const char *mode) {
    if (mode[0] == 'w') {
        return NULL;
    }

    assert(Android_application != nullptr);
    AAsset *asset = AAssetManager_open(Android_application->activity->assetManager, fname, 0);
    if (!asset) {
        return NULL;
    }

    return funopen(asset, android_read, android_write, android_seek, android_close);
}
