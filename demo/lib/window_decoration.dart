import 'package:demo/constants.dart';
import 'package:flutter/material.dart';

class WindowDecoration extends StatelessWidget {
  final Widget child;

  const WindowDecoration({
    Key? key,
    required this.child,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return ClipRRect(
      borderRadius: systemBorderRadius,
      child: Column(
        children: [
          Container(
            color: Colors.red,
            height: 30,
            width: windowWidth,
          ),
          child,
        ],
      ),
    );
  }
}
