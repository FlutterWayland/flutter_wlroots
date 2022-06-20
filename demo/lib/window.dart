import 'package:demo/window_decoration.dart';
import 'package:flutter/material.dart';

class Window extends StatefulWidget {
  final double initialX;
  final double initialY;
  final Widget child;
  final VoidCallback onTap;
  final bool shouldDecorate;
  final double width;

  const Window({
    Key? key,
    required this.initialX,
    required this.initialY,
    required this.child,
    required this.onTap,
    this.shouldDecorate = true,
    required this.width,
  }) : super(key: key);

  @override
  State<Window> createState() => _WindowState();
}

class _WindowState extends State<Window> {
  late double initialX;
  late double initialY;

  @override
  void initState() {
    super.initState();
    initialX = widget.initialX;
    initialY = widget.initialY;
  }

  @override
  Widget build(BuildContext context) {
    return Positioned(
      left: initialX,
      top: initialY,
      child: GestureDetector(
        onTap: widget.onTap,
        onPanUpdate: (DragUpdateDetails details) {
          setState(() {
            initialX += details.delta.dx;
            initialY += details.delta.dy;
          });
        },
        child: widget.shouldDecorate
            ? WindowDecoration(
                width: widget.width,
                child: widget.child,
              )
            : widget.child,
      ),
    );
  }
}
