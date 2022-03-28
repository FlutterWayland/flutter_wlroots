#pragma once

#include <wayland-util.h>
#include <wayland-server-core.h>

void fwr_server_new_output(struct wl_listener *listener, void *data);
void fwr_engine_vsync_callback(void *data, intptr_t baton);