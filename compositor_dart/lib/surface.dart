import 'dart:collection';

import 'package:compositor_dart/compositor_dart.dart';
import 'package:compositor_dart/constants.dart';
import 'package:flutter/gestures.dart';
import 'package:flutter/material.dart';
import 'package:flutter/rendering.dart';
import 'package:flutter/services.dart';

typedef _OnWidgetSizeChange = void Function(Size? size);

class _MeasureSizeRenderObject extends RenderProxyBox {
  Size? oldSize;
  final _OnWidgetSizeChange onChange;

  _MeasureSizeRenderObject(this.onChange);

  @override
  void performLayout() {
    super.performLayout();

    Size? newSize = child?.size;
    if (oldSize == newSize) return;

    oldSize = newSize;
    WidgetsBinding.instance?.addPostFrameCallback((_) {
      onChange(newSize);
    });
  }
}

class _MeasureSize extends SingleChildRenderObjectWidget {
  final _OnWidgetSizeChange onChange;

  const _MeasureSize({
    Key? key,
    required this.onChange,
    required Widget child,
  }) : super(key: key, child: child);

  @override
  RenderObject createRenderObject(BuildContext context) {
    return _MeasureSizeRenderObject(onChange);
  }
}

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
    return Listener(
      onPointerSignal: controller.dispatchPointerEvent,
      child: Focus(
        onKeyEvent: (node, event) {
          final KeyStatus status;

          if (event is KeyDownEvent) {
            status = KeyStatus.pressed;
          } else {
            status = KeyStatus.released;
          }

          int? keycode = physicalToXkbMap[event.physicalKey.usbHidUsage];

          if (keycode != null) {
            compositor.platform.surfaceSendKey(
              widget.surface,
              keycode,
              status,
              event.timeStamp,
            );

            return KeyEventResult.handled;
          }

          return KeyEventResult.ignored;
        },
        child: _MeasureSize(
          onChange: (size) {
            if (size != null) {
              controller.setSize(size);
            }
          },
          child: PlatformViewSurface(
            controller: controller,
            hitTestBehavior: PlatformViewHitTestBehavior.opaque,
            gestureRecognizers: HashSet(),
          ),
        ),
      ),
    );
  }
}

class _CompositorPlatformViewController extends PlatformViewController {
  Surface surface;
  Size size = const Size(100, 100);

  _CompositorPlatformViewController({required this.surface});

  void setSize(Size size) {
    this.size = size;
    compositor.platform.surfaceToplevelSetSize(
        surface, size.width.round(), size.height.round());
  }

  @override
  Future<void> clearFocus() => compositor.platform.clearFocus(surface);

  @override
  Future<void> dispatchPointerEvent(PointerEvent event) async {
    //print("${event.toString()}");

    final int deviceKind;
    switch (event.kind) {
      case PointerDeviceKind.mouse:
        deviceKind = pointerKindMouse;
        break;
      case PointerDeviceKind.touch:
        deviceKind = pointerKindTouch;
        break;
      default:
        deviceKind = pointerKindUnknown;
        break;
    }

    final int eventType;
    Offset scrollAmount = Offset.zero;
    if (event is PointerDownEvent) {
      eventType = pointerDownEvent;
    } else if (event is PointerUpEvent) {
      eventType = pointerUpEvent;
    } else if (event is PointerHoverEvent) {
      eventType = pointerHoverEvent;
    } else if (event is PointerMoveEvent) {
      eventType = pointerMoveEvent;
    } else if (event is PointerEnterEvent) {
      eventType = pointerEnterEvent;
    } else if (event is PointerExitEvent) {
      eventType = pointerExitEvent;
    } else if (event is PointerScrollEvent) {
      eventType = pointerScrollEvent;
      scrollAmount = event.scrollDelta;
    } else {
      eventType = pointerUnknownEvent;
    }

    List data = [
      surface.handle,
      event.buttons,
      event.delta.dx,
      event.delta.dy,
      event.device,
      event.distance,
      event.down,
      event.embedderId,
      deviceKind,
      event.localDelta.dx,
      event.localDelta.dy,
      event.localPosition.dx,
      event.localPosition.dy,
      event.obscured,
      event.orientation,
      event.platformData,
      event.pointer,
      event.position.dx,
      event.position.dy,
      event.pressure,
      event.radiusMajor,
      event.radiusMinor,
      event.size,
      event.synthesized,
      event.tilt,
      event.timeStamp.inMicroseconds,
      eventType,
      size.width,
      size.height,
      scrollAmount.dx,
      scrollAmount.dy,
    ];

    //print("pointerevent $data");

    await compositor.platform.channel.invokeMethod(
      "surface_pointer_event",
      data,
    );
  }

  @override
  Future<void> dispose() async {
    // TODO: implement dispose
    //throw UnimplementedError();
  }

  @override
  int get viewId => surface.handle;
}
