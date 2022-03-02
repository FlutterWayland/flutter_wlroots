#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <wlr/util/log.h>

#include "standard_message_codec.h"

#define kValueNull 0
#define kValueTrue 1
#define kValueFalse 2
#define kValueInt32 3
#define kValueInt64 4
#define kValueFloat64 6
#define kValueString 7
#define kValueUint8List 8
#define kValueInt32List 9
#define kValueInt64List 10
#define kValueFloat64List 11
#define kValueList 12
#define kValueMap 13
#define kValueFloat32List 14

#define CALC_SIZE_LEN(OUT, IN) \
    size_t OUT;\
    do {\
        size_t in = IN;\
        if (in > UINT16_MAX) OUT = 5;\
        else if (in >= 254) OUT = 3;\
        else OUT = 1;\
    } while(0)

#define WRITE_SIZE_LEN(STATE, IN) \
    do {\
        size_t in = IN;\
        if (in > UINT16_MAX) {\
            STATE->buffer[STATE->buffer_len++] = 255;\
            ((uint32_t*) STATE->buffer)[0] = in;\
            STATE->buffer_len += sizeof(uint32_t);\
        } else if (in >= 254) {\
            STATE->buffer[STATE->buffer_len++] = 254;\
            ((uint16_t*) STATE->buffer)[0] = in;\
            STATE->buffer_len += sizeof(uint16_t);\
        } else {\
            STATE->buffer[STATE->buffer_len++] = in;\
        }\
    } while(0)

#define ASSERT_LEVEL(SEGMENT) \
    if (SEGMENT->level != SEGMENT->state->current_level) {\
        wlr_log(WLR_ERROR, "Attempted to write to wrong level!");\
    }

struct message_builder_state {
    uint8_t *buffer;
    size_t buffer_len;
    size_t buffer_capacity;
    uint32_t current_level;
};

static size_t max(size_t a, size_t b) {
    if (a < b) return b;
    return a;
}

static size_t growth_strategy(size_t current, size_t required) {
    size_t cap = max(current * 2, required);
    return max(cap, 32);
}

static void reserve(struct message_builder_state *state, size_t required) {
    size_t remaining = state->buffer_capacity - state->buffer_len;
    if (remaining < required) {
        size_t new_size = growth_strategy(state->buffer_capacity, state->buffer_len + required);
        state->buffer = realloc(state->buffer, new_size);
        state->buffer_capacity = new_size;
    }
}

struct message_builder message_builder_new() {
    size_t initial_capacity = 32;

    struct message_builder_state *state = malloc(sizeof(struct message_builder_state));
    state->buffer = malloc(initial_capacity);
    state->buffer_capacity = initial_capacity;
    state->buffer_len = 0;
    state->current_level = 0;

    struct message_builder mb = {
        .state = state,
    };
    return mb;
}

struct message_builder_segment message_builder_segment(struct message_builder *builder) {
    struct message_builder_segment segment = {
        .state = builder->state,
        .size = 1,
        .written = 0,
        .level = 1,
    };
    builder->state->current_level = 1;
    return segment;
}

void message_builder_finish(struct message_builder *builder, uint8_t **buffer, size_t *size) {
    struct message_builder_state *state = builder->state;
    if (state->buffer_len == state->buffer_capacity) {
        *buffer = state->buffer;
    } else {
        *buffer = realloc(state->buffer, state->buffer_len);
    }
    *size = state->buffer_len;
    free(state);
}

void message_builder_segment_push_null(struct message_builder_segment *segment) {
    ASSERT_LEVEL(segment);

    struct message_builder_state *state = segment->state;
    reserve(state, 1);
    state->buffer[state->buffer_len++] = kValueNull;
    segment->written += 1;
}

void message_builder_segment_push_bool(struct message_builder_segment *segment, bool value) {
    ASSERT_LEVEL(segment);

    struct message_builder_state *state = segment->state;
    reserve(state, 1);
    if (value) {
        state->buffer[state->buffer_len++] = kValueTrue;
    } else {
        state->buffer[state->buffer_len++] = kValueFalse;
    }
}

void message_builder_segment_push_int32(struct message_builder_segment *segment, int32_t value) {
    ASSERT_LEVEL(segment);

    struct message_builder_state *state = segment->state;
    reserve(state, 1 + sizeof(int32_t));
    state->buffer[state->buffer_len++] = kValueInt32;
    ((int32_t*) (state->buffer + state->buffer_len))[0] = value;
    state->buffer_len += sizeof(int32_t);
    segment->written += 1;
}

