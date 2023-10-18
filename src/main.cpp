#include "util.h"
#include "lib.h"


void sample_main(ANativeWindow* window, int32_t command, int w, int h, int fmt) {
    DawnAndroid::Init(window, w, h, fmt);
    DawnAndroid::Frame();
}
