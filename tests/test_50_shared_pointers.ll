; ModuleID = 'mingus_module'
source_filename = "mingus_module"
target triple = "x86_64-pc-windows-msvc"

@Animal_vtable = internal constant [3 x ptr] [ptr @Test50_Animal_destructor, ptr @Test50_Animal_speak, ptr @Test50_Animal_getId]
@Dog_vtable = internal constant [3 x ptr] [ptr @Test50_Dog_destructor, ptr @Test50_Dog_speak, ptr @Test50_Animal_getId]
@Counter_vtable = internal constant [2 x ptr] [ptr @Test50_Counter_destructor, ptr @Test50_Counter_getVal]
@str = private unnamed_addr constant [22 x i8] c"  Animal(%d) created\0A\00", align 1
@str.1 = private unnamed_addr constant [24 x i8] c"  Animal(%d) destroyed\0A\00", align 1
@str.2 = private unnamed_addr constant [25 x i8] c"  Animal(%d) speaks: %s\0A\00", align 1
@str.3 = private unnamed_addr constant [19 x i8] c"  Dog(%d) created\0A\00", align 1
@str.4 = private unnamed_addr constant [21 x i8] c"  Dog(%d) destroyed\0A\00", align 1
@str.5 = private unnamed_addr constant [28 x i8] c"  Dog(%d) barks: %s the %s\0A\00", align 1
@str.6 = private unnamed_addr constant [12 x i8] c"test_basic:\00", align 1
@str.7 = private unnamed_addr constant [4 x i8] c"Rex\00", align 1
@str.9 = private unnamed_addr constant [12 x i8] c"test_scope:\00", align 1
@str.10 = private unnamed_addr constant [5 x i8] c"Luna\00", align 1
@str.11 = private unnamed_addr constant [25 x i8] c"  leaving inner scope...\00", align 1
@str.12 = private unnamed_addr constant [22 x i8] c"  back in outer scope\00", align 1
@str.13 = private unnamed_addr constant [15 x i8] c"test_reassign:\00", align 1
@str.14 = private unnamed_addr constant [4 x i8] c"Max\00", align 1
@str.15 = private unnamed_addr constant [17 x i8] c"  reassigning...\00", align 1
@str.16 = private unnamed_addr constant [6 x i8] c"Bella\00", align 1
@str.18 = private unnamed_addr constant [11 x i8] c"test_null:\00", align 1
@str.19 = private unnamed_addr constant [25 x i8] c"  null shared pointer OK\00", align 1
@str.20 = private unnamed_addr constant [18 x i8] c"test_inheritance:\00", align 1
@str.21 = private unnamed_addr constant [6 x i8] c"Buddy\00", align 1
@str.22 = private unnamed_addr constant [9 x i8] c"Labrador\00", align 1
@str.23 = private unnamed_addr constant [11 x i8] c"  id = %d\0A\00", align 1
@str.24 = private unnamed_addr constant [19 x i8] c"  leaving scope...\00", align 1
@str.25 = private unnamed_addr constant [13 x i8] c"test_delete:\00", align 1
@str.26 = private unnamed_addr constant [8 x i8] c"Charlie\00", align 1
@str.27 = private unnamed_addr constant [15 x i8] c"  after delete\00", align 1
@str.28 = private unnamed_addr constant [13 x i8] c"test_stress:\00", align 1
@str.29 = private unnamed_addr constant [18 x i8] c"  stress sum: %d\0A\00", align 1
@str.30 = private unnamed_addr constant [33 x i8] c"=== Test 50: Shared Pointers ===\00", align 1
@str.31 = private unnamed_addr constant [25 x i8] c"=== Test 50 complete ===\00", align 1

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr noundef readonly captures(none), ...) local_unnamed_addr #0

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr noundef readonly captures(none)) local_unnamed_addr #0

; Function Attrs: nofree nounwind
define void @Test50_Animal_destructor(ptr readonly captures(none) %this) #0 {
entry:
  %id_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  %id = load i32, ptr %id_ptr, align 4
  %0 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.1, i32 %id)
  ret void
}

; Function Attrs: nofree nounwind
define void @Test50_Animal_constructor(ptr writeonly captures(none) initializes((0, 12), (16, 24)) %this, i32 %id, ptr %name) local_unnamed_addr #0 {
entry:
  store ptr @Animal_vtable, ptr %this, align 8
  %id_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  store i32 %id, ptr %id_ptr, align 4
  %name_ptr = getelementptr inbounds nuw i8, ptr %this, i64 16
  store ptr %name, ptr %name_ptr, align 8
  %0 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str, i32 %id)
  ret void
}

