// ignore_for_file: public_member_api_docs, sort_constructors_first
library compositor_dart;

import 'dart:async';
import 'dart:collection';
import 'dart:io';

import 'package:compositor_dart/constants.dart';
import 'package:flutter/services.dart';
import 'package:flutter/widgets.dart';
import 'package:logging/logging.dart';

enum KeyStatus { released, pressed }

enum WindowEventType { maximize, minimize }

class WindowEvent {
  final WindowEventType windowEventType;
  final int handle;

  WindowEvent({
    required this.windowEventType,
    required this.handle,
  });
}

class Surface {
  // This is actually used as a pointer on the compositor side.
  // It should always be returned exactly as is to the compositiors,
  // performing any manipulations is undefined behavior.
  final int handle;

  final int pid;
  final int gid;
  final int uid;
  final bool isPopup;
  final int parentHandle;
  final int surfaceWidth;
  final int surfaceHeight;
  final int offsetTop;
  final int offsetLeft;
  final int offsetRight;
  final int offsetBottom;
  final int contentWidth;
  final int contentHeight;
  bool isMaximized;
  bool isMinimized;

  Surface({
    required this.handle,
    required this.pid,
    required this.gid,
    required this.uid,
    required this.isPopup,
    required this.parentHandle,
    required this.surfaceWidth,
    required this.surfaceHeight,
    required this.offsetTop,
    required this.offsetLeft,
    required this.offsetRight,
    required this.offsetBottom,
    required this.contentWidth,
    required this.contentHeight,
    this.isMaximized = false,
    this.isMinimized = false,
  });

  @override
  String toString() {
    return 'Surface(handle: $handle, pid: $pid, gid: $gid, uid: $uid, isPopup: $isPopup, parentHandle: $parentHandle, isMaximized: $isMaximized, isMinimized: $isMinimized)';
  }
}

class _CompositorPlatform {
  final MethodChannel channel = const MethodChannel("wlroots");

  final HashMap<String, Future<dynamic> Function(MethodCall)> handlers =
      HashMap();

  _CompositorPlatform() {
    channel.setMethodCallHandler((call) async {
      Future<dynamic> Function(MethodCall)? handler = handlers[call.method];
      if (handler == null) {
        print("unhandled call: ${call.method}");
      } else {
        print("handled call ${call.method}");
        return await handler(call);
      }
    });
  }

  void addHandler(String method, Future<dynamic> Function(MethodCall) handler) {
    if (handlers.containsKey(method)) {
      throw Exception("attemped to add duplicate handler for $method");
    }
    handlers[method] = handler;
  }

  Future<void> surfaceToplevelSetSize(
      Surface surface, int width, int height) async {
    await channel.invokeListMethod(
        "surface_toplevel_set_size", [surface.handle, width, height]);
  }

  Future<void> clearFocus(Surface surface) async {
    await channel.invokeMethod("surface_clear_focus", [surface.handle]);
  }

  Future<void> surfaceSendKey(Surface surface, int keycode, KeyStatus status,
      Duration timestamp) async {
    await channel.invokeListMethod(
      "surface_keyboard_key",
      [
        surface.handle,
        keycode,
        status.index,
        timestamp.inMicroseconds,
      ],
    );
  }

  Future<void> surfaceFocusViewWithHandle(int handle) async {
    await channel.invokeMethod("surface_focus_from_handle", [handle]);
  }
}

class Compositor {
  static void initLogger() {
    FlutterError.onError = (FlutterErrorDetails details) {
      FlutterError.presentError(details);
      stderr.writeln(details.toString());
    };
    Logger.root.onRecord.listen((record) {
      stdout.writeln("${record.level.name}: ${record.time}: ${record.message}");
    });
  }

  _CompositorPlatform platform = _CompositorPlatform();

  HashMap<int, Surface> surfaces = HashMap();

  // Emits an event when a surface has been added and is ready to be presented on the screen.
  StreamController<Surface> surfaceMapped = StreamController.broadcast();
  StreamController<Surface> surfaceUnmapped = StreamController.broadcast();
  StreamController<WindowEvent> windowEvents = StreamController.broadcast();

  int? keyToXkb(int physicalKey) => physicalToXkbMap[physicalKey];

  Compositor() {
    platform.addHandler("surface_map", (call) async {
      Surface surface = Surface(
        handle: call.arguments["handle"],
        pid: call.arguments["client_pid"],
        gid: call.arguments["client_gid"],
        uid: call.arguments["client_uid"],
        isPopup: call.arguments["is_popup"],
        parentHandle: call.arguments["parent_handle"],
        surfaceHeight: call.arguments['surface_height'],
        surfaceWidth: call.arguments['surface_width'],
        offsetTop: call.arguments['offset_top'],
        offsetLeft: call.arguments['offset_left'],
        offsetRight: call.arguments['offset_right'],
        offsetBottom: call.arguments['offset_bottom'],
        contentWidth: call.arguments['content_width'],
        contentHeight: call.arguments['content_height'],
      );
      surfaces.putIfAbsent(surface.handle, () => surface);

      surfaceMapped.add(surface);
    });

    platform.addHandler("surface_unmap", (call) async {
      int handle = call.arguments["handle"];
      if (surfaces.containsKey(handle)) {
        Surface surface = surfaces[handle]!;
        surfaces.remove(handle);
        surfaceUnmapped.add(surface);
      }
    });

    platform.addHandler("flutter/keyevent", (call) async {});

    platform.addHandler('window_maximize', (call) async {
      int handle = call.arguments['handle'];

      windowEvents.add(WindowEvent(
          windowEventType: WindowEventType.maximize, handle: handle));
    });

    platform.addHandler('window_minimize', (call) async {
      int handle = call.arguments['handle'];

      windowEvents.add(WindowEvent(
          windowEventType: WindowEventType.minimize, handle: handle));
    });
  }
}
