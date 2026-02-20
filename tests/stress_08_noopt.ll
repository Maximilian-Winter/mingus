; ModuleID = 'mingus_module'
source_filename = "mingus_module"

%Box = type { i32 }

@str = private unnamed_addr constant [9 x i8] c"drop %d\0A\00", align 1
@str.1 = private unnamed_addr constant [11 x i8] c"result=%d\0A\00", align 1
@str.2 = private unnamed_addr constant [14 x i8] c"Stress08 done\00", align 1

declare i32 @printf(ptr, i32, ...)

declare i32 @puts(ptr)

define void @Box_constructor(ptr %this, i32 %v) {
entry:
  %v1 = alloca i32, align 4
  store i32 %v, ptr %v1, align 4
  %value_ptr = getelementptr inbounds nuw %Box, ptr %this, i32 0, i32 0
  %v2 = load i32, ptr %v1, align 4
  store i32 %v2, ptr %value_ptr, align 4
  ret void
}

define void @Box_destructor(ptr %this) {
entry:
  %value_ptr = getelementptr inbounds nuw %Box, ptr %this, i32 0, i32 0
  %value = load i32, ptr %value_ptr, align 4
  %0 = call i32 (ptr, i32, ...) @printf(ptr @str, i32 %value)
  ret void
}

define i32 @Stress08_main() {
entry:
  %r = alloca i32, align 4
  %f = alloca { ptr, ptr }, align 8
  store { ptr, ptr } zeroinitializer, ptr %f, align 8
  %0 = call { ptr, ptr } @Stress08_make(i32 5)
  store { ptr, ptr } %0, ptr %f, align 8
  %f1 = load { ptr, ptr }, ptr %f, align 8
  %fn.ptr = extractvalue { ptr, ptr } %f1, 0
  %env.ptr = extractvalue { ptr, ptr } %f1, 1
  %1 = call i32 %fn.ptr(i32 10, ptr %env.ptr)
  store i32 %1, ptr %r, align 4
  %r2 = load i32, ptr %r, align 4
  %2 = call i32 (ptr, i32, ...) @printf(ptr @str.1, i32 %r2)
  %3 = call i32 @puts(ptr @str.2)
  ret i32 0
}

define { ptr, ptr } @Stress08_make(i32 %x) {
entry:
  %ctor.tmp = alloca %Box, align 8
  %b = alloca %Box, align 8
  %x1 = alloca i32, align 4
  store i32 %x, ptr %x1, align 4
  %x2 = load i32, ptr %x1, align 4
  call void @Box_constructor(ptr %ctor.tmp, i32 %x2)
  %ctor.val = load %Box, ptr %ctor.tmp, align 4
  store %Box %ctor.val, ptr %b, align 4
  %closure.ptr = call ptr @malloc(i64 24)
  %rc.slot = getelementptr inbounds nuw { i64, ptr, %Box }, ptr %closure.ptr, i32 0, i32 0
  store i64 1, ptr %rc.slot, align 4
  %cleanup.slot = getelementptr inbounds nuw { i64, ptr, %Box }, ptr %closure.ptr, i32 0, i32 1
  store ptr null, ptr %cleanup.slot, align 8
  %b.val = load %Box, ptr %b, align 4
  %b.slot = getelementptr inbounds nuw { i64, ptr, %Box }, ptr %closure.ptr, i32 0, i32 2
  store %Box %b.val, ptr %b.slot, align 4
  %fat.env = insertvalue { ptr, ptr } { ptr @__lambda_0, ptr undef }, ptr %closure.ptr, 1
  ret { ptr, ptr } %fat.env
}

define internal i32 @__lambda_0(i32 %y, ptr %env) {
entry:
  %b2 = alloca %Box, align 8
  %y1 = alloca i32, align 4
  store i32 %y, ptr %y1, align 4
  %b.cap = getelementptr inbounds nuw { i64, ptr, %Box }, ptr %env, i32 0, i32 2
  %b = load %Box, ptr %b.cap, align 4
  store %Box %b, ptr %b2, align 4
  %value_ptr = getelementptr inbounds nuw %Box, ptr %b2, i32 0, i32 0
  %value = load i32, ptr %value_ptr, align 4
  %y3 = load i32, ptr %y1, align 4
  %add = add i32 %value, %y3
  ret i32 %add
}

declare ptr @malloc(i64)

define internal void @__mingus_closure_release_wrapper(ptr %0) {
entry:
  %fat = load { ptr, ptr }, ptr %0, align 8
  %env.ptr = extractvalue { ptr, ptr } %fat, 1
  call void @__mingus_closure_release(ptr %env.ptr)
  ret void
}

define internal void @__mingus_closure_release(ptr %0) {
entry:
  %is_null = icmp eq ptr %0, null
  br i1 %is_null, label %done, label %do_release

do_release:                                       ; preds = %entry
  %rc_ptr = getelementptr inbounds nuw { i64, ptr }, ptr %0, i32 0, i32 0
  %rc = load i64, ptr %rc_ptr, align 4
  %rc_dec = sub i64 %rc, 1
  store i64 %rc_dec, ptr %rc_ptr, align 4
  %is_zero = icmp eq i64 %rc_dec, 0
  br i1 %is_zero, label %cleanup, label %done

cleanup:                                          ; preds = %do_release
  %cleanup_fn_ptr = getelementptr inbounds nuw { i64, ptr }, ptr %0, i32 0, i32 1
  %cleanup_fn = load ptr, ptr %cleanup_fn_ptr, align 8
  %has_cleanup = icmp ne ptr %cleanup_fn, null
  br i1 %has_cleanup, label %call_cleanup, label %do_free

call_cleanup:                                     ; preds = %cleanup
  call void %cleanup_fn(ptr %0)
  br label %do_free

do_free:                                          ; preds = %call_cleanup, %cleanup
  call void @free(ptr %0)
  br label %done

done:                                             ; preds = %do_free, %do_release, %entry
  ret void
}

declare void @free(ptr)

define i32 @main() {
entry:
  %0 = call i32 @Stress08_main()
  ret i32 %0
}
