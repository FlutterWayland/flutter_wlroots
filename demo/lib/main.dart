import 'package:compositor_dart/compositor_dart.dart';
import 'package:compositor_dart/surface.dart';
import 'package:demo/constants.dart';
import 'package:demo/window.dart';
import 'package:flutter/gestures.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

void main() {
  Compositor.initLogger();
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({Key? key}) : super(key: key);

  // This widget is the root of your application.
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Flutter Demo',
      theme: ThemeData(
        // This is the theme of your application.
        //
        // Try running your application with "flutter run". You'll see the
        // application has a blue toolbar. Then, without quitting the app, try
        // changing the primarySwatch below to Colors.green and then invoke
        // "hot reload" (press "r" in the console where you ran "flutter run",
        // or simply save your changes to "hot reload" in a Flutter IDE).
        // Notice that the counter didn't reset back to zero; the application
        // is not restarted.
        primarySwatch: Colors.blue,
      ),
      home: const MyHomePage(title: 'Flutter Demo Home Page'),
    );
  }
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({Key? key, required this.title}) : super(key: key);

  // This widget is the home page of your application. It is stateful, meaning
  // that it has a State object (defined below) that contains fields that affect
  // how it looks.

  // This class is the configuration for the state. It holds the values (in this
  // case the title) provided by the parent (in this case the App widget) and
  // used by the build method of the State. Fields in a Widget subclass are
  // always marked "final".

  final String title;

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  late Compositor compositor;
  Map<int, Surface> surfaces = {};

  int? focusedSurface;
  late double mousePositionX;
  late double mousePositionY;

  @override
  void initState() {
    super.initState();

    mousePositionX = 0;
    mousePositionY = 0;

    compositor = Compositor();
    compositor.surfaceMapped.stream.listen((Surface event) {
      setState(() {
        surfaces.putIfAbsent(event.handle, () => event);
        focusedSurface = event.handle;
      });
    });
    compositor.surfaceUnmapped.stream.listen((Surface event) {
      setState(() {
        surfaces.removeWhere((key, value) => key == event.handle);
        if (surfaces.isNotEmpty) {
          focusedSurface = surfaces.keys.last;
        } else {
          focusedSurface = null;
        }
      });
    });

    compositor.windowEvents.stream.listen((WindowEvent event) {
      if (event.windowEventType == WindowEventType.maximize) {
        setState(() {
          surfaces[event.handle]!.isMaximized =
              !surfaces[event.handle]!.isMaximized;
        });
      }

      if (event.windowEventType == WindowEventType.minimize) {
        setState(() {
          surfaces[event.handle]!.isMinimized =
              !surfaces[event.handle]!.isMinimized;
        });
      }
    });
  }

  void focusView(int handle) {
    if (focusedSurface != handle) {
      compositor.platform.surfaceFocusViewWithHandle(handle);
      focusedSurface = handle;
    }
  }

  @override
  Widget build(BuildContext context) {
    return Focus(
      onKeyEvent: (node, KeyEvent event) {
        int? keycode = compositor.keyToXkb(event.physicalKey.usbHidUsage);

        if (keycode != null && focusedSurface != null) {
          compositor.platform.surfaceSendKey(
              surfaces[focusedSurface]!,
              keycode,
              event is KeyDownEvent ? KeyStatus.pressed : KeyStatus.released,
              event.timeStamp);
        }
        return KeyEventResult.handled;
      },
      autofocus: true,
      child: Scaffold(
        appBar: AppBar(
          title: Text(widget.title),
        ),
        body: LayoutBuilder(builder: (context, constraints) {
          return SizedBox.expand(
            child: MouseRegion(
              onHover: (PointerHoverEvent event) {
                mousePositionX = event.position.dx;
                mousePositionY = event.position.dy;
              },
              child: Stack(
                children: surfaces.entries
                    .skipWhile((entry) => entry.value.isMinimized)
                    .map((MapEntry<int, Surface> entry) {
                  final isPopup = entry.value.isPopup;

                  return Window(
                    initialX: entry.value.isMaximized
                        ? 0
                        : (isPopup ? mousePositionX : initialPositionX),
                    initialY: entry.value.isMaximized
                        ? 0
                        : (isPopup ? mousePositionY : initialPositionY),
                    width: entry.value.isMaximized
                        ? constraints.maxWidth
                        : entry.value.prefferedWidth.toDouble(),
                    height: entry.value.isMaximized
                        ? constraints.maxHeight
                        : entry.value.prefferedHeight.toDouble(),
                    shouldDecorate: !isPopup,
                    isMaximized: entry.value.isMaximized,
                    onTap: () => focusView(entry.key),
                    child: SizedBox(
                      width: entry.value.isMaximized
                          ? constraints.maxWidth
                          : (entry.value.prefferedWidth != 0
                              ? entry.value.prefferedWidth.toDouble()
                              : 200),
                      height: entry.value.isMaximized
                          ? (constraints.maxHeight - windowDecorationHeight)
                          : (entry.value.prefferedHeight != 0
                              ? entry.value.prefferedHeight.toDouble()
                              : 300),
                      child: SurfaceView(
                          surface: entry.value,
                          compositor: compositor,
                          onPointerClick: (Surface surface) {
                            focusView(surface.handle);
                          }),
                    ),
                  );
                }).toList(),
              ),
            ),
          );
        }),
      ),
    );
  }
}
