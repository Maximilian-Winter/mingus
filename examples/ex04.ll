; ModuleID = 'mingus_module'
source_filename = "mingus_module"
target triple = "x86_64-pc-windows-msvc"

%Lexer = type { ptr, ptr, i32, %Token }
%Token = type { i32, i32 }
%Parser = type { ptr, %Lexer }

@Parser_vtable = internal constant [5 x ptr] [ptr @ExpressionParser_Parser_destructor, ptr @ExpressionParser_Parser_parse, ptr @ExpressionParser_Parser_parseTerm, ptr @ExpressionParser_Parser_parseExpression, ptr @ExpressionParser_Parser_parseFactor]
@Lexer_vtable = internal constant [7 x ptr] [ptr @ExpressionParser_Lexer_destructor, ptr @ExpressionParser_Lexer_peek, ptr @ExpressionParser_Lexer_consume, ptr @ExpressionParser_Lexer_parseNumber, ptr @ExpressionParser_Lexer_advance, ptr @ExpressionParser_Lexer_makeToken, ptr @ExpressionParser_Lexer_skipWhitespace]
@str = private unnamed_addr constant [26 x i8] c"=== Expression Parser ===\00", align 1
@str.1 = private unnamed_addr constant [19 x i8] c"Testing: 2 + 3 * 4\00", align 1
@str.2 = private unnamed_addr constant [26 x i8] c"Result: %d (expected 14)\0A\00", align 1
@str.3 = private unnamed_addr constant [10 x i8] c"2 + 3 * 4\00", align 1
@str.4 = private unnamed_addr constant [21 x i8] c"Testing: (2 + 3) * 4\00", align 1
@str.5 = private unnamed_addr constant [26 x i8] c"Result: %d (expected 20)\0A\00", align 1
@str.6 = private unnamed_addr constant [12 x i8] c"(2 + 3) * 4\00", align 1
@str.7 = private unnamed_addr constant [20 x i8] c"Testing: 10 - 3 - 2\00", align 1
@str.8 = private unnamed_addr constant [25 x i8] c"Result: %d (expected 5)\0A\00", align 1
@str.9 = private unnamed_addr constant [11 x i8] c"10 - 3 - 2\00", align 1
@str.10 = private unnamed_addr constant [21 x i8] c"Testing: 100 / 5 / 2\00", align 1
@str.11 = private unnamed_addr constant [26 x i8] c"Result: %d (expected 10)\0A\00", align 1
@str.12 = private unnamed_addr constant [12 x i8] c"100 / 5 / 2\00", align 1
@str.13 = private unnamed_addr constant [12 x i8] c"Testing: 42\00", align 1
@str.14 = private unnamed_addr constant [26 x i8] c"Result: %d (expected 42)\0A\00", align 1
@str.15 = private unnamed_addr constant [3 x i8] c"42\00", align 1
@str.16 = private unnamed_addr constant [27 x i8] c"Testing: 1 + 2 + 3 + 4 + 5\00", align 1
@str.17 = private unnamed_addr constant [26 x i8] c"Result: %d (expected 15)\0A\00", align 1
@str.18 = private unnamed_addr constant [18 x i8] c"1 + 2 + 3 + 4 + 5\00", align 1
@str.19 = private unnamed_addr constant [24 x i8] c"=== Parser complete ===\00", align 1

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr noundef readonly captures(none), ...) local_unnamed_addr #0

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr noundef readonly captures(none)) local_unnamed_addr #0

define void @ExpressionParser_Parser_constructor(ptr writeonly captures(none) initializes((0, 40)) %this, ptr %input) local_unnamed_addr {
entry:
  %ctor.tmp = alloca %Lexer, align 8
  store ptr @Parser_vtable, ptr %this, align 8
  %lexer_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  store ptr @Lexer_vtable, ptr %ctor.tmp, align 8
  %input_ptr.i = getelementptr inbounds nuw i8, ptr %ctor.tmp, i64 8
  store ptr %input, ptr %input_ptr.i, align 8
  %pos_ptr.i = getelementptr inbounds nuw i8, ptr %ctor.tmp, i64 16
  store i32 0, ptr %pos_ptr.i, align 8
  call void @ExpressionParser_Lexer_advance(ptr nonnull %ctor.tmp)
  %ctor.val.fca.0.load = load ptr, ptr %ctor.tmp, align 8
  %ctor.val.fca.0.insert = insertvalue %Lexer poison, ptr %ctor.val.fca.0.load, 0
  %ctor.val.fca.1.load = load ptr, ptr %input_ptr.i, align 8
  %ctor.val.fca.1.insert = insertvalue %Lexer %ctor.val.fca.0.insert, ptr %ctor.val.fca.1.load, 1
  %ctor.val.fca.2.load = load i32, ptr %pos_ptr.i, align 8
  %ctor.val.fca.2.insert = insertvalue %Lexer %ctor.val.fca.1.insert, i32 %ctor.val.fca.2.load, 2
  %ctor.val.fca.3.0.gep = getelementptr inbounds nuw i8, ptr %ctor.tmp, i64 20
  %ctor.val.fca.3.0.load = load i32, ptr %ctor.val.fca.3.0.gep, align 4
  %ctor.val.fca.3.0.insert = insertvalue %Lexer %ctor.val.fca.2.insert, i32 %ctor.val.fca.3.0.load, 3, 0
  %ctor.val.fca.3.1.gep = getelementptr inbounds nuw i8, ptr %ctor.tmp, i64 24
  %ctor.val.fca.3.1.load = load i32, ptr %ctor.val.fca.3.1.gep, align 8
  %ctor.val.fca.3.1.insert = insertvalue %Lexer %ctor.val.fca.3.0.insert, i32 %ctor.val.fca.3.1.load, 3, 1
  store %Lexer %ctor.val.fca.3.1.insert, ptr %lexer_ptr, align 8
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define void @ExpressionParser_Parser_destructor(ptr readnone captures(none) %this) #1 {
entry:
  ret void
}

