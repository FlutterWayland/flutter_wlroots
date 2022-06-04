import 'package:compositor_dart/compositor_dart.dart';
import 'package:compositor_dart/surface.dart';
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

  @override
  void initState() {
    super.initState();
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
    // This method is rerun every time setState is called, for instance as done
    // by the _incrementCounter method above.
    //
    // The Flutter framework has been optimized to make rerunning build methods
    // fast, so that you can just rebuild anything that needs updating rather
    // than having to individually change instances of widgets.
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
          // Here we take the value from the MyHomePage object that was created by
          // the App.build method, and use it to set our appbar title.
          title: Text(widget.title),
        ),
        body: Center(
          // Center is a layout widget. It takes a single child and positions it
          // in the middle of the parent.
          child: Row(
            // Column is also a layout widget. It takes a list of children and
            // arranges them vertically. By default, it sizes itself to fit its
            // children horizontally, and tries to be as tall as its parent.
            //
            // Invoke "debug painting" (press "p" in the console, choose the
            // "Toggle Debug Paint" action from the Flutter Inspector in Android
            // Studio, or the "Toggle Debug Paint" command in Visual Studio Code)
            // to see the wireframe for each widget.
            //
            // Column has various properties to control how it sizes itself and
            // how it positions its children. Here we use mainAxisAlignment to
            // center the children vertically; the main axis here is the vertical
            // axis because Columns are vertical (the cross axis would be
            // horizontal).
            mainAxisAlignment: MainAxisAlignment.center,
            children: surfaces.entries.map((MapEntry<int, Surface> entry) {
              return GestureDetector(
                onTap: () {
                  focusView(entry.key);
                },
                child: SizedBox(
                  width: 400,
                  height: 500,
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
    );
  }
}
