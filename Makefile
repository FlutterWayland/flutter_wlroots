.PHONY: demo

demo:
	cd demo; flutter build linux --debug
	cp -r demo/build/linux/x64/debug/bundle/data/* build/example/
