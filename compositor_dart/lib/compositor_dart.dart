library compositor_dart;

import 'dart:async';
import 'dart:collection';
import 'dart:io';

import 'package:compositor_dart/constants.dart';
import 'package:flutter/services.dart';
import 'package:flutter/widgets.dart';
import 'package:logging/logging.dart';

enum KeyStatus { released, pressed }

class Surface {
  // This is actually used as a pointer on the compositor side.
  // It should always be returned exactly as is to the compositiors,
  // performing any manipulations is undefined behavior.
  final int handle;

  final int pid;
  final int gid;
  final int uid;

  final Compositor compositor;

  Surface({
    required this.handle,
    required this.pid,
    required this.gid,
    required this.uid,
    required this.compositor,
  });
}

class CompositorSockets {
  CompositorSockets({required this.wayland, required this.x});
  final String wayland;
  final String x;
}

class _CompositorPlatform {
  final MethodChannel channel = const MethodChannel("wlroots");

  final HashMap<String, Future<dynamic> Function(MethodCall)> handlers = HashMap();

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

  Future<void> surfaceToplevelSetSize(Surface surface, int width, int height) async {
    await channel.invokeListMethod("surface_toplevel_set_size", [surface.handle, width, height]);
  }

  Future<void> clearFocus(Surface surface) async {
    await channel.invokeMethod("surface_clear_focus", [surface.handle]);
  }

  Future<void> surfaceSendKey(Surface surface, int keycode, KeyStatus status, Duration timestamp) async {
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

  Future<CompositorSockets> getSocketPaths() async {
    var response = await channel.invokeMethod("get_socket_paths") as Map<dynamic, dynamic>;
    return CompositorSockets(
      wayland: response["wayland"] as String,
      x: response["x"] as String,
    );
  }
}

class Compositor {
  static final Compositor compositor = Compositor();

  static void initLogger() {
    FlutterError.onError = (FlutterErrorDetails details) {
      FlutterError.presentError(details);
      stderr.writeln(details.toString());
    };
    Logger.root.onRecord.listen((record) {
      stdout.writeln("${record.level.name}: ${record.time}: ${record.message}");
    });
  }

  static bool? _isCompositor;

  _CompositorPlatform platform = _CompositorPlatform();

  HashMap<int, Surface> surfaces = HashMap();

  // Emits an event when a surface has been added and is ready to be presented on the screen.
  StreamController<Surface> surfaceMapped = StreamController.broadcast();
  StreamController<Surface> surfaceUnmapped = StreamController.broadcast();

  int? keyToXkb(int physicalKey) => physicalToXkbMap[physicalKey];

  Compositor() {
    platform.addHandler("surface_map", (call) async {
      Surface surface = Surface(
        handle: call.arguments["handle"],
        pid: call.arguments["client_pid"],
        gid: call.arguments["client_gid"],
        uid: call.arguments["client_uid"],
        compositor: this,
      );
      surfaces[surface.handle] = surface;
      surfaceMapped.add(surface);
    });

    platform.addHandler("surface_unmap", (call) async {
      int handle = call.arguments["handle"];
      Surface surface = surfaces[handle]!;
      surfaces.remove(handle);
      surfaceUnmapped.add(surface);
    });

    platform.addHandler("flutter/keyevent", (call) async {});
  }

  /// Returns `true` if we are currently running in the compositor embedder.
  /// If so, all functionality in this library is available.
  ///
  /// Returns `false` in all other cases. If so, no funcitonality in this
  /// library should be used.
  Future<bool> isCompositor() async {
    if (_isCompositor != null) return _isCompositor!;

    try {
      await platform.channel.invokeMethod("is_compositor");
      _isCompositor = true;
    } on MissingPluginException {
      _isCompositor = false;
    }

    return _isCompositor!;
  }

  /// Will return the paths of the compositor sockets.
  Future<CompositorSockets> getSocketPaths() => platform.getSocketPaths();
}