; Function Attrs: nofree nounwind
define void @Test50_Animal_speak(ptr readonly captures(none) %this) #0 {
entry:
  %id_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  %id = load i32, ptr %id_ptr, align 4
  %name_ptr = getelementptr inbounds nuw i8, ptr %this, i64 16
  %name = load ptr, ptr %name_ptr, align 8
  %0 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.2, i32 %id, ptr %name)
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read)
define i32 @Test50_Animal_getId(ptr readonly captures(none) %this) #1 {
entry:
  %id_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  %id = load i32, ptr %id_ptr, align 4
  ret i32 %id
}

; Function Attrs: nofree nounwind
define void @Test50_Dog_constructor(ptr writeonly captures(none) initializes((0, 12), (16, 32)) %this, i32 %id, ptr %name, ptr %breed) local_unnamed_addr #0 {
entry:
  store ptr @Animal_vtable, ptr %this, align 8
  %id_ptr.i = getelementptr inbounds nuw i8, ptr %this, i64 8
  store i32 %id, ptr %id_ptr.i, align 4
  %name_ptr.i = getelementptr inbounds nuw i8, ptr %this, i64 16
  store ptr %name, ptr %name_ptr.i, align 8
  %0 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str, i32 %id)
  store ptr @Dog_vtable, ptr %this, align 8
  %breed_ptr = getelementptr inbounds nuw i8, ptr %this, i64 24
  store ptr %breed, ptr %breed_ptr, align 8
  %1 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.3, i32 %id)
  ret void
}

; Function Attrs: nofree nounwind
define void @Test50_Dog_destructor(ptr readonly captures(none) %this) #0 {
entry:
  %id_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  %id = load i32, ptr %id_ptr, align 4
  %0 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.4, i32 %id)
  %id.i = load i32, ptr %id_ptr, align 4
  %1 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.1, i32 %id.i)
  ret void
}

; Function Attrs: nofree nounwind
define void @Test50_Dog_speak(ptr readonly captures(none) %this) #0 {
entry:
  %id_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  %id = load i32, ptr %id_ptr, align 4
  %name_ptr = getelementptr inbounds nuw i8, ptr %this, i64 16
  %name = load ptr, ptr %name_ptr, align 8
  %breed_ptr = getelementptr inbounds nuw i8, ptr %this, i64 24
  %breed = load ptr, ptr %breed_ptr, align 8
  %0 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.5, i32 %id, ptr %name, ptr %breed)
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write)
define void @Test50_Counter_constructor(ptr writeonly captures(none) initializes((0, 12)) %this, i32 %v) local_unnamed_addr #2 {
entry:
  store ptr @Counter_vtable, ptr %this, align 8
  %val_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  store i32 %v, ptr %val_ptr, align 4
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define void @Test50_Counter_destructor(ptr readnone captures(none) %this) #3 {
entry:
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read)
define i32 @Test50_Counter_getVal(ptr readonly captures(none) %this) #1 {
entry:
  %val_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  %val = load i32, ptr %val_ptr, align 4
  ret i32 %val
}

define void @Test50_test_scope() local_unnamed_addr {
do_release.i.i:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.9)
  %shared.rc = tail call ptr @malloc(i32 48)
  store i64 1, ptr %shared.rc, align 4
  %weak.ptr = getelementptr inbounds nuw i8, ptr %shared.rc, i64 8
  store i64 0, ptr %weak.ptr, align 4
  %cleanup.ptr = getelementptr inbounds nuw i8, ptr %shared.rc, i64 16
  store ptr @__shared_cleanup_Animal, ptr %cleanup.ptr, align 8
  %shared.obj = getelementptr i8, ptr %shared.rc, i64 24
  store ptr @Animal_vtable, ptr %shared.obj, align 8
  %id_ptr.i = getelementptr i8, ptr %shared.rc, i64 32
  store i32 2, ptr %id_ptr.i, align 4
  %name_ptr.i = getelementptr i8, ptr %shared.rc, i64 40
  store ptr @str.10, ptr %name_ptr.i, align 8
  %1 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str, i32 2)
  %vtable = load ptr, ptr %shared.obj, align 8
  %method.slot = getelementptr i8, ptr %vtable, i64 8
  %method.fn = load ptr, ptr %method.slot, align 8
  tail call void %method.fn(ptr nonnull %shared.obj)
  %2 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.11)
  %rc.i.i = load i64, ptr %shared.rc, align 4
  %rc_dec.i.i = add i64 %rc.i.i, -1
  store i64 %rc_dec.i.i, ptr %shared.rc, align 4
  %is_zero.i.i = icmp eq i64 %rc_dec.i.i, 0
  br i1 %is_zero.i.i, label %cleanup.i.i, label %__mingus_shared_release_wrapper.exit