void message_builder_segment_push_int64(struct message_builder_segment *segment, int64_t value) {
    ASSERT_LEVEL(segment);

    struct message_builder_state *state = segment->state;
    reserve(state, 1 + sizeof(int64_t));
    state->buffer[state->buffer_len++] = kValueInt64;
    ((int64_t*) (state->buffer + state->buffer_len))[0] = value;
    state->buffer_len += sizeof(int64_t);
    segment->written += 1;
}

void message_builder_segment_push_float64(struct message_builder_segment *segment, double value) {
    ASSERT_LEVEL(segment);

    struct message_builder_state *state = segment->state;
    reserve(state, 1 + sizeof(double));
    state->buffer[state->buffer_len++] = kValueFloat64;
    ((double*) (state->buffer + state->buffer_len))[0] = value;
    state->buffer_len += sizeof(double);
    segment->written += 1;
}

void message_builder_segment_push_string(struct message_builder_segment *segment, const char *string) {
    ASSERT_LEVEL(segment);

    struct message_builder_state *state = segment->state;

    size_t string_size = strlen(string);
    CALC_SIZE_LEN(len_size, string_size);

    reserve(state, 1 + len_size + string_size);

    state->buffer[state->buffer_len++] = kValueString;
    WRITE_SIZE_LEN(state, string_size);
    memcpy(state->buffer + state->buffer_len, string, string_size);
    state->buffer_len += string_size;

    segment->written += 1;
}

void message_builder_segment_push_uint8_list(struct message_builder_segment *segment, const uint8_t *vals, size_t size) {
    ASSERT_LEVEL(segment);

    struct message_builder_state *state = segment->state;

    CALC_SIZE_LEN(len_size, size);

    reserve(state, 1 + len_size + size);

    state->buffer[state->buffer_len++] = kValueUint8List;
    WRITE_SIZE_LEN(state, size);
    memcpy(state->buffer + state->buffer_len, vals, size);
    state->buffer_len += size;

    segment->written += 1;
}

void message_builder_segment_push_int32_list(struct message_builder_segment *segment, const int32_t *vals, size_t size) {
    ASSERT_LEVEL(segment);

    struct message_builder_state *state = segment->state;

    CALC_SIZE_LEN(len_size, size);

    reserve(state, 1 + len_size + (size * sizeof(int32_t)));

    state->buffer[state->buffer_len++] = kValueInt32List;
    WRITE_SIZE_LEN(state, size);
    memcpy(state->buffer + state->buffer_len, vals, size * sizeof(int32_t));
    state->buffer_len += size;

    segment->written += 1;
}

void message_builder_segment_push_int64_list(struct message_builder_segment *segment, const int64_t *vals, size_t size) {
    ASSERT_LEVEL(segment);

    struct message_builder_state *state = segment->state;

    CALC_SIZE_LEN(len_size, size);

    reserve(state, 1 + len_size + (size * sizeof(int64_t)));

    state->buffer[state->buffer_len++] = kValueInt64List;
    WRITE_SIZE_LEN(state, size);
    memcpy(state->buffer + state->buffer_len, vals, size * sizeof(int64_t));
    state->buffer_len += size;

    segment->written += 1;
}

void message_builder_segment_push_float64_list(struct message_builder_segment *segment, const double *vals, size_t size) {
    ASSERT_LEVEL(segment);

    struct message_builder_state *state = segment->state;

    CALC_SIZE_LEN(len_size, size);

    reserve(state, 1 + len_size + (size * sizeof(double)));

    state->buffer[state->buffer_len++] = kValueFloat64List;
    WRITE_SIZE_LEN(state, size);
    memcpy(state->buffer + state->buffer_len, vals, size * sizeof(double));
    state->buffer_len += size;

    segment->written += 1;
}

void message_builder_segment_push_float32_list(struct message_builder_segment *segment, const float *vals, size_t size) {
    ASSERT_LEVEL(segment);

    struct message_builder_state *state = segment->state;

    CALC_SIZE_LEN(len_size, size);

    reserve(state, 1 + len_size + (size * sizeof(float)));

    state->buffer[state->buffer_len++] = kValueFloat32List;
    WRITE_SIZE_LEN(state, size);
    memcpy(state->buffer + state->buffer_len, vals, size * sizeof(float));
    state->buffer_len += size;

    segment->written += 1;
}

struct message_builder_segment message_builder_segment_push_list(struct message_builder_segment *segment, size_t size) {
    ASSERT_LEVEL(segment);

    struct message_builder_state *state = segment->state;

    CALC_SIZE_LEN(len_size, size);

    reserve(state, 1 + len_size);

    state->buffer[state->buffer_len++] = kValueList;
    WRITE_SIZE_LEN(state, size);

    state->current_level += 1;
    struct message_builder_segment sub_segment = {
        .state = state,
        .size = size,
        .written = 0,
        .level = state->current_level,
    };

    segment->written += 1;

    return sub_segment;
}

