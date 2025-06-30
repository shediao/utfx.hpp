#ifndef __UTFX_UTFX_HPP__
#define __UTFX_UTFX_HPP__
#include <cstdint>
#include <string>

namespace utfx {

namespace detail {
using codepoint = uint32_t;
constexpr inline codepoint illegal = 0xFFFFFFFFu;
constexpr inline codepoint incomplete = 0xFFFFFFFEu;

constexpr inline bool is_valid_codepoint(codepoint v) {
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
  static constexpr size_t max_width{4};

  static int trail_length(char_type v) {
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

  static int width(codepoint v) {
    if (v < 0x80) {
      return 1;
    } else if (v < 0x800) {
      return 2;
    } else if (v < 0x10000) {
      return 3;
    }
    return 4;
  }

  static bool is_trail(char_type v) {
    unsigned char c = v;
    return (c & 0b11000000u) == 0b10000000u;
  }
  static bool is_lead(char_type v) { return !is_trail(v); }

  template <typename Iterator>
  static codepoint decode(Iterator& p, Iterator e) {
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
    unsigned char tmp;
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
      case 2:
        if (p == e) {
          return incomplete;
        }
        tmp = *p++;
        if (!trail_length(tmp)) {
          return illegal;
        }
        c = ((c << 6) | (tmp & 0x3F));
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
  static codepoint decode_valid(Iterator& p) {
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
      case 2:
        c = ((c << 6) | (0x3f & static_cast<unsigned char>(*p++)));
      case 1:
        c = ((c << 6) | (0x3f & static_cast<unsigned char>(*p++)));
    }
    return c;
  }

  template <typename Iterator>
  static Iterator encode(codepoint c, Iterator out) {
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
  static constexpr size_t max_width{2};
  // RFC2781
  static bool is_first_surrogate(uint16_t x) {
    return 0xD800 <= x && x <= 0xDBFF;
  }
  static bool is_second_surrogate(uint16_t x) {
    return 0xDC00 <= x && x <= 0xDFFF;
  }
  static codepoint combine_surrogate(uint16_t w1, uint16_t w2) {
    return ((codepoint(w1 & 0x3FF) << 10) | (w2 & 0x3FF)) + 0x10000;
  }

  static int trail_length(char_type v) {
    if (is_first_surrogate(v)) {
      return 1;
    }
    if (is_second_surrogate(v)) {
      return -1;
    }
    return 0;
  }
  static bool is_trail(char_type c) { return is_second_surrogate(c); }
  static bool is_lead(char_type c) { return !is_second_surrogate(c); }
  template <typename It>
  static codepoint decode(It& current, It last) {
    if (current == last) {
      return incomplete;
    }
    uint16_t w1 = *current++;
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
    if (w2 < 0xDC00 || 0xDFFF < w2) {
      return illegal;
    }
    return combine_surrogate(w1, w2);
  }
  template <typename It>
  static codepoint decode_valid(It& current) {
    uint16_t w1 = *current++;
    if (w1 < 0xD800 || 0xDFFF < w1) {
      return w1;
    }
    uint16_t w2 = *current++;
    return combine_surrogate(w1, w2);
  }
  static int width(codepoint u) { return u >= 0x10000 ? 2 : 1; }
  template <typename It>
  static It encode(codepoint u, It out) {
    if (u <= 0xFFFF) {
      *out++ = static_cast<char_type>(u);
    } else {
      u -= 0x10000;
      *out++ = static_cast<char_type>(0xD800 | (u >> 10));
      *out++ = static_cast<char_type>(0xDC00 | (u & 0x3FF));
    }
    return out;
  }
};  // utf-16

template <typename CharT>
struct utf_traits<CharT, 4> {
  using char_type = CharT;
  static constexpr int max_width = 1;
  static int trail_length(char_type c) {
    if (is_valid_codepoint(c)) {
      return 0;
    }
    return -1;
  }
  static bool is_trail(char_type /*c*/) { return false; }
  static bool is_lead(char_type /*c*/) { return true; }
  template <typename It>
  static codepoint decode_valid(It& current) {
    return *current++;
  }
  template <typename It>
  static codepoint decode(It& current, It last) {
    if (current == last) {
      return incomplete;
    }
    codepoint c = *current++;
    if (!is_valid_codepoint(c)) {
      return illegal;
    }
    return c;
  }
  static int width(codepoint /*u*/) { return 1; }
  template <typename It>
  static It encode(codepoint u, It out) {
    *out++ = static_cast<char_type>(u);
    return out;
  }
};  // utf32

}  // namespace detail

template <typename CharOut, typename CharIn>
std::basic_string<CharOut> utf_to_utf(const CharIn* begin, const CharIn* end) {
  std::basic_string<CharOut> result;
  result.reserve((end - begin) * detail::utf_traits<CharOut>::max_width /
                 detail::utf_traits<CharIn>::max_width);
  std::back_insert_iterator<std::basic_string<CharOut>> inserter(result);
  while (begin != end) {
    const detail::codepoint c = detail::utf_traits<CharIn>::decode(begin, end);
    if (c == detail::illegal || c == detail::incomplete) {
      // throw conversion_error();
    } else {
      detail::utf_traits<CharOut>::encode(c, inserter);
    }
  }
  return result;
}

template <typename CharOut, typename CharIn>
std::basic_string<CharOut> utf_to_utf(const std::basic_string<CharIn>& str) {
  return utf_to_utf<CharOut, CharIn>(str.c_str(), str.c_str() + str.size());
}

template <typename CharOut, typename CharIn>
std::basic_string<CharOut> utf_to_utf(const CharIn* str) {
  std::basic_string_view<CharIn> input{str};
  return utf_to_utf<CharOut, CharIn>(input.data(), input.data() + input.size());
}

#if defined(_WIN32)
template <typename ToCharT = wchar_t>
inline auto utf8_to_utf16(const char* str) {
  return utf_to_utf<ToCharT>(str);
}
inline auto utf16_to_utf8(const wchar_t* str) { return utf_to_utf<char>(str); }
inline auto utf16_to_utf8(const char16_t* str) { return utf_to_utf<char>(str); }
template <typename ToCharT = wchar_t>
inline auto utf8_to_utf16(const std::string& str) {
  return utf_to_utf<ToCharT>(str);
}
inline auto utf16_to_utf8(const std::wstring& str) {
  return utf_to_utf<char>(str);
}
inline auto utf16_to_utf8(const std::u16string& str) {
  return utf_to_utf<char>(str);
}
#else
inline auto utf8_to_utf16(const char* str) { return utf_to_utf<char16_t>(str); }
inline auto utf16_to_utf8(const char16_t* str) { return utf_to_utf<char>(str); }

inline auto utf8_to_utf16(const std::string& str) {
  return utf_to_utf<char16_t>(str);
}
inline auto utf16_to_utf8(const std::u16string& str) {
  return utf_to_utf<char>(str);
}
#endif

}  // namespace utfx

#endif  // __UTFX_UTFX_HPP__
