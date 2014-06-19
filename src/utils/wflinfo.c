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

static const char *usage_message =
    "Usage:\n"
    "    wflinfo <Required Parameters> [Options]\n"
    "\n"
    "Description:\n"
    "    Create an OpenGL or OpenGL ES context and print information about it.\n"
    "\n"
    "Required Parameters:\n"
    "    -p, --platform\n"
    "        One of: android, cgl, gbm, glx, wayland or x11_egl\n"
    "\n"
    "    -a, --api\n"
    "        One of: gl, gles1, gles2 or gles3\n"
    "\n"
    "Options:\n"
    "    -V, --version\n"
    "        For example --api=gl --version=3.2 would request OpenGL 3.2.\n"
    "\n"
    "    --profile\n"
    "        One of: core, compat or none\n"
    "\n"
    "    -v, --verbose\n"
    "        Print more information.\n"
    "\n"
    "    --forward-compatible\n"
    "        Create a forward-compatible context.\n"
    "\n"
    "    --debug-context\n"
    "        Create a debug context.\n"
    "\n"
    "    -h, --help\n"
    "        Print wflinfo usage information.\n"
    "\n"
    "Examples:\n"
    "    wflinfo --platform=glx --api=gl\n"
    "    wflinfo --platform=x11_egl --api=gl --version=3.2 --profile=core\n"
    "    wflinfo --platform=wayland --api=gles3\n"
    "    wflinfo --platform=gbm --api=gl --version=3.2 --verbose\n"
    "    wflinfo -p gbm -a gl -V 3.2 -v\n"
    ;

static bool
strneq(const char *a, const char *b, size_t n)
{
    return strncmp(a, b, n) == 0;
}

void __attribute__((noreturn))
write_usage_and_exit(FILE *f, int exit_code)
{
    fprintf(f, "%s", usage_message);
    exit(exit_code);
}

void __attribute__((noreturn))
usage_error_printf(const char *fmt, ...)
{
    fprintf(stderr, "Wflinfo usage error: ");

    if (fmt) {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
        fprintf(stderr, " ");
    }

    fprintf(stderr, "(see wflinfo --help)\n");
    exit(EXIT_FAILURE);
}

static void
error_get_gl_symbol(const char *name)
{
    error_printf("Wflinfo", "failed to get function pointer for %s", name);
}

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

/// @brief Attributes for waffle_choose_config().
struct wflinfo_config_attrs {
    /// @brief One of `WAFFLE_CONTEXT_OPENGL_*`.
    enum waffle_enum api;

    /// @brief One of `WAFFLE_CONTEXT_PROFILE_*` or `WAFFLE_NONE`.
    enum waffle_enum profile;

    /// @brief The version major number.
    int32_t major;

    /// @brief The version minor number.
    int32_t minor;

    /// @brief Create a forward-compatible context.
    bool forward_compat;

    /// @brief Create a debug context.
    bool debug;
};

static bool
wflinfo_try_create_context(struct waffle_display *dpy,
                           struct wflinfo_config_attrs attrs,
                           struct waffle_context **out_ctx,
                           struct waffle_config **out_config,
                           bool exit_on_fail)
{
    int i;
    int32_t config_attrib_list[64];
    struct waffle_context *ctx = NULL;
    struct waffle_config *config = NULL;

    i = 0;
    config_attrib_list[i++] = WAFFLE_CONTEXT_API;
    config_attrib_list[i++] = attrs.api;

    if (attrs.profile != WAFFLE_DONT_CARE) {
        config_attrib_list[i++] = WAFFLE_CONTEXT_PROFILE;
        config_attrib_list[i++] = attrs.profile;
    }

    if (attrs.major != WAFFLE_DONT_CARE && attrs.minor != WAFFLE_DONT_CARE) {
        config_attrib_list[i++] = WAFFLE_CONTEXT_MAJOR_VERSION;
        config_attrib_list[i++] = attrs.major;
        config_attrib_list[i++] = WAFFLE_CONTEXT_MINOR_VERSION;
        config_attrib_list[i++] = attrs.minor;
    }

    if (attrs.forward_compat) {
        config_attrib_list[i++] = WAFFLE_CONTEXT_FORWARD_COMPATIBLE;
        config_attrib_list[i++] = true;
    }

    if (attrs.debug) {
        config_attrib_list[i++] = WAFFLE_CONTEXT_DEBUG;
        config_attrib_list[i++] = true;
    }

    static int32_t dont_care_attribs[] = {
        WAFFLE_RED_SIZE,
        WAFFLE_GREEN_SIZE,
        WAFFLE_BLUE_SIZE,
        WAFFLE_ALPHA_SIZE,
        WAFFLE_DEPTH_SIZE,
        WAFFLE_STENCIL_SIZE,
        WAFFLE_DOUBLE_BUFFERED,
    };
    int dont_care_attribs_count =
        sizeof(dont_care_attribs) / sizeof(dont_care_attribs[0]);

    for (int j = 0; j < dont_care_attribs_count; j++) {
        config_attrib_list[i++] = dont_care_attribs[j];
        config_attrib_list[i++] = WAFFLE_DONT_CARE;
    }

    config_attrib_list[i++] = 0;

    config = waffle_config_choose(dpy, config_attrib_list);
    if (!config) {
        goto fail;
    }

    ctx = waffle_context_create(config, NULL);
    if (!ctx) {
        goto fail;
    }

    *out_ctx = ctx;
    *out_config = config;
    return true;

fail:
    if (exit_on_fail) {
        error_waffle();
    }
    if (ctx) {
        waffle_context_destroy(ctx);
    }
    if (config) {
        waffle_config_destroy(config);
    }

    return false;
}