define i32 @ExpressionParser_Parser_parse(ptr %this) {
entry:
  %vtable = load ptr, ptr %this, align 8
  %method.slot = getelementptr i8, ptr %vtable, i64 24
  %method.fn = load ptr, ptr %method.slot, align 8
  %0 = tail call i32 %method.fn(ptr nonnull %this)
  ret i32 %0
}

define i32 @ExpressionParser_Parser_parseTerm(ptr %this) {
entry:
  %vtable = load ptr, ptr %this, align 8
  %method.slot = getelementptr i8, ptr %vtable, i64 32
  %method.fn = load ptr, ptr %method.slot, align 8
  %0 = tail call i32 %method.fn(ptr nonnull %this)
  %lexer_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  br label %while.cond

while.cond:                                       ; preds = %while.cond.backedge, %entry
  %left.0 = phi i32 [ %0, %entry ], [ %left.0.be, %while.cond.backedge ]
  %vtable2 = load ptr, ptr %lexer_ptr, align 8
  %method.slot3 = getelementptr i8, ptr %vtable2, i64 8
  %method.fn4 = load ptr, ptr %method.slot3, align 8
  %1 = tail call %Token %method.fn4(ptr nonnull %lexer_ptr)
  %.fca.0.extract = extractvalue %Token %1, 0
  switch i32 %.fca.0.extract, label %elif.next [
    i32 3, label %then
    i32 4, label %elif.then
  ]

then:                                             ; preds = %while.cond
  %vtable7 = load ptr, ptr %lexer_ptr, align 8
  %method.slot8 = getelementptr i8, ptr %vtable7, i64 16
  %method.fn9 = load ptr, ptr %method.slot8, align 8
  %2 = tail call i1 %method.fn9(ptr nonnull %lexer_ptr, i32 3)
  %vtable12 = load ptr, ptr %this, align 8
  %method.slot13 = getelementptr i8, ptr %vtable12, i64 32
  %method.fn14 = load ptr, ptr %method.slot13, align 8
  %3 = tail call i32 %method.fn14(ptr nonnull %this)
  %mul = mul i32 %3, %left.0
  br label %while.cond.backedge

elif.then:                                        ; preds = %while.cond
  %vtable20 = load ptr, ptr %lexer_ptr, align 8
  %method.slot21 = getelementptr i8, ptr %vtable20, i64 16
  %method.fn22 = load ptr, ptr %method.slot21, align 8
  %4 = tail call i1 %method.fn22(ptr nonnull %lexer_ptr, i32 4)
  %vtable24 = load ptr, ptr %this, align 8
  %method.slot25 = getelementptr i8, ptr %vtable24, i64 32
  %method.fn26 = load ptr, ptr %method.slot25, align 8
  %5 = tail call i32 %method.fn26(ptr nonnull %this)
  %eq28 = icmp eq i32 %5, 0
  br i1 %eq28, label %while.cond.backedge, label %tern.else

elif.next:                                        ; preds = %while.cond
  ret i32 %left.0

tern.else:                                        ; preds = %elif.then
  %sdiv = sdiv i32 %left.0, %5
  br label %while.cond.backedge

while.cond.backedge:                              ; preds = %tern.else, %elif.then, %then
  %left.0.be = phi i32 [ %mul, %then ], [ %sdiv, %tern.else ], [ 0, %elif.then ]
  br label %while.cond
}

