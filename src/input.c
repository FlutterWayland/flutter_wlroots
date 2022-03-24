#include "flutter_embedder.h"
#include <stdint.h>
#include <stdlib.h>
#include <linux/input-event-codes.h>

#define WLR_USE_UNSTABLE
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_touch.h>
#include <wlr/backend.h>
#include <wlr/util/log.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xdg_shell.h>

#include "input.h"
#include "shaders.h"
#include "instance.h"
#include "standard_message_codec.h"
#include "constants.h"
#include "handle_map.h"
#include "messages.h"

#define NS_PER_MS 1000000

static uint32_t uapi_mouse_button_to_flutter(uint32_t uapi_button) {
  switch (uapi_button) {
    case BTN_LEFT: return 1;
    case BTN_RIGHT: return 2;
    case BTN_MIDDLE: return 3;
    case BTN_BACK: return 4;
    case BTN_FORWARD: return 5;
    case BTN_SIDE: return 6;
    case BTN_EXTRA: return 7;
    case BTN_0: return 8;
    case BTN_1: return 9;
    case BTN_2: return 10;
    case BTN_3: return 11;
    case BTN_4: return 12;
    case BTN_5: return 13;
    case BTN_6: return 14;
    case BTN_7: return 15;
    case BTN_8: return 16;
    case BTN_9: return 17;
    default: return 0;
  }
}

static uint32_t flutter_mouse_button_to_uapi(uint32_t flutter_button) {
  switch (flutter_button) {
    case 1: return BTN_LEFT;
    case 2: return BTN_RIGHT;
    case 3: return BTN_MIDDLE;
    case 4: return BTN_BACK;
    case 5: return BTN_FORWARD;
    case 6: return BTN_SIDE;
    case 7: return BTN_EXTRA;
    case 8: return BTN_0;
    case 9: return BTN_1;
    case 10: return BTN_2;
    case 11: return BTN_3;
    case 12: return BTN_4;
    case 13: return BTN_5;
    case 14: return BTN_6;
    case 15: return BTN_7;
    case 16: return BTN_8;
    case 17: return BTN_9;
    default: return 0;
  }
}

static void process_cursor_motion(struct fwr_instance *instance, uint32_t time) {
  wlr_xcursor_manager_set_cursor_image(instance->cursor_mgr, "left_ptr",
                                       instance->cursor);
}

static void on_server_cursor_motion(struct wl_listener *listener, void *data) {
  struct fwr_instance *instance = wl_container_of(listener, instance, cursor_motion);
  struct wlr_event_pointer_motion *event = data;

  wlr_cursor_move(instance->cursor, event->device, event->delta_x,
                  event->delta_y);
  process_cursor_motion(instance, event->time_msec);
}

static void on_server_cursor_motion_absolute(struct wl_listener *listener, void *data) {
  struct fwr_instance *instance = wl_container_of(listener, instance, cursor_motion_absolute);
  struct wlr_event_pointer_motion_absolute *event = data;

  wlr_cursor_warp_absolute(instance->cursor, event->device, event->x, event->y);
  process_cursor_motion(instance, event->time_msec);
}

static void on_server_cursor_button(struct wl_listener *listener, void *data) {
  struct fwr_instance *instance = wl_container_of(listener, instance, cursor_button);
  struct wlr_event_pointer_button *event = data;

  uint32_t fl_button = uapi_mouse_button_to_flutter(event->button);
  if (fl_button != 0) {
    uint32_t mask = 1 << (fl_button - 1);
    if (event->state == WLR_BUTTON_PRESSED) {
      instance->input.acc_mouse_button_mask |= mask;
    } else if (event->state == WLR_BUTTON_RELEASED) {
      instance->input.acc_mouse_button_mask &= ~mask;
    }
  }
}

static void on_server_cursor_axis(struct wl_listener *listener, void *data) {
  struct fwr_instance *instance = wl_container_of(listener, instance, cursor_axis);
  struct wlr_event_pointer_axis *event = data;

  wlr_xcursor_manager_set_cursor_image(instance->cursor_mgr, "left_ptr",
                                       instance->cursor);

  if (event->orientation == WLR_AXIS_ORIENTATION_HORIZONTAL) {
    instance->input.acc_scroll_delta_x += event->delta;
  }
  if (event->orientation == WLR_AXIS_ORIENTATION_VERTICAL) {
    instance->input.acc_scroll_delta_x += event->delta;
  }
}

