#include <gtest/gtest.h>

#include <cstring>
#include <string_view>
#include <utfx/utfx.hpp>

// ============================================================================
// transcode: UTF-8 <-> UTF-16 edge cases
// ============================================================================

TEST(TranscodeTest, UTF8ToUTF16_EmptyInput) {
  const char* empty = "";
  auto result = utfx::transcode<char16_t>(empty, empty, utfx::endian::native);
  EXPECT_TRUE(result.empty());
}

TEST(TranscodeTest, UTF16ToUTF8_EmptyInput) {
  const char16_t* empty = u"";
  auto result = utfx::transcode<char>(empty, empty, utfx::endian::native);
  EXPECT_TRUE(result.empty());
}

TEST(TranscodeTest, UTF8ToUTF16_RawPointer_NullOutput) {
  // Length calculation mode (null output pointer)
  const char input[] = "Hello";
  size_t len = utfx::transcode(
      input, input + 5, static_cast<char16_t*>(nullptr), utfx::endian::native);
  EXPECT_EQ(len, 5u);  // 5 ASCII chars = 5 UTF-16 units
}

TEST(TranscodeTest, UTF16ToUTF8_RawPointer_NullOutput) {
  // Length calculation mode
  const char16_t input[] = u"Hello";
  size_t len = utfx::transcode(input, input + 5, static_cast<char*>(nullptr),
                               utfx::endian::native);
  EXPECT_EQ(len, 5u);  // 5 ASCII chars = 5 UTF-8 bytes
}

TEST(TranscodeTest, UTF8ToUTF16_RawPointer_WithOutput) {
  const char input[] = "ABC";
  char16_t output[4] = {};
  size_t written =
      utfx::transcode(input, input + 3, output, utfx::endian::native);
  EXPECT_EQ(written, 3u);
  EXPECT_EQ(output[0], u'A');
  EXPECT_EQ(output[1], u'B');
  EXPECT_EQ(output[2], u'C');
}

TEST(TranscodeTest, UTF8ToUTF16_SupplementaryPlane) {
  // U+1F600 grinning face
  const char input[] = "\xF0\x9F\x98\x80";
  auto result =
      utfx::transcode<char16_t>(input, input + 4, utfx::endian::native);
  EXPECT_EQ(result.size(), 2u);  // Surrogate pair
  EXPECT_EQ(static_cast<uint16_t>(result[0]), 0xD83Du);
  EXPECT_EQ(static_cast<uint16_t>(result[1]), 0xDE00u);
}

TEST(TranscodeTest, UTF16ToUTF8_SupplementaryPlane) {
  // U+1F600 grinning face
  const char16_t input[] = {0xD83D, 0xDE00, 0x0000};
  auto result = utfx::transcode<char>(input, input + 2, utfx::endian::native);
  EXPECT_EQ(result.size(), 4u);
  EXPECT_EQ(static_cast<unsigned char>(result[0]), 0xF0);
  EXPECT_EQ(static_cast<unsigned char>(result[1]), 0x9F);
  EXPECT_EQ(static_cast<unsigned char>(result[2]), 0x98);
  EXPECT_EQ(static_cast<unsigned char>(result[3]), 0x80);
}

TEST(TranscodeTest, UTF8ToUTF16_EndianSwap) {
  const char input[] = "A";  // U+0041
  auto result = utfx::transcode<char16_t>(input, input + 1, utfx::endian::big);
  EXPECT_EQ(result.size(), 1u);
  if (utfx::endian::native == utfx::endian::big) {
    EXPECT_EQ(result[0], u'A');
  } else {
    // On LE machine, big-endian encoding swaps bytes
    EXPECT_EQ(static_cast<uint16_t>(result[0]),
              utfx::detail::swap_bytes<uint16_t>(u'A'));
  }
}

TEST(TranscodeTest, UTF16ToUTF8_EndianInput) {
  // Create big-endian UTF-16 data for 'A' (U+0041)
  uint16_t be_A = utfx::detail::swap_bytes<uint16_t>(u'A');
  if (utfx::endian::native == utfx::endian::big) {
    be_A = u'A';
  }
  const char16_t* input = reinterpret_cast<const char16_t*>(&be_A);
  auto result = utfx::transcode<char>(input, input + 1, utfx::endian::big);
  EXPECT_EQ(result, "A");
}