define i32 @ExpressionParser_Parser_parseExpression(ptr %this) {
entry:
  %vtable = load ptr, ptr %this, align 8
  %method.slot = getelementptr i8, ptr %vtable, i64 16
  %method.fn = load ptr, ptr %method.slot, align 8
  %0 = tail call i32 %method.fn(ptr nonnull %this)
  %lexer_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  br label %while.cond

while.cond:                                       ; preds = %while.cond.backedge, %entry
  %left.0 = phi i32 [ %0, %entry ], [ %left.0.be, %while.cond.backedge ]
  %vtable2 = load ptr, ptr %lexer_ptr, align 8
  %method.slot3 = getelementptr i8, ptr %vtable2, i64 8
  %method.fn4 = load ptr, ptr %method.slot3, align 8
  %1 = tail call %Token %method.fn4(ptr nonnull %lexer_ptr)
  %.fca.0.extract = extractvalue %Token %1, 0
  switch i32 %.fca.0.extract, label %elif.next [
    i32 1, label %then
    i32 2, label %elif.then
  ]

then:                                             ; preds = %while.cond
  %vtable7 = load ptr, ptr %lexer_ptr, align 8
  %method.slot8 = getelementptr i8, ptr %vtable7, i64 16
  %method.fn9 = load ptr, ptr %method.slot8, align 8
  %2 = tail call i1 %method.fn9(ptr nonnull %lexer_ptr, i32 1)
  %vtable12 = load ptr, ptr %this, align 8
  %method.slot13 = getelementptr i8, ptr %vtable12, i64 16
  %method.fn14 = load ptr, ptr %method.slot13, align 8
  %3 = tail call i32 %method.fn14(ptr nonnull %this)
  %add = add i32 %3, %left.0
  br label %while.cond.backedge

elif.then:                                        ; preds = %while.cond
  %vtable20 = load ptr, ptr %lexer_ptr, align 8
  %method.slot21 = getelementptr i8, ptr %vtable20, i64 16
  %method.fn22 = load ptr, ptr %method.slot21, align 8
  %4 = tail call i1 %method.fn22(ptr nonnull %lexer_ptr, i32 2)
  %vtable25 = load ptr, ptr %this, align 8
  %method.slot26 = getelementptr i8, ptr %vtable25, i64 16
  %method.fn27 = load ptr, ptr %method.slot26, align 8
  %5 = tail call i32 %method.fn27(ptr nonnull %this)
  %sub = sub i32 %left.0, %5
  br label %while.cond.backedge

while.cond.backedge:                              ; preds = %elif.then, %then
  %left.0.be = phi i32 [ %add, %then ], [ %sub, %elif.then ]
  br label %while.cond

elif.next:                                        ; preds = %while.cond
  ret i32 %left.0
}

define i32 @ExpressionParser_Parser_parseFactor(ptr %this) {
entry:
  %lexer_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  %vtable = load ptr, ptr %lexer_ptr, align 8
  %method.slot = getelementptr i8, ptr %vtable, i64 8
  %method.fn = load ptr, ptr %method.slot, align 8
  %0 = tail call %Token %method.fn(ptr nonnull %lexer_ptr)
  %.fca.0.extract = extractvalue %Token %0, 0
  switch i32 %.fca.0.extract, label %match.merge [
    i32 0, label %arm.body
    i32 5, label %arm.body6
  ]

match.merge.sink.split:                           ; preds = %arm.body, %arm.body6
  %.sink = phi i32 [ 6, %arm.body6 ], [ 0, %arm.body ]
  %match.result.ph = phi i32 [ %3, %arm.body6 ], [ %.fca.1.extract, %arm.body ]
  %vtable20 = load ptr, ptr %lexer_ptr, align 8
  %method.slot21 = getelementptr i8, ptr %vtable20, i64 16
  %method.fn22 = load ptr, ptr %method.slot21, align 8
  %1 = tail call i1 %method.fn22(ptr nonnull %lexer_ptr, i32 %.sink)
  br label %match.merge

match.merge:                                      ; preds = %match.merge.sink.split, %entry
  %match.result = phi i32 [ 0, %entry ], [ %match.result.ph, %match.merge.sink.split ]
  ret i32 %match.result

arm.body:                                         ; preds = %entry
  %.fca.1.extract = extractvalue %Token %0, 1
  br label %match.merge.sink.split

arm.body6:                                        ; preds = %entry
  %vtable11 = load ptr, ptr %lexer_ptr, align 8
  %method.slot12 = getelementptr i8, ptr %vtable11, i64 16
  %method.fn13 = load ptr, ptr %method.slot12, align 8
  %2 = tail call i1 %method.fn13(ptr nonnull %lexer_ptr, i32 5)
  %vtable15 = load ptr, ptr %this, align 8
  %method.slot16 = getelementptr i8, ptr %vtable15, i64 24
  %method.fn17 = load ptr, ptr %method.slot16, align 8
  %3 = tail call i32 %method.fn17(ptr nonnull %this)
  br label %match.merge.sink.split
}

define void @ExpressionParser_Lexer_constructor(ptr initializes((0, 20)) %this, ptr %src) local_unnamed_addr {
entry:
  store ptr @Lexer_vtable, ptr %this, align 8
  %input_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  store ptr %src, ptr %input_ptr, align 8
  %pos_ptr = getelementptr inbounds nuw i8, ptr %this, i64 16
  store i32 0, ptr %pos_ptr, align 4
  tail call void @ExpressionParser_Lexer_advance(ptr nonnull %this)
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define void @ExpressionParser_Lexer_destructor(ptr readnone captures(none) %this) #1 {
entry:
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read)
define %Token @ExpressionParser_Lexer_peek(ptr readonly captures(none) %this) #2 {
entry:
  %current_ptr = getelementptr inbounds nuw i8, ptr %this, i64 20
  %current.unpack = load i32, ptr %current_ptr, align 4
  %0 = insertvalue %Token poison, i32 %current.unpack, 0
  %current.elt1 = getelementptr inbounds nuw i8, ptr %this, i64 24
  %current.unpack2 = load i32, ptr %current.elt1, align 4
  %current3 = insertvalue %Token %0, i32 %current.unpack2, 1
  ret %Token %current3
}

