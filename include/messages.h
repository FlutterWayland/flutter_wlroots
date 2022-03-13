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

struct surface_toplevel_set_size_message {
    uint32_t surface_handle;
    int64_t size_x;
    int64_t size_y;
};

bool decode_surface_toplevel_set_size_message(struct dart_value *value, struct surface_toplevel_set_size_message *out);

struct key_event_message {
    uint64_t logical_key_id;
    uint64_t physical_key_id;
    int64_t timestamp;
    char* character;
    int key_state; // 0 - released, 1 - pressed
};

bool decode_key_event_message(struct dart_value *value, struct key_event_message *out);