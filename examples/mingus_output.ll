; ModuleID = 'mingus_module'
source_filename = "mingus_module"

@str = private unnamed_addr constant [25 x i8] c"=== Capture Showcase ===\00", align 1
@str.1 = private unnamed_addr constant [26 x i8] c"--- [] Pure functions ---\00", align 1
@str.2 = private unnamed_addr constant [17 x i8] c"double(21) = %d\0A\00", align 1
@str.3 = private unnamed_addr constant [16 x i8] c"negate(7) = %d\0A\00", align 1
@str.4 = private unnamed_addr constant [25 x i8] c"--- [=] Frozen state ---\00", align 1
@str.5 = private unnamed_addr constant [19 x i8] c"withTax(200) = %d\0A\00", align 1
@str.6 = private unnamed_addr constant [18 x i8] c"scaled(200) = %d\0A\00", align 1
@str.7 = private unnamed_addr constant [20 x i8] c"pipeline(100) = %d\0A\00", align 1
@str.8 = private unnamed_addr constant [24 x i8] c"--- [&] Live access ---\00", align 1
@str.9 = private unnamed_addr constant [12 x i8] c"count = %d\0A\00", align 1
@str.10 = private unnamed_addr constant [10 x i8] c"sum = %d\0A\00", align 1
@str.11 = private unnamed_addr constant [25 x i8] c"--- [&x] Accumulator ---\00", align 1
@str.14 = private unnamed_addr constant [20 x i8] c"running totals: %d\0A\00", align 1
@str.15 = private unnamed_addr constant [17 x i8] c"final total: %d\0A\00", align 1
@str.16 = private unnamed_addr constant [27 x i8] c"--- [=, &x] Mixed mode ---\00", align 1
@str.17 = private unnamed_addr constant [30 x i8] c"after 3 steps: position = %d\0A\00", align 1
@str.18 = private unnamed_addr constant [49 x i8] c"after 2 more (step frozen at 10): position = %d\0A\00", align 1
@str.19 = private unnamed_addr constant [29 x i8] c"--- [&, x] Reverse mixed ---\00", align 1
@str.20 = private unnamed_addr constant [19 x i8] c"lo (below 50): %d\0A\00", align 1
@str.21 = private unnamed_addr constant [23 x i8] c"hi (50 and above): %d\0A\00", align 1
@str.22 = private unnamed_addr constant [33 x i8] c"--- Higher-order composition ---\00", align 1
@str.23 = private unnamed_addr constant [30 x i8] c"compose(add10, mul3)(5) = %d\0A\00", align 1
@str.24 = private unnamed_addr constant [32 x i8] c"compose(add10, double)(7) = %d\0A\00", align 1
@str.25 = private unnamed_addr constant [24 x i8] c"--- Nested captures ---\00", align 1
@str.26 = private unnamed_addr constant [27 x i8] c"nested: 100 + 5 + 15 = %d\0A\00", align 1
@str.27 = private unnamed_addr constant [29 x i8] c"--- Capture vs call time ---\00", align 1
@str.28 = private unnamed_addr constant [21 x i8] c"[=] frozen at 1: %d\0A\00", align 1
@str.29 = private unnamed_addr constant [17 x i8] c"[&] sees 42: %d\0A\00", align 1
@fmt.30 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@str.31 = private unnamed_addr constant [34 x i8] c"=== Capture Showcase Complete ===\00", align 1

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr noundef readonly captures(none), i32 noundef, ...) local_unnamed_addr #0

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr noundef readonly captures(none)) local_unnamed_addr #0

define i32 @CaptureShowcase_apply({ ptr, ptr } %f, i32 %x) local_unnamed_addr {
entry:
  %f.fca.0.extract = extractvalue { ptr, ptr } %f, 0
  %f.fca.1.extract = extractvalue { ptr, ptr } %f, 1
  %0 = tail call i32 %f.fca.0.extract(i32 %x, ptr %f.fca.1.extract)
  ret i32 %0
}

define i32 @CaptureShowcase_applyTwice({ ptr, ptr } %f, i32 %x) local_unnamed_addr {
entry:
  %f.fca.0.extract = extractvalue { ptr, ptr } %f, 0
  %f.fca.1.extract = extractvalue { ptr, ptr } %f, 1
  %0 = tail call i32 %f.fca.0.extract(i32 %x, ptr %f.fca.1.extract)
  %1 = tail call i32 %f.fca.0.extract(i32 %0, ptr %f.fca.1.extract)
  ret i32 %1
}

