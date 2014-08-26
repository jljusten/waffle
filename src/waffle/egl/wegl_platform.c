// Copyright 2014 Emil Velikov
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// - Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// - Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <dlfcn.h>

#include "wcore_error.h"
#include "wegl_platform.h"

// XXX: Use the actual SONAME for linux ?
// Should we treat Android the same way as linux ?
static const char *libEGL_filename = "libEGL.so";

bool
wegl_platform_teardown(struct wegl_platform *self)
{
    bool ok = true;
    int error = 0;

    if (self->eglHandle) {
        error = dlclose(self->eglHandle);
        if (error) {
            ok = false;
            wcore_errorf(WAFFLE_ERROR_UNKNOWN,
                         "dlclose(\"%s\") failed: %s",
                         libEGL_filename, dlerror());
        }
    }

    ok &= wcore_platform_teardown(&self->wcore);
    return ok;
}
bool
wegl_platform_init(struct wegl_platform *self)
{
    bool ok;

    ok = wcore_platform_init(&self->wcore);
    if (!ok)
        goto error;

    self->eglHandle = dlopen(libEGL_filename, RTLD_LAZY | RTLD_LOCAL);
    if (!self->eglHandle) {
        wcore_errorf(WAFFLE_ERROR_UNKNOWN,
                     "dlopen(\"%s\") failed: %s",
                     libEGL_filename, dlerror());
        goto error;
    }

#define RETREIVE_EGL_SYMBOL(function)                                  \
    self->function = dlsym(self->eglHandle, #function);                \
    if (!self->function) {                                             \
        wcore_errorf(WAFFLE_ERROR_UNKNOWN,                             \
                     "dlsym(\"%s\", \"" #function "\") failed: %s",    \
                     libEGL_filename, dlerror());                      \
        goto error;                                                    \
    }

    RETREIVE_EGL_SYMBOL(eglMakeCurrent);
    RETREIVE_EGL_SYMBOL(eglGetProcAddress);

    // display
    RETREIVE_EGL_SYMBOL(eglGetDisplay);
    RETREIVE_EGL_SYMBOL(eglInitialize);
    RETREIVE_EGL_SYMBOL(eglQueryString);
    RETREIVE_EGL_SYMBOL(eglGetError);
    RETREIVE_EGL_SYMBOL(eglTerminate);

    // config
    RETREIVE_EGL_SYMBOL(eglChooseConfig);

    // context
    RETREIVE_EGL_SYMBOL(eglBindAPI);
    RETREIVE_EGL_SYMBOL(eglCreateContext);
    RETREIVE_EGL_SYMBOL(eglDestroyContext);

    // window
    RETREIVE_EGL_SYMBOL(eglGetConfigAttrib);
    RETREIVE_EGL_SYMBOL(eglCreateWindowSurface);
    RETREIVE_EGL_SYMBOL(eglDestroySurface);
    RETREIVE_EGL_SYMBOL(eglSwapBuffers);

#undef RETREIVE_EGL_SYMBOL

error:
    // On failure the caller of wegl_platform_init will trigger it's own
    // destruction which will execute wegl_platform_teardown.
    return ok;
}
