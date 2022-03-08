#include "messages.h"

#define DECODE(OUT, VAL, TYPE, ACCESSOR) \
    do {\
        struct dart_value *val = VAL;\
        if (val->type != TYPE) {\
            return false;\
        }\
        OUT = val->ACCESSOR;\
    } while (0)

#define DECODE_INTEGER(OUT, VAL) DECODE(OUT, VAL, dvInteger, integer)
#define DECODE_FLOAT64(OUT, VAL) DECODE(OUT, VAL, dvFloat64, f64)

bool decode_surface_pointer_event_message(struct dart_value *value, struct surface_pointer_event_message *out) {
    if (value->type != dvList) {
        return false;
    }
    if (value->list.length != 29) {
        return false;
    }

    // 0: surface.handle,
    // 1: event.buttons,
    // 2: event.delta.dx,
    // 3: event.delta.dy,
    // 4: event.device,
    // 5: event.distance,
    // 6: event.down,
    // 7: event.embedderId,
    // 8: event.kind.name,
    // 9: event.localDelta.dx,
    // 10: event.localDelta.dy,
    // 11: event.localPosition.dx,
    // 12: event.localPosition.dy,
    // 13: event.obscured,
    // 14: event.orientation,
    // 15: event.platformData,
    // 16: event.pointer,
    // 17: event.position.dx,
    // 18: event.position.dy,
    // 19: event.pressure,
    // 20: event.radiusMajor,
    // 21: event.radiusMinor,
    // 22: event.size,
    // 23: event.synthesized,
    // 24: event.tilt,
    // 25: event.timeStamp.inMicroseconds,
    // 26: event type

    DECODE_INTEGER(out->surface_handle, &value->list.values[0]);
    DECODE_INTEGER(out->buttons, &value->list.values[1]);
    DECODE_INTEGER(out->device_kind, &value->list.values[8]);
    DECODE_FLOAT64(out->local_pos_x, &value->list.values[11]);
    DECODE_FLOAT64(out->local_pos_y, &value->list.values[12]);
    DECODE_INTEGER(out->timestamp, &value->list.values[25]);
    DECODE_INTEGER(out->event_type, &value->list.values[26]);
    DECODE_FLOAT64(out->widget_size_x, &value->list.values[27]);
    DECODE_FLOAT64(out->widget_size_y, &value->list.values[28]);

    return true;
}

bool decode_surface_axis_event_message(struct dart_value *value, struct surface_axis_event_message *out) {
    if (value->type != dvList) {
        return false;
    }
    if (value->list.length != 5) {
        return false;
    }

    DECODE_INTEGER(out->surface_handle, &value->list.values[0]);
    DECODE_INTEGER(out->timestamp, &value->list.values[1]);
    DECODE_FLOAT64(out->value, &value->list.values[2]);
    DECODE_INTEGER(out->value_discrete, &value->list.values[3]);
    DECODE_INTEGER(out->orientation, &value->list.values[4]);

    return true;
}

bool decode_surface_toplevel_set_size_message(struct dart_value *value, struct surface_toplevel_set_size_message *out) {
    if (value->type != dvList) {
        return false;
    }
    if (value->list.length != 3) {
        return false;
    }

    DECODE_INTEGER(out->surface_handle, &value->list.values[0]);
    DECODE_INTEGER(out->size_x, &value->list.values[1]);
    DECODE_INTEGER(out->size_y, &value->list.values[2]);

    return true;
}