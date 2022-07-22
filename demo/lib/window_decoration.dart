import 'package:demo/constants.dart';
import 'package:flutter/material.dart';

class WindowDecoration extends StatelessWidget {
  final Widget child;
  final double width;
  final double height;

  const WindowDecoration({
    Key? key,
    required this.child,
    required this.width,
    required this.height,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return ClipRRect(
      borderRadius: systemBorderRadius,
      child: Stack(
        children: [
          Container(
            color: windowDecorationColor,
            height: height,
            width: width,
          ),
          Padding(
            padding: const EdgeInsets.only(top: windowDecorationHeight),
            child: child,
          ),
        ],
      ),
    );
  }
}
