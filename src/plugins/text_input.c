#include <stdlib.h>

#include "wlr/util/log.h"

#include "instance.h"
#include "plugin_registry.h"
#include "platform_channel.h"

#include "flutter_embedder.h"

#define TEXT_INPUT_MAX_CHARS 8192

enum text_input_type
{
  kInputTypeText,
  kInputTypeMultiline,
  kInputTypeNumber,
  kInputTypePhone,
  kInputTypeDatetime,
  kInputTypeEmailAddress,
  kInputTypeUrl,
  kInputTypeVisiblePassword,
  kInputTypeName,
  kInputTypeAddress
};

enum text_input_action
{
  kTextInputActionNone,
  kTextInputActionUnspecified,
  kTextInputActionDone,
  kTextInputActionGo,
  kTextInputActionSearch,
  kTextInputActionSend,
  kTextInputActionNext,
  kTextInputActionPrevious,
  kTextInputActionContinueAction,
  kTextInputActionJoin,
  kTextInputActionRoute,
  kTextInputActionEmergencyCall,
  kTextInputActionNewline
};

// while text input configuration has more values, we only care about these two.
struct text_input_configuration
{
  bool autocorrect;
  enum text_input_action input_action;
};

enum floating_cursor_drag_state
{
  kFloatingCursorDragStateStart,
  kFloatingCursorDragStateUpdate,
  kFloatingCursorDragStateEnd
};

struct text_input {
  int64_t connection_id;
  enum text_input_type input_type;
  bool allow_signs;
  bool has_allow_signs;
  bool allow_decimal;
  bool has_allow_decimal;
  bool autocorrect;
  enum text_input_action input_action;
  char text[TEXT_INPUT_MAX_CHARS];
  int selection_base, selection_extent;
  bool selection_affinity_is_downstream;
  bool selection_is_directional;
  int composing_base, composing_extent;
  bool warned_about_autocorrect;
};

