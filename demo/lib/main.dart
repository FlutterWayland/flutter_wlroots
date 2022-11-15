import 'package:compositor_dart/compositor_dart.dart';
import 'package:compositor_dart/keyboard/keyboard_client_controller.dart';
import 'package:compositor_dart/keyboard/platform_keyboard.dart';
import 'package:compositor_dart/platform/interceptor_widgets_binding.dart';
import 'package:compositor_dart/surface.dart';
import 'package:demo/constants.dart';
import 'package:demo/window.dart';
import 'package:demo/window_clipper.dart';
import 'package:flutter/gestures.dart';
import 'package:flutter/material.dart';
import 'package:flutter/scheduler.dart';
import 'package:flutter/services.dart';

void main() {
  Compositor.initLogger();
  InterceptorWidgetsBinding.runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({Key? key}) : super(key: key);

  @override
  State<StatefulWidget> createState() {
    return _MyAppState();
  }
}

class KeyboardTest extends StatelessWidget {
  final Function(String) onPressed;

  const KeyboardTest({Key? key, required this.onPressed}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return Directionality(
      textDirection: TextDirection.ltr,
      child: SizedBox(
        height: 500,
        child: Column(
          children: [
            TextButton(
              onPressed: () {
                onPressed("a");
              },
              child: const Text("a"),
            ),
            TextButton(
              onPressed: () {
                onPressed("b");
              },
              child: const Text("b"),
            ),
            TextButton(
              onPressed: () {
                onPressed("c");
              },
              child: const Text("c"),
            ),
          ],
        ),
      ),
    );
  }
}

class _MyAppState extends State<MyApp> {
  _MyAppState();

  KeyboardClientController? keyboardClient;
  bool shown = false;

  // This widget is the root of your application.
  @override
  Widget build(BuildContext context) {
    return PlatformKeyboardWidget(
      callbacks: PlatformKeyboardCallbacks(
        setClient: (client) {
          print("setclient");
          SchedulerBinding.instance!.addPostFrameCallback((duration) {
            setState(() {
              keyboardClient = client;
              print(client?.inputConfiguration);
            });
          });
        },
        setShown: (shown) {
          SchedulerBinding.instance!.addPostFrameCallback((duration) {
            setState(() {
              this.shown = shown;
            });
          });
        },
      ),
      child: Stack(
        textDirection: TextDirection.ltr,
        children: [
          MaterialApp(
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
          ),
          KeyboardTest(onPressed: (val) {
            keyboardClient?.addText(val);
          }),
        ],
      ),
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
              surface!, keycode, event is KeyDownEvent ? KeyStatus.pressed : KeyStatus.released, event.timeStamp);
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
                        : entry.value.contentWidth.toDouble(),
                    height: entry.value.isMaximized
                        ? constraints.maxHeight
                        : entry.value.contentHeight.toDouble(),
                    shouldDecorate: !isPopup,
                    isMaximized: entry.value.isMaximized,
                    onTap: () => focusView(entry.key),
                    child: SurfaceView(
                      surface: entry.value,
                      compositor: compositor,
                      onPointerClick: (Surface surface) {
                        focusView(surface.handle);
                      },
                    ),
                  );
                }).toList(),
              ),
              Container(
                color: Colors.amber,
                padding: const EdgeInsets.all(8.0),
                child: SizedBox(width: 500, height: 500, child: surfaceView),
              ),
            ],
          ),
        ),
        floatingActionButton: FloatingActionButton(
          onPressed: _incrementCounter,
          tooltip: 'Increment',
          child: const Icon(Icons.add),
        ), // This trailing comma makes auto-formatting nicer for build methods.
      ),
    );
  }
}

class TestSlider extends StatefulWidget {
  @override
  State<StatefulWidget> createState() {
    return TestSliderState();
  }
}

class TestSliderState extends State<TestSlider> {
  double state = 0.5;

  @override
  Widget build(BuildContext context) {
    return Slider(
      value: state,
      onChanged: (n) => setState(() {
        state = n;
      }),
    );
  }
}
