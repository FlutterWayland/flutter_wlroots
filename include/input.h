#pragma once

#include <stdbool.h>

#include "shaders.h"

struct fwr_input_state {
    bool mouse_down;
    uint32_t mouse_button_mask;
};

void fwr_input_init(struct fwr_instance *instance);