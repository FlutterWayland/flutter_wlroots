import 'dart:async';

import 'package:compositor_dart/keyboard/keyboard_client_controller.dart';
import 'package:compositor_dart/keyboard/keyboard_controller.dart';
import 'package:compositor_dart/platform/interceptor_binary_messenger.dart';
import 'package:compositor_dart/platform/interceptor_widgets_binding.dart';
import 'package:flutter/services.dart';
import 'package:flutter/widgets.dart';

class PlatformKeyboardCallbacks {
  PlatformKeyboardCallbacks({required this.setClient, required this.setShown});

  Function(bool) setShown;
  Function(KeyboardClientController?) setClient;
}

class PlatformKeyboard {
  static const String textInputChannel = "flutter/textinput";

  static bool _initialized = false;

  late InterceptorBinaryMessenger _binaryMessenger;

  final JSONMethodCodec _codec = const JSONMethodCodec();

  KeyboardClientController? _keyboardClient;

  bool _shown = false;
  PlatformKeyboardCallbacks? _keyboardCallbacks;

  PlatformKeyboardCallbacks? get keyboardCallbacks => _keyboardCallbacks;
  set keyboardCallbacks(PlatformKeyboardCallbacks? callbacks) {
    if (_keyboardCallbacks != null) {
      if (_shown) _keyboardCallbacks!.setShown(false);
      if (_keyboardClient != null) _keyboardCallbacks!.setClient(null);
    }

    if (callbacks != null) {
      if (_keyboardClient != null) callbacks.setClient(_keyboardClient);
      if (_shown) callbacks.setShown(true);
    }

    _keyboardCallbacks = callbacks;
  }

  bool _forwardHardwareKeyboard = false;

  bool get forwardHardwareKeyboard => _forwardHardwareKeyboard;
  set forwardHardwareKeyboard(bool value) {
    if (!_forwardHardwareKeyboard && value) {
      HardwareKeyboard.instance.addHandler(_handleHardwareKeyboardEvent);
    }
    if (_forwardHardwareKeyboard && !value) {
      HardwareKeyboard.instance.removeHandler(_handleHardwareKeyboardEvent);
    }
    _forwardHardwareKeyboard = value;
  }

  PlatformKeyboard() {
    assert(!_initialized, "TextEditPlatform can only be initialized per flutter instance.");
    _initialized = true;

    if (ServicesBinding.instance is! InterceptorWidgetsBinding) {
      throw Exception("TextEditPlatform can only be used in InterceptorWidgetsBinding");
    }

    var binding = ServicesBinding.instance as InterceptorWidgetsBinding;
    _binaryMessenger = binding.defaultBinaryMessenger as InterceptorBinaryMessenger;

    _binaryMessenger.setOutgoingInterceptor(textInputChannel, _handleTextInputMessage);
  }

  destroy() {
    _initialized = false;
  }

  bool _handleHardwareKeyboardEvent(KeyEvent event) {
    // If we have a client...
    if (_keyboardClient != null) {
      bool shouldAdd = event is KeyDownEvent || event is KeyRepeatEvent;

      // and the event will add a character...
      if (event.character != null && shouldAdd) {
        // we forward it to the client and return true for "event handled"
        _keyboardClient!.addText(event.character!);
        return true;
      }

      // TODO other kinds of keys?
      //if (event.logicalKey == LogicalKeyboardKey.backspace && shouldAdd) {
      //  _keyboardClient!.deleteOne();
      //  return true;
      //}
    }
    return false;
  }

  Future<ByteData?> _handleTextInputMessage(ByteData? data) async {
    var methodCall = _codec.decodeMethodCall(data);

    switch (methodCall.method) {
      case 'TextInput.show':
        // arg: null
        if (_shown) {
          _keyboardCallbacks?.setShown(true);
          _shown = true;
        }
        return _codec.encodeSuccessEnvelope(null);

      case 'TextInput.hide':
        // arg: null
        _keyboardCallbacks?.setShown(false);
        assert(!_shown, "attempted to hide while already hidden");
        _shown = false;
        return _codec.encodeSuccessEnvelope(null);

      case 'TextInput.setEditingState':
        var editingState = TextEditingValue.fromJSON(methodCall.arguments);
        _keyboardClient!.value = editingState;
        return _codec.encodeSuccessEnvelope(null);

      case 'TextInput.clearClient':
        // arg: null
        _keyboardClient?.removeListener(_onKeyboardClientValueChanged);
        _keyboardClient?.dispose();
        _keyboardClient = null;
        _keyboardCallbacks?.setClient(null);
        return _codec.encodeSuccessEnvelope(null);

      case 'TextInput.setClient':
        // arg: [client_id, https://api.flutter.dev/flutter/services/TextInputConfiguration-class.html]
        _keyboardClient = KeyboardClientController(
          connectionId: methodCall.arguments[0] as int,
          platformKeyboard: this,
          inputConfiguration: _textInputConfigurationFromJson(methodCall.arguments[1]),
        );
        _keyboardClient?.addListener(_onKeyboardClientValueChanged);
        _keyboardCallbacks?.setClient(_keyboardClient);
        return _codec.encodeSuccessEnvelope(null);
    }

    print("unhandled IME message: ${methodCall.method}: ${methodCall.arguments}");

    return null;
  }

  void _onKeyboardClientValueChanged() {
    var callbackMethodCall = MethodCall("TextInputClient.updateEditingState", [
      _keyboardClient!.connectionId,
      _keyboardClient!.value.toJSON(),
    ]);
    _binaryMessenger.handlePlatformMessage("flutter/textinput", _codec.encodeMethodCall(callbackMethodCall), (data) {});
  }

