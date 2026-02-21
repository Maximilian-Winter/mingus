; ModuleID = 'mingus_module'
source_filename = "mingus_module"

@str = private unnamed_addr constant [24 x i8] c"=== Test 01: Basics ===\00", align 1
@str.1 = private unnamed_addr constant [14 x i8] c"10 + 20 = %d\0A\00", align 1
@str.2 = private unnamed_addr constant [14 x i8] c"20 - 10 = %d\0A\00", align 1
@str.3 = private unnamed_addr constant [14 x i8] c"10 * 20 = %d\0A\00", align 1
@str.4 = private unnamed_addr constant [14 x i8] c"20 / 10 = %d\0A\00", align 1
@str.5 = private unnamed_addr constant [15 x i8] c"20 %% 3  = %d\0A\00", align 1
@str.6 = private unnamed_addr constant [15 x i8] c"sum > 25: PASS\00", align 1
@str.7 = private unnamed_addr constant [15 x i8] c"sum > 25: FAIL\00", align 1
@str.8 = private unnamed_addr constant [17 x i8] c"diff == 10: PASS\00", align 1
@str.9 = private unnamed_addr constant [17 x i8] c"diff == 10: FAIL\00", align 1
@str.10 = private unnamed_addr constant [28 x i8] c"sum(0..9) = %d (expect 45)\0A\00", align 1
@str.11 = private unnamed_addr constant [31 x i8] c"2^k >= 100: k = %d (expect 7)\0A\00", align 1
@str.12 = private unnamed_addr constant [29 x i8] c"matrix sum = %d (expect 36)\0A\00", align 1
@str.13 = private unnamed_addr constant [25 x i8] c"=== Test 01 complete ===\00", align 1

declare i32 @printf(ptr, i32, ...)

declare i32 @puts(ptr)

