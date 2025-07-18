#ifndef __UTFX_UTFX_HPP__
#define __UTFX_UTFX_HPP__
#include <cstdint>
#include <string>

namespace utfx {

enum class endian {
#if defined(_MSC_VER) && !defined(__clang__)
  little = 1234,
  big = 4321,
  native = little
#else
  little = __ORDER_LITTLE_ENDIAN__,
  big = __ORDER_BIG_ENDIAN__,
  native = __BYTE_ORDER__
#endif
};
namespace detail {
using codepoint = uint32_t;
constexpr inline codepoint illegal = 0xFFFFFFFFu;
constexpr inline codepoint incomplete = 0xFFFFFFFEu;

constexpr inline bool is_valid_codepoint(codepoint v) noexcept {
  // RFC3269
  if (v > 0x10FFFF) {
    return false;
  }
  if (0xD800 <= v && v <= 0xDFFF) {
    // RFC2781 surrogates
    return false;
  }
  return true;
}

template <typename T>
constexpr T swap_bytes(T x) noexcept {
  if constexpr (sizeof(T) == 2) {
#if defined(__GNUC__)
    return __builtin_bswap16(x);
#elif defined(_MSC_VER)
    return _byteswap_ushort(x);
#else
    uint16_t v = static_cast<uint16_t>(x);
    v = (v << 8) | (v >> 8);
    return static_cast<T>(v);
#endif
  }
  if constexpr (sizeof(T) == 4) {
#if defined(__GNUC__)
    return __builtin_bswap32(x);
#elif defined(_MSC_VER)
    return _byteswap_ulong(x);
#else
    uint32_t v = static_cast<uint32_t>(x);
    v = ((v & 0x000000FF) << 24) | ((v & 0x0000FF00) << 8) |
        ((v & 0x00FF0000) >> 8) | ((v & 0xFF000000) >> 24);
    return static_cast<T>(v);
#endif
  }
  if constexpr (sizeof(T) == 8) {
#if defined(__GNUC__)
    return __builtin_bswap64(x);
#elif defined(_MSC_VER)
    return _byteswap_uint64(x);
#else
    uint64_t v = static_cast<uint64_t>(x);
    return (((v & UINT64_C(0x00000000000000FF)) << 56) |
            ((v & UINT64_C(0x000000000000FF00)) << 40) |
            ((v & UINT64_C(0x0000000000FF0000)) << 24) |
            ((v & UINT64_C(0x00000000FF000000)) << 8) |
            ((v & UINT64_C(0x000000FF00000000)) >> 8) |
            ((v & UINT64_C(0x0000FF0000000000)) >> 24) |
            ((v & UINT64_C(0x00FF000000000000)) >> 40) |
            ((v & UINT64_C(0xFF00000000000000)) >> 56));
#endif
  }
}

template <typename CharT, size_t = sizeof(CharT)>
struct utf_traits;

// utf8
// Char. number range  |        UTF-8 octet sequence
//    (hexadecimal)    |              (binary)
// --------------------+---------------------------------------------
// 0000 0000-0000 007F | 0xxxxxxx
// 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
// 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
// 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

template <typename CharT>
struct utf_traits<CharT, 1> {
  using char_type = CharT;
  constexpr static size_t max_width{4};

  constexpr static int trail_length(char_type v) noexcept {
    unsigned char c = v;
    if (c < 128) {
      // 0x00~0x7f
      return 0;
    } else if (c < 194) {
      // (128) 10000000,00000000 - 10ffffff,ffffffff => not lead char
      // (192) 11000000,10000000 - 11000000,10111111 => 0x00~0xff (ascii)
      // (193) 11000001,10000000 - 11000001,10111111 => 0x40~0x7f (ascii)
      return -1;
    } else if (c < 224) {
      // 110xxxxx 10xxxxxx
      // 0xc000~0xdffff
      return 1;
    } else if (c < 240) {
      // 1110xxxx 10xxxxxx 10xxxxxx
      // [11100000~11110000)
      return 2;
    } else if (c <= 244) {
      // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
      // RFC3269 [0x0000~0x10FFFF]
      return 3;
    }
    return -1;
  }

  constexpr static int width(codepoint v) noexcept {
    if (v < 0x80) {
      return 1;
    } else if (v < 0x800) {
      return 2;
    } else if (v < 0x10000) {
      return 3;
    }
    return 4;
  }

