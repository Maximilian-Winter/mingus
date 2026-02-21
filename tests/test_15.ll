; ModuleID = 'mingus_module'
source_filename = "mingus_module"
target triple = "x86_64-pc-windows-msvc"

%Circle = type { ptr, i32 }
%Square = type { ptr, i32 }

@Circle_vtable = internal constant [1 x ptr] [ptr @Test15_destructor]
@Square_vtable = internal constant [1 x ptr] [ptr @Test15_destructor.1]
@Circle.Drawable.itable = internal constant [1 x ptr] [ptr @Test15_draw]
@Circle.Resizable.itable = internal constant [1 x ptr] [ptr @Test15_resize]
@Square.Drawable.itable = internal constant [1 x ptr] [ptr @Test15_draw.3]
@Square.Resizable.itable = internal constant [1 x ptr] [ptr @Test15_resize.4]
@str = private unnamed_addr constant [13 x i8] c"Circle drawn\00", align 1
@str.5 = private unnamed_addr constant [13 x i8] c"Square drawn\00", align 1
@str.6 = private unnamed_addr constant [28 x i8] c"=== Test 15: Interfaces ===\00", align 1
@str.7 = private unnamed_addr constant [21 x i8] c"--- Direct calls ---\00", align 1
@str.8 = private unnamed_addr constant [27 x i8] c"--- Interface dispatch ---\00", align 1
@str.9 = private unnamed_addr constant [18 x i8] c"--- Resizable ---\00", align 1
@str.10 = private unnamed_addr constant [16 x i8] c"resize(3) = %d\0A\00", align 1
@str.11 = private unnamed_addr constant [25 x i8] c"=== Test 15 complete ===\00", align 1

declare i32 @puts(ptr)

declare i32 @printf(ptr, i32, ...)

define void @Test15_draw(ptr %this) {
entry:
  %0 = call i32 @puts(ptr @str)
  ret void
}

define void @Test15_constructor(ptr %this, i32 %r) {
entry:
  %r1 = alloca i32, align 4
  store i32 %r, ptr %r1, align 4
  %vtable.slot = getelementptr inbounds nuw %Circle, ptr %this, i32 0, i32 0
  store ptr @Circle_vtable, ptr %vtable.slot, align 8
  %radius_ptr = getelementptr inbounds nuw %Circle, ptr %this, i32 0, i32 1
  %r2 = load i32, ptr %r1, align 4
  store i32 %r2, ptr %radius_ptr, align 4
  ret void
}

define void @Test15_destructor(ptr %this) {
entry:
  ret void
}

define i32 @Test15_resize(ptr %this, i32 %factor) {
entry:
  %factor1 = alloca i32, align 4
  store i32 %factor, ptr %factor1, align 4
  %radius_ptr = getelementptr inbounds nuw %Circle, ptr %this, i32 0, i32 1
  %radius = load i32, ptr %radius_ptr, align 4
  %factor2 = load i32, ptr %factor1, align 4
  %mul = mul i32 %radius, %factor2
  ret i32 %mul
}

define void @Test15_destructor.1(ptr %this) {
entry:
  ret void
}

define void @Test15_constructor.2(ptr %this, i32 %s) {
entry:
  %s1 = alloca i32, align 4
  store i32 %s, ptr %s1, align 4
  %vtable.slot = getelementptr inbounds nuw %Square, ptr %this, i32 0, i32 0
  store ptr @Square_vtable, ptr %vtable.slot, align 8
  %side_ptr = getelementptr inbounds nuw %Square, ptr %this, i32 0, i32 1
  %s2 = load i32, ptr %s1, align 4
  store i32 %s2, ptr %side_ptr, align 4
  ret void
}

define void @Test15_draw.3(ptr %this) {
entry:
  %0 = call i32 @puts(ptr @str.5)
  ret void
}

define i32 @Test15_resize.4(ptr %this, i32 %factor) {
entry:
  %factor1 = alloca i32, align 4
  store i32 %factor, ptr %factor1, align 4
  %side_ptr = getelementptr inbounds nuw %Square, ptr %this, i32 0, i32 1
  %side = load i32, ptr %side_ptr, align 4
  %factor2 = load i32, ptr %factor1, align 4
  %mul = mul i32 %side, %factor2
  ret i32 %mul
}

define void @Test15_renderAll({ ptr, ptr } %d) {
entry:
  %d1 = alloca { ptr, ptr }, align 8
  store { ptr, ptr } %d, ptr %d1, align 8
  %d2 = load { ptr, ptr }, ptr %d1, align 8
  %iface.obj = extractvalue { ptr, ptr } %d2, 0
  %iface.itable = extractvalue { ptr, ptr } %d2, 1
  %iface.slot = getelementptr ptr, ptr %iface.itable, i32 -1
  %iface.fn = load ptr, ptr %iface.slot, align 8
  call void %iface.fn(ptr %iface.obj)
  ret void
}

