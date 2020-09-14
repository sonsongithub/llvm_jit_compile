va_arg.md

# 元のC++のソースコード

```
int sum_i(int amount, ...) {
    int i = 0;
    int val = 0;
    int sum = 0;

    va_list vl;

    va_start(vl, amount);

    salvage(&vl);

    for (i = 0; i < amount; i++) {
        val = va_arg(vl, int);
        sum += val;
    }
    va_end(vl);

    return sum; 
}
```

# LLVM IR ソースコード

```
clang -c -S -O0 -emit-llvm hoge.cpp
```

```
%struct.__va_list_tag = type { i32, i32, i8*, i8* }

; Function Attrs: noinline optnone ssp uwtable
define i32 @_Z5sum_iiz(i32, ...) #0 {
  %2 = alloca i32, align 4
  ; i, for文のカウンタ
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

  ; %6の頭から，va_listの頭のポインタを%7に返す
  ; %8に，va_list型のポインタを char*型でキャストする
  %7 = getelementptr inbounds [1 x %struct.__va_list_tag], [1 x %struct.__va_list_tag]* %6, i64 0, i64 0
  %8 = bitcast %struct.__va_list_tag* %7 to i8*

  ; va_startをcall
  call void @llvm.va_start(i8* %8)
  call void @_Z7salvagePA1_13__va_list_tag([1 x %struct.__va_list_tag]* %6)

  ; %3を初期化
  store i32 0, i32* %3, align 4
  br label %9

9:                                                ; preds = %35, %1
  ; %2 一つ目の引数の値
  ; %3 最初は０が入る・・・・int iかな
  %10 = load i32, i32* %3, align 4
  %11 = load i32, i32* %2, align 4

  ; それらを比較する
  ; for (i = 0; i < amount; i++) {に該当する

  %12 = icmp slt i32 %10, %11
  br i1 %12, label %13, label %38

13: 
  ; %6の頭から，va_listの頭のポインタを%14に返す
  %14 = getelementptr inbounds [1 x %struct.__va_list_tag], [1 x %struct.__va_list_tag]* %6, i64 0, i64 0

  ; %14の頭から，char *のポインタを%15に返す
  %15 = getelementptr inbounds %struct.__va_list_tag, %struct.__va_list_tag* %14, i32 0, i32 0
  ; %15の差すポインタから，%16に値を読み出す
  %16 = load i32, i32* %15, align 16

  ; %16と40を比較する
  ; どうやら，４０はなんらかのオフセットポインタの値らしい
  ; https://llvm.org/docs/LangRef.html#icmp-instruction
  ; ule: unsigned less or equal
  %17 = icmp ule i32 %16, 40
  br i1 %17, label %18, label %24

18:                                               ; preds = %13
  ; i32 3?
  ; %14からポインタ３つ分ずらしたところにある値を読み取る
  ; %19に値を返す
  %19 = getelementptr inbounds %struct.__va_list_tag, %struct.__va_list_tag* %14, i32 0, i32 3
  ; %19にあるポインタ情報を%20に読み出し，%20に書き込む
  %20 = load i8*, i8** %19, align 16
  ; %20の示すポインタから，i32の%16個分オフセットを取ったポインタを%21かえす
  %21 = getelementptr i8, i8* %20, i32 %16
  ; %21のi8*のポインタを，i32*にキャストし，%22に返す
  %22 = bitcast i8* %21 to i32*

  ; %16に8をたす
  %23 = add i32 %16, 8
  ; %15に%23の値を保存する
  store i32 %23, i32* %15, align 16
  br label %29

24:                                               ; preds = %13
  ; i32 ２?
  ; %14からポインタ２つ分ずらしたところにある値を読み取る
  ; %25に値を返す
  %25 = getelementptr inbounds %struct.__va_list_tag, %struct.__va_list_tag* %14, i32 0, i32 2
  ; %25からアドレスを%26に読み出す
  %26 = load i8*, i8** %25, align 8
  ; %26をi32*にキャストして，%27に書き出す
  %27 = bitcast i8* %26 to i32*
  ; 以下の処理の意味がわからない
  %28 = getelementptr i8, i8* %26, i32 8
  store i8* %28, i8** %25, align 8
  br label %29

29:                                               ; preds = %24, %18
  ; 18と24で読み出したintのポインタを%30で受け取る
  %30 = phi i32* [ %22, %18 ], [ %27, %24 ]
  ; %30の値を%31に読み出す
  %31 = load i32, i32* %30, align 4
  ; %31の値を%4に保存
  store i32 %31, i32* %4, align 4
  ; %4の値を%32に読み出す
  %32 = load i32, i32* %4, align 4
  ; %5の値を%33に読み出す
  %33 = load i32, i32* %5, align 4
  ; add "No Signed Wrap"
  ; %32と%33を足して，%34に返す
  %34 = add nsw i32 %33, %32

  ; sum
  ; %34の足し算の結果を%5に保存する
  store i32 %34, i32* %5, align 4
  br label %35

35:                                               ; preds = %29
  ; iのアップデート
  %36 = load i32, i32* %3, align 4
  ; iをひとつインクリメント
  %37 = add nsw i32 %36, 1
  ; %3にアップデート結果を保存
  store i32 %37, i32* %3, align 4
  ; %9に戻る
  br label %9

38:                                               ; preds = %9
  ; %16から，va_listの先頭アドレスを取得
  %39 = getelementptr inbounds [1 x %struct.__va_list_tag], [1 x %struct.__va_list_tag]* %6, i64 0, i64 0
  %40 = bitcast %struct.__va_list_tag* %39 to i8*
  ; va_endを呼ぶ
  call void @llvm.va_end(i8* %40)
  ; sumの値を %41に読み出し，returnに設定する．
  %41 = load i32, i32* %5, align 4
  ret i32 %41
}
```