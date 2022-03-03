#pragma once

#include <stdbool.h>

#include "flutter_embedder.h"
#include "shaders.h"
#include "standard_message_codec.h"

struct fwr_input_state {
    bool mouse_down;
    uint32_t mouse_button_mask;
};

void fwr_input_init(struct fwr_instance *instance);

void fwr_handle_surface_pointer_event_message(struct fwr_instance *instance, const FlutterPlatformMessageResponseHandle *handle, struct dart_value *args);