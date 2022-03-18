#include "flutter_embedder.h"
#include "renderer.h"
#include "standard_message_codec.h"
#include "xdg-shell-protocol.h"

#include <bits/pthreadtypes.h>
#include <bits/types.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include <stdatomic.h>

#include <string.h>
#include <wayland-server-protocol.h>
#include <wayland-util.h>
#include <wayland-server-core.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#define WLR_USE_UNSTABLE

#include <wlr/backend.h>
#include <wlr/util/log.h>
#include <wlr/render/gles2.h>
#include <wlr/render/egl.h>
#include <wlr/render/allocator.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_presentation_time.h>

#include "wlroots_hacks.h"

#include "flutter_wlroots.h"
#include "instance.h"
#include "shaders.h"
#include "input.h"
#include "task.h"
#include "output.h"
#include "surface.h"

//#define eglGetProcAddr eglGetProcAddress
//#define __glintercept_log(...) wlr_log(WLR_INFO, __VA_ARGS__)
//#include "gl_intercept_debug.h"

#define GL_ASSERT_ERROR(instance) do {\
  GLenum err = instance->glGetError();\
  if (err != GL_NO_ERROR) {\
    wlr_log(WLR_ERROR, "GL ERROR: %d", err);\
  }\
} while (0)

static bool engine_cb_renderer_make_current(void *user_data) {
  struct fwr_instance *instance = user_data;

  eglMakeCurrent(instance->egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, instance->fwr_renderer.flutter_egl_context);

  //wlr_log(WLR_DEBUG, "engine_cb_renderer_make_current");
  return true;
}
static bool engine_cb_renderer_clear_current(void *user_data) {
  struct fwr_instance *instance = user_data;
  //wlr_egl_unset_current(instance->egl);

  eglMakeCurrent(instance->egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

  //wlr_log(WLR_DEBUG, "engine_cb_renderer_clear_current");
  return true;
}

static bool engine_cb_renderer_make_resource_current(void *user_data) {
  struct fwr_instance *instance = user_data;
  eglMakeCurrent(instance->egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, instance->fwr_renderer.flutter_resource_egl_context);
  return true;
}

static void* engine_cb_renderer_gl_proc_resolve(void *user_data, const char *name) {
  return eglGetProcAddress(name);
}

//static bool EngineRendererExternalTexture(void *user_data, int64_t texture_id, size_t width, size_t height, FlutterOpenGLTexture *texture_out) {
//  wlr_log(WLR_INFO, "EngineRendererExternalTexture");
//  return true;
//}

static uint32_t engine_cb_renderer_fbo(void *user_data, const FlutterFrameInfo *frame_info) {
#ifdef FLUTTER_COMPOSITOR
  wlr_log(WLR_ERROR, "engine_cb_renderer_fbo called!");
  return 0;
#else // FLUTTER_COMPOSITOR
  struct fwr_instance *instance = user_data;

  GLuint fbo = fwr_renderer_get_active_fbo(instance);
  wlr_log(WLR_INFO, "==== FRAME EVENT ==== Engine given fbo: %d", fbo);

  return fbo;
#endif // FLUTTER_COMPOSITOR
}
static bool engine_cb_renderer_present(void *user_data, const FlutterPresentInfo *present_info) {
#ifdef FLUTTER_COMPOSITOR
  wlr_log(WLR_ERROR, "engine_cb_renderer_present called!");
  return false;
#else // FLUTTER_COMPOSITOR
  struct fwr_instance *instance = user_data;
  wlr_log(WLR_INFO, "==== FRAME EVENT ==== Engine called present with FBO: %d", present_info->fbo_id);

  fwr_renderer_flip_fbo(instance);

  return true;
#endif // FLUTTER_COMPOSITOR
}

static void engine_cb_platform_message(
    const FlutterPlatformMessage *engine_message,
    void *user_data) {
  struct fwr_instance *instance = user_data;

  if (engine_message->struct_size != sizeof(FlutterPlatformMessage)) {
    wlr_log(WLR_ERROR, "Invalid platform message size received. Expected %ld but received %ld", sizeof(FlutterPlatformMessage), engine_message->struct_size);
    return;
  }

  struct dart_value name = {};
  struct dart_value args = {};

  if (strcmp(engine_message->channel, "wlroots") == 0) {
    size_t offset = 0;

    if (!message_read(engine_message->message, engine_message->message_size, &offset, &name)) {
      wlr_log(WLR_ERROR, "Error decoding platform message name");
      goto error;
    }

    if (!message_read(engine_message->message, engine_message->message_size, &offset, &args)) {
      wlr_log(WLR_ERROR, "Error decoding platform message args");
      goto error;
    }

    if (name.type != dvString) {
      goto error;
    }
    const char *method_name = name.string.string;

    if (strcmp(method_name, "surface_pointer_event") == 0) {
      fwr_handle_surface_pointer_event_message(instance, engine_message->response_handle, &args);
      return;
    }
    if (strcmp(method_name, "surface_toplevel_set_size") == 0) {
      fwr_handle_surface_toplevel_set_size(instance, engine_message->response_handle, &args);
      return;
    }

    if (strcmp(method_name, "is_compositor") == 0) {
      // Just send a success response.
      instance->fl_proc_table.SendPlatformMessageResponse(
          instance->engine, engine_message->response_handle,
          method_call_null_success, sizeof(method_call_null_success));
    }

    wlr_log(WLR_INFO, "Unhandled platform message: channel: %s %s", engine_message->channel, method_name);
    goto error;
  }

error:
  // TODO(hansihe): Handle messages
  wlr_log(WLR_INFO, "Unhandled platform message: channel: %s", engine_message->channel);

  message_free(&name);
  message_free(&args);

  // Send failure
  instance->fl_proc_table.SendPlatformMessageResponse(instance->engine, engine_message->response_handle, NULL, 0);
}

int quit_ctr = 0;

static void engine_cb_log_message(const char *tag, const char *message, void *user_data) {
  wlr_log(WLR_INFO, "DART [%s] %s", tag, message);
}

bool fwr_instance_create(struct fwr_instance_opts opts, struct fwr_instance **instance_out) {
  wlr_log_init(WLR_DEBUG, NULL);

  struct fwr_instance *instance = calloc(1, sizeof(struct fwr_instance));

  pthread_mutexattr_t mutex_attr;
  pthread_mutexattr_init(&mutex_attr);
  pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
  if (pthread_mutex_init(&instance->platform_task_list_mutex, &mutex_attr) != 0) {
    wlr_log(WLR_ERROR, "Could not init render task list mutex");
  }

  instance->fl_proc_table.struct_size = sizeof(FlutterEngineProcTable);
  if (FlutterEngineGetProcAddresses(&instance->fl_proc_table) != kSuccess) {
    wlr_log(WLR_ERROR, "Could not get engine proc table");
    return false;
  }

  instance->wl_display = wl_display_create();
  instance->wl_event_loop = wl_display_get_event_loop(instance->wl_display);

	instance->backend = wlr_backend_autocreate(instance->wl_display);

  //server->renderer = wlr_renderer_autocreate(server->backend);

  int drm_fd = -1;

  // TODO(hansihe): allow selection of DRM node by env variable?
  // https://gitlab.freedesktop.org/wlroots/wlroots/-/blob/master/render/wlr_renderer.c#L369

  if (drm_fd < 0) {
    drm_fd = wlr_backend_get_drm_fd(instance->backend);
  }

  if (drm_fd < 0) {
    wlr_log(WLR_ERROR, "Could not open render node!");
    return false;
  }

  struct wlr_egl *egl = NULL;
  egl = wlr_egl_create_with_drm_fd(drm_fd);
  if (egl == NULL) {
    wlr_log(WLR_ERROR, "Failed to create EGL");
    return false;
  }
  instance->egl = egl;

  struct wlr_renderer *renderer = NULL;
  renderer = wlr_gles2_renderer_create(egl);
  if (renderer == NULL) {
    wlr_log(WLR_ERROR, "Failed to create GLES2 renderer");
    wlr_egl_destroy(egl);
    return false;
  }
  instance->renderer = renderer;

  wlr_renderer_init_wl_display(instance->renderer, instance->wl_display);

  instance->allocator = wlr_allocator_autocreate(instance->backend, instance->renderer);

  wlr_compositor_create(instance->wl_display, instance->renderer);
  wlr_data_device_manager_create(instance->wl_display);

  instance->output_layout = wlr_output_layout_create();

  instance->new_output.notify = fwr_server_new_output;
  wl_signal_add(&instance->backend->events.new_output, &instance->new_output);

  instance->xdg_shell = wlr_xdg_shell_create(instance->wl_display);
  instance->new_xdg_surface.notify = fwr_new_xdg_surface;
  wl_signal_add(&instance->xdg_shell->events.new_surface, &instance->new_xdg_surface);

  fwr_input_init(instance);

  const char *socket = wl_display_add_socket_auto(instance->wl_display);
	if (!socket) {
    wlr_log(WLR_ERROR, "Failed to create Wayland socket");
		wlr_backend_destroy(instance->backend);
    return false;
	}
  wlr_log(WLR_INFO, "Wayland socket: %s", socket);

  if (!wlr_backend_start(instance->backend)) {
		wlr_backend_destroy(instance->backend);
		wl_display_destroy(instance->wl_display);
		return false;
	}

  instance->views = handle_map_new();

  fwr_renderer_init(instance, eglGetProcAddress);
  //fwr_renderer_ensure_fbo(instance, 300, 300);

  instance->presentation = wlr_presentation_create(instance->wl_display, instance->backend);

  fwr_tasks_init(instance);

  FlutterRendererConfig renderer_config = {};
  renderer_config.type = kOpenGL;
  renderer_config.open_gl.struct_size = sizeof(FlutterOpenGLRendererConfig);
  renderer_config.open_gl.make_current = engine_cb_renderer_make_current;
  renderer_config.open_gl.clear_current = engine_cb_renderer_clear_current;
  renderer_config.open_gl.make_resource_current = engine_cb_renderer_make_resource_current;
  renderer_config.open_gl.fbo_reset_after_present = true;
  renderer_config.open_gl.gl_proc_resolver = engine_cb_renderer_gl_proc_resolve;
  //renderer_config.open_gl.gl_external_texture_frame_callback = EngineRendererExternalTexture;
  renderer_config.open_gl.fbo_with_frame_info_callback = engine_cb_renderer_fbo;
  renderer_config.open_gl.present_with_info = engine_cb_renderer_present;

  FlutterProjectArgs project_args = {};
  project_args.struct_size = sizeof(FlutterProjectArgs);
  project_args.command_line_argc = opts.argc;
  project_args.command_line_argv = opts.argv;
  project_args.assets_path = opts.assets_path;
  project_args.icu_data_path = opts.icu_data_path;
  project_args.platform_message_callback = engine_cb_platform_message;
  project_args.log_message_callback = engine_cb_log_message;
  project_args.custom_task_runners = &instance->custom_task_runners;
  #ifdef FLUTTER_COMPOSITOR
  project_args.compositor = &instance->fl_compositor;
  #endif
  project_args.vsync_callback = fwr_engine_vsync_callback;

  if (instance->fl_proc_table.RunsAOTCompiledDartCode()) {
    wlr_log(WLR_ERROR, "TODO: AOT");
    return false;
  }

  wlr_log(WLR_INFO, "Pre engine run");
  FlutterEngineResult fl_result = instance->fl_proc_table.Run(
    FLUTTER_ENGINE_VERSION,
    &renderer_config,
    &project_args,
    (void*) instance,
    &instance->engine
  );

  if (fl_result != kSuccess) {
    wlr_log(WLR_ERROR, "Flutter Engine Run failed!");
  }

  if (instance->output != NULL) {
    FlutterWindowMetricsEvent window_metrics = {};
    window_metrics.struct_size = sizeof(FlutterWindowMetricsEvent);
    window_metrics.width = instance->output->wlr_output->width;
    window_metrics.height = instance->output->wlr_output->height;
    window_metrics.pixel_ratio = 1.0;
    instance->fl_proc_table.SendWindowMetricsEvent(instance->engine, &window_metrics);
  }

  FlutterPointerEvent pointer_event = {};
  pointer_event.struct_size = sizeof(FlutterPointerEvent);
  pointer_event.phase = kAdd;
  pointer_event.timestamp = instance->fl_proc_table.GetCurrentTime();
  pointer_event.x = 0;
  pointer_event.y = 0;
  pointer_event.device = 0;
  pointer_event.signal_kind = kFlutterPointerSignalKindNone;
  pointer_event.scroll_delta_x = 0;
  pointer_event.scroll_delta_y = 0;
  pointer_event.device_kind = kFlutterPointerDeviceKindMouse;
  pointer_event.buttons = 0;
  instance->fl_proc_table.SendPointerEvent(instance->engine, &pointer_event, 1);

  //pthread_join(engine_bootstrap_thread, NULL);

  wlr_log(WLR_INFO, "Engine Run success!");

  //wlr_egl_make_current(instance->egl);

  wlr_log(WLR_INFO, "Running Wayland compositor on WAYLAND_DISPLAY=%s", socket);
  wl_display_run(instance->wl_display);

  wl_display_destroy_clients(instance->wl_display);
  wl_display_destroy(instance->wl_display);

  *instance_out = instance;
  return true;
}

bool fwr_instance_run(struct fwr_instance *instance) {

  return true;
}