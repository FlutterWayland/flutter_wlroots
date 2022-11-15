#pragma once

#include <stddef.h>

#include "flutter_embedder.h"

struct fwr_instance;

typedef bool (*handle_message)(struct fwr_instance* instance, const FlutterPlatformMessage *message, void *data);

struct fwr_plugin_vtable {
    void (*destroy)(void *);
};

struct fwr_plugin {
    int id;
    void *data;
    struct fwr_plugin_vtable vtable;
};

struct fwr_plugin_channel_handler {
    int id;
    char *name;
    void *data;
    handle_message handle_message;
};

struct fwr_plugin_registry {
    int next_id;

    struct fwr_plugin *plugins;
    size_t plugins_len;
    size_t plugins_cap;

    struct fwr_plugin_channel_handler *plugin_channel_handlers;
    size_t plugin_channel_handlers_len;
    size_t plugin_channel_handlers_cap;
};

void fwr_plugin_registry_init(struct fwr_plugin_registry *registry);

int fwr_plugin_registry_register(struct fwr_plugin_registry *registry, struct fwr_plugin_vtable vtable, void *data);
void fwr_plugin_registry_unregister(struct fwr_plugin_registry *registry, int id);

int fwr_plugin_registry_channel_handler_register(struct fwr_plugin_registry *registry, char *channel_name, void *data, handle_message handle_message);
void fwr_plugin_registry_channel_handler_unregister(struct fwr_plugin_registry *registry, int id);

bool fwr_message_send_response(struct fwr_instance *instance, FlutterPlatformMessageResponseHandle *response_handle, const uint8_t *data, size_t data_length);