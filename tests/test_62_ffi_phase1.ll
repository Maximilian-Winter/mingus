; ModuleID = 'mingus_module'
source_filename = "mingus_module"
target triple = "x86_64-pc-windows-msvc"

@str = private unnamed_addr constant [22 x i8] c"Case 1 set_value: %d\0A\00", align 1
@str.1 = private unnamed_addr constant [20 x i8] c"Case 2 swap: %d %d\0A\00", align 1
@str.2 = private unnamed_addr constant [18 x i8] c"Case 3 deref: %d\0A\00", align 1
@str.3 = private unnamed_addr constant [25 x i8] c"Case 3 double deref: %d\0A\00", align 1
@str.4 = private unnamed_addr constant [24 x i8] c"Case 3 after write: %d\0A\00", align 1
@str.5 = private unnamed_addr constant [36 x i8] c"Case 4 array: %d %d %d %d count=%d\0A\00", align 1
@str.6 = private unnamed_addr constant [28 x i8] c"Case 5 rect: (%d,%d) %dx%d\0A\00", align 1
@str.7 = private unnamed_addr constant [22 x i8] c"Case 6 field ptr: %d\0A\00", align 1
@str.8 = private unnamed_addr constant [26 x i8] c"Case 7 struct ptr: %d %d\0A\00", align 1

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr noundef readonly captures(none), ...) local_unnamed_addr #0

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write)
define void @Test62_set_value(ptr writeonly captures(none) initializes((0, 4)) %p, i32 %val) local_unnamed_addr #1 {
entry:
  store i32 %val, ptr %p, align 4
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: readwrite)
define void @Test62_swap(ptr captures(none) %a, ptr captures(none) %b) local_unnamed_addr #2 {
entry:
  %deref = load i32, ptr %a, align 4
  %deref6 = load i32, ptr %b, align 4
  store i32 %deref6, ptr %a, align 4
  store i32 %deref, ptr %b, align 4
  ret void
}

; Function Attrs: nofree nounwind
define noundef i32 @Test62_main() local_unnamed_addr #0 {
entry:
  %0 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str, i32 42)
  %1 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.1, i32 20, i32 10)
  %2 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.2, i32 99)
  %3 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.3, i32 99)
  %4 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.4, i32 77)
  %5 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.5, i32 100, i32 200, i32 300, i32 400, i32 4)
  %6 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.6, i32 10, i32 20, i32 640, i32 480)
  %7 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.7, i32 55)
  %8 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.8, i32 3, i32 7)
  ret i32 0
}

attributes #0 = { nofree nounwind }
attributes #1 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write) }
attributes #2 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: readwrite) }
