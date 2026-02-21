; ModuleID = 'mingus_module'
source_filename = "mingus_module"
target triple = "x86_64-pc-windows-msvc"

%Sample = type { double, double }

@str = private unnamed_addr constant [29 x i8] c"=== Mingus Audio Effects ===\00", align 1
@str.1 = private unnamed_addr constant [17 x i8] c"Bass raw: L=%.3f\00", align 1
@str.3 = private unnamed_addr constant [22 x i8] c"After effects: L=%.3f\00", align 1
@str.4 = private unnamed_addr constant [9 x i8] c" R=%.3f\0A\00", align 1
@str.5 = private unnamed_addr constant [27 x i8] c"Mixed oscillators: L=%.3f\0A\00", align 1
@str.6 = private unnamed_addr constant [29 x i8] c"=== Audio chain complete ===\00", align 1

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr noundef readonly captures(none), ...) local_unnamed_addr #0

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr noundef readonly captures(none)) local_unnamed_addr #0

; Function Attrs: mustprogress nocallback nofree nounwind willreturn memory(errnomem: write)
declare double @sin(double) local_unnamed_addr #1

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read)
define %Sample @operator_add(ptr readonly captures(none) %this, ptr readonly captures(none) %other) local_unnamed_addr #2 {
entry:
  %other.val.unpack = load double, ptr %other, align 8
  %other.val.elt10 = getelementptr inbounds nuw i8, ptr %other, i64 8
  %other.val.unpack11 = load double, ptr %other.val.elt10, align 8
  %left = load double, ptr %this, align 8
  %fadd = fadd double %other.val.unpack, %left
  %right_ptr5 = getelementptr inbounds nuw i8, ptr %this, i64 8
  %right = load double, ptr %right_ptr5, align 8
  %fadd8 = fadd double %other.val.unpack11, %right
  %out9.fca.0.insert = insertvalue %Sample poison, double %fadd, 0
  %out9.fca.1.insert = insertvalue %Sample %out9.fca.0.insert, double %fadd8, 1
  ret %Sample %out9.fca.1.insert
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read)
define %Sample @operator_mul(ptr readonly captures(none) %this, double %gain) local_unnamed_addr #2 {
entry:
  %left = load double, ptr %this, align 8
  %fmul = fmul double %gain, %left
  %right_ptr4 = getelementptr inbounds nuw i8, ptr %this, i64 8
  %right = load double, ptr %right_ptr4, align 8
  %fmul6 = fmul double %gain, %right
  %out7.fca.0.insert = insertvalue %Sample poison, double %fmul, 0
  %out7.fca.1.insert = insertvalue %Sample %out7.fca.0.insert, double %fmul6, 1
  ret %Sample %out7.fca.1.insert
}

define %Sample @AudioEffects_processSignal(ptr readonly captures(none) %input, { ptr, ptr } %effect) local_unnamed_addr {
entry:
  %pipe.arg.tmp = alloca %Sample, align 8
  %input.val.unpack = load double, ptr %input, align 8
  %input.val.elt4 = getelementptr inbounds nuw i8, ptr %input, i64 8
  %input.val.unpack5 = load double, ptr %input.val.elt4, align 8
  %effect.fca.0.extract = extractvalue { ptr, ptr } %effect, 0
  %effect.fca.1.extract = extractvalue { ptr, ptr } %effect, 1
  store double %input.val.unpack, ptr %pipe.arg.tmp, align 8
  %input3.fca.1.insert.fca.1.gep = getelementptr inbounds nuw i8, ptr %pipe.arg.tmp, i64 8
  store double %input.val.unpack5, ptr %input3.fca.1.insert.fca.1.gep, align 8
  %pipe.result = call %Sample %effect.fca.0.extract(ptr nonnull %pipe.arg.tmp, ptr %effect.fca.1.extract)
  ret %Sample %pipe.result
}

; Function Attrs: mustprogress nofree norecurse nounwind willreturn memory(errnomem: write)
define double @AudioEffects_oscillator(i32 %wave, double %phase) local_unnamed_addr #3 {
entry:
  switch i32 %wave, label %ifmerge13 [
    i32 0, label %then
    i32 3, label %then7
    i32 1, label %then12
  ]

common.ret:                                       ; preds = %tern.then16, %tern.else17, %ifmerge13, %then7, %then
  %common.ret.op = phi double [ %0, %then ], [ %., %then7 ], [ %fsub27, %ifmerge13 ], [ %fsub, %tern.then16 ], [ %fsub23, %tern.else17 ]
  ret double %common.ret.op

then:                                             ; preds = %entry
  %fmul = fmul double %phase, 0x401921FB54411744
  %0 = tail call double @sin(double %fmul)
  br label %common.ret

then7:                                            ; preds = %entry
  %flt = fcmp olt double %phase, 5.000000e-01
  %. = select i1 %flt, double 1.000000e+00, double -1.000000e+00
  br label %common.ret

