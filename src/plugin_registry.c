#include <stdlib.h>
#include <string.h>

#include <wlr/util/log.h>

#include "instance.h"
#include "plugin_registry.h"

void fwr_plugin_registry_init(struct fwr_plugin_registry *registry) {
    registry->next_id = 0;

    registry->plugins = calloc(8, sizeof(struct fwr_plugin));
    registry->plugins_len = 0;
    registry->plugins_cap = 8;

    registry->plugin_channel_handlers = calloc(8, sizeof(struct fwr_plugin_channel_handler));
    registry->plugin_channel_handlers_len = 0;
    registry->plugin_channel_handlers_cap = 8;
}

static size_t plugin_find_idx(struct fwr_plugin_registry *registry, int id) {
    for (int i = 0; i < registry->plugins_len; i++) {
        struct fwr_plugin* plugin = &registry->plugins[i];
        if (plugin->id == id) {
            return i;
        }
    }
    return -1;
}

static size_t channel_handler_find_idx(struct fwr_plugin_registry *registry, int id) {
    for (int i = 0; i < registry->plugin_channel_handlers_len; i++) {
        struct fwr_plugin_channel_handler* handler = &registry->plugin_channel_handlers[i];
        if (handler->id == id) {
            return i;
        }
    }
    return -1;
}

int fwr_plugin_registry_register(struct fwr_plugin_registry *registry, struct fwr_plugin_vtable vtable, void *data) {
    if (registry->plugins_len >= registry->plugins_cap) {
        registry->plugins_cap *= 2;
        struct fwr_plugin* new_plugins = calloc(registry->plugins_cap, sizeof(struct fwr_plugin));
        memcpy(new_plugins, registry->plugins, sizeof(struct fwr_plugin) * registry->plugins_len);
        free(registry->plugins);
        registry->plugins = new_plugins;
    }

    registry->plugins_len += 1;
    struct fwr_plugin* plugin = &registry->plugins[registry->plugins_len];

    plugin->id = registry->next_id++;
    plugin->data = data;
    plugin->vtable = vtable;

    return plugin->id;
}

void fwr_plugin_registry_unregister(struct fwr_plugin_registry *registry, int id) {
    size_t idx = plugin_find_idx(registry, id);
    if (idx == -1) {
        wlr_log(WLR_ERROR, "Unregister called for unknown plugin id!");
        return;
    }

    // TODO remove
}

int fwr_plugin_registry_channel_handler_register(struct fwr_plugin_registry *registry, char *channel_name, void *data, handle_message handle_message) {
    if (registry->plugin_channel_handlers_len >= registry->plugin_channel_handlers_cap) {
        registry->plugin_channel_handlers_cap *= 2;
        struct fwr_plugin_channel_handler* new_handlers = calloc(registry->plugin_channel_handlers_cap, sizeof(struct fwr_plugin_channel_handler));
        memcpy(new_handlers, registry->plugin_channel_handlers, sizeof(struct fwr_plugin_channel_handler) * registry->plugin_channel_handlers_len);
        free(registry->plugin_channel_handlers);
        registry->plugin_channel_handlers = new_handlers;
    }

    registry->plugin_channel_handlers_len += 1;
    struct fwr_plugin_channel_handler* handler = &registry->plugin_channel_handlers[registry->plugin_channel_handlers_len];

    handler->id = registry->next_id++;
    handler->data = data;
    handler->handle_message = handle_message;

    size_t channel_name_len = strlen(channel_name);
    handler->name = calloc(1, channel_name_len + 1);
    memcpy(handler->name, channel_name, channel_name_len);

    return handler->id;
}

void fwr_plugin_registry_channel_handler_unregister(struct fwr_plugin_registry *registry, int id) {
    size_t idx = channel_handler_find_idx(registry, id);
    if (idx == -1) {
        wlr_log(WLR_ERROR, "Unregister called for unknown channel handler id!");
        return;
    }

    // TODO remove
}

bool fwr_message_send_response(struct fwr_instance *instance, FlutterPlatformMessageResponseHandle *response_handle, const uint8_t *data, size_t data_length) {
    if (instance->fl_proc_table.SendPlatformMessageResponse(instance->engine, response_handle, data, data_length) == kSuccess) {
        return true;
    } else {
        return false;
    }
}