TEST(TranscodeTest, UTF8ToUTF16_InvalidSequences_Skipped) {
  // Invalid UTF-8 byte: 0xFF is never valid
  const char input[] =
      "A\xFF"
      "B";
  auto result =
      utfx::transcode<char16_t>(input, input + 3, utfx::endian::native);
  // The invalid byte should be silently skipped
  EXPECT_EQ(result.size(), 2u);
  EXPECT_EQ(result[0], u'A');
  EXPECT_EQ(result[1], u'B');
}

// ============================================================================
// transcode: UTF-16 <-> UTF-32 tests
// ============================================================================

TEST(TranscodeTest, UTF16ToUTF32_Basic) {
  const char16_t input[] = u"Hi";
  auto result = utfx::transcode<char32_t>(
      input, input + 2, utfx::endian::native, utfx::endian::native);
  EXPECT_EQ(result.size(), 2u);
  EXPECT_EQ(result[0], U'H');
  EXPECT_EQ(result[1], U'i');
}

TEST(TranscodeTest, UTF32ToUTF16_Basic) {
  const char32_t input[] = U"Hi";
  auto result = utfx::transcode<char16_t>(
      input, input + 2, utfx::endian::native, utfx::endian::native);
  EXPECT_EQ(result.size(), 2u);
  EXPECT_EQ(result[0], u'H');
  EXPECT_EQ(result[1], u'i');
}

TEST(TranscodeTest, UTF16ToUTF32_Supplementary) {
  // U+1F600
  const char16_t input[] = {0xD83D, 0xDE00, 0x0000};
  auto result = utfx::transcode<char32_t>(
      input, input + 2, utfx::endian::native, utfx::endian::native);
  EXPECT_EQ(result.size(), 1u);
  EXPECT_EQ(static_cast<uint32_t>(result[0]), 0x1F600u);
}

TEST(TranscodeTest, UTF32ToUTF16_Supplementary) {
  const char32_t input[] = {0x1F600, 0x0000};
  auto result = utfx::transcode<char16_t>(
      input, input + 1, utfx::endian::native, utfx::endian::native);
  EXPECT_EQ(result.size(), 2u);
  EXPECT_EQ(static_cast<uint16_t>(result[0]), 0xD83Du);
  EXPECT_EQ(static_cast<uint16_t>(result[1]), 0xDE00u);
}

TEST(TranscodeTest, UTF16ToUTF32_EndianBoth) {
  const char16_t input[] = u"A";
  auto result = utfx::transcode<char32_t>(input, input + 1, utfx::endian::big,
                                          utfx::endian::big);
  EXPECT_EQ(result.size(), 1u);
  if (utfx::endian::native == utfx::endian::big) {
    EXPECT_EQ(result[0], U'A');
  }
}

TEST(TranscodeTest, UTF8ToUTF32_Basic) {
  const char input[] = "Hello";
  auto result =
      utfx::transcode<char32_t>(input, input + 5, utfx::endian::native);
  EXPECT_EQ(result.size(), 5u);
  EXPECT_EQ(result[0], U'H');
  EXPECT_EQ(result[1], U'e');
  EXPECT_EQ(result[2], U'l');
  EXPECT_EQ(result[3], U'l');
  EXPECT_EQ(result[4], U'o');
}

TEST(TranscodeTest, UTF32ToUTF8_Basic) {
  const char32_t input[] = U"Hello";
  auto result = utfx::transcode<char>(input, input + 5, utfx::endian::native);
  EXPECT_EQ(result, "Hello");
}

// ============================================================================
// is_utf8 edge cases
// ============================================================================

TEST(IsUTF8Test, EmptyString) { EXPECT_TRUE(utfx::is_utf8("", 0)); }

TEST(IsUTF8Test, PureASCII) { EXPECT_TRUE(utfx::is_utf8("Hello, World!", 13)); }

TEST(IsUTF8Test, ValidMultiByte) {
  const char valid[] = "\xE4\xBD\xA0\xE5\xA5\xBD";  // 你好
  EXPECT_TRUE(utfx::is_utf8(valid, 6));
}

TEST(IsUTF8Test, BOM_Skip) {
  // BOM + ASCII
  const char with_bom[] =
      "\xEF\xBB\xBF"
      "Hello";
  EXPECT_TRUE(utfx::is_utf8(with_bom, 8));
}

