; ModuleID = 'mingus_module'
source_filename = "mingus_module"

@Counter_vtable = internal constant [3 x ptr] [ptr @Counter_destructor, ptr @Counter_get, ptr @Counter_increment]
@str = private unnamed_addr constant [9 x i8] c"sum = %d\00", align 1
@str.1 = private unnamed_addr constant [13 x i8] c"counter = %d\00", align 1
@fmt.2 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@str.3 = private unnamed_addr constant [12 x i8] c"debug works\00", align 1
@str.4 = private unnamed_addr constant [12 x i8] c"Test27 done\00", align 1

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr noundef readonly captures(none), i32 noundef, ...) local_unnamed_addr #0

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr noundef readonly captures(none)) local_unnamed_addr #0

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write)
define void @Counter_constructor(ptr writeonly captures(none) initializes((0, 12)) %this, i32 %initial) local_unnamed_addr #1 {
entry:
  store ptr @Counter_vtable, ptr %this, align 8
  %value_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  store i32 %initial, ptr %value_ptr, align 4
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define void @Counter_destructor(ptr readnone captures(none) %this) #2 {
entry:
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read)
define i32 @Counter_get(ptr readonly captures(none) %this) #3 {
entry:
  %value_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  %value = load i32, ptr %value_ptr, align 4
  ret i32 %value
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: readwrite)
define i32 @Counter_increment(ptr captures(none) %this) #4 {
entry:
  %value_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  %value = load i32, ptr %value_ptr, align 4
  %add = add i32 %value, 1
  store i32 %add, ptr %value_ptr, align 4
  ret i32 %add
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define i32 @Test27_add(i32 %a, i32 %b) local_unnamed_addr #2 {
entry:
  %add = add i32 %b, %a
  ret i32 %add
}

define noundef i32 @Test27_main() local_unnamed_addr {
entry:
  %0 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str, i32 30)
  %snprintf.len = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.2)
  %needed.i64 = sext i32 %snprintf.len to i64
  %alloc.size = add nsw i64 %needed.i64, 1
  %interp.buf = tail call ptr @malloc(i64 %alloc.size)
  %buf.size = add i32 %snprintf.len, 1
  %1 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf, i32 %buf.size, ptr nonnull @fmt.2)
  %2 = tail call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf)
  %new.ptr = tail call dereferenceable_or_null(16) ptr @malloc(i32 16)
  store ptr @Counter_vtable, ptr %new.ptr, align 8
  %value_ptr.i = getelementptr inbounds nuw i8, ptr %new.ptr, i64 8
  store i32 3, ptr %value_ptr.i, align 4
  %3 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.1, i32 3)
  %snprintf.len20 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.2)
  %needed.i6421 = sext i32 %snprintf.len20 to i64
  %alloc.size22 = add nsw i64 %needed.i6421, 1
  %interp.buf23 = tail call ptr @malloc(i64 %alloc.size22)
  %buf.size24 = add i32 %snprintf.len20, 1
  %4 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf23, i32 %buf.size24, ptr nonnull @fmt.2)
  %5 = tail call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf23)
  %6 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.3)
  %del.vtable = load ptr, ptr %new.ptr, align 8
  %del.dtor.fn = load ptr, ptr %del.vtable, align 8
  tail call void %del.dtor.fn(ptr nonnull %new.ptr)
  tail call void @free(ptr %new.ptr)
  %7 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.4)
  tail call void @free(ptr nonnull %interp.buf23)
  tail call void @free(ptr nonnull %interp.buf)
  ret i32 0
}

declare i32 @snprintf(ptr, i32, ptr, ...) local_unnamed_addr

; Function Attrs: mustprogress nofree nounwind willreturn allockind("alloc,uninitialized") allocsize(0) memory(inaccessiblemem: readwrite)
declare noalias noundef ptr @malloc(i64 noundef) local_unnamed_addr #5

; Function Attrs: mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite)
declare void @free(ptr allocptr noundef captures(none)) local_unnamed_addr #6

define noundef i32 @main() local_unnamed_addr {
entry:
  %0 = tail call i32 @Test27_main()
  ret i32 0
}

attributes #0 = { nofree nounwind }
attributes #1 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write) }
attributes #2 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) }
attributes #3 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read) }
attributes #4 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: readwrite) }
attributes #5 = { mustprogress nofree nounwind willreturn allockind("alloc,uninitialized") allocsize(0) memory(inaccessiblemem: readwrite) "alloc-family"="malloc" }
attributes #6 = { mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite) "alloc-family"="malloc" }