/// @brief Return 10 * version of the current OpenGL context.
static int
gl_get_version(void)
{
    GLint major_version = 0;
    GLint minor_version = 0;

    glGetIntegerv(GL_MAJOR_VERSION, &major_version);
    if (glGetError()) {
        error_printf("Wflinfo", "glGetIntegerv(GL_MAJOR_VERSION) failed");
    }

    glGetIntegerv(GL_MINOR_VERSION, &minor_version);
    if (glGetError()) {
        error_printf("Wflinfo", "glGetIntegerv(GL_MINOR_VERSION) failed");
    }
    return 10 * major_version + minor_version;
}

/// @brief Check if current context has an extension using glGetString().
static bool
gl_has_extension_GetString(const char *name)
{
    const size_t buf_len = 4096;
    char exts[buf_len];

    const uint8_t *exts_orig = glGetString(GL_EXTENSIONS);
    if (glGetError()) {
        error_printf("Wflinfo", "glGetInteger(GL_EXTENSIONS) failed");
    }

    memcpy(exts, exts_orig, buf_len);
    exts[buf_len - 1] = 0;

    char *ext = strtok(exts, " ");
    do {
        if (strneq(ext, name, buf_len)) {
            return true;
        }
        ext = strtok(NULL, " ");
    } while (ext);

    return false;
}

/// @brief Check if current context has an extension using glGetStringi().
static bool
gl_has_extension_GetStringi(const char *name)
{
    const size_t max_ext_len = 128;
    uint32_t num_exts = 0;

    glGetIntegerv(GL_NUM_EXTENSIONS, &num_exts);
    if (glGetError()) {
        error_printf("Wflinfo", "glGetIntegerv(GL_NUM_EXTENSIONS) failed");
    }

    for (int i = 0; i < num_exts; i++) {
        const uint8_t *ext = glGetStringi(GL_EXTENSIONS, i);
        if (!ext || glGetError()) {
            error_printf("Wflinfo", "glGetStringi(GL_EXTENSIONS) failed");
        } else if (strneq((const char*) ext, name, max_ext_len)) {
            return true;
        }
    }

    return false;
}

/// @brief Check if current context has an extension.
static bool
gl_has_extension(const char *name)
{
    if (gl_get_version() >= 30) {
        return gl_has_extension_GetStringi(name);
    } else {
        return gl_has_extension_GetString(name);
    }
}

/// @brief Get the profile of a desktop OpenGL context.
///
/// Return one of WAFFLE_CONTEXT_CORE_PROFILE,
/// WAFFLE_CONTEXT_COMPATIBILITY_PROFILE, or WAFFLE_NONE.
///
/// Even though an OpenGL 3.1 context strictly has no profile, according to
/// this function a 3.1 context belongs to the core profile if and only if it
/// lacks the GL_ARB_compatibility extension.
///
/// According to this function, a context has no profile if and only if its
/// version is 3.0 or lower.
static enum waffle_enum
gl_get_profile(void)
{
    int version = gl_get_version();

    if (version >= 32) {
        uint32_t profile_mask = 0;
        glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile_mask);
        if (glGetError()) {
            error_printf("Wflinfo", "glGetIntegerv(GL_CONTEXT_PROFILE_MASK) "
                        "failed");
        } else if (profile_mask & GL_CONTEXT_CORE_PROFILE_BIT) {
            return WAFFLE_CONTEXT_CORE_PROFILE;
        } else if (profile_mask & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) {
            return WAFFLE_CONTEXT_COMPATIBILITY_PROFILE;
        } else {
            error_printf("Wflinfo", "glGetIntegerv(GL_CONTEXT_PROFILE_MASK) "
                         "return a mask with no profile bit: 0x%x",
                         profile_mask);
        }
    } else if (version == 31) {
        if (gl_has_extension("GL_ARB_compatibility")) {
            return WAFFLE_CONTEXT_CORE_PROFILE;
        } else {
            return WAFFLE_CONTEXT_COMPATIBILITY_PROFILE;
        }
    } else {
        return WAFFLE_NONE;
    }
}

