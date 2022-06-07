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
        body: SizedBox.expand(
          child: MouseRegion(
            onHover: (PointerHoverEvent event) {
              mousePositionX = event.position.dx;
              mousePositionY = event.position.dy;
            },
            child: Stack(
              children: surfaces.entries.map((MapEntry<int, Surface> entry) {
                final isPopup = entry.value.isPopup;

                return Window(
                  initialX: isPopup ? mousePositionX : initialPositionX,
                  initialY: isPopup ? mousePositionY : initialPositionY,
                  shouldDecorate: !isPopup,
                  onTap: () => focusView(entry.key),
                  child: SizedBox(
                    width: windowWidth,
                    height: windowHeight,
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
        ),
      ),
    );
  }
}