  void sendPerformAction(int connectionId, TextInputAction action) {
    var callbackMethodCall = MethodCall("TextInputClient.performAction", [connectionId, action.toString()]);
    _binaryMessenger.handlePlatformMessage("flutter/textinput", _codec.encodeMethodCall(callbackMethodCall), (data) {});
  }

  static PlatformKeyboard of(BuildContext context) {
    return context.findAncestorStateOfType<_PlatformKeyboardState>()!.controller;
  }
}

class PlatformKeyboardWidget extends StatefulWidget {
  final Widget child;

  final PlatformKeyboardCallbacks? callbacks;

  final bool forwardHardwareKeyboard;

  const PlatformKeyboardWidget({
    Key? key,
    required this.child,
    this.callbacks,
    this.forwardHardwareKeyboard = true,
  }) : super(key: key);

  @override
  State<StatefulWidget> createState() {
    return _PlatformKeyboardState();
  }
}

class _PlatformKeyboardState extends State<PlatformKeyboardWidget> {
  PlatformKeyboard controller = PlatformKeyboard();

  @override
  void initState() {
    super.initState();
    controller.keyboardCallbacks = widget.callbacks;
    controller.forwardHardwareKeyboard = widget.forwardHardwareKeyboard;
  }

  @override
  void didUpdateWidget(PlatformKeyboardWidget oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.callbacks != widget.callbacks) {
      controller.keyboardCallbacks = widget.callbacks;
    }
    if (oldWidget.forwardHardwareKeyboard != widget.forwardHardwareKeyboard) {
      controller.forwardHardwareKeyboard = widget.forwardHardwareKeyboard;
    }
  }

  @override
  Widget build(BuildContext context) {
    return widget.child;
  }
}

TextInputType _textInputTypeFromJson(Map<String, dynamic> json) {
  switch (json["name"]) {
    case 'TextInputType.text':
      return TextInputType.text;
    case 'TextInputType.multiline':
      return TextInputType.multiline;
    case 'TextInputType.number':
      return TextInputType.numberWithOptions(
        signed: json["signed"] == true,
        decimal: json["decimal"] == true,
      );
    case 'TextInputType.phone':
      return TextInputType.phone;
    case 'TextInputType.datetime':
      return TextInputType.datetime;
    case 'TextInputType.emailAddress':
      return TextInputType.emailAddress;
    case 'TextInputType.url':
      return TextInputType.url;
    case 'TextInputType.visiblePassword':
      return TextInputType.visiblePassword;
    case 'TextInputType.name':
      return TextInputType.name;
    case 'TextInputType.address':
      return TextInputType.streetAddress;
    case 'TextInputType.none':
      return TextInputType.none;
  }
  throw "expected serialization of TextInputType, got $json";
}

TextInputAction _textInputActionFromJson(dynamic json) {
  switch (json) {
    case 'TextInputAction.none':
      return TextInputAction.none;
    case 'TextInputAction.unspecified':
      return TextInputAction.unspecified;
    case 'TextInputAction.done':
      return TextInputAction.done;
    case 'TextInputAction.go':
      return TextInputAction.go;
    case 'TextInputAction.search':
      return TextInputAction.search;
    case 'TextInputAction.send':
      return TextInputAction.send;
    case 'TextInputAction.next':
      return TextInputAction.next;
    case 'TextInputAction.previous':
      return TextInputAction.previous;
    case 'TextInputAction.continueAction':
      return TextInputAction.continueAction;
    case 'TextInputAction.join':
      return TextInputAction.join;
    case 'TextInputAction.route':
      return TextInputAction.route;
    case 'TextInputAction.emergencyCall':
      return TextInputAction.emergencyCall;
    case 'TextInputAction.newline':
      return TextInputAction.newline;
    default:
      throw "expected stringification of TextInputAction, got $json";
  }
}

TextCapitalization _textCapitalizationFromJson(dynamic json) {
  switch (json) {
    case 'TextCapitalization.words':
      return TextCapitalization.words;
    case 'TextCapitalization.sentences':
      return TextCapitalization.sentences;
    case 'TextCapitalization.characters':
      return TextCapitalization.characters;
    case 'TextCapitalization.none':
      return TextCapitalization.none;
    default:
      throw "expected stringification of TextCapitalization, got $json";
  }
}

Brightness _brightnessFromJson(dynamic json) {
  switch (json) {
    case 'Brightness.dark':
      return Brightness.dark;
    case 'Brightness.light':
      return Brightness.light;
    default:
      throw "expected stringification of Brightness, got $json";
  }
}

TextInputConfiguration _textInputConfigurationFromJson(Map<String, dynamic> json) {
  return TextInputConfiguration(
    inputType: _textInputTypeFromJson(json["inputType"]),
    readOnly: json["readOnly"] == true,
    obscureText: json["obscureText"] == true,
    autocorrect: json["autocorrect"] == true,
    // TODO: smartDashesType
    // TODO: smartQuotesType
    enableSuggestions: json["enableSuggestions"] == true,
    actionLabel: json["actionLabel"] as String?,
    inputAction: _textInputActionFromJson(json["inputAction"]),
    textCapitalization: _textCapitalizationFromJson(json["textCapitalization"]),
    keyboardAppearance: _brightnessFromJson(json["keyboardAppearance"]),
    enableIMEPersonalizedLearning: json["enableIMEPersonalizedLearning"] == true,
    // TODO
    autofillConfiguration: AutofillConfiguration.disabled,
    enableDeltaModel: json["enableDeltaModel"] == true,
  );
}