then12:                                           ; preds = %entry
  %flt15 = fcmp olt double %phase, 5.000000e-01
  %fmul20 = fmul double %phase, 4.000000e+00
  br i1 %flt15, label %tern.then16, label %tern.else17

ifmerge13:                                        ; preds = %entry
  %fmul26 = fmul double %phase, 2.000000e+00
  %fsub27 = fadd double %fmul26, -1.000000e+00
  br label %common.ret

tern.then16:                                      ; preds = %then12
  %fsub = fadd double %fmul20, -1.000000e+00
  br label %common.ret

tern.else17:                                      ; preds = %then12
  %fsub23 = fsub double 3.000000e+00, %fmul20
  br label %common.ret
}

; Function Attrs: mustprogress nofree nounwind willreturn memory(write, argmem: none, inaccessiblemem: readwrite)
define { ptr, ptr } @AudioEffects_makeDelay(double %feedback) local_unnamed_addr #4 {
entry:
  %closure.ptr = tail call dereferenceable_or_null(40) ptr @malloc(i64 40)
  store i64 1, ptr %closure.ptr, align 4
  %cleanup.slot = getelementptr inbounds nuw i8, ptr %closure.ptr, i64 8
  %feedback.slot = getelementptr inbounds nuw i8, ptr %closure.ptr, i64 24
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 8 dereferenceable(16) %cleanup.slot, i8 0, i64 16, i1 false)
  store double %feedback, ptr %feedback.slot, align 8
  %prevRight.slot = getelementptr inbounds nuw i8, ptr %closure.ptr, i64 32
  store double 0.000000e+00, ptr %prevRight.slot, align 8
  %fat.env = insertvalue { ptr, ptr } { ptr @__lambda_0, ptr undef }, ptr %closure.ptr, 1
  ret { ptr, ptr } %fat.env
}

; Function Attrs: nofree nounwind
define noundef i32 @AudioEffects_main() local_unnamed_addr #0 {
__mingus_closure_release_wrapper.exit:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str)
  %1 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.1, double 0xBFE6666666666666)
  %2 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.4, double 0xBFE6666666666666)
  %3 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.3, double -1.400000e+00)
  %4 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.4, double -1.400000e+00)
  %5 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.5, double 0x3FB3333333333330)
  %6 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.6)
  ret i32 0
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read)
define internal %Sample @__lambda_0(ptr readonly captures(none) %input, ptr readonly captures(none) %env) #2 {
entry:
  %input.val.unpack = load double, ptr %input, align 8
  %input.val.elt18 = getelementptr inbounds nuw i8, ptr %input, i64 8
  %input.val.unpack19 = load double, ptr %input.val.elt18, align 8
  %prevLeft.cap = getelementptr inbounds nuw i8, ptr %env, i64 16
  %prevLeft = load double, ptr %prevLeft.cap, align 8
  %feedback.cap = getelementptr inbounds nuw i8, ptr %env, i64 24
  %feedback = load double, ptr %feedback.cap, align 8
  %prevRight.cap = getelementptr inbounds nuw i8, ptr %env, i64 32
  %prevRight = load double, ptr %prevRight.cap, align 8
  %fmul = fmul double %prevLeft, %feedback
  %fadd = fadd double %input.val.unpack, %fmul
  %fmul9 = fmul double %feedback, %prevRight
  %fadd10 = fadd double %input.val.unpack19, %fmul9
  %out17.fca.0.insert = insertvalue %Sample poison, double %fadd, 0
  %out17.fca.1.insert = insertvalue %Sample %out17.fca.0.insert, double %fadd10, 1
  ret %Sample %out17.fca.1.insert
}

; Function Attrs: mustprogress nofree nounwind willreturn allockind("alloc,uninitialized") allocsize(0) memory(inaccessiblemem: readwrite)
declare noalias noundef ptr @malloc(i64 noundef) local_unnamed_addr #5

; Function Attrs: nofree nounwind
define noundef i32 @main() local_unnamed_addr #0 {
entry:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str)
  %1 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.1, double 0xBFE6666666666666)
  %2 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.4, double 0xBFE6666666666666)
  %3 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.3, double -1.400000e+00)
  %4 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.4, double -1.400000e+00)
  %5 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.5, double 0x3FB3333333333330)
  %6 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.6)
  ret i32 0
}

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: write)
declare void @llvm.memset.p0.i64(ptr writeonly captures(none), i8, i64, i1 immarg) #6

attributes #0 = { nofree nounwind }
attributes #1 = { mustprogress nocallback nofree nounwind willreturn memory(errnomem: write) }
attributes #2 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read) }
attributes #3 = { mustprogress nofree norecurse nounwind willreturn memory(errnomem: write) }
attributes #4 = { mustprogress nofree nounwind willreturn memory(write, argmem: none, inaccessiblemem: readwrite) }
attributes #5 = { mustprogress nofree nounwind willreturn allockind("alloc,uninitialized") allocsize(0) memory(inaccessiblemem: readwrite) "alloc-family"="malloc" }
attributes #6 = { nocallback nofree nounwind willreturn memory(argmem: write) }
