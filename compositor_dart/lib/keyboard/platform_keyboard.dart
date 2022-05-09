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

  void setCallbacks(PlatformKeyboardCallbacks? callbacks) {
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

  Future<ByteData?> _handleTextInputMessage(ByteData? data) async {
    var methodCall = _codec.decodeMethodCall(data);

    print("${methodCall.method}: ${methodCall.arguments}");

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
        );
        _keyboardClient?.addListener(_onKeyboardClientValueChanged);
        _keyboardCallbacks?.setClient(_keyboardClient);
        return _codec.encodeSuccessEnvelope(null);
    }

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

  const PlatformKeyboardWidget({
    Key? key,
    required this.child,
    this.callbacks,
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
    controller.setCallbacks(widget.callbacks);
  }

  @override
  void didUpdateWidget(PlatformKeyboardWidget oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.callbacks != widget.callbacks) {
      controller.setCallbacks(widget.callbacks);
    }
  }

  @override
  Widget build(BuildContext context) {
    return widget.child;
  }
}
