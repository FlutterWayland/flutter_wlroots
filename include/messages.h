#include <stdint.h>
#include <stdbool.h>

#include "standard_message_codec.h"

struct surface_pointer_event_message {
    uint32_t surface_handle;
    int64_t buttons;
    uint8_t device_kind;
    double local_pos_x;
    double local_pos_y;
    double widget_size_x;
    double widget_size_y;
    uint8_t event_type;
    int64_t timestamp;
};

bool decode_surface_pointer_event_message(struct dart_value *value, struct surface_pointer_event_message *out);

struct surface_axis_event_message {
    uint32_t surface_handle;
    int64_t timestamp;
    double value;
    int32_t value_discrete;
    uint8_t orientation;
};

bool decode_surface_axis_event_message(struct dart_value *value, struct surface_axis_event_message *out);

struct surface_toplevel_set_size_message {
    uint32_t surface_handle;
    int64_t size_x;
    int64_t size_y;
};

bool decode_surface_toplevel_set_size_message(struct dart_value *value, struct surface_toplevel_set_size_message *out);