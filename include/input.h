#pragma once

#include <stdbool.h>

#include "flutter_embedder.h"
#include "shaders.h"
#include "standard_message_codec.h"

#define FWR_MULTITOUCH_MAX 10

struct fwr_input_state {
    uint32_t mouse_button_mask;
    uint32_t fl_mouse_button_mask;

    // Accumulated state for cursor before frame.
    uint32_t acc_mouse_button_mask;
    double acc_scroll_delta_x;
    double acc_scroll_delta_y;

    // Touch state
    bool simulating_pointer_from_touch;
    int touch_pointer_simulation_id;
    int64_t touch_ids[FWR_MULTITOUCH_MAX];
};

struct fwr_input_touch_point_state {
    double x;
    double y;
};

struct fwr_input_device_state {
    struct fwr_input_touch_point_state touch_points[10];
};

void fwr_input_init(struct fwr_instance *instance);

void fwr_handle_surface_pointer_event_message(struct fwr_instance *instance, const FlutterPlatformMessageResponseHandle *handle, struct dart_value *args);

void fwr_handle_surface_keyboard_key_message(struct fwr_instance *instance, const FlutterPlatformMessageResponseHandle *handle, struct dart_value *args);