import 'package:compositor_dart/platform/interceptor_binary_messenger.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

class InterceptorWidgetsBinding extends WidgetsFlutterBinding {
  InterceptorBinaryMessenger? _binaryMessenger;

  @override
  void initInstances() {
    super.initInstances();
    _binaryMessenger = InterceptorBinaryMessenger(super.defaultBinaryMessenger);
  }

  @override
  BinaryMessenger get defaultBinaryMessenger {
    return _binaryMessenger == null ? super.defaultBinaryMessenger : _binaryMessenger!;
  }

  static WidgetsBinding ensureInitialized() {
    if (WidgetsBinding.instance == null) {
      InterceptorWidgetsBinding();
    }
    return WidgetsBinding.instance!;
  }

  static void runApp(Widget app) {
    InterceptorWidgetsBinding.ensureInitialized()
      ..scheduleAttachRootWidget(app)
      ..scheduleWarmUpFrame();
  }
}
