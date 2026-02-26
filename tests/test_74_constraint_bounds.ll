; ModuleID = 'mingus_module'
source_filename = "mingus_module"
target triple = "x86_64-pc-windows-msvc"

@Container_vtable = internal constant [3 x ptr] [ptr @Test_Container_destructor, ptr @Test_Container_print, ptr @Test_Container_size]
@Message_vtable = internal constant [2 x ptr] [ptr @Test_Message_destructor, ptr @Test_Message_print]
@str = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@str.1 = private unnamed_addr constant [14 x i8] c"Container(%d)\00", align 1
@str.3 = private unnamed_addr constant [6 x i8] c"hello\00", align 1
@str.13 = private unnamed_addr constant [11 x i8] c"wrap(%d): \00", align 1
@str.14 = private unnamed_addr constant [11 x i8] c"[size=%d] \00", align 1
@str.15 = private unnamed_addr constant [34 x i8] c"=== Test 1: Single constraint ===\00", align 1
@str.16 = private unnamed_addr constant [37 x i8] c"=== Test 2: Multiple constraints ===\00", align 1
@str.17 = private unnamed_addr constant [54 x i8] c"=== Test 3: Single constraint with different type ===\00", align 1
@str.18 = private unnamed_addr constant [52 x i8] c"=== Test 4: Mixed constrained and unconstrained ===\00", align 1
@str.19 = private unnamed_addr constant [42 x i8] c"=== Test 5: Turbofish with constraint ===\00", align 1

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr noundef readonly captures(none), ...) local_unnamed_addr #0

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write)
define void @Test_Container_constructor(ptr writeonly captures(none) initializes((0, 12)) %this, i32 %n) local_unnamed_addr #1 {
entry:
  store ptr @Container_vtable, ptr %this, align 8
  %count_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  store i32 %n, ptr %count_ptr, align 4
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define void @Test_Container_destructor(ptr readnone captures(none) %this) #2 {
entry:
  ret void
}

; Function Attrs: nofree nounwind
define void @Test_Container_print(ptr readonly captures(none) %this) #0 {
entry:
  %count_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  %count = load i32, ptr %count_ptr, align 4
  %0 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.1, i32 %count)
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read)
define i32 @Test_Container_size(ptr readonly captures(none) %this) #3 {
entry:
  %count_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  %count = load i32, ptr %count_ptr, align 4
  ret i32 %count
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define void @Test_Message_destructor(ptr readnone captures(none) %this) #2 {
entry:
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write)
define void @Test_Message_constructor(ptr writeonly captures(none) initializes((0, 16)) %this, ptr %t) local_unnamed_addr #1 {
entry:
  store ptr @Message_vtable, ptr %this, align 8
  %text_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  store ptr %t, ptr %text_ptr, align 8
  ret void
}

; Function Attrs: nofree nounwind
define void @Test_Message_print(ptr readonly captures(none) %this) #0 {
entry:
  %text_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  %text = load ptr, ptr %text_ptr, align 8
  %0 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str, ptr %text)
  ret void
}

