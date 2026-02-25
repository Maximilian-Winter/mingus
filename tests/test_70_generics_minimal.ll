; ModuleID = 'mingus_module'
source_filename = "mingus_module"
target triple = "x86_64-pc-windows-msvc"

@str = private unnamed_addr constant [24 x i8] c"identity<int>(42) = %d\0A\00", align 1

declare i32 @printf(ptr, ...)

define void @Test_main() {
entry:
  %a = alloca i32, align 4
  %0 = call i32 @"Test_identity$G_i"(i32 42)
  store i32 %0, ptr %a, align 4
  %a1 = load i32, ptr %a, align 4
  %1 = call i32 (ptr, ...) @printf(ptr @str, i32 %a1)
  ret void
}

define i32 @"Test_identity$G_i"(i32 %value) {
entry:
  %value1 = alloca i32, align 4
  store i32 %value, ptr %value1, align 4
  %value2 = load i32, ptr %value1, align 4
  ret i32 %value2
}

define i32 @main() {
entry:
  call void @Test_main()
  ret i32 0
}
