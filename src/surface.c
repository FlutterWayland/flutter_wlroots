#include <assert.h>
#include <stdlib.h>

#include <wayland-server-core.h>
#include <wayland-util.h>

#define WLR_USE_UNSTABLE
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>

#include "instance.h"
#include "messages.h"

static void cb(const uint8_t *data, size_t size, void *user_data) {
  wlr_log(WLR_INFO, "callback");
}

static void focus_view(struct fwr_view *view, struct wlr_surface *surface) {
	/* Note: this function only deals with keyboard focus. */
	if (view == NULL) {
		return;
	}
	struct fwr_instance *instance = view->instance;
  instance->current_focused_view = view->handle;
	struct wlr_seat *seat = instance->seat;
	struct wlr_surface *prev_surface = seat->keyboard_state.focused_surface;
	if (prev_surface == surface) {
		/* Don't re-focus an already focused surface. */
		return;
	}
  // TODO: we should manage changing focus on dart side
	// if (prev_surface) {
	// 	/*
	// 	 * Deactivate the previously focused surface. This lets the client know
	// 	 * it no longer has focus and the client will repaint accordingly, e.g.
	// 	 * stop displaying a caret.
	// 	 */
	// 	struct wlr_xdg_surface *previous = wlr_xdg_surface_from_wlr_surface(
	// 				seat->keyboard_state.focused_surface);
	// 	assert(previous->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL);
	// 	wlr_xdg_toplevel_set_activated(previous->toplevel, false);
	// }
	struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(seat);
  	/* Activate the new surface */
	if(view->surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
    wlr_xdg_toplevel_set_activated(view->surface, true);
  }
	/*
	 * Tell the seat to have the keyboard enter this surface. wlroots will keep
	 * track of this and automatically send key events to the appropriate
	 * clients without additional work on your part.
	 */
	wlr_seat_keyboard_notify_enter(seat, view->surface->surface,
		keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
}

static void xdg_toplevel_map(struct wl_listener *listener, void *data) {
  struct fwr_view *view = wl_container_of(listener, view, map);
  struct fwr_instance *instance = view->instance;

  int32_t pid;
  uint32_t uid, gid;
  wl_client_get_credentials(view->surface->client->client, &pid, &uid, &gid);

  struct message_builder msg = message_builder_new();
  struct message_builder_segment msg_seg = message_builder_segment(&msg);
  message_builder_segment_push_string(&msg_seg, "surface_map");
  message_builder_segment_finish(&msg_seg);


  wlr_xdg_surface_get_geometry(view->surface, &view->geometry);

  // struct wlr_box geometry = view->surface->current.geometry;
  
  struct wlr_box geometry = view->geometry;
  struct wlr_xdg_toplevel_state toplevel_state = view->xdg_toplevel->current;

  int surface_width = view->surface->surface->current.width;
  int surface_height = view->surface->surface->current.height;

  msg_seg = message_builder_segment(&msg);
  struct message_builder_segment arg_seg =
      message_builder_segment_push_map(&msg_seg, 14);
  message_builder_segment_push_string(&arg_seg, "handle");
  wlr_log(WLR_INFO, "viewhandle %d", view->handle);
  message_builder_segment_push_int64(&arg_seg, view->handle);
  message_builder_segment_push_string(&arg_seg, "client_pid");
  message_builder_segment_push_int64(&arg_seg, pid);
  message_builder_segment_push_string(&arg_seg, "client_uid");
  message_builder_segment_push_int64(&arg_seg, uid);
  message_builder_segment_push_string(&arg_seg, "client_gid");
  message_builder_segment_push_int64(&arg_seg, gid);

  message_builder_segment_push_string(&arg_seg, "is_popup");
  message_builder_segment_push_bool(&arg_seg, view->is_popup);
  
  message_builder_segment_push_string(&arg_seg, "parent_handle");
  message_builder_segment_push_int64(&arg_seg, view->parent_handle);

  // if(view->is_popup){
  //   message_builder_segment_push_string(&arg_seg, "preffered_width");
  //   message_builder_segment_push_int64(&arg_seg, toplevel_state.width);

  //   message_builder_segment_push_string(&arg_seg, "preffered_height");
  //   message_builder_segment_push_int64(&arg_seg, toplevel_state.height);
  // } else {
  
  // this is geometry so content width
  // need to add 2 more parameters - toplevel_state.width and height as whole buffer
  message_builder_segment_push_string(&arg_seg, "surface_width");
  message_builder_segment_push_int64(&arg_seg, surface_width);

  message_builder_segment_push_string(&arg_seg, "surface_height");
  message_builder_segment_push_int64(&arg_seg, surface_height);


  message_builder_segment_push_string(&arg_seg, "offset_left");
  message_builder_segment_push_int64(&arg_seg, geometry.x);

  message_builder_segment_push_string(&arg_seg, "offset_top");
  message_builder_segment_push_int64(&arg_seg, geometry.y);

  message_builder_segment_push_string(&arg_seg, "offset_right");
  message_builder_segment_push_int64(&arg_seg, surface_width - (geometry.width + geometry.x));

  message_builder_segment_push_string(&arg_seg, "offset_bottom");
  message_builder_segment_push_int64(&arg_seg, surface_height - (geometry.height + geometry.y));

  message_builder_segment_push_string(&arg_seg, "content_width");
  message_builder_segment_push_int64(&arg_seg, geometry.width);

  message_builder_segment_push_string(&arg_seg, "content_height");
  message_builder_segment_push_int64(&arg_seg, geometry.height);
  // }

  message_builder_segment_finish(&arg_seg);
  
  message_builder_segment_finish(&msg_seg);
  uint8_t *msg_buf;
  size_t msg_buf_len;
  message_builder_finish(&msg, &msg_buf, &msg_buf_len);

  FlutterPlatformMessageResponseHandle *response_handle;
  instance->fl_proc_table.PlatformMessageCreateResponseHandle(
      instance->engine, cb, NULL, &response_handle);

  FlutterPlatformMessage platform_message = {};
  platform_message.struct_size = sizeof(FlutterPlatformMessage);
  platform_message.channel = "wlroots";
  platform_message.message = msg_buf;
  platform_message.message_size = msg_buf_len;
  platform_message.response_handle = response_handle;
  instance->fl_proc_table.SendPlatformMessage(instance->engine,
                                              &platform_message);

  free(msg_buf);

  instance->fl_proc_table.PlatformMessageReleaseResponseHandle(instance->engine,
                                                               response_handle);

  focus_view(view, view->surface);
}

static void xdg_toplevel_unmap(struct wl_listener *listener, void *data) {
  struct fwr_view *view = wl_container_of(listener, view, unmap);
  struct fwr_instance *instance = view->instance;

  struct message_builder msg = message_builder_new();
  struct message_builder_segment msg_seg = message_builder_segment(&msg);
  message_builder_segment_push_string(&msg_seg, "surface_unmap");
  message_builder_segment_finish(&msg_seg);

  msg_seg = message_builder_segment(&msg);
  struct message_builder_segment arg_seg =
      message_builder_segment_push_map(&msg_seg, 1);
  message_builder_segment_push_string(&arg_seg, "handle");
  message_builder_segment_push_int64(&arg_seg, view->handle);
  message_builder_segment_finish(&arg_seg);

  message_builder_segment_finish(&msg_seg);
  uint8_t *msg_buf;
  size_t msg_buf_len;
  message_builder_finish(&msg, &msg_buf, &msg_buf_len);

  FlutterPlatformMessageResponseHandle *response_handle;
  instance->fl_proc_table.PlatformMessageCreateResponseHandle(
      instance->engine, cb, NULL, &response_handle);

  FlutterPlatformMessage platform_message = {};
  platform_message.struct_size = sizeof(FlutterPlatformMessage);
  platform_message.channel = "wlroots";
  platform_message.message = msg_buf;
  platform_message.message_size = msg_buf_len;
  platform_message.response_handle = response_handle;
  instance->fl_proc_table.SendPlatformMessage(instance->engine,
                                              &platform_message);

  free(msg_buf);

  handle_map_remove(instance->views, view->handle);
}

static void xdg_toplevel_destroy(struct wl_listener *listener, void *data) {}


static void send_window_event(struct fwr_instance *instance, uint32_t handle, const char *event) {

  struct message_builder msg = message_builder_new();
  struct message_builder_segment msg_seg = message_builder_segment(&msg);
  message_builder_segment_push_string(&msg_seg, event);
  message_builder_segment_finish(&msg_seg);

  msg_seg = message_builder_segment(&msg);
  struct message_builder_segment arg_seg =
      message_builder_segment_push_map(&msg_seg, 1);
  message_builder_segment_push_string(&arg_seg, "handle");
  message_builder_segment_push_int64(&arg_seg, handle);
  message_builder_segment_finish(&arg_seg);

  message_builder_segment_finish(&msg_seg);
  uint8_t *msg_buf;
  size_t msg_buf_len;
  message_builder_finish(&msg, &msg_buf, &msg_buf_len);

  FlutterPlatformMessageResponseHandle *response_handle;
  instance->fl_proc_table.PlatformMessageCreateResponseHandle(
      instance->engine, cb, NULL, &response_handle);

  FlutterPlatformMessage platform_message = {};
  platform_message.struct_size = sizeof(FlutterPlatformMessage);
  platform_message.channel = "wlroots";
  platform_message.message = msg_buf;
  platform_message.message_size = msg_buf_len;
  platform_message.response_handle = response_handle;
  instance->fl_proc_table.SendPlatformMessage(instance->engine,
                                              &platform_message);


  free(msg_buf);

}

static void xdg_toplevel_request_move(
		struct wl_listener *listener, void *data) {
	/* This event is raised when a client would like to begin an interactive
	 * move, typically because the user clicked on their client-side
	 * decorations. Note that a more sophisticated compositor should check the
	 * provided serial against a list of button press serials sent to this
	 * client, to prevent the client from requesting this whenever they want. */
	struct fwr_view *view = wl_container_of(listener, view, request_move);
  // should change cursor in wlroots
	// begin_interactive(view, FWR_CURSOR_MOVE, 0);
  struct fwr_instance *instance = view->instance;
  send_window_event(instance, view->handle, "window_move");
}

static void xdg_toplevel_request_resize(
		struct wl_listener *listener, void *data) {
	/* This event is raised when a client would like to begin an interactive
	 * resize, typically because the user clicked on their client-side
	 * decorations. Note that a more sophisticated compositor should check the
	 * provided serial against a list of button press serials sent to this
	 * client, to prevent the client from requesting this whenever they want. */
	struct wlr_xdg_toplevel_resize_event *event = data;
	struct fwr_view *view = wl_container_of(listener, view, request_resize);
  wlr_log(WLR_INFO, "request resize for view %d", view->handle);
	// begin_interactive(view, FWR_CURSOR_RESIZE, event->edges);
  struct fwr_instance *instance = view->instance;
  send_window_event(instance, view->handle, "window_resize");
}

static void xdg_toplevel_request_maximize(
		struct wl_listener *listener, void *data) {
	/* This event is raised when a client would like to maximize itself,
	 * typically because the user clicked on the maximize button on
	 * client-side decorations. tinywl doesn't support maximization, but
	 * to conform to xdg-shell protocol we still must send a configure.
	 * wlr_xdg_surface_schedule_configure() is used to send an empty reply. */
	struct fwr_view *view =
		wl_container_of(listener, view, request_maximize);
	wlr_xdg_surface_schedule_configure(view->xdg_toplevel->base);
  struct fwr_instance *instance = view->instance;

  send_window_event(instance, view->handle, "window_maximize");

}

static void xdg_toplevel_request_fullscreen(
		struct wl_listener *listener, void *data) {

	/* Just as with request_maximize, we must send a configure here. */
	struct fwr_view *view =
		wl_container_of(listener, view, request_fullscreen);
	wlr_xdg_surface_schedule_configure(view->xdg_toplevel->base);
  struct fwr_instance *instance = view->instance;
  send_window_event(instance, view->handle, "window_fullscreen");
}

static void xdg_toplevel_request_minimize(
		struct wl_listener *listener, void *data) {

	/* Just as with request_minimize, we must send a configure here. */
	struct fwr_view *view =
		wl_container_of(listener, view, request_minimize);
	wlr_xdg_surface_schedule_configure(view->xdg_toplevel->base);
  struct fwr_instance *instance = view->instance;

  send_window_event(instance, view->handle, "window_minimize");
}


void fwr_new_xdg_surface(struct wl_listener *listener, void *data) {
  struct fwr_instance *instance =
      wl_container_of(listener, instance, new_xdg_surface);
  struct wlr_xdg_surface *xdg_surface = data;
  struct fwr_view *view = calloc(1, sizeof(struct fwr_view));
  xdg_surface->data = view;
  view->xdg_toplevel = xdg_surface->toplevel;

  struct wlr_xdg_surface *parent = 0;
  uint32_t parent_handle = -1;

  if (xdg_surface->role == WLR_XDG_SURFACE_ROLE_POPUP) {
    parent = wlr_xdg_surface_from_wlr_surface(xdg_surface->popup->parent);
    struct fwr_view *parent_view = parent->data;
    view->is_popup = true;
    view->parent_handle = parent_view->handle;
  } else {
    view->is_popup = false;
    view->parent_handle = -1;
  }

  view->instance = instance;
  view->surface = xdg_surface;

  view->map.notify = xdg_toplevel_map;
  wl_signal_add(&xdg_surface->events.map, &view->map);
  view->unmap.notify = xdg_toplevel_unmap;
  wl_signal_add(&xdg_surface->events.unmap, &view->unmap);
  view->destroy.notify = xdg_toplevel_destroy;
  wl_signal_add(&xdg_surface->events.destroy, &view->destroy);


  // only set window events if not pop-up, otherwise, exception for qt
  if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_POPUP) {

    struct wlr_xdg_toplevel *toplevel = xdg_surface->toplevel;
    view->request_move.notify = xdg_toplevel_request_move;
    wl_signal_add(&toplevel->events.request_move, &view->request_move);

    view->request_resize.notify = xdg_toplevel_request_resize;
    wl_signal_add(&toplevel->events.request_resize, &view->request_resize);

    view->request_maximize.notify = xdg_toplevel_request_maximize;
    wl_signal_add(&toplevel->events.request_maximize,
      &view->request_maximize);
    view->request_fullscreen.notify = xdg_toplevel_request_fullscreen;
    wl_signal_add(&toplevel->events.request_fullscreen,
      &view->request_fullscreen);

    view->request_minimize.notify = xdg_toplevel_request_minimize;
    wl_signal_add(&toplevel->events.request_minimize,
      &view->request_minimize);

  }

  uint32_t view_handle = handle_map_add(instance->views, (void *)view);

  view->handle = view_handle;
}

