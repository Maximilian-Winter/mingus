; ModuleID = 'mingus_module'
source_filename = "mingus_module"
target triple = "x86_64-pc-windows-msvc"

@str = private unnamed_addr constant [23 x i8] c"=== Test 60: C FFI ===\00", align 1
@str.1 = private unnamed_addr constant [36 x i8] c"Case 1: extern block grammar - PASS\00", align 1
@str.3 = private unnamed_addr constant [2 x i8] c"w\00", align 1
@str.4 = private unnamed_addr constant [23 x i8] c"hello from mingus FFI\0A\00", align 1
@str.5 = private unnamed_addr constant [28 x i8] c"Case 2: opaque FILE* - PASS\00", align 1
@str.6 = private unnamed_addr constant [50 x i8] c"Case 2: opaque FILE* - FAIL (could not open file)\00", align 1
@str.7 = private unnamed_addr constant [21 x i8] c"test_60_ffi_temp.txt\00", align 1
@str.8 = private unnamed_addr constant [36 x i8] c"Case 3: extern struct Point(%d, %d)\00", align 1
@str.11 = private unnamed_addr constant [37 x i8] c"Case 4: extern enum Color.Green = %d\00", align 1
@str.12 = private unnamed_addr constant [8 x i8] c" - PASS\00", align 1
@str.14 = private unnamed_addr constant [43 x i8] c"Case 5: callback interop - PASS (compiled)\00", align 1
@str.15 = private unnamed_addr constant [26 x i8] c"=== FFI test complete ===\00", align 1

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr noundef readonly captures(none), ...) local_unnamed_addr #0

; Function Attrs: nofree nounwind
declare noundef i32 @fclose(ptr noundef captures(none)) local_unnamed_addr #0

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr noundef readonly captures(none)) local_unnamed_addr #0

; Function Attrs: nofree nounwind
declare noundef i32 @remove(ptr noundef readonly captures(none)) local_unnamed_addr #0

; Function Attrs: nofree nounwind
declare noalias noundef ptr @fopen(ptr noundef readonly captures(none), ptr noundef readonly captures(none)) local_unnamed_addr #0

; Function Attrs: nofree nounwind
declare noundef i32 @fflush(ptr noundef captures(none)) local_unnamed_addr #0

; Function Attrs: nofree nounwind
define noundef i32 @Test60_main() local_unnamed_addr #0 {
entry:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str)
  %1 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.1)
  %2 = tail call ptr @fopen(ptr nonnull @str.7, ptr nonnull @str.3)
  %ptr.ne.not = icmp eq ptr %2, null
  br i1 %ptr.ne.not, label %ifmerge, label %then

then:                                             ; preds = %entry
  %3 = tail call i64 @fwrite(ptr nonnull @str.4, i64 22, i64 1, ptr nonnull %2)
  %4 = tail call i32 @fflush(ptr nonnull %2)
  %5 = tail call i32 @fclose(ptr nonnull %2)
  br label %ifmerge

ifmerge:                                          ; preds = %entry, %then
  %str.6.sink = phi ptr [ @str.5, %then ], [ @str.6, %entry ]
  %6 = tail call i32 @puts(ptr nonnull dereferenceable(1) %str.6.sink)
  %7 = tail call i32 @remove(ptr nonnull @str.7)
  %8 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.8, i32 42, i32 99)
  %9 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.12)
  %10 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.11, i32 1)
  %11 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.12)
  %12 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.14)
  %13 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.15)
  ret i32 0
}

; Function Attrs: nofree nounwind
declare noundef i64 @fwrite(ptr noundef readonly captures(none), i64 noundef, i64 noundef, ptr noundef captures(none)) local_unnamed_addr #0

attributes #0 = { nofree nounwind }
