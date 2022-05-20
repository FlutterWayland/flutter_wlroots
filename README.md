# flutter_wlroots

## Running demo

### Environment setup
1. Make sure the engine commit hash set in `subprojects/flutter_embedder/meson.build` matches what's set in `flutter/bin/internal/engine.version`. (Shorthash can be seen by doing `flutter doctor -v`)
2. If you changed the hash in the step above, make sure to run `make delete_downloaded_engine`

### Build embedder
```
meson build
ninja -C build
```

### Build demo app
```
make demo
```

### Run demo
Make sure you are in the root directory of the git repo. This is important.

Run:
```
build/examples/simple
```


## Run release binary:

Every command should be run from `demo/` project directory

YOU NEED TO COPY `libflutter_engine.so` from your built engine to `subprojects/flutter_embedder/`. (Compiled engine can be found in `engine/src/out/host_release` )
Instructions how to build engine from source can be found here: https://github.com/flutter/flutter/wiki/Setting-up-the-Engine-development-environment


Export build engine path:

```
export LOCAL_ENGINE=/home/charafau/Utils/engine/src/out/host_release
```

Get flutter packages for project using built engine
```
flutter --local-engine $LOCAL_ENGINE build linux --release
```

Generate .dill files

```
$LOCAL_ENGINE/dart-sdk/bin/dart $LOCAL_ENGINE/dart-sdk/bin/snapshots/frontend_server.dart.snapshot --sdk-root $LOCAL_ENGINE/flutter_patched_sdk --target=flutter --aot --tfa -Ddart.vm.product=true --packages .packages --output-dill build/kernel_snapshot.dill --verbose --depfile build/kernel_snapshot.d package:demo/main.dart
```

Generate app snapshots for release

```
$LOCAL_ENGINE/gen_snapshot --deterministic --snapshot_kind=app-aot-elf --elf=build/flutter_assets/app.so build/kernel_snapshot.dill
```

OR if you want to strip debug symbols

```
$LOCAL_ENGINE/gen_snapshot --deterministic --snapshot_kind=app-aot-elf --elf=build/flutter_assets/app.so --strip build/kernel_snapshot.dill
```


Copy `app.so` to `build/example/flutter_assets/`

Run:
```
build/examples/simple
```