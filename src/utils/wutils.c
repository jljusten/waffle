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
/// @brief Common util definitions and functions

#define _POSIX_C_SOURCE 199309L

#include "wutils.h"
#include <assert.h>
#include <getopt.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#ifdef __APPLE__
#    import <Foundation/NSAutoreleasePool.h>
#    import <Appkit/NSApplication.h>

static void
removeXcodeArgs(int *argc, char **argv);
#endif

GLenum (*glGetError)(void);
void (*glGetIntegerv)(GLenum pname, GLint *params);
const GLubyte * (*glGetString)(GLenum name);
const GLubyte * (*glGetStringi)(GLenum name, GLint i);

const struct enum_map platform_map[] = {
    {WAFFLE_PLATFORM_ANDROID,   "android"       },
    {WAFFLE_PLATFORM_CGL,       "cgl",          },
    {WAFFLE_PLATFORM_GBM,       "gbm"           },
    {WAFFLE_PLATFORM_GLX,       "glx"           },
    {WAFFLE_PLATFORM_WAYLAND,   "wayland"       },
    {WAFFLE_PLATFORM_X11_EGL,   "x11_egl"       },
    {0,                         0               },
};

const struct enum_map context_api_map[] = {
    {WAFFLE_CONTEXT_OPENGL,         "gl"        },
    {WAFFLE_CONTEXT_OPENGL_ES1,     "gles1"     },
    {WAFFLE_CONTEXT_OPENGL_ES2,     "gles2"     },
    {WAFFLE_CONTEXT_OPENGL_ES3,     "gles3"     },
    {0,                             0           },
};

/// @brief Translate string to `enum waffle_enum`.
///
/// @param self is a list of map items. The last item must be zero-filled.
/// @param result is altered only if @a s if found.
/// @return true if @a s was found in @a map.
bool
enum_map_translate_str(
        const struct enum_map *self,
        const char *s,
        int *result)
{
    for (const struct enum_map *i = self; i->i != 0; ++i) {
        if (!strncmp(s, i->s, strlen(i->s) + 1)) {
            *result = i->i;
            return true;
        }
    }

    return false;
}

