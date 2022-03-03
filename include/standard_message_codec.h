#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

extern uint8_t method_call_null_success[2];

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
void message_builder_segment_push_float32_list(struct message_builder_segment *segment, const float *vals, size_t size);
void message_builder_segment_push_float64_list(struct message_builder_segment *segment, const double *vals, size_t size);
void message_builder_segment_push_int32_list(struct message_builder_segment *segment, const int32_t *vals, size_t size);
void message_builder_segment_push_int64_list(struct message_builder_segment *segment, const int64_t *vals, size_t size);
struct message_builder_segment message_builder_segment_push_list(struct message_builder_segment *segment, size_t size);
struct message_builder_segment message_builder_segment_push_map(struct message_builder_segment *segment, size_t size);
void message_builder_segment_finish(struct message_builder_segment *segment);

enum dart_value_type {
    dvNull,
    dvBool,
    dvInteger,
    dvFloat64,
    dvString,
    dvU8List,
    dvInt32List,
    dvInt64List,
    dvFloat32List,
    dvFloat64List,
    dvList,
    dvMap,
};

struct dart_list {
    size_t length;
    struct dart_value *values;
};

struct dart_map {
    size_t length;
    struct dart_value *values;
};

struct dart_string {
    size_t length;
    char *string;
};

struct dart_value {
    enum dart_value_type type;
    union {
        bool boolean;
        int64_t integer;
        double f64;
        struct dart_string string;
        uint8_t *u8List;
        int32_t *i32List;
        int64_t *i64List;
        float *f32List;
        double *f64List;
        struct dart_list list;
        struct dart_map map;
    };
};

bool message_read(const uint8_t *buffer, size_t length, size_t *offset, struct dart_value *out);
void message_free(struct dart_value *value);