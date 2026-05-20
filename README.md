# utfx.hpp

A modern, **single-header** C++ library for encoding, decoding, and transcoding between UTF-8, UTF-16, and UTF-32 — with full big/little endian support.

[![CMake Multi-Platform](https://github.com/shediao/utfx.hpp/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/shediao/utfx.hpp/actions/workflows/cmake-multi-platform.yml)
[![MSYS2](https://github.com/shediao/utfx.hpp/actions/workflows/msys2.yml/badge.svg)](https://github.com/shediao/utfx.hpp/actions/workflows/msys2.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

## Features

- ✅ **Header-only** — drop `utfx.hpp` into your project and go.
- ✅ **C++11** compatible, with extensive use of `constexpr` and `noexcept`.
- ✅ UTF-8, UTF-16, and UTF-32 **encoding / decoding / transcoding**.
- ✅ Big-endian and little-endian support for UTF-16 and UTF-32.
- ✅ RFC 3629 (UTF-8) and RFC 2781 (UTF-16) compliant.
- ✅ Rejects overlong sequences, surrogates in UTF-8, and out-of-range codepoints.
- ✅ **`utf8_view`** — a `std::string_view`-style view that iterates over Unicode _code points_.
- ✅ **`utf8_char`** — a lightweight value type representing a single UTF-8 code point.
- ✅ Convenience functions: `utf8_to_utf16()`, `utf16_to_utf8()`, `is_utf8()`, `is_utf16()`.
- ✅ User-defined string literals: `"..."_utf8`, `"..."_utf16`.
- ✅ CMake integration via `FetchContent` or `find_package`.
- ✅ Cross-platform: Linux, macOS, Windows (MSVC, MinGW, MSYS2, WSL).
- ✅ Tested with complex emoji sequences (ZWJ, skin-tone modifiers, flags).

## Quick Start

### 1. Copy the header

```bash
cp include/utfx/utfx.hpp /your/project/third_party/
```

### 2. Include and use

```cpp
#include "utfx.hpp"

// UTF-8 → UTF-16
std::string utf8_str = "Hello, 世界 🌍";
std::u16string utf16_str = utfx::utf8_to_utf16(utf8_str);

// UTF-16 → UTF-8
std::string back_to_utf8 = utfx::utf16_to_utf8(utf16_str);

// Generic transcoding
std::u32string utf32_str = utfx::transcode<char32_t>(
    utf8_str.data(), utf8_str.data() + utf8_str.size(), utfx::endian::native);

// Validation
bool valid = utfx::is_utf8(utf8_str.data(), utf8_str.size());

// User-defined literals (C++14+)
using namespace utfx::literals;
auto u8  = u"Hello, 世界"_utf8;    // char16_t* → std::string (UTF-8)
auto u16 = "Hello, 世界"_utf16;    // char*     → std::u16string
```

### 3. Using `utf8_view`

```cpp
std::string text = "Café 🍕";
utfx::utf8_view view(text);

// Iterate over code points
for (utfx::utf8_char ch : view) {
    std::cout << "Codepoint: U+" << std::hex << ch.code_point() << "\n";
}

std::cout << "Code points: " << view.size() << "\n";   // 6
std::cout << "Bytes: "       << view.byte_size() << "\n";  // 10

// Substring by code points (not bytes!)
utfx::utf8_view sub = view.substr(0, 4);  // "Café"
```

## Transcoding API

### Generic: `utfx::transcode()`

| From   | To     | Signature                                                      |
| ------ | ------ | -------------------------------------------------------------- |
| UTF-8  | UTF-16 | `transcode<CharOut>(in_begin, in_end, endian)`                 |
| UTF-8  | UTF-32 | `transcode<CharOut>(in_begin, in_end, endian)`                 |
| UTF-16 | UTF-8  | `transcode<CharOut>(in_begin, in_end, endian)`                 |
| UTF-16 | UTF-32 | `transcode<CharOut>(in_begin, in_end, from_endian, to_endian)` |
| UTF-32 | UTF-8  | `transcode<CharOut>(in_begin, in_end, endian)`                 |
| UTF-32 | UTF-16 | `transcode<CharOut>(in_begin, in_end, from_endian, to_endian)` |

Returns a `std::basic_string<CharOut>`; or pass an output pointer to get the count written.

### Convenience

```cpp
// On Windows, these use wchar_t; elsewhere they use char16_t
auto u16 = utfx::utf8_to_utf16(utf8_str);          // std::string → std::wstring / std::u16string
auto u8  = utfx::utf16_to_utf8(u16_str);           // std::wstring / std::u16string → std::string
```

### Validation

```cpp
bool ok = utfx::is_utf8(data, length);
bool ok = utfx::is_utf16(data, length, utfx::endian::native);  // BOM-aware
```

## CMake Integration

### Via `FetchContent`

```cmake
include(FetchContent)
FetchContent_Declare(
    utfx
    GIT_REPOSITORY https://github.com/shediao/utfx.hpp.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(utfx)

target_link_libraries(your_target PRIVATE utfx::utfx)
```

### Via `find_package` (after installation)

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build --target install
```

```cmake
find_package(utfx REQUIRED)
target_link_libraries(your_target PRIVATE utfx::utfx)
```

## Building & Testing

```bash
# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run all tests
ctest --test-dir build --output-on-failure

# Run a single test
ctest --test-dir build -R emoji_test --output-on-failure
```

To skip building tests:

```bash
cmake -B build -DUTFX_BUILD_TESTS=OFF
```

## Cross-Compilation

Pre-configured build directories are provided for cross-compilation:

| Directory              | Target                 |
| ---------------------- | ---------------------- |
| `build/darwin-arm64`   | macOS ARM64            |
| `build/darwin-x86_64`  | macOS x86_64           |
| `build/linux-arm64`    | Linux ARM64            |
| `build/linux-x86_64`   | Linux x86_64           |
| `build/mingw64-x86_64` | Windows x86_64 (MinGW) |
| `build/windows-arm64`  | Windows ARM64 (MSVC)   |
| `build/windows-x86_64` | Windows x86_64 (MSVC)  |

```bash
cmake --build build/linux-x86_64
```

## Requirements

- **C++11** or later to use the header.
- **C++17** to build and run the test suite.
- CMake 3.15+ for the build system.

## API Overview

### Classes

| Class             | Description                                                                                               |
| ----------------- | --------------------------------------------------------------------------------------------------------- |
| `utfx::utf8_view` | A read-only view over UTF-8 text. Iterates over code points (`utf8_char`). Similar to `std::string_view`. |
| `utfx::utf8_char` | A single UTF-8 code point (1–4 bytes), referencing the underlying string.                                 |

### Enums

| Enum           | Description                                                    |
| -------------- | -------------------------------------------------------------- |
| `utfx::endian` | `little`, `big`, `native` — used for UTF-16/UTF-32 endianness. |

### Free Functions

| Function                               | Description                        |
| -------------------------------------- | ---------------------------------- |
| `utfx::transcode<To>(begin, end, ...)` | Transcode between UTF-8/16/32.     |
| `utfx::utf8_to_utf16(str)`             | Convenience: UTF-8 → UTF-16.       |
| `utfx::utf16_to_utf8(str)`             | Convenience: UTF-16 → UTF-8.       |
| `utfx::is_utf8(data, len)`             | Validate UTF-8. Skips leading BOM. |
| `utfx::is_utf16(data, len, endian)`    | Validate UTF-16. BOM-aware.        |

### Literals (namespace `utfx::literals`)

| Literal         | Input             | Output                       |
| --------------- | ----------------- | ---------------------------- |
| `u"..."_utf8`   | `const char16_t*` | `std::string` (UTF-8)        |
| `u8"..."_utf16` | `const char8_t*`  | `std::u16string`             |
| `"..."_utf16`   | `const char*`     | `std::u16string`             |
| `L"..."_utf8`   | `const wchar_t*`  | `std::string` (Windows only) |

## Standards Compliance

- **RFC 3629** — UTF-8: 1–4 byte encoding, trail byte detection, overlong-sequence rejection, surrogate rejection, codepoint range validation (`U+0000`–`U+10FFFF`).
- **RFC 2781** — UTF-16: surrogate pair handling (`0xD800`–`0xDFFF`), endian-aware decoding.

## License

MIT © 2025 [shediao.xsd](https://github.com/shediao)

---

[中文文档](README-zh_CN.md)
