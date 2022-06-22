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

#ifdef __cplusplus
}
#endif