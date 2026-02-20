; ModuleID = 'mingus_module'
source_filename = "mingus_module"

%Point = type { i32, i32 }

@MathUtils_vtable = internal constant [1 x ptr] [ptr @MathUtils_destructor]
@str = private unnamed_addr constant [14 x i8] c"add(3,4) = %d\00", align 1
@str.1 = private unnamed_addr constant [18 x i8] c"factorial(5) = %d\00", align 1
@str.3 = private unnamed_addr constant [16 x i8] c"max(10,20) = %d\00", align 1
@str.5 = private unnamed_addr constant [14 x i8] c"origin.x = %d\00", align 1
@str.7 = private unnamed_addr constant [14 x i8] c"origin.y = %d\00", align 1
@fmt.8 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@str.9 = private unnamed_addr constant [12 x i8] c"Test24 done\00", align 1

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr noundef readonly captures(none), i32 noundef, ...) local_unnamed_addr #0

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr noundef readonly captures(none)) local_unnamed_addr #0

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define i32 @MathUtils_add(i32 %a, i32 %b) local_unnamed_addr #1 {
entry:
  %add = add i32 %b, %a
  ret i32 %add
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write)
define void @MathUtils_constructor(ptr writeonly captures(none) initializes((0, 8)) %this) local_unnamed_addr #2 {
entry:
  store ptr @MathUtils_vtable, ptr %this, align 8
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define void @MathUtils_destructor(ptr readnone captures(none) %this) #1 {
entry:
  ret void
}

; Function Attrs: nofree norecurse nosync nounwind memory(none)
define i32 @MathUtils_factorial(i32 %n) local_unnamed_addr #3 {
entry:
  %sle7 = icmp slt i32 %n, 2
  br i1 %sle7, label %common.ret, label %ifmerge

common.ret:                                       ; preds = %ifmerge, %entry
  %accumulator.tr.lcssa = phi i32 [ 1, %entry ], [ %mul, %ifmerge ]
  ret i32 %accumulator.tr.lcssa

ifmerge:                                          ; preds = %entry, %ifmerge
  %n.tr9 = phi i32 [ %sub, %ifmerge ], [ %n, %entry ]
  %accumulator.tr8 = phi i32 [ %mul, %ifmerge ], [ 1, %entry ]
  %sub = add nsw i32 %n.tr9, -1
  %mul = mul i32 %n.tr9, %accumulator.tr8
  %sle = icmp samesign ult i32 %n.tr9, 3
  br i1 %sle, label %common.ret, label %ifmerge
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define i32 @MathUtils_max(i32 %a, i32 %b) local_unnamed_addr #1 {
entry:
  %common.ret.op = tail call i32 @llvm.smax.i32(i32 %a, i32 %b)
  ret i32 %common.ret.op
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define %Point @Point_origin() local_unnamed_addr #1 {
entry:
  ret %Point zeroinitializer
}

define noundef i32 @Test24_main() local_unnamed_addr {
entry:
  %0 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str, i32 7)
  %snprintf.len = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.8)
  %needed.i64 = sext i32 %snprintf.len to i64
  %alloc.size = add nsw i64 %needed.i64, 1
  %interp.buf = tail call ptr @malloc(i64 %alloc.size)
  %buf.size = add i32 %snprintf.len, 1
  %1 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf, i32 %buf.size, ptr nonnull @fmt.8)
  %2 = tail call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf)
  %3 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.1, i32 120)
  %snprintf.len3 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.8)
  %needed.i644 = sext i32 %snprintf.len3 to i64
  %alloc.size5 = add nsw i64 %needed.i644, 1
  %interp.buf6 = tail call ptr @malloc(i64 %alloc.size5)
  %buf.size7 = add i32 %snprintf.len3, 1
  %4 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf6, i32 %buf.size7, ptr nonnull @fmt.8)
  %5 = tail call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf6)
  %6 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.3, i32 20)
  %snprintf.len9 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.8)
  %needed.i6410 = sext i32 %snprintf.len9 to i64
  %alloc.size11 = add nsw i64 %needed.i6410, 1
  %interp.buf12 = tail call ptr @malloc(i64 %alloc.size11)
  %buf.size13 = add i32 %snprintf.len9, 1
  %7 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf12, i32 %buf.size13, ptr nonnull @fmt.8)
  %8 = tail call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf12)
  %9 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.5, i32 0)
  %snprintf.len14 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.8)
  %needed.i6415 = sext i32 %snprintf.len14 to i64
  %alloc.size16 = add nsw i64 %needed.i6415, 1
  %interp.buf17 = tail call ptr @malloc(i64 %alloc.size16)
  %buf.size18 = add i32 %snprintf.len14, 1
  %10 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf17, i32 %buf.size18, ptr nonnull @fmt.8)
  %11 = tail call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf17)
  %12 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.7, i32 0)
  %snprintf.len19 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.8)
  %needed.i6420 = sext i32 %snprintf.len19 to i64
  %alloc.size21 = add nsw i64 %needed.i6420, 1
  %interp.buf22 = tail call ptr @malloc(i64 %alloc.size21)
  %buf.size23 = add i32 %snprintf.len19, 1
  %13 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf22, i32 %buf.size23, ptr nonnull @fmt.8)
  %14 = tail call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf22)
  %15 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.9)
  tail call void @free(ptr nonnull %interp.buf22)
  tail call void @free(ptr nonnull %interp.buf17)
  tail call void @free(ptr nonnull %interp.buf12)
  tail call void @free(ptr nonnull %interp.buf6)
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
  %0 = tail call i32 @Test24_main()
  ret i32 0
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.smax.i32(i32, i32) #6

attributes #0 = { nofree nounwind }
attributes #1 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) }
attributes #2 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write) }
attributes #3 = { nofree norecurse nosync nounwind memory(none) }
attributes #4 = { mustprogress nofree nounwind willreturn allockind("alloc,uninitialized") allocsize(0) memory(inaccessiblemem: readwrite) "alloc-family"="malloc" }
attributes #5 = { mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite) "alloc-family"="malloc" }
attributes #6 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
