; ModuleID = 'mingus_module'
source_filename = "mingus_module"
target triple = "x86_64-pc-windows-msvc"

@str = private unnamed_addr constant [51 x i8] c"=== Mingus SQLite3 \E2\80\94 Auto-Generated Bindings ===\00", align 1
@str.1 = private unnamed_addr constant [9 x i8] c":memory:\00", align 1
@str.3 = private unnamed_addr constant [29 x i8] c"Database opened (in-memory).\00", align 1
@str.4 = private unnamed_addr constant [79 x i8] c"CREATE TABLE readings (id INTEGER PRIMARY KEY, sensor_id INTEGER, temp_c REAL)\00", align 1
@str.6 = private unnamed_addr constant [26 x i8] c"Table 'readings' created.\00", align 1
@str.8 = private unnamed_addr constant [55 x i8] c"INSERT INTO readings (sensor_id, temp_c) VALUES (?, ?)\00", align 1
@str.12 = private unnamed_addr constant [14 x i8] c"All readings:\00", align 1
@str.13 = private unnamed_addr constant [55 x i8] c"SELECT id, sensor_id, temp_c FROM readings ORDER BY id\00", align 1
@str.17 = private unnamed_addr constant [32 x i8] c"Average temperature per sensor:\00", align 1
@str.18 = private unnamed_addr constant [92 x i8] c"SELECT sensor_id, AVG(temp_c), COUNT(*) FROM readings GROUP BY sensor_id ORDER BY sensor_id\00", align 1
@fmt.21 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@str.22 = private unnamed_addr constant [6 x i8] c"Done.\00", align 1
@str.24 = private unnamed_addr constant [28 x i8] c"Inserted 5 sensor readings.\00", align 1

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr noundef readonly captures(none)) local_unnamed_addr #0

declare i32 @sqlite3_close(ptr) local_unnamed_addr

declare i32 @sqlite3_open(ptr, ptr) local_unnamed_addr

declare i32 @sqlite3_prepare_v2(ptr, ptr, i32, ptr, ptr) local_unnamed_addr

declare i32 @sqlite3_bind_double(ptr, i32, double) local_unnamed_addr

declare i32 @sqlite3_bind_int(ptr, i32, i32) local_unnamed_addr

declare i32 @sqlite3_step(ptr) local_unnamed_addr

declare i32 @sqlite3_finalize(ptr) local_unnamed_addr

declare i32 @sqlite3_reset(ptr) local_unnamed_addr

