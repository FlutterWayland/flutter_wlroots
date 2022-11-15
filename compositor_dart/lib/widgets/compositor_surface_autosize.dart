import 'package:compositor_dart/compositor_dart.dart';
import 'package:flutter/rendering.dart';
import 'package:flutter/widgets.dart';

class CompositorSurfaceAutosizeWidget extends StatelessWidget {
  final Widget child;
  final Surface surface;

  const CompositorSurfaceAutosizeWidget({Key? key, required this.child, required this.surface}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return _MeasureSize(
      onChange: (size) {
        if (size != null) {
          Compositor.compositor.platform.surfaceToplevelSetSize(surface, size.width.round(), size.height.round());
        }
      },
      child: child,
    );
  }
}

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
