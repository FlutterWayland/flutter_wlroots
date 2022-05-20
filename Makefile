.PHONY: demo

demo:
	cd demo; flutter build linux --debug
	cp -r demo/build/linux/x64/debug/bundle/data/* build/example/

delete_downloaded_engine:
	rm subprojects/flutter_embedder/flutter_embedder.h
	rm subprojects/flutter_embedder/libflutter_engine.so
	rm subprojects/flutter_embedder/linux-x64-embedder.zip

demo_release:
	cd demo; flutter build linux --release
	cp -r demo/build/linux/x64/release/bundle/data/* build/example/
