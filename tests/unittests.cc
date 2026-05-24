#include <gtest/gtest.h>

#include <string_view>
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

unsigned char utf16le_win[] = {
    0x60, 0x4f, 0x7d, 0x59, 0x4a, 0x55, 0x3e, 0xd8, 0xf6, 0xde, 0x3e,
    0xd8, 0xdc, 0xdd, 0x0d, 0x20, 0x40, 0x26, 0x0f, 0xfe, 0x3d, 0xd8,
    0xaf, 0xdc, 0x31, 0x00, 0x32, 0x00, 0x33, 0x00, 0x0d, 0x00, 0x0a,
    0x00, 0x0d, 0x00, 0x0a, 0x00, 0x4b, 0x6d, 0xd5, 0x8b, 0x75, 0x00,
    0x6e, 0x00, 0x69, 0x00, 0x63, 0x00, 0x6f, 0x00, 0x64, 0x00, 0x65,
    0x00, 0x0d, 0x00, 0x0a, 0x00, 0x0d, 0x00, 0x0a, 0x00};

unsigned char utf16be_win[] = {
    0x4f, 0x60, 0x59, 0x7d, 0x55, 0x4a, 0xd8, 0x3e, 0xde, 0xf6, 0xd8,
    0x3e, 0xdd, 0xdc, 0x20, 0x0d, 0x26, 0x40, 0xfe, 0x0f, 0xd8, 0x3d,
    0xdc, 0xaf, 0x00, 0x31, 0x00, 0x32, 0x00, 0x33, 0x00, 0x0d, 0x00,
    0x0a, 0x00, 0x0d, 0x00, 0x0a, 0x6d, 0x4b, 0x8b, 0xd5, 0x00, 0x75,
    0x00, 0x6e, 0x00, 0x69, 0x00, 0x63, 0x00, 0x6f, 0x00, 0x64, 0x00,
    0x65, 0x00, 0x0d, 0x00, 0x0a, 0x00, 0x0d, 0x00, 0x0a};

unsigned char utf8_win[] = {
    0xe4, 0xbd, 0xa0, 0xe5, 0xa5, 0xbd, 0xe5, 0x95, 0x8a, 0xf0, 0x9f,
    0xab, 0xb6, 0xf0, 0x9f, 0xa7, 0x9c, 0xe2, 0x80, 0x8d, 0xe2, 0x99,
    0x80, 0xef, 0xb8, 0x8f, 0xf0, 0x9f, 0x92, 0xaf, 0x31, 0x32, 0x33,
    0x0d, 0x0a, 0x0d, 0x0a, 0xe6, 0xb5, 0x8b, 0xe8, 0xaf, 0x95, 0x75,
    0x6e, 0x69, 0x63, 0x6f, 0x64, 0x65, 0x0d, 0x0a, 0x0d, 0x0a};

TEST(TESTUTFX, utf8_and_utf16le_win_format) {
  auto utf16le_sv = std::u16string_view{
      reinterpret_cast<char16_t*>(&utf16le_win[0]), std::size(utf16le_win) / 2};
  auto utf16be_sv = std::u16string_view{
      reinterpret_cast<char16_t*>(&utf16be_win[0]), std::size(utf16be_win) / 2};
  auto utf8le_sv = std::string_view{reinterpret_cast<char*>(&utf8_win[0]),
                                    std::size(utf8_win)};

#if defined(_WIN32)
  ASSERT_EQ(utfx::utf8_to_utf16<char16_t>(utf8le_sv), utf16le_sv);
  ASSERT_EQ(utfx::utf8_to_utf16<char16_t>(utf8le_sv, utfx::endian::big),
            utf16be_sv);
#else
  ASSERT_EQ(utfx::utf8_to_utf16(utf8le_sv), utf16le_sv);
  ASSERT_EQ(utfx::utf8_to_utf16(utf8le_sv, utfx::endian::big), utf16be_sv);
#endif

  ASSERT_EQ(utfx::utf16_to_utf8(utf16le_sv), utf8le_sv);

  ASSERT_EQ(utfx::utf16_to_utf8(utf16be_sv, utfx::endian::big), utf8le_sv);
}