struct message_builder_segment message_builder_segment_push_map(struct message_builder_segment *segment, size_t size) {
    ASSERT_LEVEL(segment);

    struct message_builder_state *state = segment->state;

    CALC_SIZE_LEN(len_size, size);

    reserve(state, 1 + len_size);

    state->buffer[state->buffer_len++] = kValueMap;
    WRITE_SIZE_LEN(state, size);

    state->current_level += 1;
    struct message_builder_segment sub_segment = {
        .state = state,
        .size = size * 2,
        .written = 0,
        .level = state->current_level,
    };

    segment->written += 1;

    return sub_segment;
}

void message_builder_segment_finish(struct message_builder_segment *segment) {
    ASSERT_LEVEL(segment);

    if (segment->written != segment->size) {
        wlr_log(WLR_ERROR, "Incomplete segment was finished! (%d != %d)", segment->written, segment->size);
    }

    struct message_builder_state *state = segment->state;

    state->current_level -= 1;
}

#define MESSAGE_READ_DEBUG true

#define REQUIRE_DATA(N) if ((length - *offset) < N) { return false; }

#define READ_LENGTH(OUT) \
    do {\
        REQUIRE_DATA(1)\
        uint8_t first = buffer[*offset];\
        *offset += 1;\
        if (first == 255) {\
            REQUIRE_DATA(4)\
            OUT = *((uint32_t *)(buffer + *offset));\
            *offset += 4;\
        } else if (first == 254) {\
            REQUIRE_DATA(2)\
            OUT = *((uint16_t *)(buffer + *offset));\
            *offset += 2;\
        } else {\
            OUT = first;\
        }\
    } while (0);

bool message_read(const uint8_t *buffer, size_t length, size_t *offset, struct dart_value *out) {
    size_t s_len;

    REQUIRE_DATA(1)
    uint8_t type = buffer[*offset];
    *offset += 1;

    switch (type) {
    case kValueNull:
        out->type = dvNull;
        if (MESSAGE_READ_DEBUG) wlr_log(WLR_INFO, "vNull");
        return true;
    case kValueTrue:
    case kValueFalse:
        out->type = dvBool;
        out->boolean = type == kValueTrue;
        if (MESSAGE_READ_DEBUG) wlr_log(WLR_INFO, "vBool %d", out->boolean);
        return true;
    case kValueInt32:
        REQUIRE_DATA(4)
        out->type = dvInt32;
        out->i32 = *((uint32_t*) (buffer + *offset));
        *offset += 4;
        if (MESSAGE_READ_DEBUG) wlr_log(WLR_INFO, "vInt32 %d", out->i32);
        return true;
    case kValueInt64:
        REQUIRE_DATA(8)
        out->type = dvInt64;
        out->i64 = *((uint64_t*) (buffer + *offset));
        *offset += 8;
        if (MESSAGE_READ_DEBUG) wlr_log(WLR_INFO, "vInt64 %ld", out->i64);
        return true;
    case kValueFloat64:
        REQUIRE_DATA(8)
        out->type = dvFloat64;
        out->f64 = *((double*) (buffer + *offset));
        *offset += 8;
        if (MESSAGE_READ_DEBUG) wlr_log(WLR_INFO, "vInt64 %f", out->f64);
        return true;
    case kValueString:
        READ_LENGTH(s_len)

        out->type = dvString;
        out->string.length = s_len;
        out->string.string = malloc(s_len + 1);
        memcpy(out->string.string, &buffer[*offset], s_len);
        out->string.string[s_len] = 0;

        if (MESSAGE_READ_DEBUG) wlr_log(WLR_INFO, "vString %s", out->string.string);

        return true;

    case kValueList:
        READ_LENGTH(s_len)

        if (MESSAGE_READ_DEBUG) wlr_log(WLR_INFO, "> vList %ld", s_len);

        out->type = dvList;
        out->list.length = s_len;
        out->list.values = malloc(sizeof(struct dart_value) * s_len);
        for (int i = 0; i < s_len; i++) {
            if (!message_read(buffer, length, offset, &out->list.values[i])) {
                return false;
            }
        }

        if (MESSAGE_READ_DEBUG) wlr_log(WLR_INFO, "< vList %ld", s_len);

        return true;

    case kValueMap:
        READ_LENGTH(s_len)

        out->type = dvList;
        out->list.length = s_len;
        out->list.values = malloc(sizeof(struct dart_value) * s_len * 2);
        for (int i = 0; i < s_len; i++) {
            size_t base = i * 2;
            if (!message_read(buffer, length, offset, &out->list.values[base])) {
                return false;
            }
            if (!message_read(buffer, length, offset, &out->list.values[base+1])) {
                return false;
            }
        }

        return true;
    }

    return false;
}