  constexpr static bool is_trail(char_type v) noexcept {
    unsigned char c = v;
    return (c & 0b11000000u) == 0b10000000u;
  }
  constexpr static bool is_lead(char_type v) { return !is_trail(v); }

  template <typename Iterator>
  constexpr static codepoint decode(Iterator& p, Iterator e) noexcept {
    if (p == e) {
      return incomplete;
    }
    unsigned char first = *p++;
    auto const trail_len = trail_length(first);
    if (trail_len < 0) {
      return illegal;
    }

    if (trail_len == 0) {
      return first;
    }

    codepoint c = first & ((1 << (6 - trail_len)) - 1);
    unsigned char tmp{0};
    switch (trail_len) {
      case 3:
        if (p == e) {
          return incomplete;
        }
        tmp = *p++;
        if (!trail_length(tmp)) {
          return illegal;
        }
        c = ((c << 6) | (tmp & 0x3F));
        [[fallthrough]];
      case 2:
        if (p == e) {
          return incomplete;
        }
        tmp = *p++;
        if (!trail_length(tmp)) {
          return illegal;
        }
        c = ((c << 6) | (tmp & 0x3F));
        [[fallthrough]];
      case 1:
        if (p == e) {
          return incomplete;
        }
        tmp = *p++;
        if (!trail_length(tmp)) {
          return illegal;
        }
        c = ((c << 6) | (tmp & 0x3F));
    }
    if (!is_valid_codepoint(c)) {
      return illegal;
    }
    if (width(c) != trail_len + 1) {
      return illegal;
    }
    return c;
  }

  template <typename Iterator>
  constexpr static codepoint decode_valid(Iterator& p) noexcept {
    unsigned char first = *p++;
    if (first < 192) {
      return first;
    }

    int trail_len = -1;
    if (first < 224) {
      trail_len = 1;
    } else if (first < 240) {
      trail_len = 2;
    } else {
      trail_len = 3;
    }
    codepoint c = first & ((1 << (6 - trail_len)) - 1);
    switch (trail_len) {
      case 3:
        c = ((c << 6) | (0x3f & static_cast<unsigned char>(*p++)));
        [[fallthrough]];
      case 2:
        c = ((c << 6) | (0x3f & static_cast<unsigned char>(*p++)));
        [[fallthrough]];
      case 1:
        c = ((c << 6) | (0x3f & static_cast<unsigned char>(*p++)));
    }
    return c;
  }

  template <typename Iterator>
  constexpr static Iterator encode(codepoint c, Iterator out) noexcept {
    if (c <= 0x7F) {
      *out++ = static_cast<char_type>(c);
    } else if (c <= 0x7FF) {
      *out++ = static_cast<char_type>((c >> 6) | 0b11000000u);
      *out++ = static_cast<char_type>((c & 0x3F) | 0b10000000u);
    } else if (c <= 0xFFFF) {
      *out++ = static_cast<char_type>((c >> 12) | 0b11100000u);
      *out++ = static_cast<char_type>(((c >> 6) & 0x3F) | 0b10000000u);
      *out++ = static_cast<char_type>((c & 0x3F) | 0b10000000u);
    } else {
      *out++ = static_cast<char_type>((c >> 18) | 0b11110000u);
      *out++ = static_cast<char_type>(((c >> 12) & 0x3F) | 0b10000000u);
      *out++ = static_cast<char_type>(((c >> 6) & 0x3F) | 0b10000000u);
      *out++ = static_cast<char_type>((c & 0x3F) | 0b10000000u);
    }
    return out;
  }
};  // utf-8

//
// 0xD800      0xDBFF     0xDFFF
// |___________|___________|
//             0xDC00
//
template <typename CharT>
struct utf_traits<CharT, 2> {
  using char_type = CharT;
  constexpr static size_t max_width{2};
  // RFC2781
  constexpr static bool is_first_surrogate(uint16_t x) noexcept {
    return 0xD800 <= x && x <= 0xDBFF;
  }
  constexpr static bool is_second_surrogate(uint16_t x) noexcept {
    return 0xDC00 <= x && x <= 0xDFFF;
  }
  constexpr static codepoint combine_surrogate(uint16_t w1,
                                               uint16_t w2) noexcept {
    return ((codepoint(w1 & 0x3FF) << 10) | (w2 & 0x3FF)) + 0x10000;
  }

