// Copyright 2014 Intel Corporation
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

/// @file
/// @brief Print OpenGL info using Waffle.
///
/// This program does the following:
///     1. Dynamically choose the platform and OpenGL API according to
///        command line arguments.
///     2. Create an OpenGL context.
///     3. Print information about the context.

#define WAFFLE_API_VERSION 0x0103

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wutils.h"

const char *wutils_utility_name = "wflinfo";
const char *wutils_utility_Name = "Wflinfo";

#define WINDOW_WIDTH  320
#define WINDOW_HEIGHT 240

static int
parse_version(const char *version)
{
    int count, major, minor;

    if (version == NULL)
        return 0;

    while (*version != '\0' && !isdigit(*version))
        version++;

    count = sscanf(version, "%d.%d", &major, &minor);
    if (count != 2)
        return 0;

    if (minor > 9)
        return 0;

    return (major * 10) + minor;
}

static void
print_extensions(bool use_stringi)
{
    GLint count = 0, i;
    const char *ext;

    printf("OpenGL extensions: ");
    if (use_stringi) {
        glGetIntegerv(GL_NUM_EXTENSIONS, &count);
        if (glGetError() != GL_NO_ERROR) {
            printf("WFLINFO_GL_ERROR");
        } else {
            for (i = 0; i < count; i++) {
              ext = (const char *) glGetStringi(GL_EXTENSIONS, i);
              if (glGetError() != GL_NO_ERROR)
                  ext = "WFLINFO_GL_ERROR";
              printf("%s%s", ext, (i + 1) < count ? " " : "");
            }
        }
    } else {
        const char *extensions = (const char *) glGetString(GL_EXTENSIONS);
        if (glGetError() != GL_NO_ERROR)
            printf("WFLINFO_GL_ERROR");
        else
            printf("%s", extensions);
    }
    printf("\n");
}

static void
print_context_flags(void)
{
    static struct {
        GLint flag;
        char *str;
    } flags[] = {
        { GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT, "FORWARD_COMPATIBLE" },
        { GL_CONTEXT_FLAG_DEBUG_BIT, "DEBUG" },
        { GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT_ARB, "ROBUST_ACCESS" },
    };
    int flag_count = sizeof(flags) / sizeof(flags[0]);
    GLint context_flags = 0;

    printf("OpenGL context flags:");

    glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
    if (glGetError() != GL_NO_ERROR) {
        printf(" WFLINFO_GL_ERROR\n");
        return;
    }

    if (context_flags == 0) {
        printf(" 0x0\n");
        return;
    }

    for (int i = 0; i < flag_count; i++) {
        if ((flags[i].flag & context_flags) != 0) {
            printf(" %s", flags[i].str);
            context_flags = context_flags & ~flags[i].flag;
        }
    }
    for (int i = 0; context_flags != 0; context_flags >>= 1, i++) {
        if ((context_flags & 1) != 0) {
            printf(" 0x%x", 1 << i);
        }
    }
    printf("\n");
}

/// @brief Print out information about the context that was created.
static bool
print_wflinfo(const struct options *opts)
{
    while(glGetError() != GL_NO_ERROR) {
        /* Clear all errors */
    }

    const char *vendor = (const char *) glGetString(GL_VENDOR);
    if (glGetError() != GL_NO_ERROR || vendor == NULL)
        vendor = "WFLINFO_GL_ERROR";

    const char *renderer = (const char *) glGetString(GL_RENDERER);
    if (glGetError() != GL_NO_ERROR || renderer == NULL)
        renderer = "WFLINFO_GL_ERROR";

    const char *version_str = (const char *) glGetString(GL_VERSION);
    if (glGetError() != GL_NO_ERROR || version_str == NULL)
        version_str = "WFLINFO_GL_ERROR";

    const char *platform = enum_map_to_str(platform_map, opts->platform);
    assert(platform != NULL);
    printf("Waffle platform: %s\n", platform);

    const char *api = enum_map_to_str(context_api_map, opts->context_api);
    assert(api != NULL);
    printf("Waffle api: %s\n", api);

    printf("OpenGL vendor string: %s\n", vendor);
    printf("OpenGL renderer string: %s\n", renderer);
    printf("OpenGL version string: %s\n", version_str);

    int version = parse_version(version_str);

    if (opts->context_api == WAFFLE_CONTEXT_OPENGL && version >= 31) {
        print_context_flags();
    }

    // OpenGL and OpenGL ES >= 3.0 support glGetStringi(GL_EXTENSION, i).
    const bool use_getstringi = version >= 30;

    if (!glGetStringi && use_getstringi)
        error_get_gl_symbol("glGetStringi");

    if (opts->verbose)
        print_extensions(use_getstringi);

    return true;
}

int
main(int argc, char **argv)
{
    bool ok;
    int i;

    struct options opts = {0};

    int32_t init_attrib_list[3];

    struct waffle_display *dpy;
    struct waffle_config *config;
    struct waffle_context *ctx;
    struct waffle_window *window;

    #ifdef __APPLE__
        cocoa_init();
    #endif

    ok = parse_args(argc, argv, &opts);
    if (!ok)
        exit(EXIT_FAILURE);

    i = 0;
    init_attrib_list[i++] = WAFFLE_PLATFORM;
    init_attrib_list[i++] = opts.platform;
    init_attrib_list[i++] = WAFFLE_NONE;

    ok = waffle_init(init_attrib_list);
    if (!ok)
        error_waffle();

    dpy = waffle_display_connect(NULL);
    if (!dpy)
        error_waffle();

    if (!waffle_display_supports_context_api(dpy, opts.context_api)) {
        error_printf("Wflinfo", "Display does not support %s",
                     waffle_enum_to_string(opts.context_api));
    }

    glGetError = waffle_get_proc_address("glGetError");
    if (!glGetError)
        error_get_gl_symbol("glGetError");

    glGetIntegerv = waffle_get_proc_address("glGetIntegerv");
    if (!glGetIntegerv)
        error_get_gl_symbol("glGetIntegerv");

    glGetString = waffle_get_proc_address("glGetString");
    if (!glGetString)
        error_get_gl_symbol("glGetString");

    glGetStringi = waffle_get_proc_address("glGetStringi");

    const struct wutils_config_attrs config_attrs = {
        .api = opts.context_api,
        .profile = opts.context_profile,
        .major = opts.context_major,
        .minor = opts.context_minor,
        .forward_compat = opts.context_forward_compatible,
        .debug = opts.context_debug,
    };

    wutils_create_context(dpy, config_attrs, &ctx, &config);

    window = waffle_window_create(config, WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!window)
        error_waffle();

    ok = waffle_make_current(dpy, window, ctx);
    if (!ok)
        error_waffle();

    ok = print_wflinfo(&opts);
    if (!ok)
        error_waffle();

    ok = waffle_window_destroy(window);
    if (!ok)
        error_waffle();

    ok = waffle_context_destroy(ctx);
    if (!ok)
        error_waffle();

    ok = waffle_config_destroy(config);
    if (!ok)
        error_waffle();

    ok = waffle_display_disconnect(dpy);
    if (!ok)
        error_waffle();

    #ifdef __APPLE__
        cocoa_finish();
    #endif

    return EXIT_SUCCESS;
}
