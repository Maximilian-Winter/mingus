; ModuleID = 'mingus_module'
source_filename = "mingus_module"
target triple = "x86_64-pc-windows-msvc"

@str = private unnamed_addr constant [24 x i8] c"=== Test 01: Basics ===\00", align 1
@str.1 = private unnamed_addr constant [14 x i8] c"10 + 20 = %d\0A\00", align 1
@str.2 = private unnamed_addr constant [14 x i8] c"20 - 10 = %d\0A\00", align 1
@str.3 = private unnamed_addr constant [14 x i8] c"10 * 20 = %d\0A\00", align 1
@str.4 = private unnamed_addr constant [14 x i8] c"20 / 10 = %d\0A\00", align 1
@str.5 = private unnamed_addr constant [15 x i8] c"20 %% 3  = %d\0A\00", align 1
@str.6 = private unnamed_addr constant [15 x i8] c"sum > 25: PASS\00", align 1
@str.8 = private unnamed_addr constant [17 x i8] c"diff == 10: PASS\00", align 1
@str.10 = private unnamed_addr constant [28 x i8] c"sum(0..9) = %d (expect 45)\0A\00", align 1
@str.11 = private unnamed_addr constant [31 x i8] c"2^k >= 100: k = %d (expect 7)\0A\00", align 1
@str.12 = private unnamed_addr constant [29 x i8] c"matrix sum = %d (expect 36)\0A\00", align 1
@str.13 = private unnamed_addr constant [25 x i8] c"=== Test 01 complete ===\00", align 1

declare i32 @printf(ptr, i32) local_unnamed_addr

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr noundef readonly captures(none)) local_unnamed_addr #0

define noundef i32 @Test01_main() local_unnamed_addr {
entry:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str)
  %1 = tail call i32 @printf(ptr nonnull @str.1, i32 30)
  %2 = tail call i32 @printf(ptr nonnull @str.2, i32 10)
  %3 = tail call i32 @printf(ptr nonnull @str.3, i32 200)
  %4 = tail call i32 @printf(ptr nonnull @str.4, i32 2)
  %5 = tail call i32 @printf(ptr nonnull @str.5, i32 2)
  %6 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.6)
  %7 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.8)
  %8 = tail call i32 @printf(ptr nonnull @str.10, i32 45)
  %9 = tail call i32 @printf(ptr nonnull @str.11, i32 7)
  %10 = tail call i32 @printf(ptr nonnull @str.12, i32 36)
  %11 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.13)
  ret i32 0
}

define noundef i32 @main() local_unnamed_addr {
entry:
  %0 = tail call i32 @Test01_main()
  ret i32 0
}

attributes #0 = { nofree nounwind }