define noundef i1 @ExpressionParser_Lexer_consume(ptr %this, i32 %expected) {
entry:
  %current_ptr = getelementptr inbounds nuw i8, ptr %this, i64 20
  %type = load i32, ptr %current_ptr, align 4
  %eq = icmp eq i32 %type, %expected
  br i1 %eq, label %then, label %common.ret

common.ret:                                       ; preds = %entry, %then
  ret i1 %eq

then:                                             ; preds = %entry
  %vtable = load ptr, ptr %this, align 8
  %method.slot = getelementptr i8, ptr %vtable, i64 32
  %method.fn = load ptr, ptr %method.slot, align 8
  tail call void %method.fn(ptr nonnull %this)
  br label %common.ret
}

; Function Attrs: nofree norecurse nounwind memory(read, argmem: readwrite, inaccessiblemem: none)
define %Token @ExpressionParser_Lexer_parseNumber(ptr captures(none) %this) #3 {
entry:
  %pos_ptr = getelementptr inbounds nuw i8, ptr %this, i64 16
  %input_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  %input = load ptr, ptr %input_ptr, align 8
  %pos_ptr.promoted = load i32, ptr %pos_ptr, align 4
  %strlen24 = tail call i64 @strlen(ptr noundef nonnull dereferenceable(1) %input)
  %len.i3225 = trunc i64 %strlen24 to i32
  %slt26 = icmp slt i32 %pos_ptr.promoted, %len.i3225
  br i1 %slt26, label %while.body, label %while.exit

while.body:                                       ; preds = %entry, %ifmerge
  %val.028 = phi i32 [ %add, %ifmerge ], [ 0, %entry ]
  %pos2327 = phi i32 [ %add15, %ifmerge ], [ %pos_ptr.promoted, %entry ]
  %idx.i64 = sext i32 %pos2327 to i64
  %str.idx = getelementptr i8, ptr %input, i64 %idx.i64
  %char = load i8, ptr %str.idx, align 1
  %0 = add i8 %char, -58
  %1 = icmp ult i8 %0, -10
  br i1 %1, label %while.exit, label %ifmerge

while.exit:                                       ; preds = %ifmerge, %while.body, %entry
  %val.0.lcssa = phi i32 [ 0, %entry ], [ %val.028, %while.body ], [ %add, %ifmerge ]
  %tok17.fca.1.insert = insertvalue %Token { i32 0, i32 poison }, i32 %val.0.lcssa, 1
  ret %Token %tok17.fca.1.insert

ifmerge:                                          ; preds = %while.body
  %sext = zext nneg i8 %char to i32
  %mul = mul i32 %val.028, 10
  %sub = add i32 %mul, -48
  %add = add i32 %sub, %sext
  %add15 = add nsw i32 %pos2327, 1
  store i32 %add15, ptr %pos_ptr, align 4
  %strlen = tail call i64 @strlen(ptr noundef nonnull dereferenceable(1) %input)
  %len.i32 = trunc i64 %strlen to i32
  %slt = icmp slt i32 %add15, %len.i32
  br i1 %slt, label %while.body, label %while.exit
}