static void on_server_cursor_frame(struct wl_listener *listener, void *data) {
  struct fwr_instance *instance = wl_container_of(listener, instance, cursor_frame);

  uint32_t last_mask = instance->input.mouse_button_mask;
  uint32_t curr_mask = instance->input.acc_mouse_button_mask;

  FlutterPointerEvent pointer_event = {};
  pointer_event.struct_size = sizeof(FlutterPointerEvent);
  if (last_mask == 0 && curr_mask != 0) {
    pointer_event.phase = kDown;
  } else if (last_mask != 0 && curr_mask == 0) {
    pointer_event.phase = kUp;
  } else if (curr_mask == 0) {
    pointer_event.phase = kHover;
  } else {
    pointer_event.phase = kMove;
  }
  pointer_event.x = instance->cursor->x;
  pointer_event.y = instance->cursor->y;
  pointer_event.device = 0;
  pointer_event.signal_kind = kFlutterPointerSignalKindNone;
  pointer_event.scroll_delta_x = instance->input.acc_scroll_delta_x;
  pointer_event.scroll_delta_y = instance->input.acc_scroll_delta_y;
  pointer_event.device_kind = kFlutterPointerDeviceKindMouse;
  pointer_event.buttons = curr_mask;
  // TODO this is not 100% right as we should return the timestamp from libinput.
  // On my machine these seem to be using the same source but differnt unit, is this a guarantee?
  pointer_event.timestamp = instance->fl_proc_table.GetCurrentTime();
  instance->fl_proc_table.SendPointerEvent(instance->engine, &pointer_event, 1);

  instance->input.mouse_button_mask = curr_mask;
  instance->input.acc_scroll_delta_x = 0.0;
  instance->input.acc_scroll_delta_y = 0.0;
}

static void on_server_cursor_touch_down(struct wl_listener *listener, void *data) {
  struct fwr_instance *instance = wl_container_of(listener, instance, cursor_touch_down);
  struct wlr_event_touch_down *event = data;
  struct fwr_input_device_state *state = event->device->data;

  if (event->touch_id >= 10) return;
  state->touch_points[event->touch_id].x = event->x;
  state->touch_points[event->touch_id].y = event->y;

  double screen_width = 1.0;
  double screen_height = 1.0;
  if (instance->output != NULL) {
    screen_width = instance->output->wlr_output->width;
    screen_height = instance->output->wlr_output->height;
  }

  wlr_log(WLR_INFO, "touch down %f %f", event->x, event->y);

  FlutterPointerEvent pointer_event = {};
  pointer_event.struct_size = sizeof(FlutterPointerEvent);
  pointer_event.device_kind = kFlutterPointerDeviceKindTouch;
  pointer_event.signal_kind = kFlutterPointerSignalKindNone;
  pointer_event.device = event->touch_id;
  pointer_event.phase = kAdd;
  pointer_event.x = event->x * screen_width;
  pointer_event.y = event->y * screen_height;
  pointer_event.scroll_delta_x = 0;
  pointer_event.scroll_delta_y = 0;
  // TODO this is not 100% right as we should return the timestamp from libinput.
  // On my machine these seem to be using the same source but differnt unit, is this a guarantee?
  pointer_event.timestamp = instance->fl_proc_table.GetCurrentTime();
  instance->fl_proc_table.SendPointerEvent(instance->engine, &pointer_event, 1);

  pointer_event.phase = kDown;
  instance->fl_proc_table.SendPointerEvent(instance->engine, &pointer_event, 1);
}

static void on_server_cursor_touch_up(struct wl_listener *listener, void *data) {
  struct fwr_instance *instance = wl_container_of(listener, instance, cursor_touch_up);
  struct wlr_event_touch_up *event = data;
  struct fwr_input_device_state *state = event->device->data;

  if (event->touch_id >= 10) return;

  double screen_width = 1.0;
  double screen_height = 1.0;
  if (instance->output != NULL) {
    screen_width = instance->output->wlr_output->width;
    screen_height = instance->output->wlr_output->height;
  }

  FlutterPointerEvent pointer_event = {};
  pointer_event.struct_size = sizeof(FlutterPointerEvent);
  pointer_event.device_kind = kFlutterPointerDeviceKindTouch;
  pointer_event.signal_kind = kFlutterPointerSignalKindNone;
  pointer_event.device = event->touch_id;
  pointer_event.phase = kUp;
  pointer_event.x = state->touch_points[event->touch_id].x * screen_width;
  pointer_event.y = state->touch_points[event->touch_id].y * screen_height;
  pointer_event.scroll_delta_x = 0;
  pointer_event.scroll_delta_y = 0;
  // TODO this is not 100% right as we should return the timestamp from libinput.
  // On my machine these seem to be using the same source but differnt unit, is this a guarantee?
  pointer_event.timestamp = instance->fl_proc_table.GetCurrentTime();
  instance->fl_proc_table.SendPointerEvent(instance->engine, &pointer_event, 1);

  pointer_event.phase = kRemove;
  instance->fl_proc_table.SendPointerEvent(instance->engine, &pointer_event, 1);
}

