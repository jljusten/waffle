// Copyright 2012 Intel Corporation
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

#define _POSIX_C_SOURCE 200112 // glib feature macro for unsetenv()

#include <stdlib.h>

#include "waffle/core/wcore_error.h"
#include "waffle/linux/linux_platform.h"

#include "xegl_config.h"
#include "xegl_context.h"
#include "xegl_display.h"
#include "xegl_platform.h"
#include "xegl_priv_egl.h"
#include "xegl_window.h"

static const struct wcore_platform_vtbl xegl_platform_wcore_vtbl;

static bool
xegl_platform_destroy(struct wcore_platform *wc_self)
{
    struct xegl_platform *self = xegl_platform(wc_self);
    bool ok = true;

    if (!self)
        return true;

    unsetenv("EGL_PLATFORM");

    if (self->linux)
        ok &= linux_platform_destroy(self->linux);

    ok &= wcore_platform_teardown(wc_self);
    free(self);
    return ok;
}

struct wcore_platform*
xegl_platform_create(void)
{
    struct xegl_platform *self;
    bool ok = true;

    self = wcore_calloc(sizeof(*self));
    if (self == NULL)
        return NULL;

    ok = wcore_platform_init(&self->wcore);
    if (!ok)
        goto error;

    self->linux = linux_platform_create();
    if (!self->linux)
        goto error;

    setenv("EGL_PLATFORM", "x11", true);

    self->wcore.vtbl = &xegl_platform_wcore_vtbl;
    return &self->wcore;

error:
    xegl_platform_destroy(&self->wcore);
    return NULL;
}

static bool
xegl_platform_make_current(struct wcore_platform *wc_self,
                           struct wcore_display *wc_dpy,
                           struct wcore_window *wc_window,
                           struct wcore_context *wc_ctx)
{
    return egl_make_current(xegl_display(wc_dpy)->egl,
                            wc_window ? xegl_window(wc_window)->egl : NULL,
                            wc_ctx ? xegl_context(wc_ctx)->egl : NULL);
}

static void*
xegl_platform_get_proc_address(struct wcore_platform *wc_self,
                               const char *name)
{
    return eglGetProcAddress(name);
}

static bool
xegl_platform_dl_can_open(struct wcore_platform *wc_self,
                          int32_t waffle_dl)
{
    return linux_platform_dl_can_open(xegl_platform(wc_self)->linux,
                                      waffle_dl);
}

static void*
xegl_platform_dl_sym(struct wcore_platform *wc_self,
                     int32_t waffle_dl,
                     const char *name)
{
    return linux_platform_dl_sym(xegl_platform(wc_self)->linux,
                                               waffle_dl,
                                               name);
}

static const struct wcore_platform_vtbl xegl_platform_wcore_vtbl = {
    .destroy = xegl_platform_destroy,
    .connect_to_display = xegl_display_connect,
    .choose_config = xegl_config_choose,
    .create_context = xegl_context_create,
    .create_window = xegl_window_create,
    .make_current = xegl_platform_make_current,
    .get_proc_address = xegl_platform_get_proc_address,
    .dl_can_open = xegl_platform_dl_can_open,
    .dl_sym = xegl_platform_dl_sym,
};