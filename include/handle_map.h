#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

struct handle_map;

struct handle_map *handle_map_new();
void handle_map_destroy(struct handle_map *map);
uint32_t handle_map_add(struct handle_map *map, void *value);
bool handle_map_get(struct handle_map *map, uint32_t handle, void **out);
bool handle_map_remove(struct handle_map *map, uint32_t handle);

void handle_map_add_reverse(struct handle_map *map, void* value, uint32_t handle);
uint32_t handle_map_find(struct handle_map *map, void* value);
bool handle_map_remove_reverse(struct handle_map *map, uint32_t handle);

#ifdef __cplusplus
}
#endif