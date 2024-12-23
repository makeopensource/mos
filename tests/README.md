# Testing

Testing is an integral part of making any large scale project.  
As such we have created an automated testing environment!  

## Building

Building can be done in two ways. Indirectly through `make test`,
or directly through `make` in the `tests` directory.  
  
`make clean` in either Makefile will clean test files

## Running

Running can be done through `make test`.  
  
There is also an option to only run selected tests.
To do so run `make test TESTS="..."` replacing `...` with `.bin` files.  
For example: `make test TESTS="ring_buffer.bin example.bin"`  
Alternatively: `make test TESTS=ring_buffer.bin`  
The default behavior without TESTS is to test all the files.  
This can be invoked from either makefile.

## Creating Tests

Creating tests requires creating at least two new files; A `.c` file anywhere in `tests/src` and a `.expect` of the same name (without the .c) anywhere in the `tests/expected`.  

Make sure you add the file you are testing for the the main Makefile as a `.o` dependency. You can include files relatively from `/src/lib` from both the testing `.c` and the `.c` you're testing for.  
  
The `.c` file can include any OS header and `tests/src/test_helper.h` is recommended. `ASSERT(condition)` in the helper will produce no output on success but will write to serial on failure. The `.c` must have some serial output to compare against the expected. The `.c` file MUST have `void test_main` instead of `int main`.  
  
The `.expect` file consists of all the output the `.c` file will produce when everything works as expected. Output is always in the form of serial output. `.got` files will be produced next to the `.expect`. `.got` files contain what the OS sent back to the testing environment.  

## Dependencies

Testing requires the same dependencies as normal operation along with the following:

* Python 3.8
  * Python venv
