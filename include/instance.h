#pragma once

#include "flutter_embedder.h"
#include "xdg-shell-protocol.h"

#include <stdatomic.h>

#include <GLES2/gl2.h>
#include <wayland-server-core.h>

#include "shaders.h"
#include "renderer.h"
#include "input.h"
#include "handle_map.h"

struct fwr_instance {
  struct wl_display *wl_display;
  struct wl_event_loop *wl_event_loop;
  struct wlr_backend *backend;
  struct wlr_egl *egl;
  struct wlr_renderer *renderer;
  struct wlr_allocator *allocator;
  struct wlr_presentation *presentation;

  struct wlr_xdg_shell *xdg_shell;
  struct wl_listener new_xdg_surface;

  // Map of `uint32_t handle` => `struct fwr_view view`
  struct handle_map *views;

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
  struct wl_listener request_cursor;
  struct fwr_input_state input;

  struct wl_listener new_input;

  struct wlr_seat *seat;

  struct wlr_output_layout *output_layout;
  // If null, we don't have an output.
  struct fwr_output *output;
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

  atomic_intptr_t vsync_baton;

  struct fwr_renderer fwr_renderer;
};

struct fwr_output {
	struct wl_list link;

  struct wlr_output *wlr_output;
  struct fwr_instance *instance;

  struct wl_listener frame;
  struct wl_listener mode;
  struct wl_listener present;
};

struct fwr_view {
  struct wl_list link;
  uint32_t handle;

  struct fwr_instance *instance;
  struct wlr_xdg_surface *surface;

  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener destroy;
  struct wl_listener commit;
};