define i32 @Test15_main() {
entry:
  %r = alloca { ptr, ptr }, align 8
  %d1 = alloca { ptr, ptr }, align 8
  %d0 = alloca { ptr, ptr }, align 8
  %ctor.tmp1 = alloca %Square, align 8
  %sq = alloca %Square, align 8
  %ctor.tmp = alloca %Circle, align 8
  %c = alloca %Circle, align 8
  %0 = call i32 @puts(ptr @str.6)
  %1 = call i32 @puts(ptr @str.7)
  store %Circle zeroinitializer, ptr %c, align 8
  %vtable.slot = getelementptr inbounds nuw %Circle, ptr %ctor.tmp, i32 0, i32 0
  store ptr @Circle_vtable, ptr %vtable.slot, align 8
  call void @Test15_constructor(ptr %ctor.tmp, i32 5)
  %ctor.val = load %Circle, ptr %ctor.tmp, align 8
  store %Circle %ctor.val, ptr %c, align 8
  store %Square zeroinitializer, ptr %sq, align 8
  %vtable.slot2 = getelementptr inbounds nuw %Square, ptr %ctor.tmp1, i32 0, i32 0
  store ptr @Square_vtable, ptr %vtable.slot2, align 8
  call void @Test15_constructor.2(ptr %ctor.tmp1, i32 3)
  %ctor.val3 = load %Square, ptr %ctor.tmp1, align 8
  store %Square %ctor.val3, ptr %sq, align 8
  call void @Test15_draw(ptr %c)
  call void @Test15_draw.3(ptr %sq)
  %2 = call i32 @puts(ptr @str.8)
  %new.ptr = call ptr @malloc(i32 16)
  call void @Test15_constructor(ptr %new.ptr, i32 10)
  %fat.obj = insertvalue { ptr, ptr } undef, ptr %new.ptr, 0
  %fat.itable = insertvalue { ptr, ptr } %fat.obj, ptr @Circle.Drawable.itable, 1
  store { ptr, ptr } %fat.itable, ptr %d0, align 8
  %new.ptr4 = call ptr @malloc(i32 16)
  call void @Test15_constructor.2(ptr %new.ptr4, i32 7)
  %fat.obj5 = insertvalue { ptr, ptr } undef, ptr %new.ptr4, 0
  %fat.itable6 = insertvalue { ptr, ptr } %fat.obj5, ptr @Square.Drawable.itable, 1
  store { ptr, ptr } %fat.itable6, ptr %d1, align 8
  %d07 = load { ptr, ptr }, ptr %d0, align 8
  call void @Test15_renderAll({ ptr, ptr } %d07)
  %d18 = load { ptr, ptr }, ptr %d1, align 8
  call void @Test15_renderAll({ ptr, ptr } %d18)
  %3 = call i32 @puts(ptr @str.9)
  %new.ptr9 = call ptr @malloc(i32 16)
  call void @Test15_constructor(ptr %new.ptr9, i32 4)
  %fat.obj10 = insertvalue { ptr, ptr } undef, ptr %new.ptr9, 0
  %fat.itable11 = insertvalue { ptr, ptr } %fat.obj10, ptr @Circle.Resizable.itable, 1
  store { ptr, ptr } %fat.itable11, ptr %r, align 8
  %r12 = load { ptr, ptr }, ptr %r, align 8
  %iface.obj = extractvalue { ptr, ptr } %r12, 0
  %iface.itable = extractvalue { ptr, ptr } %r12, 1
  %iface.slot = getelementptr ptr, ptr %iface.itable, i32 -1
  %iface.fn = load ptr, ptr %iface.slot, align 8
  %iface.result = call i32 %iface.fn(ptr %iface.obj, i32 3)
  %4 = call i32 (ptr, i32, ...) @printf(ptr @str.10, i32 %iface.result)
  %d013 = load { ptr, ptr }, ptr %d0, align 8
  %iface.del.obj = extractvalue { ptr, ptr } %d013, 0
  call void @free(ptr %iface.del.obj)
  %d114 = load { ptr, ptr }, ptr %d1, align 8
  %iface.del.obj15 = extractvalue { ptr, ptr } %d114, 0
  call void @free(ptr %iface.del.obj15)
  %r16 = load { ptr, ptr }, ptr %r, align 8
  %iface.del.obj17 = extractvalue { ptr, ptr } %r16, 0
  call void @free(ptr %iface.del.obj17)
  %5 = call i32 @puts(ptr @str.11)
  call void @Test15_destructor.1(ptr %sq)
  call void @Test15_destructor(ptr %c)
  ret i32 0
}

declare ptr @malloc(i32)

declare void @free(ptr)

define i32 @main() {
entry:
  %0 = call i32 @Test15_main()
  ret i32 %0
}
