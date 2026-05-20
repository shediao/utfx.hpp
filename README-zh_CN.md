# utfx.hpp

一个现代化的**单头文件** C++ 库，用于 UTF-8、UTF-16、UTF-32 之间的编码、解码与转码，完整支持大端/小端字节序。

[![CMake Multi-Platform](https://github.com/shediao/utfx.hpp/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/shediao/utfx.hpp/actions/workflows/cmake-multi-platform.yml)
[![MSYS2](https://github.com/shediao/utfx.hpp/actions/workflows/msys2.yml/badge.svg)](https://github.com/shediao/utfx.hpp/actions/workflows/msys2.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

## 特性

- ✅ **单头文件** — 将 `utfx.hpp` 放入项目即可使用。
- ✅ **兼容 C++11**，大量使用 `constexpr` 和 `noexcept`。
- ✅ UTF-8、UTF-16、UTF-32 的**编码 / 解码 / 转码**。
- ✅ 支持 UTF-16 和 UTF-32 的大端和小端字节序。
- ✅ 符合 RFC 3629 (UTF-8) 和 RFC 2781 (UTF-16) 标准。
- ✅ 拒绝过长序列、UTF-8 中的代理对以及超出范围的码点。
- ✅ **`utf8_view`** — 类似 `std::string_view` 的视图，按 Unicode _码点_ 迭代。
- ✅ **`utf8_char`** — 轻量级值类型，表示单个 UTF-8 码点。
- ✅ 便捷函数：`utf8_to_utf16()`、`utf16_to_utf8()`、`is_utf8()`、`is_utf16()`。
- ✅ 用户自定义字面量：`"..."_utf8`、`"..."_utf16`。
- ✅ 支持 CMake 集成（`FetchContent` 或 `find_package`）。
- ✅ 跨平台：Linux、macOS、Windows（MSVC、MinGW、MSYS2、WSL）。
- ✅ 经过复杂 emoji 序列测试（ZWJ、肤色修饰符、旗帜等）。

## 快速开始

### 1. 复制头文件

```bash
cp include/utfx/utfx.hpp /your/project/third_party/
```

### 2. 引入并使用

```cpp
#include "utfx.hpp"

// UTF-8 → UTF-16
std::string utf8_str = "Hello, 世界 🌍";
std::u16string utf16_str = utfx::utf8_to_utf16(utf8_str);

// UTF-16 → UTF-8
std::string back_to_utf8 = utfx::utf16_to_utf8(utf16_str);

// 通用转码
std::u32string utf32_str = utfx::transcode<char32_t>(
    utf8_str.data(), utf8_str.data() + utf8_str.size(), utfx::endian::native);

// 验证
bool valid = utfx::is_utf8(utf8_str.data(), utf8_str.size());

// 用户自定义字面量（C++14+）
using namespace utfx::literals;
auto u8  = u"Hello, 世界"_utf8;    // char16_t* → std::string (UTF-8)
auto u16 = "Hello, 世界"_utf16;    // char*     → std::u16string
```

### 3. 使用 `utf8_view`

```cpp
std::string text = "Café 🍕";
utfx::utf8_view view(text);

// 按码点迭代
for (utfx::utf8_char ch : view) {
    std::cout << "码点: U+" << std::hex << ch.code_point() << "\n";
}

std::cout << "码点数: " << view.size() << "\n";      // 6
std::cout << "字节数: " << view.byte_size() << "\n"; // 10

// 按码点（而非字节）取子串
utfx::utf8_view sub = view.substr(0, 4);  // "Café"
```

## 转码 API

### 通用函数：`utfx::transcode()`

| 源编码 | 目标编码 | 签名                                                           |
| ------ | -------- | -------------------------------------------------------------- |
| UTF-8  | UTF-16   | `transcode<CharOut>(in_begin, in_end, endian)`                 |
| UTF-8  | UTF-32   | `transcode<CharOut>(in_begin, in_end, endian)`                 |
| UTF-16 | UTF-8    | `transcode<CharOut>(in_begin, in_end, endian)`                 |
| UTF-16 | UTF-32   | `transcode<CharOut>(in_begin, in_end, from_endian, to_endian)` |
| UTF-32 | UTF-8    | `transcode<CharOut>(in_begin, in_end, endian)`                 |
| UTF-32 | UTF-16   | `transcode<CharOut>(in_begin, in_end, from_endian, to_endian)` |

返回 `std::basic_string<CharOut>`；也可传入输出指针获取写入数量。

### 便捷函数

```cpp
// Windows 上使用 wchar_t；其他平台使用 char16_t
auto u16 = utfx::utf8_to_utf16(utf8_str);          // std::string → std::wstring / std::u16string
auto u8  = utfx::utf16_to_utf8(u16_str);           // std::wstring / std::u16string → std::string
```

### 验证函数

```cpp
bool ok = utfx::is_utf8(data, length);
bool ok = utfx::is_utf16(data, length, utfx::endian::native);  // 支持 BOM 检测
```

## CMake 集成

### 通过 `FetchContent`

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

### 通过 `find_package`（安装后）

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build --target install
```

```cmake
find_package(utfx REQUIRED)
target_link_libraries(your_target PRIVATE utfx::utfx)
```

## 构建与测试

```bash
# 配置并构建
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# 运行所有测试
ctest --test-dir build --output-on-failure

# 运行单个测试
ctest --test-dir build -R emoji_test --output-on-failure
```

跳过测试构建：

```bash
cmake -B build -DUTFX_BUILD_TESTS=OFF
```

## 交叉编译

项目提供了预配置的交叉编译目录：

| 目录                   | 目标平台               |
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

## 依赖要求

- **C++11** 或更高版本以使用头文件。
- **C++17** 以构建和运行测试套件。
- CMake 3.15+ 用于构建系统。

## API 概览

### 类

| 类                | 说明                                                                       |
| ----------------- | -------------------------------------------------------------------------- |
| `utfx::utf8_view` | UTF-8 文本的只读视图。按码点（`utf8_char`）迭代，类似 `std::string_view`。 |
| `utfx::utf8_char` | 单个 UTF-8 码点（1–4 字节），引用底层字符串。                              |

### 枚举

| 枚举           | 说明                                                          |
| -------------- | ------------------------------------------------------------- |
| `utfx::endian` | `little`、`big`、`native` — 用于指定 UTF-16/UTF-32 的字节序。 |

### 自由函数

| 函数                                   | 说明                           |
| -------------------------------------- | ------------------------------ |
| `utfx::transcode<To>(begin, end, ...)` | 在 UTF-8/16/32 之间转码。      |
| `utfx::utf8_to_utf16(str)`             | 便捷函数：UTF-8 → UTF-16。     |
| `utfx::utf16_to_utf8(str)`             | 便捷函数：UTF-16 → UTF-8。     |
| `utfx::is_utf8(data, len)`             | 验证 UTF-8，自动跳过前导 BOM。 |
| `utfx::is_utf16(data, len, endian)`    | 验证 UTF-16，支持 BOM 检测。   |

### 字面量（命名空间 `utfx::literals`）

| 字面量          | 输入类型          | 输出类型                    |
| --------------- | ----------------- | --------------------------- |
| `u"..."_utf8`   | `const char16_t*` | `std::string` (UTF-8)       |
| `u8"..."_utf16` | `const char8_t*`  | `std::u16string`            |
| `"..."_utf16`   | `const char*`     | `std::u16string`            |
| `L"..."_utf8`   | `const wchar_t*`  | `std::string`（仅 Windows） |

## 标准合规

- **RFC 3629** — UTF-8：1–4 字节编码，尾随字节检测，过长序列拒绝，代理对拒绝，码点范围验证（`U+0000`–`U+10FFFF`）。
- **RFC 2781** — UTF-16：代理对处理（`0xD800`–`0xDFFF`），端序感知解码。

## 许可证

MIT © 2025 [shediao.xsd](https://github.com/shediao)

---

[English Documentation](README.md)
