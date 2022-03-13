#include "flutter_embedder.h"

#define WLR_USE_UNSTABLE
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_pointer.h>

#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>

#include <wlr/backend.h>
#include <wlr/util/log.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <xkbcommon/xkbcommon.h>
#include <stdlib.h>


#include "input.h"
#include "shaders.h"
#include "instance.h"
#include "standard_message_codec.h"
#include "constants.h"
#include "handle_map.h"
#include "messages.h"
#include "platform_channel.h"
#include "key_mapping.h"

#define NS_PER_MS 1000000

static uint32_t uapi_mouse_button_to_flutter(uint32_t uapi_button) {
  switch (uapi_button) {
    case 0x110: return 1; // BTN_LEFT
    case 0x111: return 2; // BTN_RIGHT
    case 0x112: return 3; // BTN_MIDDLE
    case 0x116: return 4; // BTN_BACK
    case 0x115: return 5; // BTN_FORWARD
    case 0x113: return 6; // BTN_SIDE
    case 0x114: return 7; // BTN_EXTRA
    case 0x100: return 8; // BTN_0
    case 0x101: return 9; // BTN_1
    case 0x102: return 10; // BTN_2
    case 0x103: return 11; // BTN_3
    case 0x104: return 12; // BTN_4
    case 0x105: return 13; // BTN_5
    case 0x106: return 14; // BTN_6
    case 0x107: return 15; // BTN_7
    case 0x108: return 16; // BTN_8
    case 0x109: return 17; // BTN_9
    default: return 0;
  }
}

static void process_cursor_motion(struct fwr_instance *instance, uint32_t time) {
  wlr_xcursor_manager_set_cursor_image(instance->cursor_mgr, "left_ptr",
                                       instance->cursor);
  //wlr_log(WLR_INFO, "%ld %d", instance->fl_proc_table.GetCurrentTime(), time);

  FlutterPointerEvent pointer_event = {};
  pointer_event.struct_size = sizeof(FlutterPointerEvent);
  if (instance->input.mouse_down) {
    pointer_event.phase = kMove;
  } else {
    pointer_event.phase = kHover;
  }
  pointer_event.x = instance->cursor->x;
  pointer_event.y = instance->cursor->y;
  pointer_event.device = 0;
  pointer_event.signal_kind = kFlutterPointerSignalKindNone;
  pointer_event.scroll_delta_x = 0;
  pointer_event.scroll_delta_y = 0;
  pointer_event.device_kind = kFlutterPointerDeviceKindMouse;
  pointer_event.buttons = instance->input.mouse_button_mask;
  // TODO this is not 100% right as we should return the timestamp from libinput.
  // On my machine these seem to be using the same source but differnt unit, is this a guarantee?
  pointer_event.timestamp = instance->fl_proc_table.GetCurrentTime();
  instance->fl_proc_table.SendPointerEvent(instance->engine, &pointer_event, 1);
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

  uint32_t last_mask = instance->input.mouse_button_mask;

  uint32_t fl_button = uapi_mouse_button_to_flutter(event->button);
  if (fl_button != 0) {
    uint32_t mask = 1 >> (fl_button - 1);
    if (event->state == WLR_BUTTON_PRESSED) {
      instance->input.mouse_button_mask |= mask;
    } else if (event->state == WLR_BUTTON_RELEASED) {
      instance->input.mouse_button_mask &= ~mask;
    }
  }
  uint32_t curr_mask = instance->input.mouse_button_mask;

  FlutterPointerEvent pointer_event = {};
  pointer_event.struct_size = sizeof(FlutterPointerEvent);
  if (last_mask == 0 && curr_mask != 0) {
    pointer_event.phase = kDown;
  } else if (last_mask != 0 && curr_mask == 0) {
    pointer_event.phase = kUp;
  } else {
    pointer_event.phase = kMove;
  }
  pointer_event.x = instance->cursor->x;
  pointer_event.y = instance->cursor->y;
  pointer_event.device = 0;
  pointer_event.signal_kind = kFlutterPointerSignalKindNone;
  pointer_event.scroll_delta_x = 0;
  pointer_event.scroll_delta_y = 0;
  pointer_event.device_kind = kFlutterPointerDeviceKindMouse;
  pointer_event.buttons = curr_mask;
  // TODO this is not 100% right as we should return the timestamp from libinput.
  // On my machine these seem to be using the same source but differnt unit, is this a guarantee?
  pointer_event.timestamp = instance->fl_proc_table.GetCurrentTime();
  instance->fl_proc_table.SendPointerEvent(instance->engine, &pointer_event, 1);
}

static void on_server_cursor_axis(struct wl_listener *listener, void *data) {
  struct fwr_instance *instance = wl_container_of(listener, instance, cursor_axis);
}

