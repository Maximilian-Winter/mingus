; ModuleID = 'mingus_module'
source_filename = "mingus_module"
target triple = "x86_64-pc-windows-msvc"

@Cat_vtable = internal constant [2 x ptr] [ptr @Test33_Cat_destructor, ptr @Test33_Cat_print]
@Dog_vtable = internal constant [2 x ptr] [ptr @Test33_Dog_destructor, ptr @Test33_Dog_print]
@str = private unnamed_addr constant [20 x i8] c"Dog #%d says Woof!\0A\00", align 1
@str.1 = private unnamed_addr constant [20 x i8] c"Cat #%d says Meow!\0A\00", align 1
@str.2 = private unnamed_addr constant [34 x i8] c"=== Test 33: Interface Params ===\00", align 1

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr noundef readonly captures(none), i32 noundef, ...) local_unnamed_addr #0

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr noundef readonly captures(none)) local_unnamed_addr #0

define void @Test33_printBoth({ ptr, ptr } %a, { ptr, ptr } %b) local_unnamed_addr {
entry:
  %a.fca.0.extract = extractvalue { ptr, ptr } %a, 0
  %a.fca.1.extract = extractvalue { ptr, ptr } %a, 1
  %b.fca.0.extract = extractvalue { ptr, ptr } %b, 0
  %b.fca.1.extract = extractvalue { ptr, ptr } %b, 1
  %iface.fn = load ptr, ptr %a.fca.1.extract, align 8
  tail call void %iface.fn(ptr %a.fca.0.extract)
  %iface.fn8 = load ptr, ptr %b.fca.1.extract, align 8
  tail call void %iface.fn8(ptr %b.fca.0.extract)
  ret void
}

; Function Attrs: nofree nounwind
define void @Test33_Cat_print(ptr readonly captures(none) %this) #0 {
entry:
  %id_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  %id = load i32, ptr %id_ptr, align 4
  %0 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.1, i32 %id)
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write)
define void @Test33_Cat_constructor(ptr writeonly captures(none) initializes((0, 12)) %this, i32 %i) local_unnamed_addr #1 {
entry:
  store ptr @Cat_vtable, ptr %this, align 8
  %id_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  store i32 %i, ptr %id_ptr, align 4
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define void @Test33_Cat_destructor(ptr readnone captures(none) %this) #2 {
entry:
  ret void
}

define noundef i32 @Test33_main() local_unnamed_addr {
entry:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.2)
  %new.ptr = tail call ptr @malloc(i32 16)
  store ptr @Dog_vtable, ptr %new.ptr, align 8
  %id_ptr.i = getelementptr inbounds nuw i8, ptr %new.ptr, i64 8
  store i32 1, ptr %id_ptr.i, align 4
  %new.ptr1 = tail call ptr @malloc(i32 16)
  store ptr @Cat_vtable, ptr %new.ptr1, align 8
  %id_ptr.i22 = getelementptr inbounds nuw i8, ptr %new.ptr1, i64 8
  store i32 2, ptr %id_ptr.i22, align 4
  %id.i = load i32, ptr %id_ptr.i, align 4
  %1 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str, i32 %id.i)
  %id.i25 = load i32, ptr %id_ptr.i22, align 4
  %2 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.1, i32 %id.i25)
  %id.i29 = load i32, ptr %id_ptr.i, align 4
  %3 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str, i32 %id.i29)
  %id.i27 = load i32, ptr %id_ptr.i22, align 4
  %4 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.1, i32 %id.i27)
  %id.i31 = load i32, ptr %id_ptr.i, align 4
  %5 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str, i32 %id.i31)
  %del.vtable = load ptr, ptr %new.ptr, align 8
  %del.dtor.fn = load ptr, ptr %del.vtable, align 8
  tail call void %del.dtor.fn(ptr nonnull %new.ptr)
  tail call void @free(ptr %new.ptr)
  %del.vtable19 = load ptr, ptr %new.ptr1, align 8
  %del.dtor.fn21 = load ptr, ptr %del.vtable19, align 8
  tail call void %del.dtor.fn21(ptr nonnull %new.ptr1)
  tail call void @free(ptr %new.ptr1)
  ret i32 0
}

define void @Test33_printAnimal({ ptr, ptr } %p) local_unnamed_addr {
entry:
  %p.fca.0.extract = extractvalue { ptr, ptr } %p, 0
  %p.fca.1.extract = extractvalue { ptr, ptr } %p, 1
  %iface.fn = load ptr, ptr %p.fca.1.extract, align 8
  tail call void %iface.fn(ptr %p.fca.0.extract)
  ret void
}

; Function Attrs: nofree nounwind
define void @Test33_Dog_print(ptr readonly captures(none) %this) #0 {
entry:
  %id_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  %id = load i32, ptr %id_ptr, align 4
  %0 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str, i32 %id)
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write)
define void @Test33_Dog_constructor(ptr writeonly captures(none) initializes((0, 12)) %this, i32 %i) local_unnamed_addr #1 {
entry:
  store ptr @Dog_vtable, ptr %this, align 8
  %id_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  store i32 %i, ptr %id_ptr, align 4
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define void @Test33_Dog_destructor(ptr readnone captures(none) %this) #2 {
entry:
  ret void
}

declare ptr @malloc(i32) local_unnamed_addr

; Function Attrs: mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite)
declare void @free(ptr allocptr noundef captures(none)) local_unnamed_addr #3

attributes #0 = { nofree nounwind }
attributes #1 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write) }
attributes #2 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) }
attributes #3 = { mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite) "alloc-family"="malloc" }
