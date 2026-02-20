; ModuleID = 'mingus_module'
source_filename = "mingus_module"

@Animal_vtable = internal constant [4 x ptr] [ptr @Animal_destructor, ptr @Animal_describe, ptr @Animal_getHealth, ptr @Animal_getSecret]
@Dog_vtable = internal constant [5 x ptr] [ptr @Dog_destructor, ptr @Animal_describe, ptr @Animal_getHealth, ptr @Animal_getSecret, ptr @Dog_bark]
@str = private unnamed_addr constant [10 x i8] c"secret=%d\00", align 1
@str.1 = private unnamed_addr constant [10 x i8] c"health=%d\00", align 1
@str.3 = private unnamed_addr constant [7 x i8] c"age=%d\00", align 1
@str.5 = private unnamed_addr constant [14 x i8] c"Dog health=%d\00", align 1
@str.7 = private unnamed_addr constant [11 x i8] c"Dog age=%d\00", align 1
@str.9 = private unnamed_addr constant [16 x i8] c"Dog name_len=%d\00", align 1
@str.11 = private unnamed_addr constant [14 x i8] c"direct age=%d\00", align 1
@str.13 = private unnamed_addr constant [19 x i8] c"direct name_len=%d\00", align 1
@fmt.14 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@str.15 = private unnamed_addr constant [12 x i8] c"Test22 done\00", align 1

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr noundef readonly captures(none), i32 noundef, ...) local_unnamed_addr #0

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr noundef readonly captures(none)) local_unnamed_addr #0

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write)
define void @Animal_constructor(ptr writeonly captures(none) initializes((0, 24)) %this, i32 %s, i32 %h, i32 %a, i32 %n) local_unnamed_addr #1 {
entry:
  store ptr @Animal_vtable, ptr %this, align 8
  %secret_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  store i32 %s, ptr %secret_ptr, align 4
  %health_ptr = getelementptr inbounds nuw i8, ptr %this, i64 12
  store i32 %h, ptr %health_ptr, align 4
  %age_ptr = getelementptr inbounds nuw i8, ptr %this, i64 16
  store i32 %a, ptr %age_ptr, align 4
  %name_len_ptr = getelementptr inbounds nuw i8, ptr %this, i64 20
  store i32 %n, ptr %name_len_ptr, align 4
  ret void
}

define noundef i32 @Animal_describe(ptr %this) {
entry:
  %vtable = load ptr, ptr %this, align 8
  %method.slot = getelementptr i8, ptr %vtable, i64 24
  %method.fn = load ptr, ptr %method.slot, align 8
  %0 = tail call i32 %method.fn(ptr nonnull %this)
  %vtable2 = load ptr, ptr %this, align 8
  %method.slot3 = getelementptr i8, ptr %vtable2, i64 16
  %method.fn4 = load ptr, ptr %method.slot3, align 8
  %1 = tail call i32 %method.fn4(ptr nonnull %this)
  %2 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str, i32 %0)
  %snprintf.len = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.14)
  %needed.i64 = sext i32 %snprintf.len to i64
  %alloc.size = add nsw i64 %needed.i64, 1
  %interp.buf = tail call ptr @malloc(i64 %alloc.size)
  %buf.size = add i32 %snprintf.len, 1
  %3 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf, i32 %buf.size, ptr nonnull @fmt.14)
  %4 = tail call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf)
  %5 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.1, i32 %1)
  %snprintf.len7 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.14)
  %needed.i648 = sext i32 %snprintf.len7 to i64
  %alloc.size9 = add nsw i64 %needed.i648, 1
  %interp.buf10 = tail call ptr @malloc(i64 %alloc.size9)
  %buf.size11 = add i32 %snprintf.len7, 1
  %6 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf10, i32 %buf.size11, ptr nonnull @fmt.14)
  %7 = tail call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf10)
  %age_ptr = getelementptr inbounds nuw i8, ptr %this, i64 16
  %age = load i32, ptr %age_ptr, align 4
  %8 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.3, i32 %age)
  %snprintf.len12 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.14)
  %needed.i6413 = sext i32 %snprintf.len12 to i64
  %alloc.size14 = add nsw i64 %needed.i6413, 1
  %interp.buf15 = tail call ptr @malloc(i64 %alloc.size14)
  %buf.size16 = add i32 %snprintf.len12, 1
  %9 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf15, i32 %buf.size16, ptr nonnull @fmt.14)
  %10 = tail call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf15)
  tail call void @free(ptr nonnull %interp.buf15)
  tail call void @free(ptr nonnull %interp.buf10)
  tail call void @free(ptr nonnull %interp.buf)
  ret i32 0
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define void @Animal_destructor(ptr readnone captures(none) %this) #2 {
entry:
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read)
define i32 @Animal_getHealth(ptr readonly captures(none) %this) #3 {
entry:
  %health_ptr = getelementptr inbounds nuw i8, ptr %this, i64 12
  %health = load i32, ptr %health_ptr, align 4
  ret i32 %health
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read)
define i32 @Animal_getSecret(ptr readonly captures(none) %this) #3 {
entry:
  %secret_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  %secret = load i32, ptr %secret_ptr, align 4
  ret i32 %secret
}

