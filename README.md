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

and then you can type the well known Y-combinator to get yourself started :)

    (define Y
      (lambda (f)
        ((lambda (x) (f (lambda (y) ((x x) y))))
         (lambda (x) (f (lambda (y) ((x x) y)))))))

    (define factorial
      (Y (lambda (fact)
           (lambda (n)
             (if (= n 0)
               1
               (* n (fact (- n 1))))))))

    (write (factorial 5))

`kat` is a work in progress and under heavy construction. Since no error handling is implemented,
the interpreter will crash on bad input. 

The following functions are implemented:

* `null?`
* `boolean?`
* `symbol?`
* `integer?`
* `char?`
* `string?`
* `pair?`
* `procedure?`
* `char->integer`
* `integer->char`
* `number->string`
* `string->number`
* `symbol->string`
* `string->symbol`
* `+`
* `-`
* `*`
* `quotient`
* `remainder`
* `=`
* `<`
* `>`
* `cons`
* `car`
* `cdr`
* `set-car!`
* `set-cdr!`
* `list`
* `eq?`
* `apply`
* `interaction-environment`
* `null-environment`
* `environment`
* `eval`
* `load`
* `open-input-port`
* `close-input-port`
* `input-port?`
* `open-output-port`
* `close-output-port`
* `output-port?`
* `read`
* `read-char`
* `peek-char`
* `write`
* `write-char`
* `eof-object?`
* `error`

### changes

* v0.23   A very simple object pooling strategy is implemented. The number of memory allocations
          decreased dramatically.
* v0.22   A mark & sweep garbage collector is now used for memory hadling
* v0.21   Added stdlib.scm. This file contains more function definitions written in scheme.
* v0.20   Some basic I/O functions added.