cleanup.i.i:                                      ; preds = %do_release.i.i
  %cleanup_fn.i.i = load ptr, ptr %cleanup.ptr, align 8
  %has_cleanup.not.i.i = icmp eq ptr %cleanup_fn.i.i, null
  br i1 %has_cleanup.not.i.i, label %do_free.i.i, label %call_cleanup.i.i

call_cleanup.i.i:                                 ; preds = %cleanup.i.i
  tail call void %cleanup_fn.i.i(ptr nonnull %shared.rc)
  br label %do_free.i.i

do_free.i.i:                                      ; preds = %call_cleanup.i.i, %cleanup.i.i
  %weak_count.i.i = load i64, ptr %weak.ptr, align 4
  %weak_zero.i.i = icmp eq i64 %weak_count.i.i, 0
  br i1 %weak_zero.i.i, label %actual_free.i.i, label %__mingus_shared_release_wrapper.exit

actual_free.i.i:                                  ; preds = %do_free.i.i
  tail call void @free(ptr nonnull %shared.rc)
  br label %__mingus_shared_release_wrapper.exit

__mingus_shared_release_wrapper.exit:             ; preds = %do_release.i.i, %do_free.i.i, %actual_free.i.i
  %3 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.12)
  ret void
}

define void @Test50_test_reassign() local_unnamed_addr {
do_release.i:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.13)
  %shared.rc = tail call ptr @malloc(i32 48)
  store i64 1, ptr %shared.rc, align 4
  %weak.ptr = getelementptr inbounds nuw i8, ptr %shared.rc, i64 8
  store i64 0, ptr %weak.ptr, align 4
  %cleanup.ptr = getelementptr inbounds nuw i8, ptr %shared.rc, i64 16
  store ptr @__shared_cleanup_Animal, ptr %cleanup.ptr, align 8
  %shared.obj = getelementptr i8, ptr %shared.rc, i64 24
  store ptr @Animal_vtable, ptr %shared.obj, align 8
  %id_ptr.i = getelementptr i8, ptr %shared.rc, i64 32
  store i32 3, ptr %id_ptr.i, align 4
  %name_ptr.i = getelementptr i8, ptr %shared.rc, i64 40
  store ptr @str.14, ptr %name_ptr.i, align 8
  %1 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str, i32 3)
  %vtable = load ptr, ptr %shared.obj, align 8
  %method.slot = getelementptr i8, ptr %vtable, i64 8
  %method.fn = load ptr, ptr %method.slot, align 8
  tail call void %method.fn(ptr nonnull %shared.obj)
  %2 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.15)
  %rc.i = load i64, ptr %shared.rc, align 4
  %rc_dec.i = add i64 %rc.i, -1
  store i64 %rc_dec.i, ptr %shared.rc, align 4
  %is_zero.i = icmp eq i64 %rc_dec.i, 0
  br i1 %is_zero.i, label %cleanup.i, label %do_release.i.i

cleanup.i:                                        ; preds = %do_release.i
  %cleanup_fn.i = load ptr, ptr %cleanup.ptr, align 8
  %has_cleanup.not.i = icmp eq ptr %cleanup_fn.i, null
  br i1 %has_cleanup.not.i, label %do_free.i, label %call_cleanup.i

call_cleanup.i:                                   ; preds = %cleanup.i
  tail call void %cleanup_fn.i(ptr nonnull %shared.rc)
  br label %do_free.i

do_free.i:                                        ; preds = %call_cleanup.i, %cleanup.i
  %weak_count.i = load i64, ptr %weak.ptr, align 4
  %weak_zero.i = icmp eq i64 %weak_count.i, 0
  br i1 %weak_zero.i, label %actual_free.i, label %do_release.i.i

