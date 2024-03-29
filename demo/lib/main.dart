import 'dart:io';

import 'package:compositor_dart/compositor_dart.dart';
import 'package:compositor_dart/keyboard/keyboard_client_controller.dart';
import 'package:compositor_dart/keyboard/platform_keyboard.dart';
import 'package:compositor_dart/platform/interceptor_widgets_binding.dart';
import 'package:compositor_dart/surface.dart';
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
  int _counter = 0;
  Compositor compositor = Compositor();
  Surface? surface;

  _MyHomePageState() {
    compositor.surfaceMapped.stream.listen((event) {
      setState(() {
        surface = event;
      });
    });
    compositor.surfaceUnmapped.stream.listen((event) {
      if (surface == event) {
        setState(() {
          surface = null;
        });
      }
    });
  }

  void _incrementCounter() {
    setState(() {
      // This call to setState tells the Flutter framework that something has
      // changed in this State, which causes it to rerun the build method below
      // so that the display can reflect the updated values. If we changed
      // _counter without calling setState(), then the build method would not be
      // called again, and so nothing would appear to happen.
      _counter++;
    });
  }

  @override
  Widget build(BuildContext context) {
    Widget? surfaceView;
    if (surface != null) {
      surfaceView = SurfaceView(
        surface: surface!,
      );
    }

    // This method is rerun every time setState is called, for instance as done
    // by the _incrementCounter method above.
    //
    // The Flutter framework has been optimized to make rerunning build methods
    // fast, so that you can just rebuild anything that needs updating rather
    // than having to individually change instances of widgets.
    return Focus(
      onKeyEvent: (node, KeyEvent event) {
        int? keycode = compositor.keyToXkb(event.physicalKey.usbHidUsage);

        if (keycode != null && surface != null) {
          compositor.platform.surfaceSendKey(
              surface!, keycode, event is KeyDownEvent ? KeyStatus.pressed : KeyStatus.released, event.timeStamp);
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
          child: Column(
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
            children: <Widget>[
              const Text(
                'You have pushed the button this many timesa:',
              ),
              Text(
                '$_counter',
                style: Theme.of(context).textTheme.headline4,
              ),
              Container(
                color: Colors.amber,
                padding: const EdgeInsets.all(8.0),
                child: SizedBox(width: 500, height: 500, child: surfaceView),
              ),
              TestSlider(),
              const TextField(),
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
