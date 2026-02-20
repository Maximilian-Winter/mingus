; ModuleID = 'mingus_module'
source_filename = "mingus_module"

@str = private unnamed_addr constant [16 x i8] c"f == null: true\00", align 1
@str.3 = private unnamed_addr constant [17 x i8] c"f != null: false\00", align 1
@str.4 = private unnamed_addr constant [16 x i8] c"null == f: true\00", align 1
@str.6 = private unnamed_addr constant [30 x i8] c"after assign, f != null: true\00", align 1
@str.9 = private unnamed_addr constant [31 x i8] c"after assign, f == null: false\00", align 1
@str.10 = private unnamed_addr constant [25 x i8] c"apply(f, 21) == 42: true\00", align 1
@str.11 = private unnamed_addr constant [12 x i8] c"Test21 done\00", align 1

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr noundef readonly captures(none)) local_unnamed_addr #0

define i32 @Test21_apply({ ptr, ptr } %f, i32 %x) local_unnamed_addr {
entry:
  %f.fca.0.extract = extractvalue { ptr, ptr } %f, 0
  %f.fca.1.extract = extractvalue { ptr, ptr } %f, 1
  %0 = tail call i32 %f.fca.0.extract(i32 %x, ptr %f.fca.1.extract)
  ret i32 %0
}

; Function Attrs: nofree nounwind
define noundef i32 @Test21_main() local_unnamed_addr #0 {
__mingus_closure_release_wrapper.exit:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str)
  %1 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.3)
  %2 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.4)
  %3 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.6)
  %4 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.9)
  %5 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.10)
  %6 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.11)
  ret i32 0
}

; Function Attrs: nofree nounwind
define noundef i32 @main() local_unnamed_addr #0 {
entry:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str)
  %1 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.3)
  %2 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.4)
  %3 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.6)
  %4 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.9)
  %5 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.10)
  %6 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.11)
  ret i32 0
}

attributes #0 = { nofree nounwind }
