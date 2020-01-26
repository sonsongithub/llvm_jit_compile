# llvm_jit_compile
Try to "Just In Time" compile using LLVM.

# Requirement

LLVM, you can install it mac brew. https://llvm.org

# How compile

You have to specify include and library paths when compiling and linking.
It's easy to use `llvm-config`.

## Example

```
LLVM_CONFIG=/usr/local/opt/llvm/bin/llvm-config
clang++ -c ./main.cpp -o ./main.o `${LLVM_CONFIG} --cxxflags`
clang++ -o ./a.out ./main.o `${LLVM_CONFIG} --ldflags --libs --libfiles --system-libs`
./a.out
```

## Visual Studio code

```
"args": [
    "-g",
    "${file}",
    "-o",
    "${fileDirname}/${fileBasenameNoExtension}",
    "`/usr/local/opt/llvm/bin/llvm-config", "--cxxflags`",
    "`/usr/local/opt/llvm/bin/llvm-config", "--ldflags", "--libs", "--libfiles", "--system-libs`"
],
```