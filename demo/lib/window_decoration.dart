import 'package:demo/constants.dart';
import 'package:flutter/material.dart';

class WindowDecoration extends StatelessWidget {
  final Widget child;
  final double width;

  const WindowDecoration({
    Key? key,
    required this.child,
    required this.width,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return ClipRRect(
      borderRadius: systemBorderRadius,
      child: Column(
        children: [
          Container(
            color: windowDecorationColor,
            height: windowDecorationHeight,
            width: width,
          ),
          child,
        ],
      ),
    );
  }
}