define noundef i32 @Dog_bark(ptr %this) {
entry:
  %vtable = load ptr, ptr %this, align 8
  %method.slot = getelementptr i8, ptr %vtable, i64 16
  %method.fn = load ptr, ptr %method.slot, align 8
  %0 = tail call i32 %method.fn(ptr nonnull %this)
  %1 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.5, i32 %0)
  %snprintf.len = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.14)
  %needed.i64 = sext i32 %snprintf.len to i64
  %alloc.size = add nsw i64 %needed.i64, 1
  %interp.buf = tail call ptr @malloc(i64 %alloc.size)
  %buf.size = add i32 %snprintf.len, 1
  %2 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf, i32 %buf.size, ptr nonnull @fmt.14)
  %3 = tail call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf)
  %age_ptr = getelementptr inbounds nuw i8, ptr %this, i64 16
  %age = load i32, ptr %age_ptr, align 4
  %4 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.7, i32 %age)
  %snprintf.len2 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.14)
  %needed.i643 = sext i32 %snprintf.len2 to i64
  %alloc.size4 = add nsw i64 %needed.i643, 1
  %interp.buf5 = tail call ptr @malloc(i64 %alloc.size4)
  %buf.size6 = add i32 %snprintf.len2, 1
  %5 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf5, i32 %buf.size6, ptr nonnull @fmt.14)
  %6 = tail call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf5)
  %name_len_ptr = getelementptr inbounds nuw i8, ptr %this, i64 20
  %name_len = load i32, ptr %name_len_ptr, align 4
  %7 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.9, i32 %name_len)
  %snprintf.len7 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.14)
  %needed.i648 = sext i32 %snprintf.len7 to i64
  %alloc.size9 = add nsw i64 %needed.i648, 1
  %interp.buf10 = tail call ptr @malloc(i64 %alloc.size9)
  %buf.size11 = add i32 %snprintf.len7, 1
  %8 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf10, i32 %buf.size11, ptr nonnull @fmt.14)
  %9 = tail call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf10)
  tail call void @free(ptr nonnull %interp.buf10)
  tail call void @free(ptr nonnull %interp.buf5)
  tail call void @free(ptr nonnull %interp.buf)
  ret i32 0
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write)
define void @Dog_constructor(ptr writeonly captures(none) initializes((0, 24)) %this, i32 %s, i32 %h, i32 %a) local_unnamed_addr #1 {
entry:
  %secret_ptr.i = getelementptr inbounds nuw i8, ptr %this, i64 8
  store i32 %s, ptr %secret_ptr.i, align 4
  %health_ptr.i = getelementptr inbounds nuw i8, ptr %this, i64 12
  store i32 %h, ptr %health_ptr.i, align 4
  %age_ptr.i = getelementptr inbounds nuw i8, ptr %this, i64 16
  store i32 %a, ptr %age_ptr.i, align 4
  %name_len_ptr.i = getelementptr inbounds nuw i8, ptr %this, i64 20
  store i32 3, ptr %name_len_ptr.i, align 4
  store ptr @Dog_vtable, ptr %this, align 8
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define void @Dog_destructor(ptr readnone captures(none) %this) #2 {
entry:
  ret void
}

