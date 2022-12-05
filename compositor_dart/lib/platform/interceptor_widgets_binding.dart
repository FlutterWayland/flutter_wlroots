import 'package:compositor_dart/platform/interceptor_binary_messenger.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

class InterceptorWidgetsBinding extends WidgetsFlutterBinding {
  InterceptorBinaryMessenger? _binaryMessenger;

  @override
  BinaryMessenger get defaultBinaryMessenger {
    _binaryMessenger ??= InterceptorBinaryMessenger(super.defaultBinaryMessenger);
    return _binaryMessenger!;
  }

  static WidgetsBinding ensureInitialized() {
    try {
      return WidgetsBinding.instance;
    } on FlutterError {
      var constructedBinding = InterceptorWidgetsBinding();
      assert(constructedBinding == WidgetsBinding.instance);
      return WidgetsBinding.instance;
    }
  }

  static void runApp(Widget app) {
    InterceptorWidgetsBinding.ensureInitialized()
      ..scheduleAttachRootWidget(app)
      ..scheduleWarmUpFrame();
  }
}
