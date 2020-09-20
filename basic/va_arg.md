# Original C++ code

```
int sum_i(int amount, ...) {
    int i = 0;
    int val = 0;
    int sum = 0;

    va_list vl;

    va_start(vl, amount);

    for (i = 0; i < amount; i++) {
        val = va_arg(vl, int);
        sum += val;
    }
    va_end(vl);

    return sum; 
}
```

# LLVM IR code emitted by clang.

```
clang -c -S -O0 -emit-llvm hoge.cpp
```

```
%struct.__va_list_tag = type { i32, i32, i8*, i8* }

; Function Attrs: noinline optnone ssp uwtable
define i32 @_Z5sum_iiz(i32, ...) #0 {
  %2 = alloca i32, align 4
  ; i, counter for "for" loop.
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  ; sum?
  %5 = alloca i32, align 4
  %6 = alloca [1 x %struct.__va_list_tag], align 16
  store i32 %0, i32* %2, align 4
  store i32 0, i32* %3, align 4
  store i32 0, i32* %4, align 4
  ; sum = 0?
  store i32 0, i32* %5, align 4

  ; Return a pointer to the head of va_list.
  ; It casts the pointer to va_list to %8.
  %7 = getelementptr inbounds [1 x %struct.__va_list_tag], [1 x %struct.__va_list_tag]* %6, i64 0, i64 0
  %8 = bitcast %struct.__va_list_tag* %7 to i8*

  ; call va_start.
  call void @llvm.va_start(i8* %8)
  call void @_Z7salvagePA1_13__va_list_tag([1 x %struct.__va_list_tag]* %6)

  ; initialize %3
  store i32 0, i32* %3, align 4
  br label %9

9:                                                ; preds = %35, %1
  ; %2 ... one of the arguments of this function.
  %10 = load i32, i32* %3, align 4
  %11 = load i32, i32* %2, align 4

  ; compare counter with the first argument.
  ; for (i = 0; i < amount; i++) {

  %12 = icmp slt i32 %10, %11
  br i1 %12, label %13, label %38

13: 
  ; It writes a pointer to the head of va_list to %14.
  %14 = getelementptr inbounds [1 x %struct.__va_list_tag], [1 x %struct.__va_list_tag]* %6, i64 0, i64 0

  ; It casts the pointer to va_list to %15 as (char*).
  %15 = getelementptr inbounds %struct.__va_list_tag, %struct.__va_list_tag* %14, i32 0, i32 0
  ; It loads the value of %15 to %15 register.
  %16 = load i32, i32* %15, align 16

  ; It compares %16 with 40.
  ; I think that clang stores up to 40 bytes of arguments in the register and more in the stack.
  ; https://llvm.org/docs/LangRef.html#icmp-instruction
  ; ule: unsigned less or equal
  %17 = icmp ule i32 %16, 40
  br i1 %17, label %18, label %24

18:                                               ; preds = %13
  ; It returns the pointer to the third element of va_list at the pointer pointed to by %14.
  %19 = getelementptr inbounds %struct.__va_list_tag, %struct.__va_list_tag* %14, i32 0, i32 3
  ; It writes the pointer at %19 to %20.
  %20 = load i8*, i8** %19, align 16

  ; Returns %21 the pointer offset by %16 from the pointer indicated by %20.
  %21 = getelementptr i8, i8* %20, i32 %16
  ; Casts %21 to %22 as (int*).
  %22 = bitcast i8* %21 to i32*

  ; Updates __va_list_tag.
  ; Adds 8 as offset to %16.
  %23 = add i32 %16, 8
  ; Stored the value of %24 to %15
  store i32 %23, i32* %15, align 16
  br label %29

24:                                               ; preds = %13
  ; It returns the pointer to the second element of va_list at the pointer pointed to by %14.
  %25 = getelementptr inbounds %struct.__va_list_tag, %struct.__va_list_tag* %14, i32 0, i32 2
  ; It load the value from %25 to %26.
  %26 = load i8*, i8** %25, align 8
  ; Casts %26 to %27 as (int*).
  %27 = bitcast i8* %26 to i32*

  ; Updates __va_list_tag.
  %28 = getelementptr i8, i8* %26, i32 8
  store i8* %28, i8** %25, align 8
  br label %29

29:                                               ; preds = %24, %18
  ; get value from pi node.
  %30 = phi i32* [ %22, %18 ], [ %27, %24 ]

  ; Main process.
  %31 = load i32, i32* %30, align 4
  store i32 %31, i32* %4, align 4
  %32 = load i32, i32* %4, align 4
  %33 = load i32, i32* %5, align 4
  ; add "No Signed Wrap"
  ; Add %32 to %33
  %34 = add nsw i32 %33, %32

  ; Summation
  ; Save the result of the above addition.
  store i32 %34, i32* %5, align 4
  br label %35

35:                                               ; preds = %29
  ; Updates the loop counter.
  %36 = load i32, i32* %3, align 4
  ; Increments the counter.
  %37 = add nsw i32 %36, 1
  ; Stores the counter to %3.
  store i32 %37, i32* %3, align 4
  ; Back to %9 label.
  br label %9

38:                                               ; preds = %9
  ; It writes a pointer to the head of va_list to %16.
  %39 = getelementptr inbounds [1 x %struct.__va_list_tag], [1 x %struct.__va_list_tag]* %6, i64 0, i64 0
  ; Casts it to (char*)
  %40 = bitcast %struct.__va_list_tag* %39 to i8*
  ; Call va_end to end.
  call void @llvm.va_end(i8* %40)
  ; Returns the result of summation.
  %41 = load i32, i32* %5, align 4
  ret i32 %41
}
```