TEST(IsUTF8Test, BOM_Only) {
  const char bom_only[] = "\xEF\xBB\xBF";
  EXPECT_TRUE(utfx::is_utf8(bom_only, 3));
}

TEST(IsUTF8Test, BOM_LessThan3Bytes) {
  // Too short for BOM detection
  const char short_str[] = "\xEF\xBB";
  EXPECT_FALSE(utfx::is_utf8(short_str, 2));  // Incomplete sequence
}

TEST(IsUTF8Test, IncompleteSequence) {
  const char incomplete[] = "\xE4\xBD";  // Missing third byte
  EXPECT_FALSE(utfx::is_utf8(incomplete, 2));
}

TEST(IsUTF8Test, InvalidByte) {
  // 0xFF is never valid in UTF-8
  const char invalid[] = "\xFF";
  EXPECT_FALSE(utfx::is_utf8(invalid, 1));
}

TEST(IsUTF8Test, OverlongSequence) {
  // Overlong encoding of '/'
  const char overlong[] = "\xC0\xAF";
  EXPECT_FALSE(utfx::is_utf8(overlong, 2));
}

TEST(IsUTF8Test, SurrogateEncoded) {
  // U+D800 encoded as UTF-8
  const char surrogate[] = "\xED\xA0\x80";
  EXPECT_FALSE(utfx::is_utf8(surrogate, 3));
}

TEST(IsUTF8Test, BeyondMaxCodepoint) {
  // U+110000
  const char beyond[] = "\xF4\x90\x80\x80";
  EXPECT_FALSE(utfx::is_utf8(beyond, 4));
}

TEST(IsUTF8Test, TruncatedSequence_OneByteRemaining) {
  // Lead byte for 3-byte sequence, but only one byte total
  const char truncated[] = "\xE0";
  EXPECT_FALSE(utfx::is_utf8(truncated, 1));
}

TEST(IsUTF8Test, InvalidTrailByte) {
  // Lead byte followed by ASCII byte (not a valid trail byte in UTF-8)
  // 0xE0 expects 2 trail bytes, but 'A' (0x41) has trail_length==0,
  // which triggers !trail_length == true => illegal
  const char bad_trail[] = "\xE0\x41\x80";
  EXPECT_FALSE(utfx::is_utf8(bad_trail, 3));
}

// ============================================================================
// is_utf16 edge cases
// ============================================================================

TEST(IsUTF16Test, EmptyData) { EXPECT_TRUE(utfx::is_utf16("", 0)); }

TEST(IsUTF16Test, OddLength) {
  // UTF-16 requires even number of bytes
  const char odd[] = "A";
  EXPECT_FALSE(utfx::is_utf16(odd, 1));
}

TEST(IsUTF16Test, ValidBMP) {
  const char16_t input[] = u"Hello";
  EXPECT_TRUE(utfx::is_utf16(reinterpret_cast<const char*>(input), 10));
}

TEST(IsUTF16Test, ValidSurrogatePair) {
  const char16_t input[] = {0xD83D, 0xDE00, 0x0000};  // U+1F600
  EXPECT_TRUE(utfx::is_utf16(reinterpret_cast<const char*>(input), 4));
}

TEST(IsUTF16Test, UnpairedFirstSurrogate) {
  const char16_t input[] = {0xD83D, 0x0041, 0x0000};  // First surrogate + 'A'
  EXPECT_FALSE(utfx::is_utf16(reinterpret_cast<const char*>(input), 4));
}

TEST(IsUTF16Test, UnpairedSecondSurrogate) {
  const char16_t input[] = {0xDE00, 0x0000};  // Lone second surrogate
  EXPECT_FALSE(utfx::is_utf16(reinterpret_cast<const char*>(input), 2));
}

TEST(IsUTF16Test, BOM_LE_Native) {
  // Little-endian BOM: FF FE
  const unsigned char bom_le[] = {0xFF, 0xFE, 'A', 0x00};
  // This test uses native endian (LE on most platforms), so the BOM
  // indicates LE, which matches native on LE machines.
  bool result = utfx::is_utf16(reinterpret_cast<const char*>(bom_le), 4,
                               utfx::endian::little);
  EXPECT_TRUE(result);
}