define noundef i32 @SQLite3AutoBind_main() local_unnamed_addr {
entry:
  %stmt = alloca ptr, align 8
  %db = alloca ptr, align 8
  %0 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str)
  %snprintf.len = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.21)
  %needed.i64 = sext i32 %snprintf.len to i64
  %alloc.size = add nsw i64 %needed.i64, 1
  %interp.buf = tail call ptr @malloc(i64 %alloc.size)
  %buf.size = add i32 %snprintf.len, 1
  %1 = tail call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf, i32 %buf.size, ptr nonnull @fmt.21)
  %2 = tail call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf)
  store ptr null, ptr %db, align 8
  %3 = call i32 @sqlite3_open(ptr nonnull @str.1, ptr nonnull %db)
  %4 = call i32 @puts(ptr nonnull dereferenceable(1) @str.3)
  store ptr null, ptr %stmt, align 8
  %db3 = load ptr, ptr %db, align 8
  %5 = call i32 @sqlite3_prepare_v2(ptr %db3, ptr nonnull @str.4, i32 -1, ptr nonnull %stmt, ptr null)
  %stmt8 = load ptr, ptr %stmt, align 8
  %6 = call i32 @sqlite3_step(ptr %stmt8)
  %stmt9 = load ptr, ptr %stmt, align 8
  %7 = call i32 @sqlite3_finalize(ptr %stmt9)
  %8 = call i32 @puts(ptr nonnull dereferenceable(1) @str.6)
  %snprintf.len10 = call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.21)
  %needed.i6411 = sext i32 %snprintf.len10 to i64
  %alloc.size12 = add nsw i64 %needed.i6411, 1
  %interp.buf13 = call ptr @malloc(i64 %alloc.size12)
  %buf.size14 = add i32 %snprintf.len10, 1
  %9 = call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf13, i32 %buf.size14, ptr nonnull @fmt.21)
  %10 = call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf13)
  %db15 = load ptr, ptr %db, align 8
  %11 = call i32 @sqlite3_prepare_v2(ptr %db15, ptr nonnull @str.8, i32 -1, ptr nonnull %stmt, ptr null)
  %stmt20 = load ptr, ptr %stmt, align 8
  %12 = call i32 @sqlite3_bind_int(ptr %stmt20, i32 1, i32 1)
  %stmt21 = load ptr, ptr %stmt, align 8
  %13 = call i32 @sqlite3_bind_double(ptr %stmt21, i32 2, double 2.250000e+01)
  %stmt22 = load ptr, ptr %stmt, align 8
  %14 = call i32 @sqlite3_step(ptr %stmt22)
  %stmt23 = load ptr, ptr %stmt, align 8
  %15 = call i32 @sqlite3_reset(ptr %stmt23)
  %stmt24 = load ptr, ptr %stmt, align 8
  %16 = call i32 @sqlite3_bind_int(ptr %stmt24, i32 1, i32 2)
  %stmt25 = load ptr, ptr %stmt, align 8
  %17 = call i32 @sqlite3_bind_double(ptr %stmt25, i32 2, double 1.830000e+01)
  %stmt26 = load ptr, ptr %stmt, align 8
  %18 = call i32 @sqlite3_step(ptr %stmt26)
  %stmt27 = load ptr, ptr %stmt, align 8
  %19 = call i32 @sqlite3_reset(ptr %stmt27)
  %stmt28 = load ptr, ptr %stmt, align 8
  %20 = call i32 @sqlite3_bind_int(ptr %stmt28, i32 1, i32 1)
  %stmt29 = load ptr, ptr %stmt, align 8
  %21 = call i32 @sqlite3_bind_double(ptr %stmt29, i32 2, double 2.310000e+01)
  %stmt30 = load ptr, ptr %stmt, align 8
  %22 = call i32 @sqlite3_step(ptr %stmt30)
  %stmt31 = load ptr, ptr %stmt, align 8
  %23 = call i32 @sqlite3_reset(ptr %stmt31)
  %stmt32 = load ptr, ptr %stmt, align 8
  %24 = call i32 @sqlite3_bind_int(ptr %stmt32, i32 1, i32 3)
  %stmt33 = load ptr, ptr %stmt, align 8
  %25 = call i32 @sqlite3_bind_double(ptr %stmt33, i32 2, double 2.570000e+01)
  %stmt34 = load ptr, ptr %stmt, align 8
  %26 = call i32 @sqlite3_step(ptr %stmt34)
  %stmt35 = load ptr, ptr %stmt, align 8
  %27 = call i32 @sqlite3_reset(ptr %stmt35)
  %stmt36 = load ptr, ptr %stmt, align 8
  %28 = call i32 @sqlite3_bind_int(ptr %stmt36, i32 1, i32 2)
  %stmt37 = load ptr, ptr %stmt, align 8
  %29 = call i32 @sqlite3_bind_double(ptr %stmt37, i32 2, double 1.900000e+01)
  %stmt38 = load ptr, ptr %stmt, align 8
  %30 = call i32 @sqlite3_step(ptr %stmt38)
  %stmt39 = load ptr, ptr %stmt, align 8
  %31 = call i32 @sqlite3_reset(ptr %stmt39)
  %stmt40 = load ptr, ptr %stmt, align 8
  %32 = call i32 @sqlite3_finalize(ptr %stmt40)
  %puts = call i32 @puts(ptr nonnull dereferenceable(1) @str.24)
  %snprintf.len41 = call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.21)
  %needed.i6442 = sext i32 %snprintf.len41 to i64
  %alloc.size43 = add nsw i64 %needed.i6442, 1
  %interp.buf44 = call ptr @malloc(i64 %alloc.size43)
  %buf.size45 = add i32 %snprintf.len41, 1
  %33 = call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf44, i32 %buf.size45, ptr nonnull @fmt.21)
  %34 = call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf44)
  %35 = call i32 @puts(ptr nonnull dereferenceable(1) @str.12)
  %db46 = load ptr, ptr %db, align 8
  %36 = call i32 @sqlite3_prepare_v2(ptr %db46, ptr nonnull @str.13, i32 -1, ptr nonnull %stmt, ptr null)
  %stmt51 = load ptr, ptr %stmt, align 8
  %37 = call i32 @sqlite3_step(ptr %stmt51)
  %stmt58 = load ptr, ptr %stmt, align 8
  %38 = call i32 @sqlite3_finalize(ptr %stmt58)
  %snprintf.len59 = call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.21)
  %needed.i6460 = sext i32 %snprintf.len59 to i64
  %alloc.size61 = add nsw i64 %needed.i6460, 1
  %interp.buf62 = call ptr @malloc(i64 %alloc.size61)
  %buf.size63 = add i32 %snprintf.len59, 1
  %39 = call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf62, i32 %buf.size63, ptr nonnull @fmt.21)
  %40 = call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf62)
  %41 = call i32 @puts(ptr nonnull dereferenceable(1) @str.17)
  %db64 = load ptr, ptr %db, align 8
  %42 = call i32 @sqlite3_prepare_v2(ptr %db64, ptr nonnull @str.18, i32 -1, ptr nonnull %stmt, ptr null)
  %stmt72 = load ptr, ptr %stmt, align 8
  %43 = call i32 @sqlite3_step(ptr %stmt72)
  %stmt80 = load ptr, ptr %stmt, align 8
  %44 = call i32 @sqlite3_finalize(ptr %stmt80)
  %db81 = load ptr, ptr %db, align 8
  %45 = call i32 @sqlite3_close(ptr %db81)
  %snprintf.len82 = call i32 (ptr, i32, ptr, ...) @snprintf(ptr null, i32 0, ptr nonnull @fmt.21)
  %needed.i6483 = sext i32 %snprintf.len82 to i64
  %alloc.size84 = add nsw i64 %needed.i6483, 1
  %interp.buf85 = call ptr @malloc(i64 %alloc.size84)
  %buf.size86 = add i32 %snprintf.len82, 1
  %46 = call i32 (ptr, i32, ptr, ...) @snprintf(ptr %interp.buf85, i32 %buf.size86, ptr nonnull @fmt.21)
  %47 = call i32 @puts(ptr nonnull dereferenceable(1) %interp.buf85)
  %48 = call i32 @puts(ptr nonnull dereferenceable(1) @str.22)
  call void @free(ptr nonnull %interp.buf85)
  call void @free(ptr nonnull %interp.buf62)
  call void @free(ptr nonnull %interp.buf44)
  call void @free(ptr nonnull %interp.buf13)
  call void @free(ptr nonnull %interp.buf)
  ret i32 0
}

declare i32 @snprintf(ptr, i32, ptr, ...) local_unnamed_addr

; Function Attrs: mustprogress nofree nounwind willreturn allockind("alloc,uninitialized") allocsize(0) memory(inaccessiblemem: readwrite)
declare noalias noundef ptr @malloc(i64 noundef) local_unnamed_addr #1

; Function Attrs: mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite)
declare void @free(ptr allocptr noundef captures(none)) local_unnamed_addr #2

define noundef i32 @main() local_unnamed_addr {
entry:
  %0 = tail call i32 @SQLite3AutoBind_main()
  ret i32 0
}

attributes #0 = { nofree nounwind }
attributes #1 = { mustprogress nofree nounwind willreturn allockind("alloc,uninitialized") allocsize(0) memory(inaccessiblemem: readwrite) "alloc-family"="malloc" }
attributes #2 = { mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite) "alloc-family"="malloc" }