void fwr_handle_surface_toplevel_set_size(
    struct fwr_instance *instance,
    const FlutterPlatformMessageResponseHandle *handle,
    struct dart_value *args) {
  struct surface_toplevel_set_size_message message;
  if (!decode_surface_toplevel_set_size_message(args, &message)) {
    goto error;
  }

  struct fwr_view *view;
  if (!handle_map_get(instance->views, message.surface_handle, (void**) &view)) {
    // This implies a race condition of surface removal.
    // We return success here.
    goto success;
  }

  if (view->surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
    wlr_xdg_toplevel_set_size(view->surface, message.size_x, message.size_y);
  }

success:
  instance->fl_proc_table.SendPlatformMessageResponse(instance->engine, handle, method_call_null_success, sizeof(method_call_null_success));
  return;

error:
  wlr_log(WLR_ERROR, "Invalid toplevel set size message");
  // Send failure
  instance->fl_proc_table.SendPlatformMessageResponse(instance->engine, handle, NULL, 0);
}

void fwr_handle_surface_focus(
    struct fwr_instance *instance,
    const FlutterPlatformMessageResponseHandle *handle,
    struct dart_value *args
){

  struct surface_handle_focus_message message;

   if (!decode_surface_handle_focus_message(args, &message)) {
    goto error;
  }

  struct fwr_view *view;
  if (!handle_map_get(instance->views, message.surface_handle, (void**) &view)) {
    // This implies a race condition of surface removal.
    // We return success here.
    goto success;
  }

  focus_view(view, view->surface->surface);

success:
  instance->fl_proc_table.SendPlatformMessageResponse(instance->engine, handle, method_call_null_success, sizeof(method_call_null_success));
  return;
  
error:
  wlr_log(WLR_ERROR, "Invalid toplevel set size message");
  // Send failure
  instance->fl_proc_table.SendPlatformMessageResponse(instance->engine, handle, NULL, 0);

}