TEST(IsUTF16Test, BOM_BE_WithBEEndian) {
  // Big-endian BOM: FE FF
  const unsigned char bom_be[] = {0xFE, 0xFF, 0x00, 'A'};
  bool result = utfx::is_utf16(reinterpret_cast<const char*>(bom_be), 4,
                               utfx::endian::big);
  EXPECT_TRUE(result);
}

TEST(IsUTF16Test, BOM_Mismatch_BE_BOM_With_LE_Endian) {
  // Big-endian BOM but we say it's LE
  const unsigned char bom_be[] = {0xFE, 0xFF, 0x00, 'A'};
  bool result = utfx::is_utf16(reinterpret_cast<const char*>(bom_be), 4,
                               utfx::endian::little);
  EXPECT_FALSE(result);
}

TEST(IsUTF16Test, BOM_Mismatch_LE_BOM_With_BE_Endian) {
  // Little-endian BOM but we say it's BE
  const unsigned char bom_le[] = {0xFF, 0xFE, 'A', 0x00};
  bool result = utfx::is_utf16(reinterpret_cast<const char*>(bom_le), 4,
                               utfx::endian::big);
  EXPECT_FALSE(result);
}

// ============================================================================
// utf8_to_utf16 / utf16_to_utf8 edge cases (convenience functions)
// ============================================================================

TEST(ConvenienceTest, UTF8ToUTF16_Roundtrip_CJK) {
  const char utf8_str[] = "你好世界";
#if defined(_WIN32)
  auto utf16_str = utfx::utf8_to_utf16(utf8_str);
  auto back_to_utf8 = utfx::utf16_to_utf8(utf16_str);
  EXPECT_EQ(back_to_utf8, utf8_str);
  // Also roundtrip the other way
  EXPECT_EQ(utfx::utf8_to_utf16(back_to_utf8), utf16_str);
#else
  auto utf16_str = utfx::utf8_to_utf16(utf8_str);
  auto back_to_utf8 = utfx::utf16_to_utf8(utf16_str);
  EXPECT_EQ(back_to_utf8, utf8_str);
  EXPECT_EQ(utfx::utf8_to_utf16(back_to_utf8), utf16_str);
#endif
}

TEST(ConvenienceTest, UTF8ToUTF16_Roundtrip_Emoji) {
  const char utf8_str[] = "🔥🎉😀🚀";
#if defined(_WIN32)
  auto utf16_str = utfx::utf8_to_utf16(utf8_str);
  auto back_to_utf8 = utfx::utf16_to_utf8(utf16_str);
  EXPECT_EQ(back_to_utf8, utf8_str);
#else
  auto utf16_str = utfx::utf8_to_utf16(utf8_str);
  auto back_to_utf8 = utfx::utf16_to_utf8(utf16_str);
  EXPECT_EQ(back_to_utf8, utf8_str);
#endif
}

TEST(ConvenienceTest, UTF8ToUTF16_Roundtrip_SupplementaryEmoji) {
  // U+1F600 grinning face (4-byte UTF-8, surrogate pair in UTF-16)
  const char utf8_str[] = "\xF0\x9F\x98\x80";
#if defined(_WIN32)
  auto utf16_str = utfx::utf8_to_utf16(utf8_str);
  auto back_to_utf8 = utfx::utf16_to_utf8(utf16_str);
  EXPECT_EQ(back_to_utf8, utf8_str);
#else
  auto utf16_str = utfx::utf8_to_utf16(utf8_str);
  auto back_to_utf8 = utfx::utf16_to_utf8(utf16_str);
  EXPECT_EQ(back_to_utf8, utf8_str);
#endif
}

TEST(ConvenienceTest, UTF8ToUTF16_StdString) {
#if defined(_WIN32)
  std::string input = "Test";
  auto result = utfx::utf8_to_utf16(input);
  EXPECT_EQ(result, L"Test");
#else
  std::string input = "Test";
  auto result = utfx::utf8_to_utf16(input);
  EXPECT_EQ(result, u"Test");
#endif
}

TEST(ConvenienceTest, UTF16ToUTF8_StdU16String) {
#if defined(_WIN32)
  std::wstring input = L"Test";
  auto result = utfx::utf16_to_utf8(input);
  EXPECT_EQ(result, "Test");
#else
  std::u16string input = u"Test";
  auto result = utfx::utf16_to_utf8(input);
  EXPECT_EQ(result, "Test");
#endif
}

