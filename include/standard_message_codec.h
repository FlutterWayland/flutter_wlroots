#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct message_builder {
    struct message_builder_state *state;
};

struct message_builder_segment {
    struct message_builder_state *state;
    uint32_t size;
    uint32_t written;
    uint32_t level;
};

struct message_builder message_builder_new();
struct message_builder_segment message_builder_segment(struct message_builder *builder);
void message_builder_finish(struct message_builder *builder, uint8_t **buffer, size_t *size);

void message_builder_segment_push_null(struct message_builder_segment *segment);
void message_builder_segment_push_bool(struct message_builder_segment *segment, bool value);
void message_builder_segment_push_int32(struct message_builder_segment *segment, int32_t value);
void message_builder_segment_push_int64(struct message_builder_segment *segment, int64_t value);
void message_builder_segment_push_float64(struct message_builder_segment *segment, double value);
void message_builder_segment_push_string(struct message_builder_segment *segment, const char *string);
void message_builder_segment_push_uint8_list(struct message_builder_segment *segment, const uint8_t *vals, size_t size);
struct message_builder_segment message_builder_segment_push_list(struct message_builder_segment *segment, size_t size);
struct message_builder_segment message_builder_segment_push_map(struct message_builder_segment *segment, size_t size);
void message_builder_segment_finish(struct message_builder_segment *segment);