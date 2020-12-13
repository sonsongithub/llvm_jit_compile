# Build LLVM on Apple Silicon(M1)

## How bo build

### Build cmake

Maybe, we have to build and install cmake from source code.
We can build and install cmake using vanilla sources from [cmake.org](https://cmake.org).

```
tar zxf cmake-3.19.1.tar.gz
cd cmake-3.19.1
make install
```

### Build LLVM

At first, to avoid using libxml2, edit `CMakeLists.txt` as follows,

```
set(LLVM_ENABLE_LIBXML2 "OFF" CACHE STRING "Use libxml2 if available. Can be ON, OFF, or FORCE_ON")
```

Without this modification, linker can not build llvm because it can not find and link libxml.
I do not use libxml in my codes. If you want to use libxml in LLVM, I can not help you.

Next, prepare and build any files using cmake.

```
mkdir build
cd build
cmake .. $LLVM_SRC_DIR -DCMAKE_BUILD_TYPE=Release -DLLVM_TARGETS_TO_BUILD="ARM;X86;AArch64"
make -j
```

## Build sources

I do not copy any llvm files into `/usr/`, because I want to use multiple versions of LLVM switching between them.

For example, we can build source codes using LLVM as follows,

```
 /usr/bin/clang++ add.cpp -o add `<path to llvm>/build/bin/llvm-config --cxxflags --ldflags --libs --libfiles core orcjit mcjit native --system-libs`
```

## Compile speed

I compared the compile speed between Apple Silicon M1 and Intel Core i9, using `add.cpp`.

* Apple Silicon(M1) 1.93[sec]
* Intel(Core i9) 4.6[sec]

As a result, M1 was 2.4 times faster than Core i9.