; ModuleID = 'mingus_module'
source_filename = "mingus_module"
target triple = "x86_64-pc-windows-msvc"

@str.3 = private unnamed_addr constant [21 x i8] c"const int* read: %d\0A\00", align 1
@str.4 = private unnamed_addr constant [24 x i8] c"non-const to const: %d\0A\00", align 1
@str.7 = private unnamed_addr constant [15 x i8] c"strlen = %llu\0A\00", align 1
@str.10 = private unnamed_addr constant [23 x i8] c"memcmp result < 0: %d\0A\00", align 1
@str.11 = private unnamed_addr constant [42 x i8] c"=== Test 1: Calling convention syntax ===\00", align 1
@str.12 = private unnamed_addr constant [34 x i8] c"calling convention attributes: OK\00", align 1
@str.13 = private unnamed_addr constant [36 x i8] c"=== Test 2: Const pointer types ===\00", align 1
@str.14 = private unnamed_addr constant [41 x i8] c"=== Test 3: Extern with const params ===\00", align 1

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr noundef readonly captures(none), ...) local_unnamed_addr #0

; Function Attrs: nofree nounwind
define void @Test_testCallingConvSyntax() local_unnamed_addr #0 {
entry:
  %puts = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.11)
  %puts1 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.12)
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read)
define i32 @Test_readConstPtr(ptr readonly captures(none) %p) local_unnamed_addr #1 {
entry:
  %elem = load i32, ptr %p, align 4
  ret i32 %elem
}

; Function Attrs: nofree nounwind
define void @Test_testConstPointer() local_unnamed_addr #0 {
entry:
  %puts = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.13)
  %0 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.3, i32 42)
  %1 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.4, i32 42)
  ret void
}

; Function Attrs: nofree nounwind
define void @Test_testConstExternFunc() local_unnamed_addr #0 {
entry:
  %puts = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.14)
  %0 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.7, i64 11)
  %1 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.10, i32 1)
  ret void
}

; Function Attrs: nofree nounwind
define void @Test_main() local_unnamed_addr #0 {
entry:
  %puts.i = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.11)
  %puts1.i = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.12)
  %puts.i1 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.13)
  %0 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.3, i32 42)
  %1 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.4, i32 42)
  %puts.i2 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.14)
  %2 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.7, i64 11)
  %3 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.10, i32 1)
  ret void
}

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr noundef readonly captures(none)) local_unnamed_addr #0

attributes #0 = { nofree nounwind }
attributes #1 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read) }
