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