// ============================================================================
// literals tests
// ============================================================================

TEST(LiteralsTest, UTF8Literal_Basic) {
  using namespace utfx::literals;
  auto result = u"Hi"_utf8;
  EXPECT_EQ(result, "Hi");
}

TEST(LiteralsTest, UTF8Literal_Emoji) {
  using namespace utfx::literals;
  auto result = u"🔥"_utf8;
  EXPECT_EQ(result, "\xF0\x9F\x94\xA5");
}

TEST(LiteralsTest, UTF8Literal_Supplementary) {
  using namespace utfx::literals;
  auto result = u"\xD83D\xDE00"_utf8;  // U+1F600
  EXPECT_EQ(result, "\xF0\x9F\x98\x80");
}

TEST(LiteralsTest, UTF16Literal_Basic) {
  using namespace utfx::literals;
  auto result = "Hi"_utf16;
  EXPECT_EQ(result, u"Hi");
}

TEST(LiteralsTest, UTF16Literal_Emoji) {
  using namespace utfx::literals;
  auto result = "🔥"_utf16;
  EXPECT_EQ(result, u"🔥");
}

TEST(LiteralsTest, UTF16Literal_Supplementary) {
  using namespace utfx::literals;
  auto result = "\xF0\x9F\x98\x80"_utf16;  // U+1F600
  EXPECT_EQ(result.size(), 2u);
  EXPECT_EQ(static_cast<uint16_t>(result[0]), 0xD83Du);
  EXPECT_EQ(static_cast<uint16_t>(result[1]), 0xDE00u);
}

#if defined(__cpp_lib_char8_t)
TEST(LiteralsTest, UTF16Literal_Char8T) {
  using namespace utfx::literals;
  auto result = u8"Hi"_utf16;
  EXPECT_EQ(result, u"Hi");

  auto result2 = u8"🔥"_utf16;
  EXPECT_EQ(result2, u"🔥");
}
#endif

#if defined(_WIN32)
TEST(LiteralsTest, UTF8Literal_WideString) {
  using namespace utfx::literals;
  auto result = L"Hi"_utf8;
  EXPECT_EQ(result, "Hi");

  auto result2 = L"🔥"_utf8;
  EXPECT_EQ(result2, "\xF0\x9F\x94\xA5");
}
#endif

// ============================================================================
// transcode string version (returns std::basic_string)
// ============================================================================

TEST(TranscodeStringTest, UTF8ToUTF16_String) {
  const char input[] = "Hello";
  auto result =
      utfx::transcode<char16_t>(input, input + 5, utfx::endian::native);
  EXPECT_EQ(result, u"Hello");
}

TEST(TranscodeStringTest, UTF16ToUTF8_String) {
  const char16_t input[] = u"Hello";
  auto result = utfx::transcode<char>(input, input + 5, utfx::endian::native);
  EXPECT_EQ(result, "Hello");
}

TEST(TranscodeStringTest, UTF16ToUTF32_String) {
  const char16_t input[] = u"AB";
  auto result = utfx::transcode<char32_t>(
      input, input + 2, utfx::endian::native, utfx::endian::native);
  EXPECT_EQ(result.size(), 2u);
  EXPECT_EQ(result[0], U'A');
  EXPECT_EQ(result[1], U'B');
}

TEST(TranscodeStringTest, UTF32ToUTF16_String) {
  const char32_t input[] = U"AB";
  auto result = utfx::transcode<char16_t>(
      input, input + 2, utfx::endian::native, utfx::endian::native);
  EXPECT_EQ(result.size(), 2u);
  EXPECT_EQ(result[0], u'A');
  EXPECT_EQ(result[1], u'B');
}

TEST(TranscodeStringTest, UTF8ToUTF16_NullChar) {
  // String containing null character (U+0000)
  const char input[] =
      "A\0"
      "B";
  auto result =
      utfx::transcode<char16_t>(input, input + 3, utfx::endian::native);
  EXPECT_EQ(result.size(), 3u);
  EXPECT_EQ(result[0], u'A');
  EXPECT_EQ(result[1], u'\0');
  EXPECT_EQ(result[2], u'B');
}

// ============================================================================
// cross-type transcode
// ============================================================================

