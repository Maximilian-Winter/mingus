; ModuleID = 'mingus_module'
source_filename = "mingus_module"
target triple = "x86_64-pc-windows-msvc"

@.str = private unnamed_addr constant [12 x i8] c"TestDefines\00"
@str = private unnamed_addr constant [13 x i8] c"Version: %d\0A\00", align 1
@str.1 = private unnamed_addr constant [9 x i8] c"Max: %d\0A\00", align 1
@str.2 = private unnamed_addr constant [12 x i8] c"Flag A: %d\0A\00", align 1
@str.3 = private unnamed_addr constant [12 x i8] c"Flag B: %d\0A\00", align 1
@str.4 = private unnamed_addr constant [9 x i8] c"Hex: %u\0A\00", align 1
@str.5 = private unnamed_addr constant [8 x i8] c"PI: %f\0A\00", align 1
@str.6 = private unnamed_addr constant [10 x i8] c"Half: %f\0A\00", align 1
@str.7 = private unnamed_addr constant [10 x i8] c"Name: %s\0A\00", align 1
@str.8 = private unnamed_addr constant [11 x i8] c"Flags: %d\0A\00", align 1

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr noundef readonly captures(none), ...) local_unnamed_addr #0

; Function Attrs: nofree nounwind
define noundef i32 @Test66_main() local_unnamed_addr #0 {
entry:
  %0 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str, i32 1)
  %1 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.1, i32 1024)
  %2 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.2, i32 1)
  %3 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.3, i32 2)
  %4 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.4, i32 -559038737)
  %5 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.5, double 3.141590e+00)
  %6 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.6, double 5.000000e-01)
  %7 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.7, ptr nonnull @.str)
  %8 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.8, i32 3)
  ret i32 0
}

attributes #0 = { nofree nounwind }
