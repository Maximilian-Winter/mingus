; ModuleID = 'mingus_module'
source_filename = "mingus_module"
target triple = "x86_64-pc-windows-msvc"

@str = private unnamed_addr constant [38 x i8] c"=== Test 32: Closure + Ref Params ===\00", align 1
@str.1 = private unnamed_addr constant [15 x i8] c"after inc: %d\0A\00", align 1
@str.2 = private unnamed_addr constant [19 x i8] c"after HOF inc: %d\0A\00", align 1
@str.3 = private unnamed_addr constant [19 x i8] c"after addStep: %d\0A\00", align 1
@str.4 = private unnamed_addr constant [18 x i8] c"after double: %d\0A\00", align 1
@str.5 = private unnamed_addr constant [18 x i8] c"a after swap: %d\0A\00", align 1
@str.6 = private unnamed_addr constant [18 x i8] c"b after swap: %d\0A\00", align 1

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr noundef readonly captures(none), i32 noundef, ...) local_unnamed_addr #0

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr noundef readonly captures(none)) local_unnamed_addr #0

define void @Test32_applyRef({ ptr, ptr } %f, ptr %val.ref) local_unnamed_addr {
entry:
  %f.fca.0.extract = extractvalue { ptr, ptr } %f, 0
  %f.fca.1.extract = extractvalue { ptr, ptr } %f, 1
  tail call void %f.fca.0.extract(ptr %val.ref, ptr %f.fca.1.extract)
  ret void
}

; Function Attrs: nofree nounwind
define noundef i32 @Test32_main() local_unnamed_addr #0 {
__mingus_closure_release_wrapper.exit67:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str)
  %1 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.1, i32 11)
  %2 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.2, i32 12)
  %3 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.3, i32 17)
  %4 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.4, i32 34)
  %5 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.5, i32 200)
  %6 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.6, i32 100)
  ret i32 0
}

; Function Attrs: nofree nounwind
define noundef i32 @main() local_unnamed_addr #0 {
entry:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str)
  %1 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.1, i32 11)
  %2 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.2, i32 12)
  %3 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.3, i32 17)
  %4 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.4, i32 34)
  %5 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.5, i32 200)
  %6 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.6, i32 100)
  ret i32 0
}

attributes #0 = { nofree nounwind }