define noundef i32 @Test22_main() local_unnamed_addr {
entry:
  %new.ptr = tail call dereferenceable_or_null(24) ptr @malloc(i32 24)
  store ptr @Animal_vtable, ptr %new.ptr, align 8
  %secret_ptr.i = getelementptr inbounds nuw i8, ptr %new.ptr, i64 8
  store i32 42, ptr %secret_ptr.i, align 4
  %health_ptr.i = getelementptr inbounds nuw i8, ptr %new.ptr, i64 12
  store i32 100, ptr %health_ptr.i, align 4
  %age_ptr.i = getelementptr inbounds nuw i8, ptr %new.ptr, i64 16
  store i32 5, ptr %age_ptr.i, align 4
  %name_len_ptr.i = getelementptr inbounds nuw i8, ptr %new.ptr, i64 20
  store i32 6, ptr %name_len_ptr.i, align 4
  %0 = tail call i32 @Animal_describe(ptr nonnull %new.ptr)
  %age = load i32, ptr %age_ptr.i, align 4
  %1 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.11, i32 %age)
  %snprintf.len = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.14)
  %needed.i64 = sext i32 %snprintf.len to i64
  %alloc.size = add nsw i64 %needed.i64, 1
  %interp.buf = tail call ptr @malloc(i64 %alloc.size)
  %buf.size = add i32 %snprintf.len, 1
  %2 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf, i32 %buf.size, ptr nonnull @fmt.14)
  %3 = tail call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf)
  %name_len = load i32, ptr %name_len_ptr.i, align 4
  %4 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.13, i32 %name_len)
  %snprintf.len4 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.14)
  %needed.i645 = sext i32 %snprintf.len4 to i64
  %alloc.size6 = add nsw i64 %needed.i645, 1
  %interp.buf7 = tail call ptr @malloc(i64 %alloc.size6)
  %buf.size8 = add i32 %snprintf.len4, 1
  %5 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf7, i32 %buf.size8, ptr nonnull @fmt.14)
  %6 = tail call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf7)
  %new.ptr9 = tail call dereferenceable_or_null(24) ptr @malloc(i32 24)
  %secret_ptr.i.i = getelementptr inbounds nuw i8, ptr %new.ptr9, i64 8
  store i32 99, ptr %secret_ptr.i.i, align 4
  %health_ptr.i.i = getelementptr inbounds nuw i8, ptr %new.ptr9, i64 12
  store i32 200, ptr %health_ptr.i.i, align 4
  %age_ptr.i.i = getelementptr inbounds nuw i8, ptr %new.ptr9, i64 16
  store i32 3, ptr %age_ptr.i.i, align 4
  %name_len_ptr.i.i = getelementptr inbounds nuw i8, ptr %new.ptr9, i64 20
  store i32 3, ptr %name_len_ptr.i.i, align 4
  store ptr @Dog_vtable, ptr %new.ptr9, align 8
  %7 = tail call i32 @Dog_bark(ptr nonnull %new.ptr9)
  %del.vtable = load ptr, ptr %new.ptr9, align 8
  %del.dtor.fn = load ptr, ptr %del.vtable, align 8
  tail call void %del.dtor.fn(ptr nonnull %new.ptr9)
  tail call void @free(ptr %new.ptr9)
  %del.vtable18 = load ptr, ptr %new.ptr, align 8
  %del.dtor.fn20 = load ptr, ptr %del.vtable18, align 8
  tail call void %del.dtor.fn20(ptr nonnull %new.ptr)
  tail call void @free(ptr %new.ptr)
  %8 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.15)
  tail call void @free(ptr nonnull %interp.buf7)
  tail call void @free(ptr nonnull %interp.buf)
  ret i32 0
}

declare i32 @snprintf(ptr, i32, ptr, ...) local_unnamed_addr

; Function Attrs: mustprogress nofree nounwind willreturn allockind("alloc,uninitialized") allocsize(0) memory(inaccessiblemem: readwrite)
declare noalias noundef ptr @malloc(i64 noundef) local_unnamed_addr #4

; Function Attrs: mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite)
declare void @free(ptr allocptr noundef captures(none)) local_unnamed_addr #5

define noundef i32 @main() local_unnamed_addr {
entry:
  %0 = tail call i32 @Test22_main()
  ret i32 0
}

attributes #0 = { nofree nounwind }
attributes #1 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write) }
attributes #2 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) }
attributes #3 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read) }
attributes #4 = { mustprogress nofree nounwind willreturn allockind("alloc,uninitialized") allocsize(0) memory(inaccessiblemem: readwrite) "alloc-family"="malloc" }
attributes #5 = { mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite) "alloc-family"="malloc" }
