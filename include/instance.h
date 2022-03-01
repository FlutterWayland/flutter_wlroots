#pragma once

#include "flutter_embedder.h"
#include "xdg-shell-protocol.h"

#include <stdatomic.h>

#include <GLES2/gl2.h>
#include <wayland-server-core.h>

#include "shaders.h"
#include "renderer.h"
#include "input.h"

struct fwr_instance {
  struct wl_display *wl_display;
  struct wl_event_loop *wl_event_loop;
  struct wlr_backend *backend;
  struct wlr_egl *egl;
  struct wlr_renderer *renderer;
  struct wlr_allocator *allocator;

  struct wlr_xdg_shell *xdg_shell;
  struct wl_listener new_xdg_surface;

  struct wlr_cursor *cursor;
  struct wlr_xcursor_manager *cursor_mgr;
  struct wl_listener cursor_motion;
  struct wl_listener cursor_motion_absolute;
  struct wl_listener cursor_button;
  struct wl_listener cursor_axis;
  struct wl_listener cursor_frame;
  struct wl_listener cursor_touch_down;
  struct wl_listener cursor_touch_up;
  struct wl_listener cursor_touch_motion;
  struct wl_listener cursor_touch_frame;
  struct fwr_input_state input;

  struct wl_listener new_input;

  struct wlr_seat *seat;

  struct wlr_output_layout *output_layout;
  // If null, we don't have an output.
  struct output *output;
  struct wl_listener new_output;

  FlutterEngineProcTable fl_proc_table;
  FlutterEngine engine;
  FlutterCustomTaskRunners custom_task_runners;
  //FlutterTaskRunnerDescription render_task_runner;
  FlutterTaskRunnerDescription platform_task_runner;
  FlutterCompositor fl_compositor;

  pid_t platform_tid;

  int platform_notify_fd;
  struct wl_event_source *platform_notify_event_source;
  struct wl_event_source *platform_timer_event_source;
  pthread_mutex_t platform_task_list_mutex;
  struct fwr_render_task *queued_platform_tasks;

  atomic_llong vsync_baton;

  struct fwr_renderer fwr_renderer;
};