#include <iterator>
#include <stdint.h>
#include <map>

#include "handle_map.h"

struct handle_map {
    uint32_t next;
    std::map<uint32_t, void*> map;
};

extern "C" handle_map *handle_map_new() {
    handle_map *map = new handle_map({1, {}});
    return map;
}

extern "C" void handle_map_destroy(handle_map *map) {
    delete map;
}

extern "C" uint32_t handle_map_add(handle_map *map, void *value) {
    uint32_t key = map->next++;
    map->map.insert({key, value});
    return key;
}

extern "C" bool handle_map_remove(handle_map *map, uint32_t handle) {
    auto entry = map->map.find(handle);
    if (entry == map->map.end()) {
        return false;
    } else {
        map->map.erase(entry);
        return true;
    }
}

extern "C" bool handle_map_get(handle_map *map, uint32_t handle, void **out) {
    auto entry = map->map.find(handle);
    if (entry == map->map.end()) {
        return false;
    } else {
        *out = entry->second;
        return true;
    }
}

