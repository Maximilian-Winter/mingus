; ModuleID = 'mingus_module'
source_filename = "mingus_module"
target triple = "x86_64-pc-windows-msvc"

@str = private unnamed_addr constant [34 x i8] c"=== Test 1: Generic identity ===\0A\00", align 1
@str.1 = private unnamed_addr constant [24 x i8] c"identity<int>(42) = %d\0A\00", align 1
@str.2 = private unnamed_addr constant [31 x i8] c"identity<double>(3.14) = %.2f\0A\00", align 1
@str.3 = private unnamed_addr constant [29 x i8] c"=== Test 2: Generic max ===\0A\00", align 1
@str.4 = private unnamed_addr constant [23 x i8] c"max<int>(10, 20) = %d\0A\00", align 1
@str.5 = private unnamed_addr constant [32 x i8] c"max<double>(2.71, 3.14) = %.2f\0A\00", align 1
@str.6 = private unnamed_addr constant [29 x i8] c"=== Test 3: Generic add ===\0A\00", align 1
@str.7 = private unnamed_addr constant [23 x i8] c"add<int>(15, 27) = %d\0A\00", align 1
@str.8 = private unnamed_addr constant [30 x i8] c"add<double>(1.5, 2.5) = %.1f\0A\00", align 1
@str.9 = private unnamed_addr constant [38 x i8] c"=== Test 4: Multiple type params ===\0A\00", align 1
@str.10 = private unnamed_addr constant [34 x i8] c"first<int,double>(42, 3.14) = %d\0A\00", align 1
@str.11 = private unnamed_addr constant [42 x i8] c"=== Test 5: Reuse same instantiation ===\0A\00", align 1
@str.12 = private unnamed_addr constant [30 x i8] c"max<int>(5, 3) = %d (cached)\0A\00", align 1

declare i32 @printf(ptr, ...)

define void @Test_main() {
entry:
  %m3 = alloca i32, align 4
  %f1 = alloca i32, align 4
  %s2 = alloca double, align 8
  %s1 = alloca i32, align 4
  %m2 = alloca double, align 8
  %m1 = alloca i32, align 4
  %b = alloca double, align 8
  %a = alloca i32, align 4
  %0 = call i32 (ptr, ...) @printf(ptr @str)
  %1 = call i32 @"Test_identity$G_i"(i32 42)
  store i32 %1, ptr %a, align 4
  %a1 = load i32, ptr %a, align 4
  %2 = call i32 (ptr, ...) @printf(ptr @str.1, i32 %a1)
  %3 = call double @"Test_identity$G_d"(double 3.140000e+00)
  store double %3, ptr %b, align 8
  %b2 = load double, ptr %b, align 8
  %4 = call i32 (ptr, ...) @printf(ptr @str.2, double %b2)
  %5 = call i32 (ptr, ...) @printf(ptr @str.3)
  %6 = call i32 @"Test_max$G_i"(i32 10, i32 20)
  store i32 %6, ptr %m1, align 4
  %m13 = load i32, ptr %m1, align 4
  %7 = call i32 (ptr, ...) @printf(ptr @str.4, i32 %m13)
  %8 = call double @"Test_max$G_d"(double 2.710000e+00, double 3.140000e+00)
  store double %8, ptr %m2, align 8
  %m24 = load double, ptr %m2, align 8
  %9 = call i32 (ptr, ...) @printf(ptr @str.5, double %m24)
  %10 = call i32 (ptr, ...) @printf(ptr @str.6)
  %11 = call i32 @"Test_add$G_i"(i32 15, i32 27)
  store i32 %11, ptr %s1, align 4
  %s15 = load i32, ptr %s1, align 4
  %12 = call i32 (ptr, ...) @printf(ptr @str.7, i32 %s15)
  %13 = call double @"Test_add$G_d"(double 1.500000e+00, double 2.500000e+00)
  store double %13, ptr %s2, align 8
  %s26 = load double, ptr %s2, align 8
  %14 = call i32 (ptr, ...) @printf(ptr @str.8, double %s26)
  %15 = call i32 (ptr, ...) @printf(ptr @str.9)
  %16 = call i32 @"Test_first$G_i_d"(i32 42, double 3.140000e+00)
  store i32 %16, ptr %f1, align 4
  %f17 = load i32, ptr %f1, align 4
  %17 = call i32 (ptr, ...) @printf(ptr @str.10, i32 %f17)
  %18 = call i32 (ptr, ...) @printf(ptr @str.11)
  %19 = call i32 @"Test_max$G_i"(i32 5, i32 3)
  store i32 %19, ptr %m3, align 4
  %m38 = load i32, ptr %m3, align 4
  %20 = call i32 (ptr, ...) @printf(ptr @str.12, i32 %m38)
  ret void
}

