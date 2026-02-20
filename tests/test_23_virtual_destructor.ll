; ModuleID = 'mingus_module'
source_filename = "mingus_module"

@Base_vtable = internal constant [2 x ptr] [ptr @Base_destructor, ptr @Base_greet]
@Derived_vtable = internal constant [2 x ptr] [ptr @Derived_destructor, ptr @Derived_greet]
@str = private unnamed_addr constant [6 x i8] c"~Base\00", align 1
@str.1 = private unnamed_addr constant [11 x i8] c"Base.greet\00", align 1
@str.2 = private unnamed_addr constant [9 x i8] c"~Derived\00", align 1
@str.3 = private unnamed_addr constant [14 x i8] c"Derived.greet\00", align 1
@str.4 = private unnamed_addr constant [22 x i8] c"--- direct delete ---\00", align 1
@str.5 = private unnamed_addr constant [24 x i8] c"--- base ptr delete ---\00", align 1
@str.6 = private unnamed_addr constant [20 x i8] c"--- base delete ---\00", align 1
@str.7 = private unnamed_addr constant [12 x i8] c"Test23 done\00", align 1

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr noundef readonly captures(none)) local_unnamed_addr #0

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write)
define void @Base_constructor(ptr writeonly captures(none) initializes((0, 12)) %this, i32 %i) local_unnamed_addr #1 {
entry:
  store ptr @Base_vtable, ptr %this, align 8
  %id_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  store i32 %i, ptr %id_ptr, align 4
  ret void
}

; Function Attrs: nofree nounwind
define void @Base_destructor(ptr readnone captures(none) %this) #0 {
entry:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str)
  ret void
}

; Function Attrs: nofree nounwind
define noundef i32 @Base_greet(ptr readnone captures(none) %this) #0 {
entry:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.1)
  ret i32 0
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write)
define void @Derived_constructor(ptr writeonly captures(none) initializes((0, 16)) %this, i32 %i, i32 %e) local_unnamed_addr #1 {
entry:
  %id_ptr.i = getelementptr inbounds nuw i8, ptr %this, i64 8
  store i32 %i, ptr %id_ptr.i, align 4
  store ptr @Derived_vtable, ptr %this, align 8
  %extra_ptr = getelementptr inbounds nuw i8, ptr %this, i64 12
  store i32 %e, ptr %extra_ptr, align 4
  ret void
}

; Function Attrs: nofree nounwind
define void @Derived_destructor(ptr readnone captures(none) %this) #0 {
entry:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.2)
  %1 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str)
  ret void
}

; Function Attrs: nofree nounwind
define noundef i32 @Derived_greet(ptr readnone captures(none) %this) #0 {
entry:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.3)
  ret i32 0
}

define noundef i32 @Test23_main() local_unnamed_addr {
entry:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.4)
  %new.ptr = tail call ptr @malloc(i32 16)
  %id_ptr.i.i = getelementptr inbounds nuw i8, ptr %new.ptr, i64 8
  store i32 1, ptr %id_ptr.i.i, align 4
  store ptr @Derived_vtable, ptr %new.ptr, align 8
  %extra_ptr.i = getelementptr inbounds nuw i8, ptr %new.ptr, i64 12
  store i32 2, ptr %extra_ptr.i, align 4
  %1 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.3)
  %del.vtable = load ptr, ptr %new.ptr, align 8
  %del.dtor.fn = load ptr, ptr %del.vtable, align 8
  tail call void %del.dtor.fn(ptr nonnull %new.ptr)
  tail call void @free(ptr %new.ptr)
  %2 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.5)
  %new.ptr3 = tail call ptr @malloc(i32 16)
  %id_ptr.i.i25 = getelementptr inbounds nuw i8, ptr %new.ptr3, i64 8
  store i32 3, ptr %id_ptr.i.i25, align 4
  store ptr @Derived_vtable, ptr %new.ptr3, align 8
  %extra_ptr.i26 = getelementptr inbounds nuw i8, ptr %new.ptr3, i64 12
  store i32 4, ptr %extra_ptr.i26, align 4
  %3 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.3)
  %del.vtable11 = load ptr, ptr %new.ptr3, align 8
  %del.dtor.fn13 = load ptr, ptr %del.vtable11, align 8
  tail call void %del.dtor.fn13(ptr nonnull %new.ptr3)
  tail call void @free(ptr %new.ptr3)
  %4 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.6)
  %new.ptr14 = tail call ptr @malloc(i32 16)
  store ptr @Base_vtable, ptr %new.ptr14, align 8
  %id_ptr.i = getelementptr inbounds nuw i8, ptr %new.ptr14, i64 8
  store i32 5, ptr %id_ptr.i, align 4
  %5 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.1)
  %del.vtable22 = load ptr, ptr %new.ptr14, align 8
  %del.dtor.fn24 = load ptr, ptr %del.vtable22, align 8
  tail call void %del.dtor.fn24(ptr nonnull %new.ptr14)
  tail call void @free(ptr %new.ptr14)
  %6 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.7)
  ret i32 0
}

declare ptr @malloc(i32) local_unnamed_addr

; Function Attrs: mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite)
declare void @free(ptr allocptr noundef captures(none)) local_unnamed_addr #2

define noundef i32 @main() local_unnamed_addr {
entry:
  %0 = tail call i32 @Test23_main()
  ret i32 0
}

attributes #0 = { nofree nounwind }
attributes #1 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write) }
attributes #2 = { mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite) "alloc-family"="malloc" }
