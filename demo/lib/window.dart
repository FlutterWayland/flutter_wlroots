import 'package:demo/constants.dart';
import 'package:demo/window_decoration.dart';
import 'package:flutter/material.dart';

class Window extends StatefulWidget {
  final double initialX;
  final double initialY;
  final Widget child;
  final VoidCallback onTap;
  final bool shouldDecorate;
  final double width;
  final double height;
  final bool isMaximized;

  const Window({
    Key? key,
    required this.initialX,
    required this.initialY,
    required this.child,
    required this.onTap,
    this.shouldDecorate = true,
    required this.width,
    required this.height,
    required this.isMaximized,
  }) : super(key: key);

  @override
  State<Window> createState() => _WindowState();
}

class _WindowState extends State<Window> {
  late double initialX;
  late double initialY;
  late double windowWidth;
  late double windowHeight;
  late bool isResizingLeft;
  late bool isResizingRight;
  late bool isResizingTop;
  late bool isResizingBottom;

  @override
  void initState() {
    super.initState();
    initialX = widget.initialX;
    initialY = widget.initialY;
    if (widget.shouldDecorate) {
      windowWidth = widget.width + borderWidth * 2;
      windowHeight = widget.height + borderWidth + windowDecorationHeight;
    } else {
      windowWidth = widget.width + borderWidth * 2;
      windowHeight = widget.height + borderWidth;
    }
    isResizingLeft = false;
    isResizingRight = false;
    isResizingTop = false;
    isResizingBottom = false;
  }

  @override
  Widget build(BuildContext context) {
    final child = widget.shouldDecorate
        ? WindowDecoration(
            width: windowWidth + borderWidth * 2,
            height: windowHeight + borderWidth + windowDecorationHeight,
            child: widget.child,
          )
        : widget.child;

    return Positioned(
      left: widget.isMaximized ? 0 : initialX,
      top: widget.isMaximized ? 0 : initialY,
      child: GestureDetector(
        onTap: widget.onTap,
        onPanStart: (details) {
          if (details.localPosition.dx <= 10) {
            isResizingLeft = true;
          }
          if (details.localPosition.dx >= windowWidth - 10) {
            isResizingRight = true;
          }
          if (details.localPosition.dy <= 10) {
            isResizingTop = true;
          }
          if (details.localPosition.dy >= windowHeight - 10) {
            isResizingBottom = true;
          }
          setState(() {});
        },
        onPanUpdate: (DragUpdateDetails details) {
          setState(() {
            if (isResizingLeft) {
              initialX += details.delta.dx;
              windowWidth += (-details.delta.dx);
            }
            if (isResizingRight) {
              windowWidth += details.delta.dx;
            }
            if (isResizingTop) {
              initialY += details.delta.dy;
              windowHeight += (-details.delta.dy);
            }
            if (isResizingBottom) {
              windowHeight += details.delta.dy;
            }

            if (!isResizingLeft &&
                !isResizingRight &&
                !isResizingTop &&
                !isResizingBottom) {
              initialX += details.delta.dx;
              initialY += details.delta.dy;
            }
          });
        },
        onPanEnd: (details) {
          setState(() {
            isResizingLeft = false;
            isResizingRight = false;
            isResizingTop = false;
            isResizingBottom = false;
          });
        },
        child: SizedBox(
          width: windowWidth,
          height: windowHeight,
          child: child,
        ),
      ),
    );
  }
}
