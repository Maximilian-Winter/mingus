; ModuleID = 'mingus_module'
source_filename = "mingus_module"

%Guard = type { i32 }

@str = private unnamed_addr constant [12 x i8] c"destroy %d\0A\00", align 1
@str.1 = private unnamed_addr constant [14 x i8] c"Stress04 done\00", align 1

declare i32 @printf(ptr, i32, ...)

declare i32 @puts(ptr)

define void @Guard_constructor(ptr %this, i32 %i) {
entry:
  %i1 = alloca i32, align 4
  store i32 %i, ptr %i1, align 4
  %id_ptr = getelementptr inbounds nuw %Guard, ptr %this, i32 0, i32 0
  %i2 = load i32, ptr %i1, align 4
  store i32 %i2, ptr %id_ptr, align 4
  ret void
}

define void @Guard_destructor(ptr %this) {
entry:
  %id_ptr = getelementptr inbounds nuw %Guard, ptr %this, i32 0, i32 0
  %id = load i32, ptr %id_ptr, align 4
  %0 = call i32 (ptr, i32, ...) @printf(ptr @str, i32 %id)
  ret void
}

define i32 @Stress04_main() {
entry:
  %i = alloca i32, align 4
  store i32 0, ptr %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.iter, %entry
  %i1 = load i32, ptr %i, align 4
  %slt = icmp slt i32 %i1, 10
  br i1 %slt, label %for.body, label %for.exit

for.body:                                         ; preds = %for.cond
  %i2 = load i32, ptr %i, align 4
  %0 = call i32 @Stress04_test(i32 %i2)
  br label %for.iter

for.iter:                                         ; preds = %for.body
  %old = load i32, ptr %i, align 4
  %inc = add i32 %old, 1
  store i32 %inc, ptr %i, align 4
  br label %for.cond

for.exit:                                         ; preds = %for.cond
  %1 = call i32 @puts(ptr @str.1)
  ret i32 0
}

define i32 @Stress04_test(i32 %v) {
entry:
  %ctor.tmp2 = alloca %Guard, align 8
  %g2 = alloca %Guard, align 8
  %ctor.tmp = alloca %Guard, align 8
  %g1 = alloca %Guard, align 8
  %v1 = alloca i32, align 4
  store i32 %v, ptr %v1, align 4
  call void @Guard_constructor(ptr %ctor.tmp, i32 1)
  %ctor.val = load %Guard, ptr %ctor.tmp, align 4
  store %Guard %ctor.val, ptr %g1, align 4
  call void @Guard_constructor(ptr %ctor.tmp2, i32 2)
  %ctor.val3 = load %Guard, ptr %ctor.tmp2, align 4
  store %Guard %ctor.val3, ptr %g2, align 4
  %v4 = load i32, ptr %v1, align 4
  %srem = srem i32 %v4, 2
  %eq = icmp eq i32 %srem, 0
  br i1 %eq, label %then, label %ifmerge

then:                                             ; preds = %entry
  ret i32 42

ifmerge:                                          ; preds = %entry
  ret i32 7
}

define i32 @main() {
entry:
  %0 = call i32 @Stress04_main()
  ret i32 %0
}