const char *
enum_map_to_str(const struct enum_map *self,
                int val)
{
    for (const struct enum_map *i = self; i->i != 0; ++i) {
        if (i->i == val) {
            return i->s;
        }
    }

    return NULL;
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

enum {
    OPT_PLATFORM = 'p',
    OPT_API = 'a',
    OPT_VERSION = 'V',
    OPT_PROFILE,
    OPT_VERBOSE = 'v',
    OPT_DEBUG_CONTEXT,
    OPT_FORWARD_COMPATIBLE,
    OPT_HELP = 'h',
};

static const struct option get_opts[] = {
    { .name = "platform",       .has_arg = required_argument,     .val = OPT_PLATFORM },
    { .name = "api",            .has_arg = required_argument,     .val = OPT_API },
    { .name = "version",        .has_arg = required_argument,     .val = OPT_VERSION },
    { .name = "profile",        .has_arg = required_argument,     .val = OPT_PROFILE },
    { .name = "verbose",        .has_arg = no_argument,           .val = OPT_VERBOSE },
    { .name = "debug-context",  .has_arg = no_argument,           .val = OPT_DEBUG_CONTEXT },
    { .name = "forward-compatible", .has_arg = no_argument,       .val = OPT_FORWARD_COMPATIBLE },
    { .name = "help",           .has_arg = no_argument,           .val = OPT_HELP },
    { 0 },
};

/// @return true on success.
bool
parse_args(int argc, char *argv[], struct options *opts)
{
    bool ok;
    bool loop_get_opt = true;

#ifdef __APPLE__
    removeXcodeArgs(&argc, argv);
#endif

    // Set options to default values.
    opts->context_profile = WAFFLE_NONE;
    opts->context_major = WAFFLE_DONT_CARE;
    opts->context_minor = WAFFLE_DONT_CARE;

    // prevent getopt_long from printing an error message
    opterr = 0;

    while (loop_get_opt) {
        int opt = getopt_long(argc, argv, "a:hp:vV:", get_opts, NULL);
        switch (opt) {
            case -1:
                loop_get_opt = false;
                break;
            case '?':
                goto error_unrecognized_arg;
            case OPT_PLATFORM:
                ok = enum_map_translate_str(platform_map, optarg,
                                            &opts->platform);
                if (!ok) {
                    usage_error_printf("'%s' is not a valid platform",
                                       optarg);
                }
                break;
            case OPT_API:
                ok = enum_map_translate_str(context_api_map, optarg,
                                            &opts->context_api);
                if (!ok) {
                    usage_error_printf("'%s' is not a valid API for an OpenGL "
                                       "context", optarg);
                }
                break;
            case OPT_VERSION: {
                int major;
                int minor;
                int match_count;

                match_count = sscanf(optarg, "%d.%d", &major, &minor);
                if (match_count != 2 || major < 0 || minor < 0) {
                    usage_error_printf("'%s' is not a valid OpenGL version",
                                       optarg);
                }
                opts->context_major = major;
                opts->context_minor = minor;
                break;
            }
            case OPT_PROFILE:
                if (strcmp(optarg, "none") == 0) {
                    opts->context_profile = WAFFLE_NONE;
                } else if (strcmp(optarg, "core") == 0) {
                    opts->context_profile = WAFFLE_CONTEXT_CORE_PROFILE;
                } else if (strcmp(optarg, "compat") == 0) {
                    opts->context_profile = WAFFLE_CONTEXT_COMPATIBILITY_PROFILE;
                } else {
                    usage_error_printf("'%s' is not a valid OpenGL profile",
                                       optarg);
                }
                break;
            case OPT_VERBOSE:
                opts->verbose = true;
                break;
            case OPT_FORWARD_COMPATIBLE:
                opts->context_forward_compatible = true;
                break;
            case OPT_DEBUG_CONTEXT:
                opts->context_debug = true;
                break;
            case OPT_HELP:
                write_usage_and_exit(stdout, EXIT_SUCCESS);
                break;
            default:
                abort();
                loop_get_opt = false;
                break;
        }
    }

    if (optind < argc) {
        goto error_unrecognized_arg;
    }

    if (!opts->platform) {
        usage_error_printf("--platform is required");
    }

    if (!opts->context_api) {
        usage_error_printf("--api is required");
    }

    // Set dl.
    switch (opts->context_api) {
        case WAFFLE_CONTEXT_OPENGL:     opts->dl = WAFFLE_DL_OPENGL;      break;
        case WAFFLE_CONTEXT_OPENGL_ES1: opts->dl = WAFFLE_DL_OPENGL_ES1;  break;
        case WAFFLE_CONTEXT_OPENGL_ES2: opts->dl = WAFFLE_DL_OPENGL_ES2;  break;
        case WAFFLE_CONTEXT_OPENGL_ES3: opts->dl = WAFFLE_DL_OPENGL_ES3;  break;
        default:
            abort();
            break;
    }

    return true;

error_unrecognized_arg:
    if (optarg)
        usage_error_printf("unrecognized option '%s'", optarg);
    else if (optopt)
        usage_error_printf("unrecognized option '-%c'", optopt);
    else
        usage_error_printf("unrecognized option");
}

void __attribute__((noreturn))
error_waffle(void)
{
    const struct waffle_error_info *info = waffle_error_get_info();
    const char *code = waffle_error_to_string(info->code);

    if (info->message_length > 0)
        error_printf("Waffle", "0x%x %s: %s", info->code, code, info->message);
    else
        error_printf("Waffle", "0x%x %s", info->code, code);
}

void __attribute__((noreturn))
error_printf(const char *module, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "%s error: ", module);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);

    exit(EXIT_FAILURE);
}

void
error_get_gl_symbol(const char *name)
{
    error_printf(wutils_utility_Name, "failed to get function pointer for %s", name);
}

static const char *usage_message =
    "Usage:\n"
    "    %s <Required Parameters> [Options]\n"
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
    "        Print %s usage information.\n"
    "\n"
    "Examples:\n"
    "    %s --platform=glx --api=gl\n"
    "    %s --platform=x11_egl --api=gl --version=3.2 --profile=core\n"
    "    %s --platform=wayland --api=gles3\n"
    "    %s --platform=gbm --api=gl --version=3.2 --verbose\n"
    "    %s -p gbm -a gl -V 3.2 -v\n"
    ;

void __attribute__((noreturn))
write_usage_and_exit(FILE *f, int exit_code)
{
    fprintf(f, usage_message, wutils_utility_name, wutils_utility_name,
            wutils_utility_name, wutils_utility_name, wutils_utility_name,
            wutils_utility_name, wutils_utility_name);
    exit(exit_code);
}

static bool
strneq(const char *a, const char *b, size_t n)
{
    return strncmp(a, b, n) == 0;
}