define void @ExpressionParser_Lexer_advance(ptr %this) {
entry:
  %vtable = load ptr, ptr %this, align 8
  %method.slot = getelementptr i8, ptr %vtable, i64 48
  %method.fn = load ptr, ptr %method.slot, align 8
  tail call void %method.fn(ptr nonnull %this)
  %pos_ptr = getelementptr inbounds nuw i8, ptr %this, i64 16
  %pos = load i32, ptr %pos_ptr, align 4
  %input_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  %input = load ptr, ptr %input_ptr, align 8
  %strlen = tail call i64 @strlen(ptr noundef nonnull dereferenceable(1) %input)
  %len.i32 = trunc i64 %strlen to i32
  %sge.not = icmp slt i32 %pos, %len.i32
  br i1 %sge.not, label %ifmerge, label %then

common.ret:                                       ; preds = %match.merge, %then
  ret void

then:                                             ; preds = %entry
  %current_ptr = getelementptr inbounds nuw i8, ptr %this, i64 20
  %current_ptr.repack65 = getelementptr inbounds nuw i8, ptr %this, i64 24
  store i32 0, ptr %current_ptr.repack65, align 4
  store i32 7, ptr %current_ptr, align 4
  br label %common.ret

ifmerge:                                          ; preds = %entry
  %idx.i64 = sext i32 %pos to i64
  %str.idx = getelementptr i8, ptr %input, i64 %idx.i64
  %char = load i8, ptr %str.idx, align 1
  %current_ptr6 = getelementptr inbounds nuw i8, ptr %this, i64 20
  switch i8 %char, label %arm.test41 [
    i8 43, label %arm.body
    i8 45, label %arm.body12
    i8 42, label %arm.body19
    i8 47, label %arm.body26
    i8 40, label %arm.body33
    i8 41, label %arm.body40
  ]

match.merge:                                      ; preds = %arm.body57, %arm.body47, %arm.body40, %arm.body33, %arm.body26, %arm.body19, %arm.body12, %arm.body
  %match.result = phi %Token [ %0, %arm.body ], [ %1, %arm.body12 ], [ %2, %arm.body19 ], [ %3, %arm.body26 ], [ %4, %arm.body33 ], [ %5, %arm.body40 ], [ %8, %arm.body47 ], [ %9, %arm.body57 ]
  %match.result.elt = extractvalue %Token %match.result, 0
  store i32 %match.result.elt, ptr %current_ptr6, align 4
  %current_ptr6.repack63 = getelementptr inbounds nuw i8, ptr %this, i64 24
  %match.result.elt64 = extractvalue %Token %match.result, 1
  store i32 %match.result.elt64, ptr %current_ptr6.repack63, align 4
  br label %common.ret

arm.body:                                         ; preds = %ifmerge
  %vtable9 = load ptr, ptr %this, align 8
  %method.slot10 = getelementptr i8, ptr %vtable9, i64 40
  %method.fn11 = load ptr, ptr %method.slot10, align 8
  %0 = tail call %Token %method.fn11(ptr nonnull %this, i32 1, i32 0)
  br label %match.merge

arm.body12:                                       ; preds = %ifmerge
  %vtable16 = load ptr, ptr %this, align 8
  %method.slot17 = getelementptr i8, ptr %vtable16, i64 40
  %method.fn18 = load ptr, ptr %method.slot17, align 8
  %1 = tail call %Token %method.fn18(ptr nonnull %this, i32 2, i32 0)
  br label %match.merge

arm.body19:                                       ; preds = %ifmerge
  %vtable23 = load ptr, ptr %this, align 8
  %method.slot24 = getelementptr i8, ptr %vtable23, i64 40
  %method.fn25 = load ptr, ptr %method.slot24, align 8
  %2 = tail call %Token %method.fn25(ptr nonnull %this, i32 3, i32 0)
  br label %match.merge

arm.body26:                                       ; preds = %ifmerge
  %vtable30 = load ptr, ptr %this, align 8
  %method.slot31 = getelementptr i8, ptr %vtable30, i64 40
  %method.fn32 = load ptr, ptr %method.slot31, align 8
  %3 = tail call %Token %method.fn32(ptr nonnull %this, i32 4, i32 0)
  br label %match.merge

arm.body33:                                       ; preds = %ifmerge
  %vtable37 = load ptr, ptr %this, align 8
  %method.slot38 = getelementptr i8, ptr %vtable37, i64 40
  %method.fn39 = load ptr, ptr %method.slot38, align 8
  %4 = tail call %Token %method.fn39(ptr nonnull %this, i32 5, i32 0)
  br label %match.merge

arm.body40:                                       ; preds = %ifmerge
  %vtable44 = load ptr, ptr %this, align 8
  %method.slot45 = getelementptr i8, ptr %vtable44, i64 40
  %method.fn46 = load ptr, ptr %method.slot45, align 8
  %5 = tail call %Token %method.fn46(ptr nonnull %this, i32 6, i32 0)
  br label %match.merge

arm.test41:                                       ; preds = %ifmerge
  %6 = add i8 %char, -48
  %7 = icmp ult i8 %6, 10
  %vtable54 = load ptr, ptr %this, align 8
  br i1 %7, label %arm.body47, label %arm.body57

arm.body47:                                       ; preds = %arm.test41
  %method.slot55 = getelementptr i8, ptr %vtable54, i64 24
  %method.fn56 = load ptr, ptr %method.slot55, align 8
  %8 = tail call %Token %method.fn56(ptr nonnull %this)
  br label %match.merge

arm.body57:                                       ; preds = %arm.test41
  %method.slot60 = getelementptr i8, ptr %vtable54, i64 40
  %method.fn61 = load ptr, ptr %method.slot60, align 8
  %9 = tail call %Token %method.fn61(ptr nonnull %this, i32 7, i32 0)
  br label %match.merge
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: readwrite)
define %Token @ExpressionParser_Lexer_makeToken(ptr captures(none) %this, i32 %t, i32 %v) #4 {
entry:
  %pos_ptr = getelementptr inbounds nuw i8, ptr %this, i64 16
  %pos = load i32, ptr %pos_ptr, align 4
  %add = add i32 %pos, 1
  store i32 %add, ptr %pos_ptr, align 4
  %tok6.fca.0.insert = insertvalue %Token poison, i32 %t, 0
  %tok6.fca.1.insert = insertvalue %Token %tok6.fca.0.insert, i32 %v, 1
  ret %Token %tok6.fca.1.insert
}

