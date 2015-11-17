## `kat`

`kat` is a basic scheme interpreter written in `C++`. In order to build `kat` from sources, the
following are required:

* cmake

If all requirements are met, perform the following steps:

	mkdir build
	cd build
	cmake -DCMAKE_BUILD_TYPE=Release -G Ninja ../
	ninja

You can run kat by typing:

	./kat

`kat` is a work in progress and under heavy construction.

### changes

* v0.8.1. Refactored current functionality to different classes. No major changes.