static bool
wutils_try_create_context(struct waffle_display *dpy,
                          struct wutils_config_attrs attrs,
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
        error_printf(wutils_utility_Name, "glGetIntegerv(GL_MAJOR_VERSION) failed");
    }

    glGetIntegerv(GL_MINOR_VERSION, &minor_version);
    if (glGetError()) {
        error_printf(wutils_utility_Name, "glGetIntegerv(GL_MINOR_VERSION) failed");
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
        error_printf(wutils_utility_Name, "glGetInteger(GL_EXTENSIONS) failed");
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
        error_printf(wutils_utility_Name, "glGetIntegerv(GL_NUM_EXTENSIONS) failed");
    }

    for (int i = 0; i < num_exts; i++) {
        const uint8_t *ext = glGetStringi(GL_EXTENSIONS, i);
        if (!ext || glGetError()) {
            error_printf(wutils_utility_Name, "glGetStringi(GL_EXTENSIONS) failed");
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
            error_printf(wutils_utility_Name, "glGetIntegerv(GL_CONTEXT_PROFILE_MASK) "
                        "failed");
        } else if (profile_mask & GL_CONTEXT_CORE_PROFILE_BIT) {
            return WAFFLE_CONTEXT_CORE_PROFILE;
        } else if (profile_mask & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) {
            return WAFFLE_CONTEXT_COMPATIBILITY_PROFILE;
        } else {
            error_printf(wutils_utility_Name, "glGetIntegerv(GL_CONTEXT_PROFILE_MASK) "
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
/// of how wutils determines the profile of a context). If context creation
/// succeeds but its profile is incorrect, then return false.
///
/// On failure, @a out_ctx and @out_config remain unmodified.
///
static bool
wutils_try_create_context_gl31(struct waffle_display *dpy,
                               struct wutils_config_attrs attrs,
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
    wutils_try_create_context(dpy, attrs, &ctx, &config,
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
void
wutils_create_context(struct waffle_display *dpy,
                      struct wutils_config_attrs attrs,
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
            ok = wutils_try_create_context(dpy, attrs,
                                           out_ctx, out_config, false);
            if (ok) {
                return;
            }
        }

        // Handle OpenGL 3.1 separately because profiles are weird in 3.1.
        ok = wutils_try_create_context_gl31(
                dpy, attrs, out_ctx, out_config,
                /*exit_if_ctx_creation_fails*/ false);
        if (ok) {
            return;
        }

        error_printf(wutils_utility_Name, "Failed to create context; Try choosing a "
                     "specific context version with --version");
    } else if (attrs.api == WAFFLE_CONTEXT_OPENGL &&
               attrs.major == 3 &&
               attrs.minor == 1) {
        // The user requested a specific profile of an OpenGL 3.1 context.
        // Strictly speaking, an OpenGL 3.1 context has no profile, but let's
        // do what the user wants.
        ok = wutils_try_create_context_gl31(
                dpy, attrs, out_ctx, out_config,
                /*exit_if_ctx_creation_fails*/ true);
        if (ok) {
            return;
        }

        printf("%s warn: Succesfully requested an OpenGL 3.1 context, but returned\n"
               "%s warn: context had the wrong profile.  Fallback to requesting an\n"
               "%s warn: OpenGL 3.2 context, which is guaranteed to have the correct\n"
               "%s warn: profile if context creation succeeds.\n",
               wutils_utility_Name, wutils_utility_Name, wutils_utility_Name,
               wutils_utility_Name);
        attrs.major = 3;
        attrs.minor = 2;
        assert(attrs.profile == WAFFLE_CONTEXT_CORE_PROFILE ||
               attrs.profile == WAFFLE_CONTEXT_COMPATIBILITY_PROFILE);
        ok = wutils_try_create_context(dpy, attrs, out_ctx, out_config,
                                       /*exit_on_fail*/ false);
        if (ok) {
            return;
        }

        error_printf(wutils_utility_Name, "Failed to create an OpenGL 3.1 or later "
                     "context with requested profile");
    } else {
        wutils_try_create_context(dpy, attrs, out_ctx, out_config,
                                  /*exit_on_fail*/ true);
    }
}

#ifdef __APPLE__

static NSAutoreleasePool *pool;

void
cocoa_init(void)
{
    // From the NSApplication Class Reference:
    //     [...] if you do need to use Cocoa classes within the main()
    //     function itself (other than to load nib files or to instantiate
    //     NSApplication), you should create an autorelease pool before using
    //     the classes and then release the pool when you’re done.
    pool = [[NSAutoreleasePool alloc] init];

    // From the NSApplication Class Reference:
    //     The sharedApplication class method initializes the display
    //     environment and connects your program to the window server and the
    //     display server.
    //
    // It also creates the singleton NSApp if it does not yet exist.
    [NSApplication sharedApplication];
}

void
cocoa_finish(void)
{
    [pool drain];
}

static void
removeArg(int index, int *argc, char **argv)
{
    --*argc;
    for (; index < *argc; ++index)
        argv[index] = argv[index + 1];
}

static void
removeXcodeArgs(int *argc, char **argv)
{
    // Xcode sometimes adds additional arguments.
    for (int i = 1; i < *argc; )
    {
        if (strcmp(argv[i], "-NSDocumentRevisionsDebugMode") == 0 ||
            strcmp(argv[i], "-ApplePersistenceIgnoreState" ) == 0)
        {
            removeArg(i, argc, argv);
            removeArg(i, argc, argv);
        } else
            ++i;
    }
}

#endif // __APPLE__

static int64_t
wutils_get_microseconds(void)
{
    struct timespec t;
    int r = clock_gettime(CLOCK_MONOTONIC, &t);
    if (r >= 0)
        return (t.tv_sec * 1000000) + (t.tv_nsec / 1000);
    else
        return -1LL;
}

int64_t
elapsed_ms(void)
{
    return wutils_get_microseconds() / 1000;
}
