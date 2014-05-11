/*
 * 3-D gear wheels.  This program is in the public domain.
 *
 * Brian Paul
 *
 * Conversion to Waffle by Jordan Justen
 */

#pragma once

#include "wutils.h"

#define WINDOW_WIDTH  300
#define WINDOW_HEIGHT 300

#ifndef M_PI
#define M_PI 3.14159265
#endif

#define REQ_GL_FUNC(f) { #f, (void**)&f }

struct gl_func_name_and_ptr {
    const char *name;
    void **ptr;
};

#define GL_FLAT					0x1D00
#define GL_SMOOTH				0x1D01
#define GL_QUADS				0x0007
#define GL_QUAD_STRIP				0x0008
#define GL_PROJECTION				0x1701
#define GL_MODELVIEW				0x1700
#define GL_LIGHT0				0x4000
#define GL_POSITION				0x1203
#define GL_CULL_FACE				0x0B44
#define GL_LIGHTING				0x0B50
#define GL_DEPTH_TEST				0x0B71
#define GL_COMPILE				0x1300
#define GL_FRONT				0x0404
#define GL_AMBIENT_AND_DIFFUSE			0x1602
#define GL_NORMALIZE				0x0BA1
#define GL_COLOR_BUFFER_BIT			0x00004000
#define GL_DEPTH_BUFFER_BIT			0x00000100

void
wflgears_post_draw(struct waffle_window *window);

bool
display_wflgears_legacy_gl(struct waffle_window *window);
