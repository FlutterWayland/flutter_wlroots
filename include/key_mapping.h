#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

uint64_t xkb_to_physical_key(uint64_t xkb_key);


uint64_t gtk_key_to_logical_key(uint64_t xkb_key);

#ifdef __cplusplus
}
#endif