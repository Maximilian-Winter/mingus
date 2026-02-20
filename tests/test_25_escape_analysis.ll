; ModuleID = 'mingus_module'
source_filename = "mingus_module"

@str = private unnamed_addr constant [22 x i8] c"apply(x+base, 5) = %d\00", align 1
@str.1 = private unnamed_addr constant [28 x i8] c"apply(x*multiplier, 7) = %d\00", align 1
@str.3 = private unnamed_addr constant [24 x i8] c"applyTwice(x+1, 0) = %d\00", align 1
@str.5 = private unnamed_addr constant [24 x i8] c"apply(addBase, 20) = %d\00", align 1
@str.7 = private unnamed_addr constant [18 x i8] c"chain result = %d\00", align 1
@fmt.8 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@str.9 = private unnamed_addr constant [12 x i8] c"Test25 done\00", align 1

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr noundef readonly captures(none), i32 noundef, ...) local_unnamed_addr #0

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr noundef readonly captures(none)) local_unnamed_addr #0

define i32 @Test25_apply({ ptr, ptr } %f, i32 %x) local_unnamed_addr {
entry:
  %f.fca.0.extract = extractvalue { ptr, ptr } %f, 0
  %f.fca.1.extract = extractvalue { ptr, ptr } %f, 1
  %0 = tail call i32 %f.fca.0.extract(i32 %x, ptr %f.fca.1.extract)
  ret i32 %0
}

define i32 @Test25_applyTwice({ ptr, ptr } %f, i32 %x) local_unnamed_addr {
entry:
  %f.fca.0.extract = extractvalue { ptr, ptr } %f, 0
  %f.fca.1.extract = extractvalue { ptr, ptr } %f, 1
  %0 = tail call i32 %f.fca.0.extract(i32 %x, ptr %f.fca.1.extract)
  %1 = tail call i32 %f.fca.0.extract(i32 %0, ptr %f.fca.1.extract)
  ret i32 %1
}

define noundef i32 @Test25_main() local_unnamed_addr {
__mingus_closure_release_wrapper.exit120:
  %0 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str, i32 15)
  %snprintf.len = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.8)
  %needed.i64 = sext i32 %snprintf.len to i64
  %alloc.size = add nsw i64 %needed.i64, 1
  %interp.buf = tail call ptr @malloc(i64 %alloc.size)
  %buf.size = add i32 %snprintf.len, 1
  %1 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf, i32 %buf.size, ptr nonnull @fmt.8)
  %2 = tail call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf)
  %3 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.1, i32 21)
  %snprintf.len8 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.8)
  %needed.i649 = sext i32 %snprintf.len8 to i64
  %alloc.size10 = add nsw i64 %needed.i649, 1
  %interp.buf11 = tail call ptr @malloc(i64 %alloc.size10)
  %buf.size12 = add i32 %snprintf.len8, 1
  %4 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf11, i32 %buf.size12, ptr nonnull @fmt.8)
  %5 = tail call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf11)
  %6 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.3, i32 2)
  %snprintf.len15 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.8)
  %needed.i6416 = sext i32 %snprintf.len15 to i64
  %alloc.size17 = add nsw i64 %needed.i6416, 1
  %interp.buf18 = tail call ptr @malloc(i64 %alloc.size17)
  %buf.size19 = add i32 %snprintf.len15, 1
  %7 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf18, i32 %buf.size19, ptr nonnull @fmt.8)
  %8 = tail call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf18)
  %9 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.5, i32 30)
  %snprintf.len28 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.8)
  %needed.i6429 = sext i32 %snprintf.len28 to i64
  %alloc.size30 = add nsw i64 %needed.i6429, 1
  %interp.buf31 = tail call ptr @malloc(i64 %alloc.size30)
  %buf.size32 = add i32 %snprintf.len28, 1
  %10 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf31, i32 %buf.size32, ptr nonnull @fmt.8)
  %11 = tail call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf31)
  %12 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str.7, i32 50)
  %snprintf.len37 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.8)
  %needed.i6438 = sext i32 %snprintf.len37 to i64
  %alloc.size39 = add nsw i64 %needed.i6438, 1
  %interp.buf40 = tail call ptr @malloc(i64 %alloc.size39)
  %buf.size41 = add i32 %snprintf.len37, 1
  %13 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf40, i32 %buf.size41, ptr nonnull @fmt.8)
  %14 = tail call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf40)
  %15 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.9)
  tail call void @free(ptr nonnull %interp.buf40)
  tail call void @free(ptr nonnull %interp.buf31)
  tail call void @free(ptr nonnull %interp.buf18)
  tail call void @free(ptr nonnull %interp.buf11)
  tail call void @free(ptr nonnull %interp.buf)
  ret i32 0
}

; Function Attrs: mustprogress nofree nounwind willreturn allockind("alloc,uninitialized") allocsize(0) memory(inaccessiblemem: readwrite)
declare noalias noundef ptr @malloc(i64 noundef) local_unnamed_addr #1

; Function Attrs: mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite)
declare void @free(ptr allocptr noundef captures(none)) local_unnamed_addr #2

declare i32 @snprintf(ptr, i32, ptr, ...) local_unnamed_addr

define noundef i32 @main() local_unnamed_addr {
entry:
  %0 = tail call i32 @Test25_main()
  ret i32 0
}

attributes #0 = { nofree nounwind }
attributes #1 = { mustprogress nofree nounwind willreturn allockind("alloc,uninitialized") allocsize(0) memory(inaccessiblemem: readwrite) "alloc-family"="malloc" }
attributes #2 = { mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite) "alloc-family"="malloc" }
