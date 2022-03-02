import 'dart:collection';

import 'package:compositor_dart/compositor_dart.dart';
import 'package:flutter/material.dart';
import 'package:flutter/rendering.dart';
import 'package:flutter/services.dart';

class SurfaceView extends StatefulWidget {
  final Surface surface;

  const SurfaceView({Key? key, required this.surface}) : super(key: key);

  @override
  State<StatefulWidget> createState() {
    return _SurfaceViewState();
  }
}

class _SurfaceViewState extends State<SurfaceView> {
  late _CompositorPlatformViewController controller;

  @override
  void initState() {
    super.initState();
    controller = _CompositorPlatformViewController(surface: widget.surface);
  }

  @override
  void didUpdateWidget(SurfaceView oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.surface != widget.surface) {
      controller.dispose();
      controller = _CompositorPlatformViewController(surface: widget.surface);
    }
  }

  @override
  Widget build(BuildContext context) {
    return PlatformViewSurface(
      controller: controller, 
      hitTestBehavior: PlatformViewHitTestBehavior.opaque, 
      gestureRecognizers: HashSet(),
    );
  }

}

class _CompositorPlatformViewController extends PlatformViewController {
  Surface surface;

  _CompositorPlatformViewController({required this.surface});

  @override
  Future<void> clearFocus() => compositor.platform.clearFocus(surface);

  @override
  Future<void> dispatchPointerEvent(PointerEvent event) async {
    // TODO: implement dispatchPointerEvent
    //throw UnimplementedError();
  }

  @override
  Future<void> dispose() async {
    // TODO: implement dispose
    //throw UnimplementedError();
  }

  @override
  int get viewId => surface.handle;

}