; Function Attrs: mustprogress nofree nounwind willreturn
define { ptr, ptr } @CaptureShowcase_compose({ ptr, ptr } %f, { ptr, ptr } %g) local_unnamed_addr #1 {
entry:
  %f.fca.1.extract = extractvalue { ptr, ptr } %f, 1
  %g.fca.1.extract = extractvalue { ptr, ptr } %g, 1
  %closure.ptr = tail call dereferenceable_or_null(48) ptr @malloc(i64 48)
  store i64 1, ptr %closure.ptr, align 4
  %cleanup.slot = getelementptr inbounds nuw i8, ptr %closure.ptr, i64 8
  store ptr @__closure_cleanup_0, ptr %cleanup.slot, align 8
  %f.slot = getelementptr inbounds nuw i8, ptr %closure.ptr, i64 16
  %f.elt = extractvalue { ptr, ptr } %f, 0
  store ptr %f.elt, ptr %f.slot, align 8
  %f.slot.repack3 = getelementptr inbounds nuw i8, ptr %closure.ptr, i64 24
  store ptr %f.fca.1.extract, ptr %f.slot.repack3, align 8
  %is_null.i = icmp eq ptr %f.fca.1.extract, null
  br i1 %is_null.i, label %__mingus_closure_retain.exit, label %do_retain.i

do_retain.i:                                      ; preds = %entry
  %rc.i = load i64, ptr %f.fca.1.extract, align 4
  %rc_inc.i = add i64 %rc.i, 1
  store i64 %rc_inc.i, ptr %f.fca.1.extract, align 4
  br label %__mingus_closure_retain.exit

__mingus_closure_retain.exit:                     ; preds = %entry, %do_retain.i
  %g.slot = getelementptr inbounds nuw i8, ptr %closure.ptr, i64 32
  %g.elt = extractvalue { ptr, ptr } %g, 0
  store ptr %g.elt, ptr %g.slot, align 8
  %g.slot.repack5 = getelementptr inbounds nuw i8, ptr %closure.ptr, i64 40
  store ptr %g.fca.1.extract, ptr %g.slot.repack5, align 8
  %is_null.i7 = icmp eq ptr %g.fca.1.extract, null
  br i1 %is_null.i7, label %__mingus_closure_retain.exit11, label %do_retain.i8

do_retain.i8:                                     ; preds = %__mingus_closure_retain.exit
  %rc.i9 = load i64, ptr %g.fca.1.extract, align 4
  %rc_inc.i10 = add i64 %rc.i9, 1
  store i64 %rc_inc.i10, ptr %g.fca.1.extract, align 4
  br label %__mingus_closure_retain.exit11

__mingus_closure_retain.exit11:                   ; preds = %__mingus_closure_retain.exit, %do_retain.i8
  %fat.env = insertvalue { ptr, ptr } { ptr @__lambda_0, ptr undef }, ptr %closure.ptr, 1
  ret { ptr, ptr } %fat.env
}

define noundef i32 @CaptureShowcase_main() local_unnamed_addr {
__mingus_closure_release_wrapper.exit439:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str)
  %snprintf.len = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.30)
  %needed.i64 = sext i32 %snprintf.len to i64
  %alloc.size = add nsw i64 %needed.i64, 1
  %interp.buf = tail call ptr @malloc(i64 %alloc.size)
  %buf.size = add i32 %snprintf.len, 1
  %1 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf, i32 %buf.size, ptr nonnull @fmt.30)
  %2 = tail call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf)
  %3 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.1)
  %4 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.2, i32 42)
  %5 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.3, i32 -7)
  %6 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.4)
  %7 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.5, i32 210)
  %8 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.6, i32 600)
  %9 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.7, i32 315)
  %10 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.8)
  %11 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.9, i32 3)
  %12 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.10, i32 60)
  %13 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.11)
  %14 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.14, i32 5)
  %15 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.14, i32 20)
  %16 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.14, i32 50)
  %17 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.15, i32 50)
  %18 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.16)
  %19 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.17, i32 30)
  %20 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.18, i32 50)
  %21 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.19)
  %22 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.20, i32 160)
  %23 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.21, i32 140)
  %24 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.22)
  %25 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.23, i32 25)
  %26 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.24, i32 24)
  %27 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.25)
  %28 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.26, i32 120)
  %29 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.27)
  %30 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.28, i32 1)
  %31 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.29, i32 42)
  %snprintf.len116 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.30)
  %needed.i64117 = sext i32 %snprintf.len116 to i64
  %alloc.size118 = add nsw i64 %needed.i64117, 1
  %interp.buf119 = tail call ptr @malloc(i64 %alloc.size118)
  %buf.size120 = add i32 %snprintf.len116, 1
  %32 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf119, i32 %buf.size120, ptr nonnull @fmt.30)
  %33 = tail call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf119)
  %34 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.31)
  tail call void @free(ptr nonnull %interp.buf119)
  tail call void @free(ptr nonnull %interp.buf)
  ret i32 0
}

define internal i32 @__lambda_0(i32 %x, ptr readonly captures(none) %env) {
entry:
  %f.cap = getelementptr inbounds nuw i8, ptr %env, i64 16
  %f.unpack = load ptr, ptr %f.cap, align 8
  %f.elt9 = getelementptr inbounds nuw i8, ptr %env, i64 24
  %f.unpack10 = load ptr, ptr %f.elt9, align 8
  %g.cap = getelementptr inbounds nuw i8, ptr %env, i64 32
  %g.unpack = load ptr, ptr %g.cap, align 8
  %g.elt12 = getelementptr inbounds nuw i8, ptr %env, i64 40
  %g.unpack13 = load ptr, ptr %g.elt12, align 8
  %0 = tail call i32 %g.unpack(i32 %x, ptr %g.unpack13)
  %1 = tail call i32 %f.unpack(i32 %0, ptr %f.unpack10)
  ret i32 %1
}

