#ifndef __UTFX_UTFX_HPP__
#define __UTFX_UTFX_HPP__
#include <cstdint>
#include <iterator>
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

// ============================================================================
// utf8_char — A single UTF-8 code point (1–4 bytes) within a larger string.
// ============================================================================
class utf8_char {
 public:
  using value_type = char;
  using pointer = const char*;
  using const_pointer = const char*;
  using iterator = const char*;
  using const_iterator = const char*;
  using size_type = size_t;

  constexpr utf8_char() noexcept : data_(nullptr), len_(0) {}
  constexpr utf8_char(const char* data, size_type len) noexcept
      : data_(data), len_(len) {}

  /// Pointer to the raw UTF-8 bytes of this character.
  constexpr const char* data() const noexcept { return data_; }
  /// Number of bytes (1–4).
  constexpr size_type size() const noexcept { return len_; }
  /// Number of bytes (1–4).
  constexpr size_type length() const noexcept { return len_; }
  /// Number of bytes (1–4).
  constexpr size_type byte_size() const noexcept { return len_; }

  constexpr const char* begin() const noexcept { return data_; }
  constexpr const char* end() const noexcept { return data_ + len_; }
  constexpr const char* cbegin() const noexcept { return data_; }
  constexpr const char* cend() const noexcept { return data_ + len_; }

  /// The decoded Unicode code point (char32_t).
  constexpr detail::codepoint code_point() const noexcept {
    if (len_ == 0) {
      return detail::illegal;
    }
    const char* p = data_;
    return detail::utf_traits<char>::decode_valid(p);
  }

  constexpr char operator[](size_type i) const noexcept { return data_[i]; }
  constexpr bool empty() const noexcept { return len_ == 0; }

  friend constexpr bool operator==(utf8_char a, utf8_char b) noexcept {
    return a.code_point() == b.code_point();
  }
  friend constexpr bool operator!=(utf8_char a, utf8_char b) noexcept {
    return !(a == b);
  }
  friend constexpr bool operator<(utf8_char a, utf8_char b) noexcept {
    return a.code_point() < b.code_point();
  }
  friend constexpr bool operator<=(utf8_char a, utf8_char b) noexcept {
    return a.code_point() <= b.code_point();
  }
  friend constexpr bool operator>(utf8_char a, utf8_char b) noexcept {
    return a.code_point() > b.code_point();
  }
  friend constexpr bool operator>=(utf8_char a, utf8_char b) noexcept {
    return a.code_point() >= b.code_point();
  }

 private:
  const char* data_;
  size_type len_;
};

// ============================================================================
// utf8_view — A lightweight, read-only view over UTF-8 encoded text.
//
// Provides an interface similar to std::string_view, but iterates over
// Unicode code points (utf8_char) rather than raw bytes.
//
// Complexity notes:
//   size(), operator[], substr(), and remove_prefix/remove_suffix are O(n)
//   in the number of code points (they must scan UTF-8 boundaries).
//   byte_size(), data(), empty(), begin(), end() are O(1).
// ============================================================================
class utf8_view {
 public:
  // --- Member types ---
  using value_type = utf8_char;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using reference = utf8_char;
  using const_reference = utf8_char;

  /// Forward iterator that decodes UTF-8 on the fly.
  class iterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = utf8_char;
    using difference_type = ptrdiff_t;
    using pointer = void;
    using reference = utf8_char;

    constexpr iterator() noexcept : pos_(nullptr) {}

    constexpr reference operator*() const noexcept {
      int trail = detail::utf_traits<char>::trail_length(*pos_);
      size_type len = trail < 0 ? 1 : static_cast<size_type>(trail + 1);
      return utf8_char(pos_, len);
    }

    constexpr iterator& operator++() noexcept {
      int trail = detail::utf_traits<char>::trail_length(*pos_);
      pos_ += trail < 0 ? 1 : static_cast<size_type>(trail + 1);
      return *this;
    }

    constexpr iterator operator++(int) noexcept {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    constexpr bool operator==(const iterator& other) const noexcept {
      return pos_ == other.pos_;
    }
    constexpr bool operator!=(const iterator& other) const noexcept {
      return !(*this == other);
    }

   private:
    friend class utf8_view;
    constexpr explicit iterator(const char* pos) noexcept : pos_(pos) {}
    const char* pos_;
  };

  using const_iterator = iterator;

  // --- Construction ---
  constexpr utf8_view() noexcept : data_(nullptr), byte_size_(0) {}

  /// Construct from a null-terminated C-string.
  /*implicit*/ constexpr utf8_view(const char* str) noexcept
      : data_(str), byte_size_(str ? std::char_traits<char>::length(str) : 0) {}

