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

`kat` is a work in progress and under heavy construction. Since no error handling is implemented,
the interpreter will crash on bad input. 

### changes

* v0.22   A mark & sweep garbage collector is now used for memory hadling
* v0.21   Added stdlib.scm. This file contains more function definitions written in scheme.
* v0.20   Some basic I/O functions added.
