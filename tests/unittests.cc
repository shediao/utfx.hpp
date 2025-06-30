#include <gtest/gtest.h>

#include <utfx/utfx.hpp>

#if defined(_WIN32)
TEST(TESTUTFX, utf8_to_utf16_win) {
  const char utf8_str[] = "你好🔥!";
  const wchar_t utf16_str[] = L"你好🔥!";

  ASSERT_EQ(utfx::utf16_to_utf8(utf16_str), utf8_str);
  ASSERT_EQ(utfx::utf8_to_utf16(utf8_str), utf16_str);

  ASSERT_EQ(utfx::utf8_to_utf16(utfx::utf16_to_utf8(utf16_str)), utf16_str);
  ASSERT_EQ(utfx::utf16_to_utf8(utfx::utf8_to_utf16(utf8_str)), utf8_str);
}
#else
TEST(TESTUTFX, utf8_to_utf16) {
  const char utf8_str[] = "你好🔥!";
  const char16_t utf16_str[] = u"你好🔥!";

  ASSERT_EQ(utfx::utf16_to_utf8(utf16_str), utf8_str);
  ASSERT_EQ(utfx::utf8_to_utf16(utf8_str), utf16_str);

  ASSERT_EQ(utfx::utf8_to_utf16(utfx::utf16_to_utf8(utf16_str)), utf16_str);
  ASSERT_EQ(utfx::utf16_to_utf8(utfx::utf8_to_utf16(utf8_str)), utf8_str);
}
#endif