actual_free.i:                                    ; preds = %do_free.i
  tail call void @free(ptr nonnull %shared.rc)
  br label %do_release.i.i

do_release.i.i:                                   ; preds = %actual_free.i, %do_free.i, %do_release.i
  %shared.rc3 = tail call ptr @malloc(i32 48)
  store i64 1, ptr %shared.rc3, align 4
  %weak.ptr5 = getelementptr inbounds nuw i8, ptr %shared.rc3, i64 8
  store i64 0, ptr %weak.ptr5, align 4
  %cleanup.ptr6 = getelementptr inbounds nuw i8, ptr %shared.rc3, i64 16
  store ptr @__shared_cleanup_Animal, ptr %cleanup.ptr6, align 8
  %shared.obj7 = getelementptr i8, ptr %shared.rc3, i64 24
  store ptr @Animal_vtable, ptr %shared.obj7, align 8
  %id_ptr.i14 = getelementptr i8, ptr %shared.rc3, i64 32
  store i32 4, ptr %id_ptr.i14, align 4
  %name_ptr.i15 = getelementptr i8, ptr %shared.rc3, i64 40
  store ptr @str.16, ptr %name_ptr.i15, align 8
  %3 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str, i32 4)
  %vtable11 = load ptr, ptr %shared.obj7, align 8
  %method.slot12 = getelementptr i8, ptr %vtable11, i64 8
  %method.fn13 = load ptr, ptr %method.slot12, align 8
  tail call void %method.fn13(ptr nonnull %shared.obj7)
  %4 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.24)
  %rc.i.i = load i64, ptr %shared.rc3, align 4
  %rc_dec.i.i = add i64 %rc.i.i, -1
  store i64 %rc_dec.i.i, ptr %shared.rc3, align 4
  %is_zero.i.i = icmp eq i64 %rc_dec.i.i, 0
  br i1 %is_zero.i.i, label %cleanup.i.i, label %__mingus_shared_release_wrapper.exit

cleanup.i.i:                                      ; preds = %do_release.i.i
  %cleanup_fn.i.i = load ptr, ptr %cleanup.ptr6, align 8
  %has_cleanup.not.i.i = icmp eq ptr %cleanup_fn.i.i, null
  br i1 %has_cleanup.not.i.i, label %do_free.i.i, label %call_cleanup.i.i

call_cleanup.i.i:                                 ; preds = %cleanup.i.i
  tail call void %cleanup_fn.i.i(ptr nonnull %shared.rc3)
  br label %do_free.i.i

do_free.i.i:                                      ; preds = %call_cleanup.i.i, %cleanup.i.i
  %weak_count.i.i = load i64, ptr %weak.ptr5, align 4
  %weak_zero.i.i = icmp eq i64 %weak_count.i.i, 0
  br i1 %weak_zero.i.i, label %actual_free.i.i, label %__mingus_shared_release_wrapper.exit

actual_free.i.i:                                  ; preds = %do_free.i.i
  tail call void @free(ptr nonnull %shared.rc3)
  br label %__mingus_shared_release_wrapper.exit

__mingus_shared_release_wrapper.exit:             ; preds = %do_release.i.i, %do_free.i.i, %actual_free.i.i
  ret void
}

; Function Attrs: nofree nounwind
define void @Test50_test_null() local_unnamed_addr #0 {
__mingus_shared_release_wrapper.exit:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.18)
  %1 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.19)
  ret void
}

define void @Test50_test_basic() local_unnamed_addr {
do_release.i.i:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.6)
  %shared.rc = tail call ptr @malloc(i32 48)
  store i64 1, ptr %shared.rc, align 4
  %weak.ptr = getelementptr inbounds nuw i8, ptr %shared.rc, i64 8
  store i64 0, ptr %weak.ptr, align 4
  %cleanup.ptr = getelementptr inbounds nuw i8, ptr %shared.rc, i64 16
  store ptr @__shared_cleanup_Animal, ptr %cleanup.ptr, align 8
  %shared.obj = getelementptr i8, ptr %shared.rc, i64 24
  store ptr @Animal_vtable, ptr %shared.obj, align 8
  %id_ptr.i = getelementptr i8, ptr %shared.rc, i64 32
  store i32 1, ptr %id_ptr.i, align 4
  %name_ptr.i = getelementptr i8, ptr %shared.rc, i64 40
  store ptr @str.7, ptr %name_ptr.i, align 8
  %1 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str, i32 1)
  %vtable = load ptr, ptr %shared.obj, align 8
  %method.slot = getelementptr i8, ptr %vtable, i64 8
  %method.fn = load ptr, ptr %method.slot, align 8
  tail call void %method.fn(ptr nonnull %shared.obj)
  %2 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.24)
  %rc.i.i = load i64, ptr %shared.rc, align 4
  %rc_dec.i.i = add i64 %rc.i.i, -1
  store i64 %rc_dec.i.i, ptr %shared.rc, align 4
  %is_zero.i.i = icmp eq i64 %rc_dec.i.i, 0
  br i1 %is_zero.i.i, label %cleanup.i.i, label %__mingus_shared_release_wrapper.exit

