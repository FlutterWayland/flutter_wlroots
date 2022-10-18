import 'package:compositor_dart/compositor_dart.dart';
import 'package:flutter/material.dart';
import 'package:flutter/rendering.dart';

/// Renders a compositor surface.
class CompositorSurfaceRenderWidget extends LeafRenderObjectWidget {
  final Surface surface;

  const CompositorSurfaceRenderWidget({
    Key? key,
    required this.surface,
  }) : super(key: key);

  @override
  RenderObject createRenderObject(BuildContext context) {
    return CompositorSurfaceRenderBox(viewId: surface.handle);
  }

  @override
  void updateRenderObject(BuildContext context, CompositorSurfaceRenderBox renderObject) {
    renderObject.viewId = surface.handle;
  }

}

class CompositorSurfaceRenderBox extends RenderBox {
  int viewId;

  CompositorSurfaceRenderBox({required this.viewId});

  @override
  bool get sizedByParent => true;

  @override
  bool get alwaysNeedsCompositing => true;

  @override
  bool get isRepaintBoundary => true;

  @override
  Size computeDryLayout(BoxConstraints constraints) {
    return constraints.biggest;
  }

  @override
  void paint(PaintingContext context, Offset offset) {
    context.addLayer(PlatformViewLayer(
      rect: offset & size, 
      viewId: viewId,
    ));
  }

  @override
  void describeSemanticsConfiguration(SemanticsConfiguration config) {
    super.describeSemanticsConfiguration(config);
    config.isSemanticBoundary = true;
    config.platformViewId = viewId;
  }
}