/*
 * 3-D gear wheels.  This program is in the public domain.
 *
 * Brian Paul
 *
 * Conversion to Waffle by Jordan Justen
 */


#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "wflgears.h"

const char *wutils_utility_name = "wflgears";
const char *wutils_utility_Name = "Wflgears";

const char *usage_message =
    "Usage:\n"
    "    wflgears <Required Parameters> [Options]\n"
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
    "        Print wflgears usage information.\n"
    "\n"
    "Examples:\n"
    "    wflgears --platform=glx --api=gl\n"
    "    wflgears --platform=x11_egl --api=gl --version=3.2 --profile=core\n"
    "    wflgears --platform=wayland --api=gles3\n"
    "    wflgears --platform=gbm --api=gl --version=3.2 --verbose\n"
    "    wflgears -p gbm -a gl -V 3.2 -v\n"
    ;

static GLint T0 = 0;
static GLint Frames = 0;

void
wflgears_post_draw(struct waffle_window *window)
{
  bool ok;
  static bool window_shown = false;
  if (!window_shown) {
      ok = waffle_window_show(window);
      if (!ok) {
        error_printf("Wflgears", "Error showing window");
      }
      window_shown = true;
  }

  ok = waffle_window_swap_buffers(window);
  if (!ok) {
      error_printf("Wflgears", "Error swapping buffers");
  }

  Frames++;

  {
    GLint t = elapsed_ms();
    if (t - T0 >= 5000) {
      GLfloat seconds = (t - T0) / 1000.0;
      GLfloat fps = Frames / seconds;
      printf("%d frames in %6.3f seconds = %6.3f FPS\n", Frames, seconds, fps);
      fflush(stdout);
      T0 = t;
      Frames = 0;
    }
  }
}

static bool
display_wflgears(struct waffle_window *window, const struct wutils_config_attrs *config)
{
    if (config->api != WAFFLE_CONTEXT_OPENGL) {
        error_printf("Wflgears", "OpenGL ES is not currently supported");
    }
    if (config->profile == WAFFLE_CONTEXT_CORE_PROFILE) {
        error_printf("Wflgears", "Core profiles are not currently supported");
    }
    T0 = elapsed_ms();
    return display_wflgears_legacy_gl(window);
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
        error_printf("Wflgears", "Display does not support %s",
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

    ok = display_wflgears(window, &config_attrs);
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