cleanup.i.i:                                      ; preds = %do_release.i.i
  %cleanup_fn.i.i = load ptr, ptr %cleanup.ptr, align 8
  %has_cleanup.not.i.i = icmp eq ptr %cleanup_fn.i.i, null
  br i1 %has_cleanup.not.i.i, label %do_free.i.i, label %call_cleanup.i.i

call_cleanup.i.i:                                 ; preds = %cleanup.i.i
  tail call void %cleanup_fn.i.i(ptr nonnull %shared.rc)
  br label %do_free.i.i

do_free.i.i:                                      ; preds = %call_cleanup.i.i, %cleanup.i.i
  %weak_count.i.i = load i64, ptr %weak.ptr, align 4
  %weak_zero.i.i = icmp eq i64 %weak_count.i.i, 0
  br i1 %weak_zero.i.i, label %actual_free.i.i, label %__mingus_shared_release_wrapper.exit

actual_free.i.i:                                  ; preds = %do_free.i.i
  tail call void @free(ptr nonnull %shared.rc)
  br label %__mingus_shared_release_wrapper.exit

__mingus_shared_release_wrapper.exit:             ; preds = %do_release.i.i, %do_free.i.i, %actual_free.i.i
  ret void
}

define void @Test50_test_inheritance() local_unnamed_addr {
do_release.i.i:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.20)
  %shared.rc = tail call ptr @malloc(i32 56)
  store i64 1, ptr %shared.rc, align 4
  %weak.ptr = getelementptr inbounds nuw i8, ptr %shared.rc, i64 8
  store i64 0, ptr %weak.ptr, align 4
  %cleanup.ptr = getelementptr inbounds nuw i8, ptr %shared.rc, i64 16
  store ptr @__shared_cleanup_Dog, ptr %cleanup.ptr, align 8
  %shared.obj = getelementptr i8, ptr %shared.rc, i64 24
  store ptr @Animal_vtable, ptr %shared.obj, align 8
  %id_ptr.i.i = getelementptr i8, ptr %shared.rc, i64 32
  store i32 5, ptr %id_ptr.i.i, align 4
  %name_ptr.i.i = getelementptr i8, ptr %shared.rc, i64 40
  store ptr @str.21, ptr %name_ptr.i.i, align 8
  %1 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str, i32 5)
  store ptr @Dog_vtable, ptr %shared.obj, align 8
  %breed_ptr.i = getelementptr i8, ptr %shared.rc, i64 48
  store ptr @str.22, ptr %breed_ptr.i, align 8
  %2 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.3, i32 5)
  %vtable = load ptr, ptr %shared.obj, align 8
  %method.slot = getelementptr i8, ptr %vtable, i64 8
  %method.fn = load ptr, ptr %method.slot, align 8
  tail call void %method.fn(ptr nonnull %shared.obj)
  %vtable6 = load ptr, ptr %shared.obj, align 8
  %method.slot7 = getelementptr i8, ptr %vtable6, i64 16
  %method.fn8 = load ptr, ptr %method.slot7, align 8
  %3 = tail call i32 %method.fn8(ptr nonnull %shared.obj)
  %4 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.23, i32 %3)
  %5 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.24)
  %rc.i.i = load i64, ptr %shared.rc, align 4
  %rc_dec.i.i = add i64 %rc.i.i, -1
  store i64 %rc_dec.i.i, ptr %shared.rc, align 4
  %is_zero.i.i = icmp eq i64 %rc_dec.i.i, 0
  br i1 %is_zero.i.i, label %cleanup.i.i, label %__mingus_shared_release_wrapper.exit

