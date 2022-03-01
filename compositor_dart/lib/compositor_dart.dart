library compositor_dart;

import 'dart:developer';

import 'package:flutter/services.dart';

/// A Calculator.
class Calculator {
  /// Returns [value] plus 1.
  int addOne(int value) => value + 1;
}


class Compositor {
  static const platform = MethodChannel("wlroots");

  Compositor() {
    platform.setMethodCallHandler((call) async {
      print("got call ${call.method}(${call.arguments})");
    });
    print("sending platform message");
    platform.invokeMethod("testing").then((value) => {
      print("testing message response")
    })
    .catchError((error) => {
      print("error")
    });
  }

}