  constexpr static int trail_length(char_type v) noexcept {
    if (is_first_surrogate(v)) {
      return 1;
    }
    if (is_second_surrogate(v)) {
      return -1;
    }
    return 0;
  }
  constexpr static bool is_trail(char_type c) noexcept {
    return is_second_surrogate(c);
  }
  constexpr static bool is_lead(char_type c) noexcept {
    return !is_second_surrogate(c);
  }
  template <typename Iterator>
  constexpr static codepoint decode(Iterator& current, Iterator last,
                                    endian e = endian::native) noexcept {
    if (current == last) {
      return incomplete;
    }
    uint16_t w1 = *current++;
    if (e != endian::native) {
      w1 = swap_bytes(w1);
    }
    if (w1 < 0xD800 || 0xDFFF < w1) {
      return w1;
    }
    if (w1 > 0xDBFF) {
      return illegal;
    }
    if (current == last) {
      return incomplete;
    }
    uint16_t w2 = *current++;
    if (e != endian::native) {
      w2 = swap_bytes(w2);
    }
    if (w2 < 0xDC00 || 0xDFFF < w2) {
      return illegal;
    }
    return combine_surrogate(w1, w2);
  }
  template <typename Iterator>
  constexpr static codepoint decode_valid(Iterator& current,
                                          endian e = endian::native) noexcept {
    uint16_t w1 = *current++;
    if (e != endian::native) {
      w1 = swap_bytes(w1);
    }
    if (w1 < 0xD800 || 0xDFFF < w1) {
      return w1;
    }
    uint16_t w2 = *current++;
    return combine_surrogate(w1, w2);
  }
  constexpr static int width(codepoint u) noexcept {
    return u >= 0x10000 ? 2 : 1;
  }
  template <typename Iterator>
  constexpr static Iterator encode(codepoint u, Iterator out,
                                   endian e = endian::native) noexcept {
    if (u <= 0xFFFF) {
      uint16_t w = static_cast<uint16_t>(u);
      if (e != endian::native) {
        w = swap_bytes(w);
      }
      *out++ = static_cast<char_type>(w);
    } else {
      u -= 0x10000;
      uint16_t w1 = 0xD800 | (u >> 10);
      uint16_t w2 = 0xDC00 | (u & 0x3FF);
      if (e != endian::native) {
        w1 = swap_bytes(w1);
        w2 = swap_bytes(w2);
      }
      *out++ = static_cast<char_type>(w1);
      *out++ = static_cast<char_type>(w2);
    }
    return out;
  }
};  // utf-16

template <typename CharT>
struct utf_traits<CharT, 4> {
  using char_type = CharT;
  constexpr static int max_width = 1;
  constexpr static int trail_length(char_type c) noexcept {
    if (is_valid_codepoint(c)) {
      return 0;
    }
    return -1;
  }
  constexpr static bool is_trail(char_type /*c*/) noexcept { return false; }
  constexpr static bool is_lead(char_type /*c*/) noexcept { return true; }
  template <typename Iterator>
  constexpr static codepoint decode_valid(Iterator& current,
                                          endian e = endian::native) noexcept {
    codepoint c = *current++;
    if (e != endian::native) {
      c = swap_bytes(c);
    }
    return c;
  }
  template <typename Iterator>
  constexpr static codepoint decode(Iterator& current, Iterator last,
                                    endian e = endian::native) noexcept {
    if (current == last) {
      return incomplete;
    }
    codepoint c = *current++;
    if (e != endian::native) {
      c = swap_bytes(c);
    }
    if (!is_valid_codepoint(c)) {
      return illegal;
    }
    return c;
  }
  constexpr static int width(codepoint /*u*/) noexcept { return 1; }
  template <typename Iterator>
  constexpr static Iterator encode(codepoint u, Iterator out,
                                   endian e = endian::native) noexcept {
    if (e != endian::native) {
      u = swap_bytes(u);
    }
    *out++ = static_cast<char_type>(u);
    return out;
  }
};  // utf32

}  // namespace detail

template <typename CharOut, typename CharIn,
          typename = typename std::enable_if<
              (sizeof(CharIn) == 1 || sizeof(CharOut) == 1) &&
                  (sizeof(CharIn) != sizeof(CharOut)),
              void>::type>
constexpr size_t utf_to_utf(const CharIn* begin, const CharIn* end,
                            CharOut* out, utfx::endian from_or_to) {
  CharOut* p = out;
  size_t len = 0;
  while (begin != end) {
    detail::codepoint c{0};
    if constexpr (sizeof(CharIn) != 1) {
      c = detail::utf_traits<CharIn>::decode(begin, end, from_or_to);
    } else {
      c = detail::utf_traits<CharIn>::decode(begin, end);
    }
    len += detail::utf_traits<CharOut>::width(c);
    if (c == detail::illegal || c == detail::incomplete) {
      // throw conversion_error();
    } else {
      if (p != nullptr) {
        if constexpr (sizeof(CharOut) != 1) {
          p = detail::utf_traits<CharOut>::encode(c, p, from_or_to);
        } else {
          p = detail::utf_traits<CharOut>::encode(c, p);
        }
      }
    }
  }
  return p == nullptr ? len : p - out;
}

template <typename CharOut, typename CharIn,
          typename = typename std::enable_if<
              (sizeof(CharIn) == 1 || sizeof(CharOut) == 1) &&
                  (sizeof(CharIn) != sizeof(CharOut)),
              void>::type>
std::basic_string<CharOut> utf_to_utf(const CharIn* begin, const CharIn* end,
                                      utfx::endian from_or_to) {
  std::basic_string<CharOut> result;
  result.reserve((end - begin) * detail::utf_traits<CharOut>::max_width /
                 detail::utf_traits<CharIn>::max_width);
  std::back_insert_iterator<std::basic_string<CharOut>> inserter(result);
  while (begin != end) {
    detail::codepoint c;
    if constexpr (sizeof(CharIn) != 1) {
      c = detail::utf_traits<CharIn>::decode(begin, end, from_or_to);
    } else {
      c = detail::utf_traits<CharIn>::decode(begin, end);
    }
    if (c == detail::illegal || c == detail::incomplete) {
      // throw conversion_error();
    } else {
      if constexpr (sizeof(CharOut) != 1) {
        detail::utf_traits<CharOut>::encode(c, inserter, from_or_to);
      } else {
        detail::utf_traits<CharOut>::encode(c, inserter);
      }
    }
  }
  return result;
}

template <typename CharOut, typename CharIn,
          typename = typename std::enable_if<
              (sizeof(CharIn) != 1 && sizeof(CharOut) != 1), void>::type>
size_t utf_to_utf(const CharIn* begin, const CharIn* end, CharOut* out,
                  utfx::endian from, utfx::endian to) {
  CharOut* p = out;
  size_t len = 0;
  while (begin != end) {
    detail::codepoint c = detail::utf_traits<CharIn>::decode(begin, end, from);
    len += detail::utf_traits<CharOut>::width(c);
    if (c == detail::illegal || c == detail::incomplete) {
      // throw conversion_error();
    } else {
      if (p != nullptr) {
        p = detail::utf_traits<CharOut>::encode(c, p, to);
      }
    }
  }
  return p == nullptr ? len : p - out;
}

template <typename CharOut, typename CharIn,
          typename = typename std::enable_if<
              (sizeof(CharIn) != 1 && sizeof(CharOut) != 1), void>::type>
std::basic_string<CharOut> utf_to_utf(const CharIn* begin, const CharIn* end,
                                      utfx::endian from, utfx::endian to) {
  std::basic_string<CharOut> result;
  result.reserve((end - begin) * detail::utf_traits<CharOut>::max_width /
                 detail::utf_traits<CharIn>::max_width);
  std::back_insert_iterator<std::basic_string<CharOut>> inserter(result);
  while (begin != end) {
    detail::codepoint c = detail::utf_traits<CharIn>::decode(begin, end, from);
    if (c == detail::illegal || c == detail::incomplete) {
      // throw conversion_error();
    } else {
      detail::utf_traits<CharOut>::encode(c, inserter, to);
    }
  }
  return result;
}

#if defined(_WIN32)
template <typename ToCharT = wchar_t>
inline auto utf8_to_utf16(const char* str,
                          utfx::endian e = utfx::endian::native) {
  std::basic_string_view<char> s{str};
  return utf_to_utf<ToCharT>(s.data(), s.data() + s.size(), e);
}
inline auto utf16_to_utf8(const wchar_t* str,
                          utfx::endian e = utfx::endian::native) {
  std::basic_string_view<wchar_t> s{str};
  return utf_to_utf<char>(s.data(), s.data() + s.size(), e);
}
inline auto utf16_to_utf8(const char16_t* str,
                          utfx::endian e = utfx::endian::native) {
  std::basic_string_view<char16_t> s{str};
  return utf_to_utf<char>(s.data(), s.data() + s.size(), e);
}
template <typename ToCharT = wchar_t>
inline auto utf8_to_utf16(const std::string& str,
                          utfx::endian e = utfx::endian::native) {
  return utf_to_utf<ToCharT>(str.data(), str.data() + str.size(), e);
}
inline auto utf16_to_utf8(const std::wstring& str,
                          utfx::endian e = utfx::endian::native) {
  return utf_to_utf<char>(str.data(), str.data() + str.size(), e);
}
inline auto utf16_to_utf8(const std::u16string& str,
                          utfx::endian e = utfx::endian::native) {
  return utf_to_utf<char>(str.data(), str.data() + str.size(), e);
}
#else
inline auto utf8_to_utf16(const char* str,
                          utfx::endian e = utfx::endian::native) {
  std::basic_string_view<char> s{str};
  return utf_to_utf<char16_t>(s.data(), s.data() + s.size(), e);
}
inline auto utf16_to_utf8(const char16_t* str,
                          utfx::endian e = utfx::endian::native) {
  std::basic_string_view<char16_t> s{str};
  return utf_to_utf<char>(s.data(), s.data() + s.size(), e);
}

inline auto utf8_to_utf16(const std::string& str,
                          utfx::endian e = utfx::endian::native) {
  return utf_to_utf<char16_t>(str.data(), str.data() + str.size(), e);
}
inline auto utf16_to_utf8(const std::u16string& s,
                          utfx::endian e = utfx::endian::native) {
  return utf_to_utf<char>(s.data(), s.data() + s.size(), e);
}
#endif

inline bool is_utf8(const char* str, size_t len) {
  const char* begin = str;
  const char* end = str + len;
  if (len > 3) {
    unsigned char bom[3] = {static_cast<unsigned char>(str[0]),
                            static_cast<unsigned char>(str[1]),
                            static_cast<unsigned char>(str[2])};
    if (bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF) {
      begin = str + 3;
    }
  }
  while (begin != end) {
    const detail::codepoint c = detail::utf_traits<char>::decode(begin, end);
    if (c == detail::incomplete || c == detail::illegal) {
      return false;
    }
  }
  return true;
}

inline bool is_utf16(const char* str, size_t len,
                     utfx::endian endian = utfx::endian::native) {
  if (len % 2 != 0) {
    return false;
  }

  const char16_t* begin = reinterpret_cast<const char16_t*>(str);
  const char16_t* end = begin + len / 2;

  if (len > 2) {
    unsigned char bom[2] = {static_cast<unsigned char>(str[0]),
                            static_cast<unsigned char>(str[1])};
    if ((endian == utfx::endian::big) && (bom[0] == 0xFF && bom[1] == 0xFE)) {
      return false;
    }
    if ((endian == utfx::endian::little) &&
        (bom[0] == 0xFE && bom[1] == 0xFF)) {
      return false;
    }
    if ((bom[0] == 0xFF && bom[1] == 0xFE) ||
        (bom[0] == 0xFE && bom[1] == 0xFF)) {
      begin++;
    }
  }

  while (begin != end) {
    const detail::codepoint c =
        detail::utf_traits<char16_t>::decode(begin, end, endian);
    if (c == detail::incomplete || c == detail::illegal) {
      return false;
    }
  }
  return true;
}

namespace literals {
inline std::string operator""_utf8(const char16_t* s, std::size_t len) {
  return utf_to_utf<char>(s, s + len, utfx::endian::native);
}

inline std::u16string operator""_utf16(const char* s, std::size_t len) {
  return utf_to_utf<char16_t>(s, s + len, utfx::endian::native);
}

#if defined(__cpp_lib_char8_t)
inline std::u16string operator""_utf16(const char8_t* s, std::size_t len) {
  return utf_to_utf<char16_t>(s, s + len, utfx::endian::native);
}
#endif

#if defined(_WIN32)
inline std::string operator""_utf8(const wchar_t* s, std::size_t len) {
  return utf_to_utf<char>(s, s + len, utfx::endian::native);
}
#endif

}  // namespace literals
}  // namespace utfx

#endif  // __UTFX_UTFX_HPP__