cleanup.i.i:                                      ; preds = %do_release.i.i
  %cleanup_fn.i.i = load ptr, ptr %cleanup.ptr, align 8
  %has_cleanup.not.i.i = icmp eq ptr %cleanup_fn.i.i, null
  br i1 %has_cleanup.not.i.i, label %do_free.i.i, label %call_cleanup.i.i

call_cleanup.i.i:                                 ; preds = %cleanup.i.i
  tail call void %cleanup_fn.i.i(ptr nonnull %shared.rc)
  br label %do_free.i.i

do_free.i.i:                                      ; preds = %call_cleanup.i.i, %cleanup.i.i
  %weak_count.i.i = load i64, ptr %weak.ptr, align 4
  %weak_zero.i.i = icmp eq i64 %weak_count.i.i, 0
  br i1 %weak_zero.i.i, label %actual_free.i.i, label %__mingus_shared_release_wrapper.exit

actual_free.i.i:                                  ; preds = %do_free.i.i
  tail call void @free(ptr nonnull %shared.rc)
  br label %__mingus_shared_release_wrapper.exit

__mingus_shared_release_wrapper.exit:             ; preds = %do_release.i.i, %do_free.i.i, %actual_free.i.i
  ret void
}

define void @Test50_test_delete() local_unnamed_addr {
do_release.i:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.25)
  %shared.rc = tail call ptr @malloc(i32 48)
  store i64 1, ptr %shared.rc, align 4
  %weak.ptr = getelementptr inbounds nuw i8, ptr %shared.rc, i64 8
  store i64 0, ptr %weak.ptr, align 4
  %cleanup.ptr = getelementptr inbounds nuw i8, ptr %shared.rc, i64 16
  store ptr @__shared_cleanup_Animal, ptr %cleanup.ptr, align 8
  %shared.obj = getelementptr i8, ptr %shared.rc, i64 24
  store ptr @Animal_vtable, ptr %shared.obj, align 8
  %id_ptr.i = getelementptr i8, ptr %shared.rc, i64 32
  store i32 6, ptr %id_ptr.i, align 4
  %name_ptr.i = getelementptr i8, ptr %shared.rc, i64 40
  store ptr @str.26, ptr %name_ptr.i, align 8
  %1 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str, i32 6)
  %vtable = load ptr, ptr %shared.obj, align 8
  %method.slot = getelementptr i8, ptr %vtable, i64 8
  %method.fn = load ptr, ptr %method.slot, align 8
  tail call void %method.fn(ptr nonnull %shared.obj)
  %rc.i = load i64, ptr %shared.rc, align 4
  %rc_dec.i = add i64 %rc.i, -1
  store i64 %rc_dec.i, ptr %shared.rc, align 4
  %is_zero.i = icmp eq i64 %rc_dec.i, 0
  br i1 %is_zero.i, label %cleanup.i, label %do_release.i.i

cleanup.i:                                        ; preds = %do_release.i
  %cleanup_fn.i = load ptr, ptr %cleanup.ptr, align 8
  %has_cleanup.not.i = icmp eq ptr %cleanup_fn.i, null
  br i1 %has_cleanup.not.i, label %do_free.i, label %call_cleanup.i

call_cleanup.i:                                   ; preds = %cleanup.i
  tail call void %cleanup_fn.i(ptr nonnull %shared.rc)
  br label %do_free.i

do_free.i:                                        ; preds = %call_cleanup.i, %cleanup.i
  %weak_count.i = load i64, ptr %weak.ptr, align 4
  %weak_zero.i = icmp eq i64 %weak_count.i, 0
  br i1 %weak_zero.i, label %actual_free.i, label %do_release.i.i

actual_free.i:                                    ; preds = %do_free.i
  tail call void @free(ptr nonnull %shared.rc)
  br label %do_release.i.i

do_release.i.i:                                   ; preds = %actual_free.i, %do_free.i, %do_release.i
  %2 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.27)
  %rc.i.i = load i64, ptr %shared.rc, align 4
  %rc_dec.i.i = add i64 %rc.i.i, -1
  store i64 %rc_dec.i.i, ptr %shared.rc, align 4
  %is_zero.i.i = icmp eq i64 %rc_dec.i.i, 0
  br i1 %is_zero.i.i, label %cleanup.i.i, label %__mingus_shared_release_wrapper.exit

cleanup.i.i:                                      ; preds = %do_release.i.i
  %cleanup_fn.i.i = load ptr, ptr %cleanup.ptr, align 8
  %has_cleanup.not.i.i = icmp eq ptr %cleanup_fn.i.i, null
  br i1 %has_cleanup.not.i.i, label %do_free.i.i, label %call_cleanup.i.i