; Function Attrs: nofree norecurse nounwind memory(read, argmem: readwrite, inaccessiblemem: none)
define void @ExpressionParser_Lexer_skipWhitespace(ptr captures(none) %this) #3 {
entry:
  %pos_ptr = getelementptr inbounds nuw i8, ptr %this, i64 16
  %input_ptr = getelementptr inbounds nuw i8, ptr %this, i64 8
  %input = load ptr, ptr %input_ptr, align 8
  %pos_ptr.promoted = load i32, ptr %pos_ptr, align 4
  %strlen14 = tail call i64 @strlen(ptr noundef nonnull dereferenceable(1) %input)
  %len.i3215 = trunc i64 %strlen14 to i32
  %slt16 = icmp slt i32 %pos_ptr.promoted, %len.i3215
  br i1 %slt16, label %while.body, label %while.exit

while.body:                                       ; preds = %entry, %ifmerge
  %pos1317 = phi i32 [ %add, %ifmerge ], [ %pos_ptr.promoted, %entry ]
  %idx.i64 = sext i32 %pos1317 to i64
  %str.idx = getelementptr i8, ptr %input, i64 %idx.i64
  %char = load i8, ptr %str.idx, align 1
  switch i8 %char, label %while.exit [
    i8 32, label %ifmerge
    i8 9, label %ifmerge
  ]

while.exit:                                       ; preds = %ifmerge, %while.body, %entry
  ret void

ifmerge:                                          ; preds = %while.body, %while.body
  %add = add nsw i32 %pos1317, 1
  store i32 %add, ptr %pos_ptr, align 4
  %strlen = tail call i64 @strlen(ptr noundef nonnull dereferenceable(1) %input)
  %len.i32 = trunc i64 %strlen to i32
  %slt = icmp slt i32 %add, %len.i32
  br i1 %slt, label %while.body, label %while.exit
}