static void on_server_cursor_touch_motion(struct wl_listener *listener, void *data) {
  struct fwr_instance *instance = wl_container_of(listener, instance, cursor_touch_motion);
  struct wlr_event_touch_motion *event = data;
  struct fwr_input_device_state *state = event->device->data;

  if (event->touch_id >= 10) return;
  state->touch_points[event->touch_id].x = event->x;
  state->touch_points[event->touch_id].y = event->y;

  double screen_width = 1.0;
  double screen_height = 1.0;
  if (instance->output != NULL) {
    screen_width = instance->output->wlr_output->width;
    screen_height = instance->output->wlr_output->height;
  }

  FlutterPointerEvent pointer_event = {};
  pointer_event.struct_size = sizeof(FlutterPointerEvent);
  pointer_event.device_kind = kFlutterPointerDeviceKindTouch;
  pointer_event.signal_kind = kFlutterPointerSignalKindNone;
  pointer_event.device = event->touch_id;
  pointer_event.phase = kMove;
  pointer_event.x = event->x * screen_width;
  pointer_event.y = event->y * screen_height;
  pointer_event.scroll_delta_x = 0;
  pointer_event.scroll_delta_y = 0;
  // TODO this is not 100% right as we should return the timestamp from libinput.
  // On my machine these seem to be using the same source but differnt unit, is this a guarantee?
  pointer_event.timestamp = instance->fl_proc_table.GetCurrentTime();
  instance->fl_proc_table.SendPointerEvent(instance->engine, &pointer_event, 1);
}

static void on_server_cursor_touch_frame(struct wl_listener *listener, void *data) {
  struct fwr_instance *instance = wl_container_of(listener, instance, cursor_touch_frame);
}

static void server_new_keyboard(struct fwr_instance *instance,
		struct wlr_input_device *device) {
}

static void server_new_pointer(struct fwr_instance *instance,
		struct wlr_input_device *device) {
  wlr_cursor_attach_input_device(instance->cursor, device);
}

static void server_new_touch(struct fwr_instance *instance,
		struct wlr_input_device *device) {
  wlr_cursor_attach_input_device(instance->cursor, device);
}

static void on_server_new_input(struct wl_listener *listener, void *data) {
  struct fwr_instance *instance = wl_container_of(listener, instance, new_input);
  struct wlr_input_device *device = data;

  struct fwr_input_device_state *state = calloc(1, sizeof(struct fwr_input_device_state));
  device->data = state;

  switch (device->type) {
	case WLR_INPUT_DEVICE_KEYBOARD:
		server_new_keyboard(instance, device);
		break;
	case WLR_INPUT_DEVICE_POINTER:
		server_new_pointer(instance, device);
		break;
  case WLR_INPUT_DEVICE_TOUCH:
    server_new_touch(instance, device);
    break;
	default:
		break;
	}

  // TODO seat caps
  uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
  //if (!wl_list_empty(&server->keyboards)) {
  //  caps |= WL_SEAT_CAPABILITY_KEYBOARD;
  //}
  wlr_seat_set_capabilities(instance->seat, caps);
}

void fwr_input_init(struct fwr_instance *instance) {
  instance->input.mouse_button_mask = 0;
  instance->input.fl_mouse_button_mask = 0;

  instance->cursor = wlr_cursor_create();
	wlr_cursor_attach_output_layout(instance->cursor, instance->output_layout);

  instance->cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
	wlr_xcursor_manager_load(instance->cursor_mgr, 1);

	instance->seat = wlr_seat_create(instance->wl_display, "seat0");

  instance->cursor_motion.notify = on_server_cursor_motion;
  wl_signal_add(&instance->cursor->events.motion, &instance->cursor_motion);
  instance->cursor_motion_absolute.notify = on_server_cursor_motion_absolute;
  wl_signal_add(&instance->cursor->events.motion_absolute, &instance->cursor_motion_absolute);
  instance->cursor_button.notify = on_server_cursor_button;
  wl_signal_add(&instance->cursor->events.button, &instance->cursor_button);
  instance->cursor_axis.notify = on_server_cursor_axis;
  wl_signal_add(&instance->cursor->events.axis, &instance->cursor_axis);
  instance->cursor_frame.notify = on_server_cursor_frame;
  wl_signal_add(&instance->cursor->events.frame, &instance->cursor_frame);
  instance->cursor_touch_down.notify = on_server_cursor_touch_down;
  wl_signal_add(&instance->cursor->events.touch_down, &instance->cursor_touch_down);
  instance->cursor_touch_up.notify = on_server_cursor_touch_up;
  wl_signal_add(&instance->cursor->events.touch_up, &instance->cursor_touch_up);
  instance->cursor_touch_motion.notify = on_server_cursor_touch_motion;
  wl_signal_add(&instance->cursor->events.touch_motion, &instance->cursor_touch_motion);
  instance->cursor_touch_frame.notify = on_server_cursor_touch_frame;
  wl_signal_add(&instance->cursor->events.touch_frame, &instance->cursor_touch_frame);

  instance->new_input.notify = on_server_new_input;
	wl_signal_add(&instance->backend->events.new_input, &instance->new_input);
}

