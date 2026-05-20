This Content provides guidance to Ai when working with code in this repository.

## Build & Test Commands

```bash
# Configure and build (from repo root)
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run all tests
ctest --test-dir build --output-on-failure

# Run a single test by name
ctest --test-dir build -R <test_name> --output-on-failure
# e.g.: ctest --test-dir build -R TESTUTFX.utf8_and_utf16le_win_format

# Run with a specific compiler
cmake -B build -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

To skip building tests: `cmake -B build -DUTFX_BUILD_TESTS=OFF`

## Cross-Compilation

> **Note:** Cross-compilation environment is currently only available on macOS and Linux platforms.

When directories matching `build/{platform}-{arch}` exist (e.g., `build/darwin-arm64`, `build/linux-x86_64`, `build/mingw64-x86_64`, `build/windows-arm64`), the cross-compilation environment is already configured. Build directly with:

```bash
cmake --build build/{platform}-{arch}
```

Supported build directories:

| Directory              | Target                 |
| ---------------------- | ---------------------- |
| `build/darwin-arm64`   | macOS ARM64            |
| `build/darwin-x86_64`  | macOS x86_64           |
| `build/linux-arm64`    | Linux ARM64            |
| `build/linux-x86_64`   | Linux x86_64           |
| `build/mingw64-x86_64` | Windows x86_64 (MinGW) |
| `build/windows-arm64`  | Windows ARM64 (MSVC)   |
| `build/windows-x86_64` | Windows x86_64 (MSVC)  |

When source files are added/removed or CMake options change (making it necessary to re-run CMake configuration), simply `touch CMakeLists.txt` and the next `cmake --build` will automatically re-generate the build system:

```bash
touch CMakeLists.txt
cmake --build build/{platform}-{arch}
```

## Architecture

This is a **single-header** C++ library (`include/utfx/utfx.hpp`) for UTF-8, UTF-16, and UTF-32 encoding, decoding, and transcoding with big/little endian support.

### Core Design: `utfx::detail::utf_traits`

The central abstraction is `utf_traits<CharT, sizeof(CharT)>`, specialized by character width:

- **1 byte** → UTF-8 implementation. Implements RFC 3629 4-byte encoding, trail byte detection, overlong-sequence rejection, and surrogate rejection. Key statics: `trail_length()`, `decode()`, `decode_valid()`, `encode()`, `width()`, `is_trail()`, `is_lead()`.
- **2 bytes** → UTF-16 implementation per RFC 2781. Handles surrogate pairs (`0xD800-0xDBFF` / `0xDC00-0xDFFF`). All decode/encode functions take an optional `utfx::endian` parameter.
- **4 bytes** → UTF-32 implementation. Validates codepoints, supports endian swapping.

All functions are `constexpr` and `noexcept` where possible.

### Public API (`utfx` namespace)

- **`transcode()`** — Overloaded family of functions. Two categories separated by SFINAE:
  - When one type is 1-byte (UTF-8) and the other isn't: takes a single `endian` for the non-UTF-8 side.
  - When neither type is 1-byte (UTF-16 ↔ UTF-32): takes both a `from` and `to` endian.
  - Returns `size_t` (raw pointer output) or `std::basic_string<CharOut>` (string output). Passing `nullptr` as output calculates required length without writing.

- **`utf8_to_utf16()` / `utf16_to_utf8()`** — Convenience wrappers. On Windows (`_WIN32`), use `wchar_t` by default; on other platforms, use `char16_t`. Accept `std::string`/`std::wstring`/`std::u16string` or raw C-string pointers.

- **`is_utf8()` / `is_utf16()`** — Validation functions that scan the entire input. `is_utf8` skips a leading BOM (`EF BB BF`). `is_utf16` handles BOM-aware endian detection (checks `FF FE` / `FE FF` and adjusts the start pointer).

- **`utfx::literals`** — User-defined string literals: `"..."_utf8` (from `char16_t*`), `"..."_utf16` (from `char*` and optionally `char8_t*`). On Windows, `_utf8` also accepts `wchar_t*`.

- **`utfx::endian`** — Enum with `little`, `big`, `native`. Uses compiler built-ins (`__BYTE_ORDER__`) or MSVC fallback (always little-endian).

### Illegal/Incomplete sentinels

`detail::illegal` (`0xFFFFFFFF`) and `detail::incomplete` (`0xFFFFFFFE`) are returned by `decode()` for invalid sequences and truncated input respectively. Current `transcode()` silently skips these (commented-out `throw` statements exist).

### Tests

Located in `tests/`. Uses Google Test (fetched via CMake FetchContent at v1.15.2). Each `.cc` file produces a separate test executable, plus an `all_test` executable that combines all of them. Tests are compiled with C++17, `-Wall -Wextra -Werror` (GCC/Clang) or `/W4` (MSVC).

`emoji_sequences.h` is an auto-generated file from Unicode emoji sequence data used for round-trip testing of complex multi-codepoint emoji (including ZWJ sequences and skin-tone modifiers).

## Code Style

- Based on Google style (`.clang-format`), with `InsertBraces: true` and `InsertNewlineAtEOF: true`.
- C++ standard: header targets C++11; tests require C++17; `.clangd` uses `-std=c++20` for IDE diagnostics.
- The include guard style is `__UTFX_UTFX_HPP__`.
- `constexpr inline` is used extensively in the detail namespace.
- Uses compiler intrinsics for byte swapping: `__builtin_bswap*` (GCC/Clang), `_byteswap_*` (MSVC), or manual fallback.

clang-format (Google style) is configured. A pre-commit hook auto-formats staged C/C++ and CMake files. The repo enforces `-Wall -Wextra -Werror` (or `/W4 /WX` for MSVC).

## CI

## CI

GitHub Actions run on push/PR to `main`:

- **cmake-multi-platform.yml**: Matrix of ubuntu/windows/macos × Release/Debug × gcc/clang/cl.
- **msys2.yml**: Windows MSYS2 environments (MSYS, UCRT64, MINGW64) × gcc/clang × Release/Debug with Ninja generator.

If the remote repository URL is `http://github.com/*` or `git@github.com:*`, then GitHub Actions (configuration files located at `.github/workflows/*.yml`) is used as CI.