TEST(TESTUTFX, is_utf8) {
  ASSERT_TRUE(utfx::is_utf8((const char*)utf8_win, sizeof(utf8_win)));
  ASSERT_FALSE(utfx::is_utf8((const char*)utf16le_win, sizeof(utf16le_win)));
}

TEST(TESTUTFX, is_utf16) {
  ASSERT_TRUE(utfx::is_utf16((const char*)utf16le_win, sizeof(utf16le_win),
                             utfx::endian::little));
  ASSERT_FALSE(utfx::is_utf16((const char*)utf16le_win, sizeof(utf16le_win),
                              utfx::endian::big));

  ASSERT_TRUE(utfx::is_utf16((const char*)utf16be_win, sizeof(utf16be_win),
                             utfx::endian::big));
  ASSERT_FALSE(utfx::is_utf16((const char*)utf16be_win, sizeof(utf16be_win),
                              utfx::endian::little));
}

TEST(TESTUTFX, literals_test) {
  using namespace utfx::literals;
  ASSERT_EQ(u"你好🔥🔥🔥!"_utf8, "你好🔥🔥🔥!");
  ASSERT_EQ("你好🔥🔥🔥!"_utf16, u"你好🔥🔥🔥!");
  ASSERT_EQ(u8"你好🔥🔥🔥!"_utf16, u"你好🔥🔥🔥!");
}

TEST(TESTUTFX, string_and_string_view_overloads) {
  std::string utf8_str = "Hello UTF \xe6\xb5\x8b\xe8\xaf\x95 \xf0\x9f\x94\xa5!";
  std::string_view utf8_sv = utf8_str;

#if defined(_WIN32)
  // Windows: wchar_t overloads
  std::wstring utf16_str = L"Hello UTF 测试 🔥!";
  std::wstring_view utf16_sv = utf16_str;

  // utf8_to_utf16 with std::string and std::string_view
  EXPECT_EQ(utfx::utf8_to_utf16(utf8_str), utf16_str);
  EXPECT_EQ(utfx::utf8_to_utf16(utf8_sv), utf16_str);

  // utf8_to_utf16<char16_t> with std::string and std::string_view
  auto from_str = utfx::utf8_to_utf16<char16_t>(utf8_str);
  auto from_sv = utfx::utf8_to_utf16<char16_t>(utf8_sv);
  EXPECT_EQ(from_str, from_sv);

  // utf16_to_utf8 with std::wstring and std::wstring_view
  EXPECT_EQ(utfx::utf16_to_utf8(utf16_str), utf8_str);
  EXPECT_EQ(utfx::utf16_to_utf8(utf16_sv), utf8_str);

  // utf16_to_utf8 with std::u16string and std::u16string_view
  std::u16string u16str = u"Hello UTF 测试 🔥!";
  std::u16string_view u16sv = u16str;
  EXPECT_EQ(utfx::utf16_to_utf8(u16str), utf8_str);
  EXPECT_EQ(utfx::utf16_to_utf8(u16sv), utf8_str);
#else
  // Non-Windows: char16_t overloads
  std::u16string utf16_str = u"Hello UTF 测试 🔥!";
  std::u16string_view utf16_sv = utf16_str;

  // utf8_to_utf16 with std::string and std::string_view
  EXPECT_EQ(utfx::utf8_to_utf16(utf8_str), utf16_str);
  EXPECT_EQ(utfx::utf8_to_utf16(utf8_sv), utf16_str);

  // utf16_to_utf8 with std::u16string and std::u16string_view
  EXPECT_EQ(utfx::utf16_to_utf8(utf16_str), utf8_str);
  EXPECT_EQ(utfx::utf16_to_utf8(utf16_sv), utf8_str);
#endif

  // Round-trip via string_view
  EXPECT_EQ(utfx::utf16_to_utf8(utfx::utf8_to_utf16(utf8_sv)), utf8_str);
}