/// @brief Create an OpenGL >= 3.1 context.
///
/// If the requested profile is WAFFLE_NONE or WAFFLE_DONT_CARE and context
/// creation succeeds, then return true.
///
/// If a specific profile of OpenGL 3.1 is requested, then this function tries
/// to honor the intent of that request even though, strictly speaking, an
/// OpenGL 3.1 context has no profile.  (See gl_get_profile() for a description
/// of how wflinfo determines the profile of a context). If context creation
/// succeeds but its profile is incorrect, then return false.
///
/// On failure, @a out_ctx and @out_config remain unmodified.
///
static bool
wflinfo_try_create_context_gl31(struct waffle_display *dpy,
                                struct wflinfo_config_attrs attrs,
                                struct waffle_context **out_ctx,
                                struct waffle_config **out_config,
                                bool exit_if_ctx_creation_fails)
{
    struct waffle_config *config = NULL;
    struct waffle_context *ctx = NULL;
    bool ok;

    // It's illegal to request a waffle_config with WAFFLE_CONTEXT_PROFILE
    // != WAFFLE_NONE. Therefore, request an OpenGL 3.1 config without
    // a profile and later verify that the desired and actual profile
    // agree.
    const enum waffle_enum desired_profile = attrs.profile;
    attrs.major = 3;
    attrs.minor = 1;
    attrs.profile = WAFFLE_NONE;
    wflinfo_try_create_context(dpy, attrs, &ctx, &config,
                               exit_if_ctx_creation_fails);

    if (desired_profile == WAFFLE_NONE ||
        desired_profile == WAFFLE_DONT_CARE) {
        goto success;
    }

    // The user cares about the profile. We must bind the context to inspect
    // its profile.
    //
    // Skip window creation. No window is needed when binding an OpenGL >= 3.0
    // context.
    ok = waffle_make_current(dpy, NULL, ctx);
    if (!ok) {
        error_waffle();
    }

    const enum waffle_enum actual_profile = gl_get_profile();
    waffle_make_current(dpy, NULL, NULL);
    if (actual_profile == desired_profile) {
        goto success;
    }

    return false;

success:
    *out_ctx = ctx;
    *out_config = config;
    return true;
}

/// Exit on failure.
static void
wflinfo_create_context(struct waffle_display *dpy,
                       struct wflinfo_config_attrs attrs,
                       struct waffle_context **out_ctx,
                       struct waffle_config **out_config)
{
    bool ok = false;

    if (attrs.api == WAFFLE_CONTEXT_OPENGL &&
        attrs.profile != WAFFLE_NONE &&
        attrs.major == WAFFLE_DONT_CARE) {

        // If the user requested OpenGL and a CORE or COMPAT profile,
        // but they didn't specify a version, then we'll try a set
        // of known versions from highest to lowest.

        static int known_gl_profile_versions[] =
            { 32, 33, 40, 41, 42, 43, 44 };

        for (int i = ARRAY_SIZE(known_gl_profile_versions) - 1; i >= 0; i--) {
            attrs.major = known_gl_profile_versions[i] / 10;
            attrs.minor = known_gl_profile_versions[i] % 10;
            ok = wflinfo_try_create_context(dpy, attrs,
                                            out_ctx, out_config, false);
            if (ok) {
                return;
            }
        }

        // Handle OpenGL 3.1 separately because profiles are weird in 3.1.
        ok = wflinfo_try_create_context_gl31(
                dpy, attrs, out_ctx, out_config,
                /*exit_if_ctx_creation_fails*/ false);
        if (ok) {
            return;
        }

        error_printf("Wflinfo", "Failed to create context; Try choosing a "
                     "specific context version with --version");
    } else if (attrs.api == WAFFLE_CONTEXT_OPENGL &&
               attrs.major == 3 &&
               attrs.minor == 1) {
        // The user requested a specific profile of an OpenGL 3.1 context.
        // Strictly speaking, an OpenGL 3.1 context has no profile, but let's
        // do what the user wants.
        ok = wflinfo_try_create_context_gl31(
                dpy, attrs, out_ctx, out_config,
                /*exit_if_ctx_creation_fails*/ true);
        if (ok) {
            return;
        }

        printf("Wflinfo warn: Succesfully requested an OpenGL 3.1 context, but returned\n"
               "Wflinfo warn: context had the wrong profile.  Fallback to requesting an\n"
               "Wflinfo warn: OpenGL 3.2 context, which is guaranteed to have the correct\n"
               "Wflinfo warn: profile if context creation succeeds.\n");
        attrs.major = 3;
        attrs.minor = 2;
        assert(attrs.profile == WAFFLE_CONTEXT_CORE_PROFILE ||
               attrs.profile == WAFFLE_CONTEXT_COMPATIBILITY_PROFILE);
        ok = wflinfo_try_create_context(dpy, attrs, out_ctx, out_config,
                                        /*exit_on_fail*/ false);
        if (ok) {
            return;
        }

        error_printf("Wflinfo", "Failed to create an OpenGL 3.1 or later "
                     "context with requested profile");
    } else {
        wflinfo_try_create_context(dpy, attrs, out_ctx, out_config,
                                  /*exit_on_fail*/ true);
    }
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

    const struct wflinfo_config_attrs config_attrs = {
        .api = opts.context_api,
        .profile = opts.context_profile,
        .major = opts.context_major,
        .minor = opts.context_minor,
        .forward_compat = opts.context_forward_compatible,
        .debug = opts.context_debug,
    };

    wflinfo_create_context(dpy, config_attrs, &ctx, &config);

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