  /// Construct from a pointer and byte length.
  constexpr utf8_view(const char* str, size_type len) noexcept
      : data_(str), byte_size_(len) {}

  /// Construct from std::string.
  /*implicit*/ utf8_view(const std::string& str) noexcept
      : data_(str.data()), byte_size_(str.size()) {}

  /// Construct from std::string_view.
  /*implicit*/ constexpr utf8_view(std::string_view sv) noexcept
      : data_(sv.data()), byte_size_(sv.size()) {}

  // --- Iterators ---
  constexpr iterator begin() const noexcept { return iterator(data_); }
  constexpr iterator end() const noexcept {
    return iterator(data_ + byte_size_);
  }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }

  // --- Size / capacity ---
  /// Number of code points. O(n) — scans the entire view.
  constexpr size_type size() const noexcept {
    size_type count = 0;
    for (auto it = begin(); it != end(); ++it) {
      ++count;
    }
    return count;
  }
  /// Number of code points. O(n).
  constexpr size_type length() const noexcept { return size(); }
  /// Number of raw bytes. O(1).
  constexpr size_type byte_size() const noexcept { return byte_size_; }
  /// True if the view contains no bytes. O(1).
  constexpr bool empty() const noexcept { return byte_size_ == 0; }
  /// Maximum possible size.
  static constexpr size_type npos = ~size_type(0);
  constexpr size_type max_size() const noexcept { return npos - 1; }

  // --- Element access ---
  /// Pointer to the raw underlying bytes. O(1).
  constexpr const char* data() const noexcept { return data_; }

  /// First code point. O(1). UB if empty.
  constexpr utf8_char front() const noexcept {
    int trail = detail::utf_traits<char>::trail_length(*data_);
    size_type len = trail < 0 ? 1 : static_cast<size_type>(trail + 1);
    return utf8_char(data_, len);
  }

  /// Last code point. O(last-character-length). UB if empty.
  constexpr utf8_char back() const noexcept {
    const char* p = data_ + byte_size_;
    while (p > data_ && detail::utf_traits<char>::is_trail(*(p - 1))) {
      --p;
    }
    --p;
    return utf8_char(p, static_cast<size_type>(data_ + byte_size_ - p));
  }

  /// Nth code point. O(n). UB if n >= size().
  constexpr utf8_char operator[](size_type n) const noexcept {
    auto it = begin();
    while (n-- > 0) {
      ++it;
    }
    return *it;
  }

  // --- Modifiers (view-level) ---
  /// Remove the first n code points from the view. O(n).
  constexpr void remove_prefix(size_type n) noexcept {
    while (n-- > 0 && byte_size_ > 0) {
      int trail = detail::utf_traits<char>::trail_length(*data_);
      size_type len = trail < 0 ? 1 : static_cast<size_type>(trail + 1);
      data_ += len;
      byte_size_ -= len;
    }
  }

  /// Remove the last n code points from the view. O(n).
  constexpr void remove_suffix(size_type n) noexcept {
    while (n-- > 0 && byte_size_ > 0) {
      const char* p = data_ + byte_size_;
      while (p > data_ && detail::utf_traits<char>::is_trail(*(p - 1))) {
        --p;
      }
      --p;
      byte_size_ = static_cast<size_type>(p - data_);
    }
  }

  // --- Substring ---
  /// Returns a view of the substring [pos, pos+count). O(pos+count).
  constexpr utf8_view substr(size_type pos = 0,
                             size_type count = npos) const noexcept {
    const char* start = data_;
    const char* end_pos = data_ + byte_size_;
    while (pos-- > 0 && start < end_pos) {
      int trail = detail::utf_traits<char>::trail_length(*start);
      start += trail < 0 ? 1 : static_cast<size_type>(trail + 1);
    }
    if (start >= end_pos) {
      return utf8_view();
    }
    const char* sub_end = start;
    while (count-- > 0 && sub_end < end_pos) {
      int trail = detail::utf_traits<char>::trail_length(*sub_end);
      sub_end += trail < 0 ? 1 : static_cast<size_type>(trail + 1);
    }
    return utf8_view(start, static_cast<size_type>(sub_end - start));
  }

  // --- Swap ---
  constexpr void swap(utf8_view& other) noexcept {
    const char* tmp_data = data_;
    size_type tmp_size = byte_size_;
    data_ = other.data_;
    byte_size_ = other.byte_size_;
    other.data_ = tmp_data;
    other.byte_size_ = tmp_size;
  }

  // --- Conversion ---
  /// Implicit conversion to std::string_view (raw bytes).
  constexpr operator std::string_view() const noexcept {
    return std::string_view(data_, byte_size_);
  }

  // --- Comparison (bytewise, consistent with code-point order for valid UTF-8)
  friend constexpr bool operator==(utf8_view a, utf8_view b) noexcept {
    return std::string_view(a.data_, a.byte_size_) ==
           std::string_view(b.data_, b.byte_size_);
  }
  friend constexpr bool operator!=(utf8_view a, utf8_view b) noexcept {
    return !(a == b);
  }
  friend constexpr bool operator<(utf8_view a, utf8_view b) noexcept {
    return std::string_view(a.data_, a.byte_size_) <
           std::string_view(b.data_, b.byte_size_);
  }
  friend constexpr bool operator<=(utf8_view a, utf8_view b) noexcept {
    return std::string_view(a.data_, a.byte_size_) <=
           std::string_view(b.data_, b.byte_size_);
  }
  friend constexpr bool operator>(utf8_view a, utf8_view b) noexcept {
    return std::string_view(a.data_, a.byte_size_) >
           std::string_view(b.data_, b.byte_size_);
  }
  friend constexpr bool operator>=(utf8_view a, utf8_view b) noexcept {
    return std::string_view(a.data_, a.byte_size_) >=
           std::string_view(b.data_, b.byte_size_);
  }

 private:
  const char* data_;
  size_type byte_size_;
};

