; ModuleID = 'mingus_module'
source_filename = "mingus_module"

@str = private unnamed_addr constant [12 x i8] c"destroy %d\0A\00", align 1
@str.1 = private unnamed_addr constant [14 x i8] c"Stress04 done\00", align 1

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr noundef readonly captures(none), i32 noundef, ...) local_unnamed_addr #0

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr noundef readonly captures(none)) local_unnamed_addr #0

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write)
define void @Guard_constructor(ptr writeonly captures(none) initializes((0, 4)) %this, i32 %i) local_unnamed_addr #1 {
entry:
  store i32 %i, ptr %this, align 4
  ret void
}

; Function Attrs: nofree nounwind
define void @Guard_destructor(ptr readonly captures(none) %this) local_unnamed_addr #0 {
entry:
  %id = load i32, ptr %this, align 4
  %0 = tail call i32 (ptr, i32, ...) @printf(ptr nonnull dereferenceable(1) @str, i32 %id)
  ret void
}

; Function Attrs: nofree nounwind
define noundef i32 @Stress04_main() local_unnamed_addr #0 {
entry:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.1)
  ret i32 0
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define range(i32 7, 43) i32 @Stress04_test(i32 %v) local_unnamed_addr #2 {
entry:
  %0 = and i32 %v, 1
  %eq = icmp eq i32 %0, 0
  %. = select i1 %eq, i32 42, i32 7
  ret i32 %.
}

; Function Attrs: nofree nounwind
define noundef i32 @main() local_unnamed_addr #0 {
entry:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.1)
  ret i32 0
}

attributes #0 = { nofree nounwind }
attributes #1 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: write) }
attributes #2 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) }