TEST(CrossTypeTest, UTF8ToUTF32) {
  const char input[] = "\xE4\xBD\xA0";  // U+4F60
  auto result =
      utfx::transcode<char32_t>(input, input + 3, utfx::endian::native);
  EXPECT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0], U'\u4F60');
}

TEST(CrossTypeTest, UTF32ToUTF8) {
  const char32_t input[] = U"\u4F60";
  auto result = utfx::transcode<char>(input, input + 1, utfx::endian::native);
  EXPECT_EQ(result, "\xE4\xBD\xA0");
}

TEST(CrossTypeTest, UTF32ToUTF8_Supplementary) {
  const char32_t input[] = {0x1F600, 0x0000};
  auto result = utfx::transcode<char>(input, input + 1, utfx::endian::native);
  EXPECT_EQ(result, "\xF0\x9F\x98\x80");
}

TEST(CrossTypeTest, UTF8ToUTF32_Supplementary) {
  const char input[] = "\xF0\x9F\x98\x80";  // U+1F600
  auto result =
      utfx::transcode<char32_t>(input, input + 4, utfx::endian::native);
  EXPECT_EQ(result.size(), 1u);
  EXPECT_EQ(static_cast<uint32_t>(result[0]), 0x1F600u);
}

// ============================================================================
// Complex roundtrip tests
// ============================================================================

TEST(RoundtripTest, UTF8_UTF16_UTF8) {
  // All BMP range
  const char original[] = "Hello, 你好, 🔥!";
  auto utf16 = utfx::transcode<char16_t>(original, original + strlen(original),
                                         utfx::endian::native);
  auto back = utfx::transcode<char>(utf16.data(), utf16.data() + utf16.size(),
                                    utfx::endian::native);
  EXPECT_EQ(back, original);
}

TEST(RoundtripTest, UTF8_UTF32_UTF8) {
  const char original[] = "Test: \xF0\x9F\x98\x80\xE4\xBD\xA0!";
  auto utf32 = utfx::transcode<char32_t>(original, original + strlen(original),
                                         utfx::endian::native);
  auto back = utfx::transcode<char>(utf32.data(), utf32.data() + utf32.size(),
                                    utfx::endian::native);
  EXPECT_EQ(back, original);
}

TEST(RoundtripTest, UTF16_UTF32_UTF16) {
  const char16_t original[] = u"Hi \xD83D\xDE00!";  // "Hi 😀!"
  auto utf32 = utfx::transcode<char32_t>(
      original, original + 6, utfx::endian::native, utfx::endian::native);
  auto back =
      utfx::transcode<char16_t>(utf32.data(), utf32.data() + utf32.size(),
                                utfx::endian::native, utfx::endian::native);
  EXPECT_EQ(back.size(), 6u);
  for (size_t i = 0; i < 6; i++) {
    EXPECT_EQ(back[i], original[i]);
  }
}

// ============================================================================
// Invalid input handling in transcode (skipped silently)
// ============================================================================

TEST(InvalidHandlingTest, Transcode_SkipsIllegalBytes) {
  // Invalid UTF-8 byte in the middle of valid input
  const char input[] =
      "A\xFF"
      "B\xFE"
      "C";
  auto result =
      utfx::transcode<char16_t>(input, input + 5, utfx::endian::native);
  EXPECT_EQ(result.size(), 3u);
  EXPECT_EQ(result[0], u'A');
  EXPECT_EQ(result[1], u'B');
  EXPECT_EQ(result[2], u'C');
}

TEST(InvalidHandlingTest, Transcode_HandlesIncompleteAtEnd) {
  // Truncated 3-byte sequence at the end
  const char input[] = "AB\xE4\xBD";  // Incomplete
  auto result =
      utfx::transcode<char16_t>(input, input + 4, utfx::endian::native);
  EXPECT_EQ(result.size(), 2u);
  EXPECT_EQ(result[0], u'A');
  EXPECT_EQ(result[1], u'B');
}

TEST(InvalidHandlingTest, UTF16ToUTF8_SkipsUnpairedSurrogate) {
  // Lone second surrogate
  const char16_t input[] = {u'A', 0xDE00, u'B', 0x0000};
  auto result = utfx::transcode<char>(input, input + 3, utfx::endian::native);
  EXPECT_EQ(result, "AB");
}