static int on_set_client(struct fwr_instance *instance, struct text_input *data, struct platch_obj *object, FlutterPlatformMessageResponseHandle *response_handle) {

  if ((object->json_arg.type != kJsonArray) || (object->json_arg.size != 2)) {
    return platch_respond_illegal_arg_json(instance, response_handle, "Expected `arg` as array with length 2");
  }
  struct json_value *arr = object->json_arg.array;

  if (arr[0].type != kJsonNumber) {
    return platch_respond_illegal_arg_json(instance, response_handle, "Expected `arg[0]` to be a number.");
  }

  if (arr[1].type != kJsonObject) {
    return platch_respond_illegal_arg_json(instance, response_handle, "Expected `arg[1]` to be a map.");
  }

  struct json_value *config = &arr[1];

  bool autocorrect;
  struct json_value *autocorrect_obj = jsobject_get(config,  "autocorrect");
  if (autocorrect_obj == NULL || (autocorrect_obj->type != kJsonTrue && autocorrect_obj->type != kJsonFalse)) {
    return platch_respond_illegal_arg_json(instance, response_handle, "Expected `arg[1].autocorrect` to be a boolean.");
  } else {
    autocorrect = autocorrect_obj->type == kJsonTrue;
  }

  struct json_value *input_action_obj = jsobject_get(config, "inputAction");
  if (input_action_obj == NULL || input_action_obj->type != kJsonString) {
    return platch_respond_illegal_arg_json(instance, response_handle, "Expected `arg[1].inputAction` to be a string.");
  }

  char *input_action_str = input_action_obj->string_value;
  enum text_input_action input_action;
  if (strcmp("TextInputAction.none", input_action_str) == 0) {
    input_action = kTextInputActionNone;
  } else if (strcmp("TextInputAction.unspecified", input_action_str) == 0) {
    input_action = kTextInputActionUnspecified;
  } else if (strcmp("TextInputAction.done", input_action_str) == 0) {
    input_action = kTextInputActionDone;
  } else if (strcmp("TextInputAction.go", input_action_str) == 0) {
    input_action = kTextInputActionGo;
  } else if (strcmp("TextInputAction.search", input_action_str) == 0) {
    input_action = kTextInputActionSearch;
  } else if (strcmp("TextInputAction.send", input_action_str) == 0) {
    input_action = kTextInputActionSend;
  } else if (strcmp("TextInputAction.next", input_action_str) == 0) {
    input_action = kTextInputActionNext;
  } else if (strcmp("TextInputAction.previous", input_action_str) == 0) {
    input_action = kTextInputActionPrevious;
  } else if (strcmp("TextInputAction.continueAction", input_action_str) == 0) {
    input_action = kTextInputActionContinueAction;
  } else if (strcmp("TextInputAction.join", input_action_str) == 0) {
    input_action = kTextInputActionJoin;
  } else if (strcmp("TextInputAction.route", input_action_str) == 0) {
    input_action = kTextInputActionRoute;
  } else if (strcmp("TextInputAction.emergencyCall", input_action_str) == 0) {
    input_action = kTextInputActionEmergencyCall;
  } else if (strcmp("TextInputAction.newline", input_action_str) == 0) {
    input_action = kTextInputActionNewline;
  } else {
    return platch_respond_illegal_arg_json(instance, response_handle, "Expected `arg[1].inputAction` to be a string-ification of `TextInputAction`.");
  }

  struct json_value *input_type_obj = jsobject_get(config, "inputType");
  if (input_action_obj == NULL || config->type != kJsonObject) {
    return platch_respond_illegal_arg_json(instance, response_handle, "Expected `arg[1].inputType` to be a map.");
  }

  bool has_allow_signs;
  bool allow_signs = false;
  struct json_value *input_type_signed_obj = jsobject_get(input_type_obj, "signed");
  if (input_type_signed_obj == NULL || input_type_signed_obj->type == kJsonNull) {
    has_allow_signs = false;
  } else if (input_type_signed_obj->type == kJsonFalse || input_type_signed_obj->type == kJsonTrue) {
    has_allow_signs = true;
    allow_signs = input_type_signed_obj->type == kJsonTrue;
  } else {
    return platch_respond_illegal_arg_json(instance, response_handle, "Expected `arg[1].inputType.signed` to be a boolean or null.");
  }

  bool has_allow_decimal;
  bool allow_decimal = false;
  struct json_value *input_type_decimal_obj = jsobject_get(input_type_obj, "decimal");
  if (input_type_decimal_obj == NULL || input_type_decimal_obj->type == kJsonNull) {
    has_allow_decimal = false;
  } else if (input_type_decimal_obj->type == kJsonFalse || input_type_decimal_obj->type == kJsonTrue) {
    has_allow_signs = true;
    allow_signs = input_type_decimal_obj->type == kJsonTrue;
  } else {
    return platch_respond_illegal_arg_json(instance, response_handle, "Expected `arg[1].inputType.decimal` to be a boolean or null.");
  }

  struct json_value *input_type_name_obj = jsobject_get(input_type_obj, "name");
  if (input_type_name_obj == NULL || input_type_name_obj->type != kJsonString) {
    return platch_respond_illegal_arg_json(instance, response_handle, "Expected `arg[1].inputType.name` to be a string.");
  }

  enum text_input_type input_type;
  char *input_type_name_str = input_type_name_obj->string_value;
  if (strcmp("TextInputType.text", input_type_name_str) == 0) {
    input_type = kInputTypeText;
  } else if (strcmp("TextInputType.multiline", input_type_name_str) == 0) {
    input_type = kInputTypeMultiline;
  } else if (strcmp("TextInputType.number", input_type_name_str) == 0) {
    input_type = kInputTypeNumber;
  } else if (strcmp("TextInputType.phone", input_type_name_str) == 0) {
    input_type = kInputTypePhone;
  } else if (strcmp("TextInputType.datetime", input_type_name_str) == 0) {
    input_type = kInputTypeDatetime;
  } else if (strcmp("TextInputType.emailAddress", input_type_name_str) == 0) {
    input_type = kInputTypeEmailAddress;
  } else if (strcmp("TextInputType.url", input_type_name_str) == 0) {
    input_type = kInputTypeUrl;
  } else if (strcmp("TextInputType.visiblePassword", input_type_name_str) == 0) {
    input_type = kInputTypeVisiblePassword;
  } else if (strcmp("TextInputType.name", input_type_name_str) == 0) {
    input_type = kInputTypeName;
  } else if (strcmp("TextInputType.address", input_type_name_str) == 0) {
    input_type = kInputTypeAddress;
  } else {
    return platch_respond_illegal_arg_json(instance, response_handle, "Expected `arg[1].inputType.name` to be a string-ification of `TextInputType`.");
  }

  int32_t new_id = (int32_t) object->json_arg.array[0].number_value;

  data->connection_id = new_id;
  data->autocorrect = autocorrect;
  data->input_action = input_action;
  data->input_type = input_type;
  data->has_allow_decimal = has_allow_decimal;
  data->allow_decimal = allow_decimal;
  data->has_allow_signs = has_allow_signs;
  data->allow_signs = allow_signs;

  return platch_respond(
    instance, 
    response_handle,
    &(struct platch_obj) {
      .codec = kJSONMethodCallResponse,
      .success = true,
      .json_result = {.type = kJsonNull},
    }
  );
}

static bool text_input_handle_message(struct fwr_instance *instance, const FlutterPlatformMessage *message, void *data)
{
  struct text_input *text_input = data;
  wlr_log(WLR_INFO, "Channel message: flutter/textinput: %.*s", (int)message->message_size, message->message);

  struct platch_obj object;
  if (platch_decode(message->message, message->message_size, kJSONMethodCall, &object) != 0) {
    wlr_log(WLR_ERROR, "Platch decode failed!");
    return false;
  }

  if (strcmp("TextInput.setClient", object.method) == 0) {
    on_set_client(instance, text_input, &object, message->response_handle);
  }

  return false;
}

void fwr_plugin_text_input_init(struct fwr_instance *instance)
{
  struct text_input *data = calloc(1, sizeof(struct text_input));

  data->connection_id = -1;
  data->input_type = kInputTypeText;
  data->allow_signs = false;
  data->has_allow_signs = false;
  data->allow_decimal = false;
  data->has_allow_decimal = false;
  data->autocorrect = false;
  data->input_action = kTextInputActionNone;
  data->text[0] = '\0';
  data->selection_base = 0;
  data->selection_extent = 0;
  data->selection_affinity_is_downstream = false;
  data->selection_is_directional = false;
  data->composing_base = 0;
  data->composing_extent = 0;
  data->warned_about_autocorrect = false;

  fwr_plugin_registry_channel_handler_register(&instance->plugin_registry, "flutter/textinput", data, text_input_handle_message);
}