void fwr_handle_surface_pointer_event_message(struct fwr_instance *instance, const FlutterPlatformMessageResponseHandle *handle, struct dart_value *args) {
  struct surface_pointer_event_message message;
  if (!decode_surface_pointer_event_message(args, &message)) {
    goto error;
  }

  struct fwr_view *view;
  if (!handle_map_get(instance->views, message.surface_handle, (void**) &view)) {
    // This implies a race condition of surface removal.
    // We return success here.
    goto success;
  }

  //wlr_log(WLR_INFO, "yay pointer event %d %ld", message.event_type, message.buttons);

  struct wlr_surface_state *surface_state = &view->surface->surface->current;

  double transformed_local_pos_x = message.local_pos_x / message.widget_size_x * surface_state->width;
  double transformed_local_pos_y = message.local_pos_y / message.widget_size_y * surface_state->height;

  switch (message.device_kind) {
    case pointerKindMouse: {
      wlr_seat_pointer_notify_enter(instance->seat, view->surface->surface, transformed_local_pos_x, transformed_local_pos_y);
      wlr_seat_pointer_notify_motion(instance->seat, message.timestamp / NS_PER_MS, transformed_local_pos_x, transformed_local_pos_y);

      for (int n = 0; n < 32; n++) {
        bool last = ((instance->input.fl_mouse_button_mask >> n) & 0b1) != 0;
        bool curr = ((message.buttons >> n) & 0b1) != 0;

        uint32_t uapi_button = flutter_mouse_button_to_uapi(n);

        if (!last & curr) {
          wlr_seat_pointer_notify_button(instance->seat,
                                        message.timestamp / NS_PER_MS, uapi_button,
                                        WLR_BUTTON_PRESSED);
        } else if (last & !curr) {
          wlr_seat_pointer_notify_button(instance->seat,
                                        message.timestamp / NS_PER_MS, uapi_button,
                                        WLR_BUTTON_RELEASED);
        }
      }
      instance->input.fl_mouse_button_mask = message.buttons;

      if (message.scroll_delta_x != 0) {
        // TODO: Pass this through from the original event instead.
        // Dependent on getting https://github.com/flutter/flutter/issues/100680 merged.
        int32_t discrete_scroll_amount = message.scroll_delta_x >= 0 ? 1 : -1;

        wlr_seat_pointer_notify_axis(
            instance->seat, message.timestamp / NS_PER_MS,
            WLR_AXIS_ORIENTATION_HORIZONTAL, message.scroll_delta_x,
            discrete_scroll_amount, WLR_AXIS_SOURCE_WHEEL);
      }

      if (message.scroll_delta_y != 0) {
        // TODO: Pass this through from the original event instead.
        // Dependent on getting https://github.com/flutter/flutter/issues/100680 merged.
        int32_t discrete_scroll_amount = message.scroll_delta_y >= 0 ? 1 : -1;

        wlr_seat_pointer_notify_axis(
            instance->seat, message.timestamp / NS_PER_MS,
            WLR_AXIS_ORIENTATION_VERTICAL, message.scroll_delta_y,
            discrete_scroll_amount, WLR_AXIS_SOURCE_WHEEL);
      }

      if(message.event_type == pointerExitEvent) {
        wlr_seat_pointer_clear_focus(instance->seat);
      }

      wlr_seat_pointer_notify_frame(instance->seat);

      break;
    }
  }


success:
  instance->fl_proc_table.SendPlatformMessageResponse(instance->engine, handle, method_call_null_success, sizeof(method_call_null_success));
  return;

error:
  wlr_log(WLR_ERROR, "Invalid surface pointer event message");
  // Send failure
  instance->fl_proc_table.SendPlatformMessageResponse(instance->engine, handle, NULL, 0);
}