define double @"Test_max$G_d"(double %a, double %b) {
entry:
  %b2 = alloca double, align 8
  %a1 = alloca double, align 8
  store double %a, ptr %a1, align 8
  store double %b, ptr %b2, align 8
  %a3 = load double, ptr %a1, align 8
  %b4 = load double, ptr %b2, align 8
  %fgt = fcmp ogt double %a3, %b4
  br i1 %fgt, label %then, label %ifmerge

then:                                             ; preds = %entry
  %a5 = load double, ptr %a1, align 8
  ret double %a5

ifmerge:                                          ; preds = %entry
  %b6 = load double, ptr %b2, align 8
  ret double %b6
}

define i32 @"Test_identity$G_i"(i32 %value) {
entry:
  %value1 = alloca i32, align 4
  store i32 %value, ptr %value1, align 4
  %value2 = load i32, ptr %value1, align 4
  ret i32 %value2
}

define i32 @"Test_max$G_i"(i32 %a, i32 %b) {
entry:
  %b2 = alloca i32, align 4
  %a1 = alloca i32, align 4
  store i32 %a, ptr %a1, align 4
  store i32 %b, ptr %b2, align 4
  %a3 = load i32, ptr %a1, align 4
  %b4 = load i32, ptr %b2, align 4
  %sgt = icmp sgt i32 %a3, %b4
  br i1 %sgt, label %then, label %ifmerge

then:                                             ; preds = %entry
  %a5 = load i32, ptr %a1, align 4
  ret i32 %a5

ifmerge:                                          ; preds = %entry
  %b6 = load i32, ptr %b2, align 4
  ret i32 %b6
}

define double @"Test_identity$G_d"(double %value) {
entry:
  %value1 = alloca double, align 8
  store double %value, ptr %value1, align 8
  %value2 = load double, ptr %value1, align 8
  ret double %value2
}

define i32 @"Test_add$G_i"(i32 %a, i32 %b) {
entry:
  %b2 = alloca i32, align 4
  %a1 = alloca i32, align 4
  store i32 %a, ptr %a1, align 4
  store i32 %b, ptr %b2, align 4
  %a3 = load i32, ptr %a1, align 4
  %b4 = load i32, ptr %b2, align 4
  %add = add i32 %a3, %b4
  ret i32 %add
}

define double @"Test_add$G_d"(double %a, double %b) {
entry:
  %b2 = alloca double, align 8
  %a1 = alloca double, align 8
  store double %a, ptr %a1, align 8
  store double %b, ptr %b2, align 8
  %a3 = load double, ptr %a1, align 8
  %b4 = load double, ptr %b2, align 8
  %fadd = fadd double %a3, %b4
  ret double %fadd
}

define i32 @"Test_first$G_i_d"(i32 %a, double %b) {
entry:
  %b2 = alloca double, align 8
  %a1 = alloca i32, align 4
  store i32 %a, ptr %a1, align 4
  store double %b, ptr %b2, align 8
  %a3 = load i32, ptr %a1, align 4
  ret i32 %a3
}

define i32 @main() {
entry:
  call void @Test_main()
  ret i32 0
}
