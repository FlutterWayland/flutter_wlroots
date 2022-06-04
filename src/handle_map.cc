#include <iterator>
#include <stdint.h>
#include <map>

#include "handle_map.h"

struct handle_map {
    uint32_t next;
    std::map<uint32_t, void*> map;
    std::map<void*, uint32_t> reverse_map;
};

extern "C" handle_map *handle_map_new() {
    handle_map *map = new handle_map({1, {}, {}});
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

extern "C" void handle_map_add_reverse(handle_map *map, void* value, uint32_t handle) {
    map->reverse_map.insert({value, handle});
}

extern "C" uint32_t handle_map_find(handle_map *map, void* value) {

    auto entry = map->reverse_map.find(value);
    if (entry == map->reverse_map.end()) {
        return -2;
    } else {
        return entry->second;
    }

}

extern "C" bool handle_map_remove_reverse(handle_map *map, uint32_t handle) {

    for (auto it = map->reverse_map.begin(); it != map->reverse_map.end(); ++it) {
        if (it->second == handle){
            map->reverse_map.erase(it);
            return true;
        }
    }

    return false;
}
