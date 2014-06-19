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
/// @brief Common util definitions and function prototypes
///

#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define WAFFLE_API_VERSION 0x0103
#include "waffle.h"

extern const char *wutils_utility_name;
extern const char *wutils_utility_Name;

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

typedef float GLclampf;
typedef unsigned int GLbitfield;
typedef unsigned int GLint;
typedef int GLsizei;
typedef unsigned int GLenum;
typedef void GLvoid;
typedef unsigned char GLubyte;

enum {
    // Copied from <GL/gl*.h>.
    GL_NO_ERROR = 0,

    GL_CONTEXT_FLAGS = 0x821e,
    GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT = 0x00000001,
    GL_CONTEXT_FLAG_DEBUG_BIT              = 0x00000002,
    GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT_ARB  = 0x00000004,

    GL_VENDOR                              = 0x1F00,
    GL_RENDERER                            = 0x1F01,
    GL_VERSION                             = 0x1F02,
    GL_EXTENSIONS                          = 0x1F03,
    GL_NUM_EXTENSIONS                      = 0x821D,
};

#define GL_MAJOR_VERSION                  0x821B
#define GL_MINOR_VERSION                  0x821C
#define GL_CONTEXT_PROFILE_MASK           0x9126
#define GL_CONTEXT_CORE_PROFILE_BIT       0x00000001
#define GL_CONTEXT_COMPATIBILITY_PROFILE_BIT 0x00000002

extern GLenum (*glGetError)(void);
extern void (*glGetIntegerv)(GLenum pname, GLint *params);
extern const GLubyte * (*glGetString)(GLenum name);
extern const GLubyte * (*glGetStringi)(GLenum name, GLint i);

struct enum_map {
    int i;
    const char *s;
};

extern const struct enum_map platform_map[];
extern const struct enum_map context_api_map[];

bool
enum_map_translate_str(
        const struct enum_map *self,
        const char *s,
        int *result);

const char *
enum_map_to_str(const struct enum_map *self,
                int val);

/// @brief Command line options.
struct options {
    /// @brief One of `WAFFLE_PLATFORM_*`.
    int platform;

    /// @brief One of `WAFFLE_CONTEXT_OPENGL_*`.
    int context_api;

    /// @brief One of `WAFFLE_CONTEXT_PROFILE_*` or `WAFFLE_NONE`.
    int context_profile;

    int context_major;
    int context_minor;

    bool verbose;

    bool context_forward_compatible;
    bool context_debug;

    /// @brief One of `WAFFLE_DL_*`.
    int dl;
};

void __attribute__((noreturn))
error_waffle(void);

void __attribute__((noreturn))
error_printf(const char *module, const char *fmt, ...);

void __attribute__((noreturn))
usage_error_printf(const char *fmt, ...);

void __attribute__((noreturn))
write_usage_and_exit(FILE *f, int exit_code);

bool
parse_args(int argc, char *argv[], struct options *opts);

#ifdef __APPLE__

void
cocoa_init(void);

void
cocoa_finish(void);

#endif // __APPLE__