define i32 @ExpressionParser_evaluate(ptr %expr) local_unnamed_addr {
entry:
  %ctor.tmp.i = alloca %Lexer, align 8
  %parser = alloca %Parser, align 8
  %.fca.1.0.gep = getelementptr inbounds nuw i8, ptr %parser, i64 8
  %.fca.1.1.gep = getelementptr inbounds nuw i8, ptr %parser, i64 16
  %.fca.1.2.gep = getelementptr inbounds nuw i8, ptr %parser, i64 24
  %.fca.1.3.0.gep = getelementptr inbounds nuw i8, ptr %parser, i64 28
  %.fca.1.3.1.gep = getelementptr inbounds nuw i8, ptr %parser, i64 32
  call void @llvm.lifetime.start.p0(i64 32, ptr nonnull %ctor.tmp.i)
  store ptr @Lexer_vtable, ptr %ctor.tmp.i, align 8
  %input_ptr.i.i = getelementptr inbounds nuw i8, ptr %ctor.tmp.i, i64 8
  store ptr %expr, ptr %input_ptr.i.i, align 8
  %pos_ptr.i.i = getelementptr inbounds nuw i8, ptr %ctor.tmp.i, i64 16
  store i32 0, ptr %pos_ptr.i.i, align 8
  call void @ExpressionParser_Lexer_advance(ptr nonnull %ctor.tmp.i)
  %ctor.val.fca.0.load.i = load ptr, ptr %ctor.tmp.i, align 8
  %ctor.val.fca.1.load.i = load ptr, ptr %input_ptr.i.i, align 8
  %ctor.val.fca.2.load.i = load i32, ptr %pos_ptr.i.i, align 8
  %ctor.val.fca.3.0.gep.i = getelementptr inbounds nuw i8, ptr %ctor.tmp.i, i64 20
  %ctor.val.fca.3.0.load.i = load i32, ptr %ctor.val.fca.3.0.gep.i, align 4
  %ctor.val.fca.3.1.gep.i = getelementptr inbounds nuw i8, ptr %ctor.tmp.i, i64 24
  %ctor.val.fca.3.1.load.i = load i32, ptr %ctor.val.fca.3.1.gep.i, align 8
  call void @llvm.lifetime.end.p0(i64 32, ptr nonnull %ctor.tmp.i)
  store ptr @Parser_vtable, ptr %parser, align 8
  store ptr %ctor.val.fca.0.load.i, ptr %.fca.1.0.gep, align 8
  store ptr %ctor.val.fca.1.load.i, ptr %.fca.1.1.gep, align 8
  store i32 %ctor.val.fca.2.load.i, ptr %.fca.1.2.gep, align 8
  store i32 %ctor.val.fca.3.0.load.i, ptr %.fca.1.3.0.gep, align 4
  store i32 %ctor.val.fca.3.1.load.i, ptr %.fca.1.3.1.gep, align 8
  %method.slot.i33 = getelementptr i8, ptr %ctor.val.fca.0.load.i, i64 8
  %method.fn.i34 = load ptr, ptr %method.slot.i33, align 8
  %0 = call %Token %method.fn.i34(ptr nonnull %.fca.1.0.gep)
  %.fca.0.extract.i35 = extractvalue %Token %0, 0
  switch i32 %.fca.0.extract.i35, label %while.cond.i12.preheader [
    i32 0, label %arm.body.i
    i32 5, label %arm.body6.i
  ]

match.merge.sink.split.i:                         ; preds = %arm.body6.i, %arm.body.i
  %.sink.i = phi i32 [ 6, %arm.body6.i ], [ 0, %arm.body.i ]
  %match.result.ph.i = phi i32 [ %3, %arm.body6.i ], [ %.fca.1.extract.i, %arm.body.i ]
  %vtable20.i36 = load ptr, ptr %.fca.1.0.gep, align 8
  %method.slot21.i37 = getelementptr i8, ptr %vtable20.i36, i64 16
  %method.fn22.i38 = load ptr, ptr %method.slot21.i37, align 8
  %1 = call i1 %method.fn22.i38(ptr nonnull %.fca.1.0.gep, i32 %.sink.i)
  br label %while.cond.i12.preheader

while.cond.i12.preheader:                         ; preds = %entry, %match.merge.sink.split.i
  %left.0.i13.ph = phi i32 [ %match.result.ph.i, %match.merge.sink.split.i ], [ 0, %entry ]
  br label %while.cond.i12

arm.body.i:                                       ; preds = %entry
  %.fca.1.extract.i = extractvalue %Token %0, 1
  br label %match.merge.sink.split.i

arm.body6.i:                                      ; preds = %entry
  %vtable11.i = load ptr, ptr %.fca.1.0.gep, align 8
  %method.slot12.i = getelementptr i8, ptr %vtable11.i, i64 16
  %method.fn13.i = load ptr, ptr %method.slot12.i, align 8
  %2 = call i1 %method.fn13.i(ptr nonnull %.fca.1.0.gep, i32 5)
  %vtable15.i = load ptr, ptr %parser, align 8
  %method.slot16.i = getelementptr i8, ptr %vtable15.i, i64 24
  %method.fn17.i = load ptr, ptr %method.slot16.i, align 8
  %3 = call i32 %method.fn17.i(ptr nonnull %parser)
  br label %match.merge.sink.split.i

while.cond.i12:                                   ; preds = %while.cond.i12.backedge, %while.cond.i12.preheader
  %left.0.i13 = phi i32 [ %left.0.i13.ph, %while.cond.i12.preheader ], [ %left.0.i13.be, %while.cond.i12.backedge ]
  %vtable2.i14 = load ptr, ptr %.fca.1.0.gep, align 8
  %method.slot3.i15 = getelementptr i8, ptr %vtable2.i14, i64 8
  %method.fn4.i16 = load ptr, ptr %method.slot3.i15, align 8
  %4 = call %Token %method.fn4.i16(ptr nonnull %.fca.1.0.gep)
  %.fca.0.extract.i17 = extractvalue %Token %4, 0
  switch i32 %.fca.0.extract.i17, label %while.cond.i [
    i32 3, label %then.i24
    i32 4, label %elif.then.i18
  ]

then.i24:                                         ; preds = %while.cond.i12
  %vtable7.i25 = load ptr, ptr %.fca.1.0.gep, align 8
  %method.slot8.i26 = getelementptr i8, ptr %vtable7.i25, i64 16
  %method.fn9.i27 = load ptr, ptr %method.slot8.i26, align 8
  %5 = call i1 %method.fn9.i27(ptr nonnull %.fca.1.0.gep, i32 3)
  %vtable12.i28 = load ptr, ptr %parser, align 8
  %method.slot13.i29 = getelementptr i8, ptr %vtable12.i28, i64 32
  %method.fn14.i30 = load ptr, ptr %method.slot13.i29, align 8
  %6 = call i32 %method.fn14.i30(ptr nonnull %parser)
  %mul.i = mul i32 %6, %left.0.i13
  br label %while.cond.i12.backedge

elif.then.i18:                                    ; preds = %while.cond.i12
  %vtable20.i19 = load ptr, ptr %.fca.1.0.gep, align 8
  %method.slot21.i20 = getelementptr i8, ptr %vtable20.i19, i64 16
  %method.fn22.i21 = load ptr, ptr %method.slot21.i20, align 8
  %7 = call i1 %method.fn22.i21(ptr nonnull %.fca.1.0.gep, i32 4)
  %vtable24.i = load ptr, ptr %parser, align 8
  %method.slot25.i = getelementptr i8, ptr %vtable24.i, i64 32
  %method.fn26.i = load ptr, ptr %method.slot25.i, align 8
  %8 = call i32 %method.fn26.i(ptr nonnull %parser)
  %eq28.i = icmp eq i32 %8, 0
  br i1 %eq28.i, label %while.cond.i12.backedge, label %tern.else.i

tern.else.i:                                      ; preds = %elif.then.i18
  %sdiv.i = sdiv i32 %left.0.i13, %8
  br label %while.cond.i12.backedge

while.cond.i12.backedge:                          ; preds = %tern.else.i, %elif.then.i18, %then.i24
  %left.0.i13.be = phi i32 [ %mul.i, %then.i24 ], [ %sdiv.i, %tern.else.i ], [ 0, %elif.then.i18 ]
  br label %while.cond.i12

while.cond.i:                                     ; preds = %while.cond.i12, %while.cond.i.backedge
  %left.0.i = phi i32 [ %left.0.i.be, %while.cond.i.backedge ], [ %left.0.i13, %while.cond.i12 ]
  %vtable2.i = load ptr, ptr %.fca.1.0.gep, align 8
  %method.slot3.i = getelementptr i8, ptr %vtable2.i, i64 8
  %method.fn4.i = load ptr, ptr %method.slot3.i, align 8
  %9 = call %Token %method.fn4.i(ptr nonnull %.fca.1.0.gep)
  %.fca.0.extract.i = extractvalue %Token %9, 0
  switch i32 %.fca.0.extract.i, label %ExpressionParser_Parser_parseExpression.exit [
    i32 1, label %then.i
    i32 2, label %elif.then.i
  ]

then.i:                                           ; preds = %while.cond.i
  %vtable7.i = load ptr, ptr %.fca.1.0.gep, align 8
  %method.slot8.i = getelementptr i8, ptr %vtable7.i, i64 16
  %method.fn9.i = load ptr, ptr %method.slot8.i, align 8
  %10 = call i1 %method.fn9.i(ptr nonnull %.fca.1.0.gep, i32 1)
  %vtable12.i = load ptr, ptr %parser, align 8
  %method.slot13.i = getelementptr i8, ptr %vtable12.i, i64 16
  %method.fn14.i = load ptr, ptr %method.slot13.i, align 8
  %11 = call i32 %method.fn14.i(ptr nonnull %parser)
  %add.i = add i32 %11, %left.0.i
  br label %while.cond.i.backedge

elif.then.i:                                      ; preds = %while.cond.i
  %vtable20.i = load ptr, ptr %.fca.1.0.gep, align 8
  %method.slot21.i = getelementptr i8, ptr %vtable20.i, i64 16
  %method.fn22.i = load ptr, ptr %method.slot21.i, align 8
  %12 = call i1 %method.fn22.i(ptr nonnull %.fca.1.0.gep, i32 2)
  %vtable25.i = load ptr, ptr %parser, align 8
  %method.slot26.i = getelementptr i8, ptr %vtable25.i, i64 16
  %method.fn27.i = load ptr, ptr %method.slot26.i, align 8
  %13 = call i32 %method.fn27.i(ptr nonnull %parser)
  %sub.i = sub i32 %left.0.i, %13
  br label %while.cond.i.backedge

while.cond.i.backedge:                            ; preds = %elif.then.i, %then.i
  %left.0.i.be = phi i32 [ %add.i, %then.i ], [ %sub.i, %elif.then.i ]
  br label %while.cond.i

ExpressionParser_Parser_parseExpression.exit:     ; preds = %while.cond.i
  ret i32 %left.0.i
}