define i32 @Test01_main() {
entry:
  %col = alloca i32, align 4
  %row = alloca i32, align 4
  %matrix_sum = alloca i32, align 4
  %n = alloca i32, align 4
  %count = alloca i32, align 4
  %i = alloca i32, align 4
  %total = alloca i32, align 4
  %rem = alloca i32, align 4
  %quot = alloca i32, align 4
  %prod = alloca i32, align 4
  %diff = alloca i32, align 4
  %sum = alloca i32, align 4
  %y = alloca i32, align 4
  %x = alloca i32, align 4
  %0 = call i32 @puts(ptr @str)
  store i32 10, ptr %x, align 4
  store i32 20, ptr %y, align 4
  %x1 = load i32, ptr %x, align 4
  %y2 = load i32, ptr %y, align 4
  %add = add i32 %x1, %y2
  store i32 %add, ptr %sum, align 4
  %y3 = load i32, ptr %y, align 4
  %x4 = load i32, ptr %x, align 4
  %sub = sub i32 %y3, %x4
  store i32 %sub, ptr %diff, align 4
  %x5 = load i32, ptr %x, align 4
  %y6 = load i32, ptr %y, align 4
  %mul = mul i32 %x5, %y6
  store i32 %mul, ptr %prod, align 4
  %y7 = load i32, ptr %y, align 4
  %x8 = load i32, ptr %x, align 4
  %sdiv = sdiv i32 %y7, %x8
  store i32 %sdiv, ptr %quot, align 4
  %y9 = load i32, ptr %y, align 4
  %srem = srem i32 %y9, 3
  store i32 %srem, ptr %rem, align 4
  %sum10 = load i32, ptr %sum, align 4
  %1 = call i32 (ptr, i32, ...) @printf(ptr @str.1, i32 %sum10)
  %diff11 = load i32, ptr %diff, align 4
  %2 = call i32 (ptr, i32, ...) @printf(ptr @str.2, i32 %diff11)
  %prod12 = load i32, ptr %prod, align 4
  %3 = call i32 (ptr, i32, ...) @printf(ptr @str.3, i32 %prod12)
  %quot13 = load i32, ptr %quot, align 4
  %4 = call i32 (ptr, i32, ...) @printf(ptr @str.4, i32 %quot13)
  %rem14 = load i32, ptr %rem, align 4
  %5 = call i32 (ptr, i32, ...) @printf(ptr @str.5, i32 %rem14)
  %sum15 = load i32, ptr %sum, align 4
  %sgt = icmp sgt i32 %sum15, 25
  br i1 %sgt, label %then, label %else

then:                                             ; preds = %entry
  %6 = call i32 @puts(ptr @str.6)
  br label %ifmerge

ifmerge:                                          ; preds = %else, %then
  %diff16 = load i32, ptr %diff, align 4
  %eq = icmp eq i32 %diff16, 10
  br i1 %eq, label %then17, label %else19

else:                                             ; preds = %entry
  %7 = call i32 @puts(ptr @str.7)
  br label %ifmerge

then17:                                           ; preds = %ifmerge
  %8 = call i32 @puts(ptr @str.8)
  br label %ifmerge18

ifmerge18:                                        ; preds = %else19, %then17
  store i32 0, ptr %total, align 4
  store i32 0, ptr %i, align 4
  br label %for.cond

else19:                                           ; preds = %ifmerge
  %9 = call i32 @puts(ptr @str.9)
  br label %ifmerge18

for.cond:                                         ; preds = %for.iter, %ifmerge18
  %i20 = load i32, ptr %i, align 4
  %slt = icmp slt i32 %i20, 10
  br i1 %slt, label %for.body, label %for.exit

for.body:                                         ; preds = %for.cond
  %total21 = load i32, ptr %total, align 4
  %i22 = load i32, ptr %i, align 4
  %add23 = add i32 %total21, %i22
  store i32 %add23, ptr %total, align 4
  br label %for.iter

for.iter:                                         ; preds = %for.body
  %old = load i32, ptr %i, align 4
  %inc = add i32 %old, 1
  store i32 %inc, ptr %i, align 4
  br label %for.cond

for.exit:                                         ; preds = %for.cond
  %total24 = load i32, ptr %total, align 4
  %10 = call i32 (ptr, i32, ...) @printf(ptr @str.10, i32 %total24)
  store i32 0, ptr %count, align 4
  store i32 1, ptr %n, align 4
  br label %while.cond

while.cond:                                       ; preds = %while.body, %for.exit
  %n25 = load i32, ptr %n, align 4
  %slt26 = icmp slt i32 %n25, 100
  br i1 %slt26, label %while.body, label %while.exit

while.body:                                       ; preds = %while.cond
  %n27 = load i32, ptr %n, align 4
  %mul28 = mul i32 %n27, 2
  store i32 %mul28, ptr %n, align 4
  %old29 = load i32, ptr %count, align 4
  %inc30 = add i32 %old29, 1
  store i32 %inc30, ptr %count, align 4
  br label %while.cond

while.exit:                                       ; preds = %while.cond
  %count31 = load i32, ptr %count, align 4
  %11 = call i32 (ptr, i32, ...) @printf(ptr @str.11, i32 %count31)
  store i32 0, ptr %matrix_sum, align 4
  store i32 0, ptr %row, align 4
  br label %for.cond32

for.cond32:                                       ; preds = %for.iter34, %while.exit
  %row36 = load i32, ptr %row, align 4
  %slt37 = icmp slt i32 %row36, 3
  br i1 %slt37, label %for.body33, label %for.exit35

for.body33:                                       ; preds = %for.cond32
  store i32 0, ptr %col, align 4
  br label %for.cond38

for.iter34:                                       ; preds = %for.exit41
  %old52 = load i32, ptr %row, align 4
  %inc53 = add i32 %old52, 1
  store i32 %inc53, ptr %row, align 4
  br label %for.cond32

for.exit35:                                       ; preds = %for.cond32
  %matrix_sum54 = load i32, ptr %matrix_sum, align 4
  %12 = call i32 (ptr, i32, ...) @printf(ptr @str.12, i32 %matrix_sum54)
  %13 = call i32 @puts(ptr @str.13)
  ret i32 0

for.cond38:                                       ; preds = %for.iter40, %for.body33
  %col42 = load i32, ptr %col, align 4
  %slt43 = icmp slt i32 %col42, 3
  br i1 %slt43, label %for.body39, label %for.exit41

for.body39:                                       ; preds = %for.cond38
  %matrix_sum44 = load i32, ptr %matrix_sum, align 4
  %row45 = load i32, ptr %row, align 4
  %mul46 = mul i32 %row45, 3
  %col47 = load i32, ptr %col, align 4
  %add48 = add i32 %mul46, %col47
  %add49 = add i32 %matrix_sum44, %add48
  store i32 %add49, ptr %matrix_sum, align 4
  br label %for.iter40

for.iter40:                                       ; preds = %for.body39
  %old50 = load i32, ptr %col, align 4
  %inc51 = add i32 %old50, 1
  store i32 %inc51, ptr %col, align 4
  br label %for.cond38

for.exit41:                                       ; preds = %for.cond38
  br label %for.iter34
}

define i32 @main() {
entry:
  %0 = call i32 @Test01_main()
  ret i32 %0
}