static void on_server_cursor_frame(struct wl_listener *listener, void *data) {
  struct fwr_instance *instance = wl_container_of(listener, instance, cursor_frame);
}

static void on_server_cursor_touch_down(struct wl_listener *listener, void *data) {
  struct fwr_instance *instance = wl_container_of(listener, instance, cursor_touch_down);
}

static void on_server_cursor_touch_up(struct wl_listener *listener, void *data) {
  struct fwr_instance *instance = wl_container_of(listener, instance, cursor_touch_up);
}

static void on_server_cursor_touch_motion(struct wl_listener *listener, void *data) {
  struct fwr_instance *instance = wl_container_of(listener, instance, cursor_touch_motion);
}

static void on_server_cursor_touch_frame(struct wl_listener *listener, void *data) {
  struct fwr_instance *instance = wl_container_of(listener, instance, cursor_touch_frame);
}

static void cb(const uint8_t *data, size_t size, void *user_data) {
  wlr_log(WLR_INFO, "callback");
}


const uint64_t kValueMask = 0x000ffffffff;
const uint64_t kUnicodePlane = 0x00000000000;
const uint64_t kGtkPlane = 0x01500000000;

static uint64_t apply_id_plane(uint64_t logical_id, uint64_t plane) {
  return (logical_id & kValueMask) | plane;
}

static void send_key_to_flutter(struct fwr_instance *instance, struct wlr_event_keyboard_key *event, char *buffer) {

  FlutterPlatformMessageResponseHandle *response_handle;
  instance->fl_proc_table.PlatformMessageCreateResponseHandle(instance->engine, cb, NULL, &response_handle);

  char *type;
  FlutterKeyEventType flType;

  switch(event->state) {
    case WL_KEYBOARD_KEY_STATE_PRESSED:
      type = "keydown";
      flType = kFlutterKeyEventTypeDown;
      break;
    case WL_KEYBOARD_KEY_STATE_RELEASED:
    default:
      type = "keyup";
      flType = kFlutterKeyEventTypeUp;
      break;
  }

  wlr_log(WLR_INFO, "should send key %d", event->keycode);

  platch_send(
    instance,
    "flutter/keyevent",
    &(struct platch_obj) {
      .codec = kJSONMessageCodec,
      .json_value = {
        .type = kJsonObject,
        .size = 7,
        .keys = (char*[7]) {
          "keymap",
          "toolkit",
          "unicodeScalarValues",
          "keyCode",
          "scanCode",
          "modifiers",
          "type"
        },
        .values = (struct json_value[7]) {
          /* keymap */                {.type = kJsonString, .string_value = "linux"},
          /* toolkit */               {.type = kJsonString, .string_value = "gtk"},
          /* unicodeScalarValues */   {.type = kJsonNumber, .number_value = (flType == kFlutterKeyEventTypeDown ? 0x0410 : 0x0)},
          /* keyCode */               {.type = kJsonNumber, .number_value = apply_id_plane(0x041, kUnicodePlane)},
          /* scanCode */              {.type = kJsonNumber, .number_value = 0x00070004},
          /* modifiers */             {.type = kJsonNumber, .number_value = 0x0},
          /* type */                  {.type = kJsonString, .string_value = type}
        }
      }
    },
    kJSONMessageCodec,
    NULL,
    NULL
  );

}

static void keyboard_handle_modifiers(
		struct wl_listener *listener, void *data) {
    
  /* This event is raised when a modifier key, such as shift or alt, is
	 * pressed. We simply communicate this to the client. */
	struct fwr_keyboard *keyboard =
		wl_container_of(listener, keyboard, modifiers);
	/*
	 * A seat can only have one keyboard, but this is a limitation of the
	 * Wayland protocol - not wlroots. We assign all connected keyboards to the
	 * same seat. You can swap out the underlying wlr_keyboard like this and
	 * wlr_seat handles this transparently.
	 */
	wlr_seat_set_keyboard(keyboard->instance->seat, keyboard->device);
	/* Send modifiers to the client. */
	wlr_seat_keyboard_notify_modifiers(keyboard->instance->seat,
		&keyboard->device->keyboard->modifiers);


}