template <typename CharOut, typename CharIn,
          typename = typename std::enable_if<
              (sizeof(CharIn) == 1 || sizeof(CharOut) == 1) &&
                  (sizeof(CharIn) != sizeof(CharOut)),
              void>::type>
constexpr size_t transcode(const CharIn* begin, const CharIn* end, CharOut* out,
                           utfx::endian from_or_to) {
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
std::basic_string<CharOut> transcode(const CharIn* begin, const CharIn* end,
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
size_t transcode(const CharIn* begin, const CharIn* end, CharOut* out,
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
std::basic_string<CharOut> transcode(const CharIn* begin, const CharIn* end,
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
  return transcode<ToCharT>(s.data(), s.data() + s.size(), e);
}
inline auto utf16_to_utf8(const wchar_t* str,
                          utfx::endian e = utfx::endian::native) {
  std::basic_string_view<wchar_t> s{str};
  return transcode<char>(s.data(), s.data() + s.size(), e);
}
inline auto utf16_to_utf8(const char16_t* str,
                          utfx::endian e = utfx::endian::native) {
  std::basic_string_view<char16_t> s{str};
  return transcode<char>(s.data(), s.data() + s.size(), e);
}
template <typename ToCharT = wchar_t>
inline auto utf8_to_utf16(const std::string& str,
                          utfx::endian e = utfx::endian::native) {
  return transcode<ToCharT>(str.data(), str.data() + str.size(), e);
}
inline auto utf16_to_utf8(const std::wstring& str,
                          utfx::endian e = utfx::endian::native) {
  return transcode<char>(str.data(), str.data() + str.size(), e);
}
inline auto utf16_to_utf8(const std::u16string& str,
                          utfx::endian e = utfx::endian::native) {
  return transcode<char>(str.data(), str.data() + str.size(), e);
}
#else
inline auto utf8_to_utf16(const char* str,
                          utfx::endian e = utfx::endian::native) {
  std::basic_string_view<char> s{str};
  return transcode<char16_t>(s.data(), s.data() + s.size(), e);
}
inline auto utf16_to_utf8(const char16_t* str,
                          utfx::endian e = utfx::endian::native) {
  std::basic_string_view<char16_t> s{str};
  return transcode<char>(s.data(), s.data() + s.size(), e);
}

inline auto utf8_to_utf16(const std::string& str,
                          utfx::endian e = utfx::endian::native) {
  return transcode<char16_t>(str.data(), str.data() + str.size(), e);
}
inline auto utf16_to_utf8(const std::u16string& s,
                          utfx::endian e = utfx::endian::native) {
  return transcode<char>(s.data(), s.data() + s.size(), e);
}
#endif

inline bool is_utf8(const char* str, size_t len) {
  const char* begin = str;
  const char* end = str + len;
  if (len >= 3) {
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

  if (len >= 2) {
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
  return transcode<char>(s, s + len, utfx::endian::native);
}

inline std::u16string operator""_utf16(const char* s, std::size_t len) {
  return transcode<char16_t>(s, s + len, utfx::endian::native);
}

#if defined(__cpp_lib_char8_t)
inline std::u16string operator""_utf16(const char8_t* s, std::size_t len) {
  return transcode<char16_t>(s, s + len, utfx::endian::native);
}
#endif

#if defined(_WIN32)
inline std::string operator""_utf8(const wchar_t* s, std::size_t len) {
  return transcode<char>(s, s + len, utfx::endian::native);
}
#endif

}  // namespace literals
}  // namespace utfx

#endif  // __UTFX_UTFX_HPP__
