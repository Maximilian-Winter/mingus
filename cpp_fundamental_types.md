# C++ Fundamental Types — Summary

*Based on [cppreference.com — Fundamental types](https://en.cppreference.com/w/cpp/language/types.html)*

---

## 1. `void`

- Type with an empty set of values (incomplete type, cannot be completed).
- No objects, arrays, or references of type `void`.
- Pointers to `void` and functions returning `void` are permitted.

## 2. `std::nullptr_t` (since C++11)

- Type of the null pointer literal `nullptr`.
- Distinct type — not a pointer type itself.
- `sizeof(std::nullptr_t) == sizeof(void*)`.

---

## 3. Integral Types

### 3.1 Standard Integer Types

| Type Specifier | Equivalent Type | Min Width (Standard) | LP32 | ILP32 | LLP64 | LP64 |
|---|---|---|---|---|---|---|
| `signed char` | `signed char` | ≥ 8 bits | 8 | 8 | 8 | 8 |
| `unsigned char` | `unsigned char` | ≥ 8 bits | 8 | 8 | 8 | 8 |
| `short` / `short int` / `signed short` | `short int` | ≥ 16 bits | 16 | 16 | 16 | 16 |
| `unsigned short` | `unsigned short int` | ≥ 16 bits | 16 | 16 | 16 | 16 |
| `int` / `signed` / `signed int` | `int` | ≥ 16 bits | 16 | 32 | 32 | 32 |
| `unsigned` / `unsigned int` | `unsigned int` | ≥ 16 bits | 16 | 32 | 32 | 32 |
| `long` / `long int` / `signed long` | `long int` | ≥ 32 bits | 32 | 32 | 32 | 64 |
| `unsigned long` | `unsigned long int` | ≥ 32 bits | 32 | 32 | 32 | 64 |
| `long long` / `signed long long` | `long long int` (C++11) | ≥ 64 bits | 64 | 64 | 64 | 64 |
| `unsigned long long` | `unsigned long long int` (C++11) | ≥ 64 bits | 64 | 64 | 64 | 64 |

**Size guarantee:** `1 == sizeof(char) ≤ sizeof(short) ≤ sizeof(int) ≤ sizeof(long) ≤ sizeof(long long)`

### 3.2 Boolean Type

- `bool` — holds `true` or `false`. `sizeof(bool)` is implementation-defined (may differ from 1).

### 3.3 Character Types

| Type | Description | Width |
|---|---|---|
| `char` | Character representation; same as `signed char` or `unsigned char` (implementation-defined), but always a distinct type. | 8 bits |
| `signed char` | Signed character representation. | 8 bits |
| `unsigned char` | Unsigned character representation. Also used for raw memory inspection. | 8 bits |
| `wchar_t` | Wide character. 32-bit UTF-32 on Linux/macOS; 16-bit UTF-16 on Windows. | Platform-dependent |
| `char8_t` (C++20) | UTF-8 code unit. Same size/signedness as `unsigned char`. | 8 bits |
| `char16_t` (C++11) | UTF-16 code unit. Same size as `std::uint_least16_t`. | ≥ 16 bits |
| `char32_t` (C++11) | UTF-32 code unit. Same size as `std::uint_least32_t`. | ≥ 32 bits |

---

## 4. Floating-Point Types

### 4.1 Standard Floating-Point Types

| Type | Typical Format | Precision |
|---|---|---|
| `float` | IEEE-754 binary32 | Single precision |
| `double` | IEEE-754 binary64 | Double precision |
| `long double` | Varies: x87 80-bit, IEEE-754 binary128, or double-double | Extended precision |

### 4.2 Special Values (if supported)

- **±Infinity** — `INFINITY`
- **Negative zero** (`-0.0`) — compares equal to `+0.0` but differs in some operations
- **NaN** (Not-a-Number) — does not compare equal to anything, including itself

---

## 5. Range of Values

> **Note:** As of C++20, two's complement is the only allowed signed integer representation.
> Signed N-bit range: **−2^(N−1)** to **+2^(N−1) − 1**

### 5.1 Integer Ranges

| Bits | Signedness | Exact Range |
|---|---|---|
| 8 | signed | −128 to 127 |
| 8 | unsigned | 0 to 255 |
| 16 | signed | −32,768 to 32,767 |
| 16 | unsigned | 0 to 65,535 |
| 32 | signed | −2,147,483,648 to 2,147,483,647 |
| 32 | unsigned | 0 to 4,294,967,295 |
| 64 | signed | −9,223,372,036,854,775,808 to 9,223,372,036,854,775,807 |
| 64 | unsigned | 0 to 18,446,744,073,709,551,615 |

### 5.2 Floating-Point Ranges

| Bits | Format | Min Subnormal | Min Normal | Max |
|---|---|---|---|---|
| 32 | IEEE-754 binary32 | ± 1.4013 × 10⁻⁴⁵ | ± 1.1755 × 10⁻³⁸ | ± 3.4028 × 10³⁸ |
| 64 | IEEE-754 binary64 | ± 4.9407 × 10⁻³²⁴ | ± 2.2251 × 10⁻³⁰⁸ | ± 1.7977 × 10³⁰⁸ |
| 80 | x86 extended | ± 3.6452 × 10⁻⁴⁹⁵¹ | ± 3.3621 × 10⁻⁴⁹³² | ± 1.1897 × 10⁴⁹³² |
| 128 | IEEE-754 binary128 | ± 6.4752 × 10⁻⁴⁹⁶⁶ | ± 3.3621 × 10⁻⁴⁹³² | ± 1.1897 × 10⁴⁹³² |

> **Note:** The 80-bit type object representation typically occupies 96 bits (32-bit platforms) or 128 bits (64-bit platforms).

---

## 6. Data Models

| Model | int | long | pointer | Used by |
|---|---|---|---|---|
| **LP32** (2/4/4) | 16 | 32 | 32 | Win16 API |
| **ILP32** (4/4/4) | 32 | 32 | 32 | Win32, Unix/Linux/macOS (32-bit) |
| **LLP64** (4/4/8) | 32 | 32 | 64 | Win64 (x86-64, AArch64) |
| **LP64** (4/8/8) | 32 | 64 | 64 | Unix/Linux/macOS (64-bit) |

---

## 7. Keywords

`void`, `bool`, `true`, `false`, `char`, `char8_t`, `char16_t`, `char32_t`, `wchar_t`,
`int`, `short`, `long`, `signed`, `unsigned`, `float`, `double`
