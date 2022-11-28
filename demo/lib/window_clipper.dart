import 'package:flutter/widgets.dart';

class WindowClipper extends CustomClipper<Rect> {
  final int offsetLeft;
  final int offsetTop;
  final int contentWidth;
  final int contentHeight;

  WindowClipper({
    required this.offsetLeft,
    required this.offsetTop,
    required this.contentWidth,
    required this.contentHeight,
  });

  @override
  Rect getClip(Size size) {
    return Rect.fromLTRB(
        offsetLeft.toDouble(),
        offsetTop.toDouble(),
        contentWidth.toDouble() + offsetLeft,
        contentHeight.toDouble() + offsetTop);
  }

  @override
  bool shouldReclip(covariant CustomClipper<Rect> oldClipper) {
    return true;
  }
}