call_cleanup.i.i:                                 ; preds = %cleanup.i.i
  tail call void %cleanup_fn.i.i(ptr nonnull %shared.rc)
  br label %do_free.i.i

do_free.i.i:                                      ; preds = %call_cleanup.i.i, %cleanup.i.i
  %weak_count.i.i = load i64, ptr %weak.ptr, align 4
  %weak_zero.i.i = icmp eq i64 %weak_count.i.i, 0
  br i1 %weak_zero.i.i, label %actual_free.i.i, label %__mingus_shared_release_wrapper.exit

actual_free.i.i:                                  ; preds = %do_free.i.i
  tail call void @free(ptr nonnull %shared.rc)
  br label %__mingus_shared_release_wrapper.exit

__mingus_shared_release_wrapper.exit:             ; preds = %do_release.i.i, %do_free.i.i, %actual_free.i.i
  ret void
}

define void @Test50_test_stress() local_unnamed_addr {
entry:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.28)
  br label %__mingus_shared_release_wrapper.exit

__mingus_shared_release_wrapper.exit:             ; preds = %entry, %__mingus_shared_release_wrapper.exit
  %i.012 = phi i32 [ 0, %entry ], [ %add7, %__mingus_shared_release_wrapper.exit ]
  %shared.rc = tail call ptr @malloc(i32 40)
  tail call void @free(ptr nonnull %shared.rc)
  %add7 = add nuw nsw i32 %i.012, 1
  %slt = icmp samesign ult i32 %i.012, 999
  br i1 %slt, label %__mingus_shared_release_wrapper.exit, label %for.exit

for.exit:                                         ; preds = %__mingus_shared_release_wrapper.exit
  %1 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.29, i32 499500)
  ret void
}

define noundef i32 @Test50_main() local_unnamed_addr {
entry:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.30)
  tail call void @Test50_test_basic()
  tail call void @Test50_test_scope()
  tail call void @Test50_test_reassign()
  %1 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.18)
  %2 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.19)
  tail call void @Test50_test_inheritance()
  tail call void @Test50_test_delete()
  %3 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.28)
  br label %__mingus_shared_release_wrapper.exit.i

__mingus_shared_release_wrapper.exit.i:           ; preds = %__mingus_shared_release_wrapper.exit.i, %entry
  %i.012.i = phi i32 [ 0, %entry ], [ %add7.i, %__mingus_shared_release_wrapper.exit.i ]
  %shared.rc.i = tail call ptr @malloc(i32 40)
  tail call void @free(ptr nonnull %shared.rc.i)
  %add7.i = add nuw nsw i32 %i.012.i, 1
  %slt.i = icmp samesign ult i32 %i.012.i, 999
  br i1 %slt.i, label %__mingus_shared_release_wrapper.exit.i, label %Test50_test_stress.exit

Test50_test_stress.exit:                          ; preds = %__mingus_shared_release_wrapper.exit.i
  %4 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.29, i32 499500)
  %5 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.31)
  ret i32 0
}

declare ptr @malloc(i32) local_unnamed_addr

define internal void @__shared_cleanup_Animal(ptr %0) {
entry:
  %shared.obj = getelementptr i8, ptr %0, i64 24
  %vtable = load ptr, ptr %shared.obj, align 8
  %dtor.fn = load ptr, ptr %vtable, align 8
  tail call void %dtor.fn(ptr nonnull %shared.obj)
  ret void
}

; Function Attrs: mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite)
declare void @free(ptr allocptr noundef captures(none)) local_unnamed_addr #4

define internal void @__shared_cleanup_Dog(ptr %0) {
entry:
  %shared.obj = getelementptr i8, ptr %0, i64 24
  %vtable = load ptr, ptr %shared.obj, align 8
  %dtor.fn = load ptr, ptr %vtable, align 8
  tail call void %dtor.fn(ptr nonnull %shared.obj)
  ret void
}

define noundef i32 @main() local_unnamed_addr {
entry:
  %0 = tail call i32 @Test50_main()
  ret i32 0
}

attributes #0 = { nofree nounwind }
attributes #1 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read) }
attributes #2 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write) }
attributes #3 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) }
attributes #4 = { mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite) "alloc-family"="malloc" }