define void @Test_main() local_unnamed_addr {
entry:
  %puts = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.15)
  %new.ptr = tail call ptr @malloc(i32 16)
  store ptr @Message_vtable, ptr %new.ptr, align 8
  %text_ptr.i = getelementptr inbounds nuw i8, ptr %new.ptr, i64 8
  store ptr @str.3, ptr %text_ptr.i, align 8
  %0 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str, ptr nonnull @str.3)
  %putchar = tail call i32 @putchar(i32 10)
  %puts13 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.16)
  %new.ptr2 = tail call ptr @malloc(i32 16)
  store ptr @Container_vtable, ptr %new.ptr2, align 8
  %count_ptr.i = getelementptr inbounds nuw i8, ptr %new.ptr2, i64 8
  store i32 5, ptr %count_ptr.i, align 4
  %1 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.14, i32 5)
  %vtable5.i = load ptr, ptr %new.ptr2, align 8
  %method.slot6.i = getelementptr i8, ptr %vtable5.i, i64 8
  %method.fn7.i = load ptr, ptr %method.slot6.i, align 8
  tail call void %method.fn7.i(ptr nonnull %new.ptr2)
  %putchar14 = tail call i32 @putchar(i32 10)
  %puts15 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.17)
  %vtable.i24 = load ptr, ptr %new.ptr2, align 8
  %method.slot.i25 = getelementptr i8, ptr %vtable.i24, i64 8
  %method.fn.i26 = load ptr, ptr %method.slot.i25, align 8
  tail call void %method.fn.i26(ptr nonnull %new.ptr2)
  %putchar16 = tail call i32 @putchar(i32 10)
  %puts17 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.18)
  %2 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.13, i32 42)
  %vtable.i27 = load ptr, ptr %new.ptr, align 8
  %method.slot.i28 = getelementptr i8, ptr %vtable.i27, i64 8
  %method.fn.i29 = load ptr, ptr %method.slot.i28, align 8
  tail call void %method.fn.i29(ptr nonnull %new.ptr)
  %putchar18 = tail call i32 @putchar(i32 10)
  %puts19 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.19)
  %vtable.i30 = load ptr, ptr %new.ptr2, align 8
  %method.slot.i31 = getelementptr i8, ptr %vtable.i30, i64 8
  %method.fn.i32 = load ptr, ptr %method.slot.i31, align 8
  tail call void %method.fn.i32(ptr nonnull %new.ptr2)
  %putchar20 = tail call i32 @putchar(i32 10)
  %del.vtable = load ptr, ptr %new.ptr, align 8
  %del.dtor.fn = load ptr, ptr %del.vtable, align 8
  tail call void %del.dtor.fn(ptr nonnull %new.ptr)
  tail call void @free(ptr %new.ptr)
  %del.vtable10 = load ptr, ptr %new.ptr2, align 8
  %del.dtor.fn12 = load ptr, ptr %del.vtable10, align 8
  tail call void %del.dtor.fn12(ptr nonnull %new.ptr2)
  tail call void @free(ptr %new.ptr2)
  ret void
}

define void @"Test_wrapPrint$G_i_Message"(i32 %value, ptr %printer) local_unnamed_addr {
entry:
  %0 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.13, i32 %value)
  %vtable = load ptr, ptr %printer, align 8
  %method.slot = getelementptr i8, ptr %vtable, i64 8
  %method.fn = load ptr, ptr %method.slot, align 8
  tail call void %method.fn(ptr nonnull %printer)
  ret void
}

define void @"Test_printIt$G_Container"(ptr %obj) local_unnamed_addr {
entry:
  %vtable = load ptr, ptr %obj, align 8
  %method.slot = getelementptr i8, ptr %vtable, i64 8
  %method.fn = load ptr, ptr %method.slot, align 8
  tail call void %method.fn(ptr nonnull %obj)
  ret void
}

define void @"Test_printWithSize$G_Container"(ptr %obj) local_unnamed_addr {
entry:
  %vtable = load ptr, ptr %obj, align 8
  %method.slot = getelementptr i8, ptr %vtable, i64 16
  %method.fn = load ptr, ptr %method.slot, align 8
  %0 = tail call i32 %method.fn(ptr nonnull %obj)
  %1 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.14, i32 %0)
  %vtable5 = load ptr, ptr %obj, align 8
  %method.slot6 = getelementptr i8, ptr %vtable5, i64 8
  %method.fn7 = load ptr, ptr %method.slot6, align 8
  tail call void %method.fn7(ptr nonnull %obj)
  ret void
}

define void @"Test_printIt$G_Message"(ptr %obj) local_unnamed_addr {
entry:
  %vtable = load ptr, ptr %obj, align 8
  %method.slot = getelementptr i8, ptr %vtable, i64 8
  %method.fn = load ptr, ptr %method.slot, align 8
  tail call void %method.fn(ptr nonnull %obj)
  ret void
}

declare ptr @malloc(i32) local_unnamed_addr

; Function Attrs: mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite)
declare void @free(ptr allocptr noundef captures(none)) local_unnamed_addr #4

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr noundef readonly captures(none)) local_unnamed_addr #0

; Function Attrs: nofree nounwind
declare noundef i32 @putchar(i32 noundef) local_unnamed_addr #0

attributes #0 = { nofree nounwind }
attributes #1 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write) }
attributes #2 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) }
attributes #3 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read) }
attributes #4 = { mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite) "alloc-family"="malloc" }