define noundef i32 @ExpressionParser_main() local_unnamed_addr {
entry:
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str)
  %1 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.1)
  %2 = tail call i32 @ExpressionParser_evaluate(ptr nonnull @str.3)
  %3 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.2, i32 %2)
  %4 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.4)
  %5 = tail call i32 @ExpressionParser_evaluate(ptr nonnull @str.6)
  %6 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.5, i32 %5)
  %7 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.7)
  %8 = tail call i32 @ExpressionParser_evaluate(ptr nonnull @str.9)
  %9 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.8, i32 %8)
  %10 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.10)
  %11 = tail call i32 @ExpressionParser_evaluate(ptr nonnull @str.12)
  %12 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.11, i32 %11)
  %13 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.13)
  %14 = tail call i32 @ExpressionParser_evaluate(ptr nonnull @str.15)
  %15 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.14, i32 %14)
  %16 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.16)
  %17 = tail call i32 @ExpressionParser_evaluate(ptr nonnull @str.18)
  %18 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @str.17, i32 %17)
  %19 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.19)
  ret i32 0
}

; Function Attrs: mustprogress nocallback nofree nounwind willreturn memory(argmem: read)
declare i64 @strlen(ptr captures(none)) local_unnamed_addr #5

define noundef i32 @main() local_unnamed_addr {
entry:
  %0 = tail call i32 @ExpressionParser_main()
  ret i32 0
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(i64 immarg, ptr captures(none)) #6

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p0(i64 immarg, ptr captures(none)) #6

attributes #0 = { nofree nounwind }
attributes #1 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) }
attributes #2 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read) }
attributes #3 = { nofree norecurse nounwind memory(read, argmem: readwrite, inaccessiblemem: none) }
attributes #4 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: readwrite) }
attributes #5 = { mustprogress nocallback nofree nounwind willreturn memory(argmem: read) }
attributes #6 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