static void keyboard_handle_key(
		struct wl_listener *listener, void *data) {

	struct fwr_keyboard *keyboard =	wl_container_of(listener, keyboard, key);
  struct fwr_instance *instance = keyboard->instance;
  struct wlr_event_keyboard_key *event = data;
  struct wlr_seat *seat = instance->seat;

  // translate libinput keycode to xkbcommon
  uint32_t keycode = event->keycode + 8;
  xkb_keysym_t codepoint = xkb_state_key_get_one_sym(keyboard->device->keyboard->xkb_state, keycode);

  // get list of keysyms basd on the keymap for this keyboard
  const xkb_keysym_t *syms;
  int nsyms = xkb_state_key_get_syms(keyboard->device->keyboard->xkb_state, keycode, &syms);

  bool handled = false;

   FlutterKeyEventType flutter_key_event_type;

   switch (event->state)
   {
     case WL_KEYBOARD_KEY_STATE_PRESSED:
      flutter_key_event_type = kFlutterKeyEventTypeDown;
      break;
    case WL_KEYBOARD_KEY_STATE_RELEASED:
      flutter_key_event_type = kFlutterKeyEventTypeUp;
      break;
    default:
      flutter_key_event_type = kFlutterKeyEventTypeUp;
      break;
   }
  
  
  char *buffer;
  int size;

   // First find the needed size; return value is the same as snprintf(3).
   size = xkb_state_key_get_utf8(keyboard->device->keyboard->xkb_state, keycode, NULL, 0) + 1;
   if (size > 1) {
     buffer = malloc(size);

     xkb_state_key_get_utf8(keyboard->device->keyboard->xkb_state, keycode, buffer, size);
   }  

  send_key_to_flutter(instance, event, buffer);

}

static void server_new_keyboard(struct fwr_instance *instance,
		struct wlr_input_device *device) {

  struct fwr_keyboard *keyboard = calloc(1, sizeof(struct fwr_keyboard));
  keyboard->instance = instance;
  keyboard->device = device;

  // Prepare XKB keymap and asing to keyboard, default layout is "us"
  struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  struct xkb_keymap *keymap = xkb_keymap_new_from_names(context, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);

  wlr_keyboard_set_keymap(device->keyboard, keymap);
  xkb_keymap_unref(keymap);
  xkb_context_unref(context);
  wlr_keyboard_set_repeat_info(device->keyboard, 25, 600);

  // here we set up listeners for keyboard events
  keyboard->modifiers.notify = keyboard_handle_modifiers;
  wl_signal_add(&device->keyboard->events.modifiers, &keyboard->modifiers);
  keyboard->key.notify = keyboard_handle_key;
  wl_signal_add(&device->keyboard->events.key, &keyboard->key);

  wlr_seat_set_keyboard(instance->seat, device);

  // add keyboard to list of keyboards
  wl_list_insert(&instance->keyboards, &keyboard->link);


}

static void server_new_pointer(struct fwr_instance *instance,
		struct wlr_input_device *device) {
  wlr_cursor_attach_input_device(instance->cursor, device);
}

static void server_new_touch(struct fwr_instance *instance,
		struct wlr_input_device *device) {

}

static void on_server_new_input(struct wl_listener *listener, void *data) {
  struct fwr_instance *instance = wl_container_of(listener, instance, new_input);
  struct wlr_input_device *device = data;

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
  if (!wl_list_empty(&instance->keyboards)) {
   caps |= WL_SEAT_CAPABILITY_KEYBOARD;
  }
  wlr_seat_set_capabilities(instance->seat, caps);
}

void fwr_input_init(struct fwr_instance *instance) {
  instance->input.mouse_down = false;
  instance->input.mouse_button_mask = 0;

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

  wl_list_init(&instance->keyboards);

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
      switch (message.event_type) {
        case pointerDownEvent:
          wlr_seat_pointer_notify_button(instance->seat, message.timestamp / NS_PER_MS, 0x110, WLR_BUTTON_PRESSED);
          break;
        case pointerUpEvent:
          wlr_seat_pointer_notify_button(instance->seat, message.timestamp / NS_PER_MS, 0x110, WLR_BUTTON_RELEASED);
          break;
        case pointerHoverEvent:
        case pointerEnterEvent:
        case pointerMoveEvent: {
          wlr_seat_pointer_notify_enter(instance->seat, view->surface->surface, transformed_local_pos_x, transformed_local_pos_y);
          wlr_seat_pointer_notify_motion(instance->seat, message.timestamp / NS_PER_MS, transformed_local_pos_x, transformed_local_pos_y);
          break;
        }
        case pointerExitEvent: {
          wlr_seat_pointer_clear_focus(instance->seat);
          break;
        }
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


void fwr_handle_key_press(struct fwr_instance *instance, const FlutterPlatformMessageResponseHandle *handle, struct dart_value *args){
  struct wlr_seat *seat = instance->seat;
	
  struct key_event_message message;
  if (!decode_key_event_message(args, &message)) {
    wlr_log(WLR_ERROR, "Invalid key event message");
    return;
  }

  wlr_log(WLR_INFO, "should send key %s", message);
	
  wlr_seat_keyboard_notify_key(seat, message.timestamp,
			30, message.key_state);
}