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

#include "wutils.h"
#include <getopt.h>
#include <stdarg.h>
#include <string.h>

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
