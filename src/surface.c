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

  struct wlr_box geometry = view->surface->current.geometry;

  msg_seg = message_builder_segment(&msg);
  struct message_builder_segment arg_seg =
      message_builder_segment_push_map(&msg_seg, 8);
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

  message_builder_segment_push_string(&arg_seg, "preffered_width");
  message_builder_segment_push_int64(&arg_seg, geometry.width);

  message_builder_segment_push_string(&arg_seg, "preffered_height");
  message_builder_segment_push_int64(&arg_seg, geometry.height);

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

void fwr_new_xdg_surface(struct wl_listener *listener, void *data) {
  struct fwr_instance *instance =
      wl_container_of(listener, instance, new_xdg_surface);
  struct wlr_xdg_surface *xdg_surface = data;
  struct fwr_view *view = calloc(1, sizeof(struct fwr_view));
  xdg_surface->data = view;

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