; Function Attrs: mustprogress nofree nounwind willreturn allockind("alloc,uninitialized") allocsize(0) memory(inaccessiblemem: readwrite)
declare noalias noundef ptr @malloc(i64 noundef) local_unnamed_addr #2

define internal void @__closure_cleanup_0(ptr readonly captures(none) %0) {
entry:
  %f.fat.elt1 = getelementptr inbounds nuw i8, ptr %0, i64 24
  %f.fat.unpack2 = load ptr, ptr %f.fat.elt1, align 8
  %is_null.i = icmp eq ptr %f.fat.unpack2, null
  br i1 %is_null.i, label %__mingus_closure_release.exit, label %do_release.i

do_release.i:                                     ; preds = %entry
  %rc.i = load i64, ptr %f.fat.unpack2, align 4
  %rc_dec.i = add i64 %rc.i, -1
  store i64 %rc_dec.i, ptr %f.fat.unpack2, align 4
  %is_zero.i = icmp eq i64 %rc_dec.i, 0
  br i1 %is_zero.i, label %cleanup.i, label %__mingus_closure_release.exit

cleanup.i:                                        ; preds = %do_release.i
  %cleanup_fn_ptr.i = getelementptr inbounds nuw i8, ptr %f.fat.unpack2, i64 8
  %cleanup_fn.i = load ptr, ptr %cleanup_fn_ptr.i, align 8
  %has_cleanup.not.i = icmp eq ptr %cleanup_fn.i, null
  br i1 %has_cleanup.not.i, label %do_free.i, label %call_cleanup.i

call_cleanup.i:                                   ; preds = %cleanup.i
  tail call void %cleanup_fn.i(ptr nonnull %f.fat.unpack2)
  br label %do_free.i

do_free.i:                                        ; preds = %call_cleanup.i, %cleanup.i
  tail call void @free(ptr nonnull %f.fat.unpack2)
  br label %__mingus_closure_release.exit

__mingus_closure_release.exit:                    ; preds = %entry, %do_release.i, %do_free.i
  %g.fat.elt4 = getelementptr inbounds nuw i8, ptr %0, i64 40
  %g.fat.unpack5 = load ptr, ptr %g.fat.elt4, align 8
  %is_null.i7 = icmp eq ptr %g.fat.unpack5, null
  br i1 %is_null.i7, label %__mingus_closure_release.exit18, label %do_release.i8

do_release.i8:                                    ; preds = %__mingus_closure_release.exit
  %rc.i9 = load i64, ptr %g.fat.unpack5, align 4
  %rc_dec.i10 = add i64 %rc.i9, -1
  store i64 %rc_dec.i10, ptr %g.fat.unpack5, align 4
  %is_zero.i11 = icmp eq i64 %rc_dec.i10, 0
  br i1 %is_zero.i11, label %cleanup.i12, label %__mingus_closure_release.exit18

cleanup.i12:                                      ; preds = %do_release.i8
  %cleanup_fn_ptr.i13 = getelementptr inbounds nuw i8, ptr %g.fat.unpack5, i64 8
  %cleanup_fn.i14 = load ptr, ptr %cleanup_fn_ptr.i13, align 8
  %has_cleanup.not.i15 = icmp eq ptr %cleanup_fn.i14, null
  br i1 %has_cleanup.not.i15, label %do_free.i17, label %call_cleanup.i16

call_cleanup.i16:                                 ; preds = %cleanup.i12
  tail call void %cleanup_fn.i14(ptr nonnull %g.fat.unpack5)
  br label %do_free.i17

do_free.i17:                                      ; preds = %call_cleanup.i16, %cleanup.i12
  tail call void @free(ptr nonnull %g.fat.unpack5)
  br label %__mingus_closure_release.exit18

__mingus_closure_release.exit18:                  ; preds = %__mingus_closure_release.exit, %do_release.i8, %do_free.i17
  ret void
}

; Function Attrs: mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite)
declare void @free(ptr allocptr noundef captures(none)) local_unnamed_addr #3

declare i32 @snprintf(ptr, i32, ptr, ...) local_unnamed_addr

define noundef i32 @main() local_unnamed_addr {
entry:
  %0 = tail call i32 @CaptureShowcase_main()
  ret i32 0
}

attributes #0 = { nofree nounwind }
attributes #1 = { mustprogress nofree nounwind willreturn }
attributes #2 = { mustprogress nofree nounwind willreturn allockind("alloc,uninitialized") allocsize(0) memory(inaccessiblemem: readwrite) "alloc-family"="malloc" }
attributes #3 = { mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite) "alloc-family"="malloc" }