Browse and manage GitHub Actions via the `gh` command:

```bash
# View recent workflow runs
gh run list

# View details of a specific run
gh run view <run-id>

# View logs of a specific run
gh run view <run-id> --log

# Manually trigger a workflow
gh workflow run <workflow-name>

# List all workflows
gh workflow list

# View workflow file contents
gh workflow view <workflow-name>
```

**When the user wants to resolve GitHub Actions failures, they should first use `gh` commands to get logs and analyze the problem**, rather than blindly guessing the cause. How to get logs:

```bash
# Get logs of the latest run (usually the failed run)
gh run list --limit 1 --json databaseId -q '.[].databaseId' | xargs gh run view --log

# Get logs of a specific run-id (including failed steps)
gh run view <run-id> --log

# Get logs of the run corresponding to a specific commit
gh run list --commit <commit-sha> --limit 1 --json databaseId -q '.[].databaseId' | xargs gh run view --log

# Only view failed runs
gh run list --status failure --limit 5

# View logs of a failed job in a run (if the run has multiple jobs)
gh run view <run-id> --log --job <job-id>
```

After getting the logs, identify the specific failure cause based on the error messages in the logs (compilation errors, test failures, environment issues, etc.), then make targeted fixes.

**Fix Verification**: If the failure occurs on a platform different from the current machine (e.g., currently on macOS ARM64, but the failure is on Linux x86_64), and a corresponding cross-compilation environment `build/{platform}-{arch}/` exists locally, after fixing, you **must** verify the build through that cross-compilation environment to ensure the fix also passes on the target platform:

```bash
# For example: fixed a compilation error on Linux x86_64, with build/linux-x86_64/ available locally
cmake --build build/linux-x86_64
```

If the corresponding cross-compilation environment does not exist locally, just push the fix directly and let CI verify.
