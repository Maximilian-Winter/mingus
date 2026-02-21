; ModuleID = 'mingus_module'
source_filename = "mingus_module"
target triple = "x86_64-pc-windows-msvc"

%Vec2 = type { double, double }

@str = private unnamed_addr constant [41 x i8] c"=== Test 31: Closure + Struct Params ===\00", align 1
@str.1 = private unnamed_addr constant [13 x i8] c"regular: %f\0A\00", align 1
@str.2 = private unnamed_addr constant [20 x i8] c"closure direct: %f\0A\00", align 1
@str.3 = private unnamed_addr constant [17 x i8] c"closure HOF: %f\0A\00", align 1
@str.4 = private unnamed_addr constant [20 x i8] c"lambda literal: %f\0A\00", align 1
@str.5 = private unnamed_addr constant [23 x i8] c"capturing closure: %f\0A\00", align 1

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr noundef readonly captures(none), double noundef, ...) local_unnamed_addr #0

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr noundef readonly captures(none)) local_unnamed_addr #0

define double @Test31_applyToVec(ptr readonly captures(none) %v, { ptr, ptr } %f) local_unnamed_addr {
entry:
  %v1 = alloca %Vec2, align 8
  %v.val.unpack = load double, ptr %v, align 8
  %v.val.elt5 = getelementptr inbounds nuw i8, ptr %v, i64 8
  %v.val.unpack6 = load double, ptr %v.val.elt5, align 8
  store double %v.val.unpack, ptr %v1, align 8
  %v.val.fca.1.gep = getelementptr inbounds nuw i8, ptr %v1, i64 8
  store double %v.val.unpack6, ptr %v.val.fca.1.gep, align 8
  %f.fca.0.extract = extractvalue { ptr, ptr } %f, 0
  %f.fca.1.extract = extractvalue { ptr, ptr } %f, 1
  %0 = call double %f.fca.0.extract(ptr nonnull %v1, ptr %f.fca.1.extract)
  ret double %0
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read)
define double @Test31_magnitudeSq(ptr readonly captures(none) %v) local_unnamed_addr #1 {
entry:
  %v.val.unpack = load double, ptr %v, align 8
  %v.val.elt7 = getelementptr inbounds nuw i8, ptr %v, i64 8
  %v.val.unpack8 = load double, ptr %v.val.elt7, align 8
  %fmul = fmul double %v.val.unpack, %v.val.unpack
  %fmul6 = fmul double %v.val.unpack8, %v.val.unpack8
  %fadd = fadd double %fmul, %fmul6
  ret double %fadd
}

; Function Attrs: nofree nounwind
define noundef i32 @Test31_main() local_unnamed_addr #0 {
__mingus_closure_release_wrapper.exit68:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str)
  %1 = tail call i32 (ptr, double, ...) @printf(ptr nonnull dereferenceable(1) @str.1, double 2.500000e+01)
  %2 = tail call i32 (ptr, double, ...) @printf(ptr nonnull dereferenceable(1) @str.2, double 2.500000e+01)
  %3 = tail call i32 (ptr, double, ...) @printf(ptr nonnull dereferenceable(1) @str.3, double 2.500000e+01)
  %4 = tail call i32 (ptr, double, ...) @printf(ptr nonnull dereferenceable(1) @str.4, double 7.000000e+00)
  %5 = tail call i32 (ptr, double, ...) @printf(ptr nonnull dereferenceable(1) @str.5, double 1.400000e+01)
  ret i32 0
}

; Function Attrs: nofree nounwind
define noundef i32 @main() local_unnamed_addr #0 {
entry:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str)
  %1 = tail call i32 (ptr, double, ...) @printf(ptr nonnull dereferenceable(1) @str.1, double 2.500000e+01)
  %2 = tail call i32 (ptr, double, ...) @printf(ptr nonnull dereferenceable(1) @str.2, double 2.500000e+01)
  %3 = tail call i32 (ptr, double, ...) @printf(ptr nonnull dereferenceable(1) @str.3, double 2.500000e+01)
  %4 = tail call i32 (ptr, double, ...) @printf(ptr nonnull dereferenceable(1) @str.4, double 7.000000e+00)
  %5 = tail call i32 (ptr, double, ...) @printf(ptr nonnull dereferenceable(1) @str.5, double 1.400000e+01)
  ret i32 0
}

attributes #0 = { nofree nounwind }
attributes #1 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read) }
