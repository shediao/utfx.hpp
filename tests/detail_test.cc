#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <utfx/utfx.hpp>

using namespace utfx::detail;

// ============================================================================
// is_valid_codepoint tests
// ============================================================================

TEST(DetailTest, IsValidCodepoint_Boundaries) {
  // Valid ASCII range
  EXPECT_TRUE(is_valid_codepoint(0x0000));
  EXPECT_TRUE(is_valid_codepoint(0x007F));
  EXPECT_TRUE(is_valid_codepoint(0x0080));

  // Just before surrogate range
  EXPECT_TRUE(is_valid_codepoint(0xD7FF));

  // Surrogate range (invalid)
  EXPECT_FALSE(is_valid_codepoint(0xD800));
  EXPECT_FALSE(is_valid_codepoint(0xD8FF));
  EXPECT_FALSE(is_valid_codepoint(0xDBFF));
  EXPECT_FALSE(is_valid_codepoint(0xDC00));
  EXPECT_FALSE(is_valid_codepoint(0xDFFF));

  // Just after surrogate range
  EXPECT_TRUE(is_valid_codepoint(0xE000));

  // BMP maximum
  EXPECT_TRUE(is_valid_codepoint(0xFFFF));

  // Supplementary plane
  EXPECT_TRUE(is_valid_codepoint(0x10000));
  EXPECT_TRUE(is_valid_codepoint(0x10FFFF));

  // Beyond max
  EXPECT_FALSE(is_valid_codepoint(0x110000));
  EXPECT_FALSE(is_valid_codepoint(0xFFFFFFFF));
  EXPECT_FALSE(is_valid_codepoint(0x1FFFFF));
}

TEST(DetailTest, IsValidCodepoint_EdgeCases) {
  // Common invalid values
  EXPECT_FALSE(is_valid_codepoint(illegal));
  EXPECT_FALSE(is_valid_codepoint(incomplete));
}

// ============================================================================
// swap_bytes tests
// ============================================================================

TEST(DetailTest, SwapBytes_Uint16) {
  EXPECT_EQ(swap_bytes<uint16_t>(0x0000), 0x0000);
  EXPECT_EQ(swap_bytes<uint16_t>(0xFFFF), 0xFFFF);
  EXPECT_EQ(swap_bytes<uint16_t>(0x1234), 0x3412);
  EXPECT_EQ(swap_bytes<uint16_t>(0x00FF), 0xFF00);
  EXPECT_EQ(swap_bytes<uint16_t>(0xFF00), 0x00FF);
  EXPECT_EQ(swap_bytes<uint16_t>(0xAA55), 0x55AA);
}

TEST(DetailTest, SwapBytes_Uint32) {
  EXPECT_EQ(swap_bytes<uint32_t>(0x00000000), 0x00000000u);
  EXPECT_EQ(swap_bytes<uint32_t>(0xFFFFFFFF), 0xFFFFFFFFu);
  EXPECT_EQ(swap_bytes<uint32_t>(0x12345678), 0x78563412u);
  EXPECT_EQ(swap_bytes<uint32_t>(0x000000FF), 0xFF000000u);
  EXPECT_EQ(swap_bytes<uint32_t>(0xFF000000), 0x000000FFu);
  EXPECT_EQ(swap_bytes<uint32_t>(0xAABBCCDD), 0xDDCCBBAAu);
}

TEST(DetailTest, SwapBytes_Uint64) {
  EXPECT_EQ(swap_bytes<uint64_t>(0x0000000000000000ULL), 0x0000000000000000ULL);
  EXPECT_EQ(swap_bytes<uint64_t>(0xFFFFFFFFFFFFFFFFULL), 0xFFFFFFFFFFFFFFFFULL);
  EXPECT_EQ(swap_bytes<uint64_t>(0x0123456789ABCDEFULL), 0xEFCDAB8967452301ULL);
  EXPECT_EQ(swap_bytes<uint64_t>(0x00000000000000FFULL), 0xFF00000000000000ULL);
}

TEST(DetailTest, SwapBytes_Roundtrip) {
  // Double swap should return original
  for (uint16_t v : {uint16_t{0x0000}, uint16_t{0x1234}, uint16_t{0xFFFF},
                     uint16_t{0x00FF}, uint16_t{0xFF00}, uint16_t{0xAA55}}) {
    EXPECT_EQ(swap_bytes(swap_bytes(v)), v);
  }
  for (uint32_t v :
       {0x00000000u, 0x12345678u, 0xFFFFFFFFu, 0x00FF00FFu, 0xDEADBEEFu}) {
    EXPECT_EQ(swap_bytes(swap_bytes(v)), v);
  }
  for (uint64_t v :
       {0x0000000000000000ULL, 0x0123456789ABCDEFULL, 0xFFFFFFFFFFFFFFFFULL}) {
    EXPECT_EQ(swap_bytes(swap_bytes(v)), v);
  }
}

// ============================================================================
// utf_traits<char, 1> (UTF-8) tests
// ============================================================================

using utf8_traits = utf_traits<char, 1>;

TEST(DetailUTF8, TrailLength) {
  // ASCII range: 0
  EXPECT_EQ(utf8_traits::trail_length('\x00'), 0);
  EXPECT_EQ(utf8_traits::trail_length('\x7F'), 0);

  // Trail bytes / invalid: -1
  EXPECT_EQ(utf8_traits::trail_length('\x80'), -1);
  EXPECT_EQ(utf8_traits::trail_length('\xBF'), -1);
  EXPECT_EQ(utf8_traits::trail_length('\xC0'), -1);  // Invalid lead
  EXPECT_EQ(utf8_traits::trail_length('\xC1'), -1);  // Invalid lead

  // 2-byte lead: 1
  EXPECT_EQ(utf8_traits::trail_length('\xC2'), 1);
  EXPECT_EQ(utf8_traits::trail_length('\xDF'), 1);

  // 3-byte lead: 2
  EXPECT_EQ(utf8_traits::trail_length('\xE0'), 2);
  EXPECT_EQ(utf8_traits::trail_length('\xEF'), 2);

  // 4-byte lead: 3
  EXPECT_EQ(utf8_traits::trail_length('\xF0'), 3);
  EXPECT_EQ(utf8_traits::trail_length('\xF4'), 3);

  // Beyond RFC 3629: -1
  EXPECT_EQ(utf8_traits::trail_length('\xF5'), -1);
  EXPECT_EQ(utf8_traits::trail_length('\xFF'), -1);
}

TEST(DetailUTF8, Width) {
  // 1 byte: U+0000 to U+007F
  EXPECT_EQ(utf8_traits::width(0x0000), 1);
  EXPECT_EQ(utf8_traits::width(0x007F), 1);

  // 2 bytes: U+0080 to U+07FF
  EXPECT_EQ(utf8_traits::width(0x0080), 2);
  EXPECT_EQ(utf8_traits::width(0x07FF), 2);

  // 3 bytes: U+0800 to U+FFFF
  EXPECT_EQ(utf8_traits::width(0x0800), 3);
  EXPECT_EQ(utf8_traits::width(0xFFFF), 3);

  // 4 bytes: U+10000 to U+10FFFF
  EXPECT_EQ(utf8_traits::width(0x10000), 4);
  EXPECT_EQ(utf8_traits::width(0x10FFFF), 4);
}

TEST(DetailUTF8, IsTrail) {
  // Trail bytes
  EXPECT_TRUE(utf8_traits::is_trail('\x80'));
  EXPECT_TRUE(utf8_traits::is_trail('\xBF'));
  EXPECT_TRUE(utf8_traits::is_trail('\xA0'));

  // Non-trail bytes
  EXPECT_FALSE(utf8_traits::is_trail('\x00'));
  EXPECT_FALSE(utf8_traits::is_trail('\x7F'));
  EXPECT_FALSE(utf8_traits::is_trail('\xC0'));
  EXPECT_FALSE(utf8_traits::is_trail('\xC2'));
  EXPECT_FALSE(utf8_traits::is_trail('\xE0'));
  EXPECT_FALSE(utf8_traits::is_trail('\xF0'));
}

TEST(DetailUTF8, IsLead) {
  // is_lead is !is_trail
  EXPECT_FALSE(utf8_traits::is_lead('\x80'));
  EXPECT_FALSE(utf8_traits::is_lead('\xBF'));

  EXPECT_TRUE(utf8_traits::is_lead('\x00'));
  EXPECT_TRUE(utf8_traits::is_lead('\x7F'));
  EXPECT_TRUE(utf8_traits::is_lead('\xC2'));
  EXPECT_TRUE(utf8_traits::is_lead('\xE0'));
  EXPECT_TRUE(utf8_traits::is_lead('\xF0'));
}

TEST(DetailUTF8, Decode_ASCII) {
  const char input[] = "Hello";
  const char* p = input;
  const char* e = input + 5;
  EXPECT_EQ(utf8_traits::decode(p, e), 'H');
  EXPECT_EQ(utf8_traits::decode(p, e), 'e');
  EXPECT_EQ(utf8_traits::decode(p, e), 'l');
  EXPECT_EQ(utf8_traits::decode(p, e), 'l');
  EXPECT_EQ(utf8_traits::decode(p, e), 'o');
  EXPECT_EQ(p, e);
}

TEST(DetailUTF8, Decode_TwoByte) {
  // U+00A2 = C2 A2 (cent sign)
  const char input[] = "\xC2\xA2";
  const char* p = input;
  const char* e = input + 2;
  EXPECT_EQ(utf8_traits::decode(p, e), 0xA2u);
  EXPECT_EQ(p, e);

  // U+07FF = DF BF (max 2-byte)
  const char input2[] = "\xDF\xBF";
  p = input2;
  e = input2 + 2;
  EXPECT_EQ(utf8_traits::decode(p, e), 0x07FFu);
  EXPECT_EQ(p, e);

  // U+0080 = C2 80 (min 2-byte)
  const char input3[] = "\xC2\x80";
  p = input3;
  e = input3 + 2;
  EXPECT_EQ(utf8_traits::decode(p, e), 0x0080u);
  EXPECT_EQ(p, e);
}

TEST(DetailUTF8, Decode_ThreeByte) {
  // U+0800 = E0 A0 80 (min 3-byte)
  const char input[] = "\xE0\xA0\x80";
  const char* p = input;
  const char* e = input + 3;
  EXPECT_EQ(utf8_traits::decode(p, e), 0x0800u);
  EXPECT_EQ(p, e);

  // U+FFFF = EF BF BF (max 3-byte)
  const char input2[] = "\xEF\xBF\xBF";
  p = input2;
  e = input2 + 3;
  EXPECT_EQ(utf8_traits::decode(p, e), 0xFFFFu);
  EXPECT_EQ(p, e);

  // Some CJK character: U+4F60 = E4 BD A0
  const char input3[] = "\xE4\xBD\xA0";
  p = input3;
  e = input3 + 3;
  EXPECT_EQ(utf8_traits::decode(p, e), 0x4F60u);
  EXPECT_EQ(p, e);
}

TEST(DetailUTF8, Decode_FourByte) {
  // U+10000 = F0 90 80 80 (min 4-byte)
  const char input[] = "\xF0\x90\x80\x80";
  const char* p = input;
  const char* e = input + 4;
  EXPECT_EQ(utf8_traits::decode(p, e), 0x10000u);
  EXPECT_EQ(p, e);

  // U+10FFFF = F4 8F BF BF (max 4-byte)
  const char input2[] = "\xF4\x8F\xBF\xBF";
  p = input2;
  e = input2 + 4;
  EXPECT_EQ(utf8_traits::decode(p, e), 0x10FFFFu);
  EXPECT_EQ(p, e);

  // Emoji: U+1F600 = F0 9F 98 80 (grinning face)
  const char input3[] = "\xF0\x9F\x98\x80";
  p = input3;
  e = input3 + 4;
  EXPECT_EQ(utf8_traits::decode(p, e), 0x1F600u);
  EXPECT_EQ(p, e);
}

TEST(DetailUTF8, Decode_EmptyInput) {
  // When p == e, decode returns incomplete
  const char* p2 = "";
  const char* e2 = "";
  EXPECT_EQ(utf8_traits::decode(p2, e2), incomplete);
}

TEST(DetailUTF8, Decode_IncompleteSequences) {
  // 2-byte sequence missing trail byte
  {
    const char input[] = "\xC2";
    const char* p = input;
    const char* e = input + 1;
    EXPECT_EQ(utf8_traits::decode(p, e), incomplete);
  }
  // 3-byte sequence missing 2 trail bytes
  {
    const char input[] = "\xE0";
    const char* p = input;
    const char* e = input + 1;
    EXPECT_EQ(utf8_traits::decode(p, e), incomplete);
  }
  // 3-byte sequence missing 1 trail byte
  {
    const char input[] = "\xE0\xA0";
    const char* p = input;
    const char* e = input + 2;
    EXPECT_EQ(utf8_traits::decode(p, e), incomplete);
  }
  // 4-byte sequence missing trail bytes
  {
    const char input[] = "\xF0";
    const char* p = input;
    const char* e = input + 1;
    EXPECT_EQ(utf8_traits::decode(p, e), incomplete);
  }
  {
    const char input[] = "\xF0\x90";
    const char* p = input;
    const char* e = input + 2;
    EXPECT_EQ(utf8_traits::decode(p, e), incomplete);
  }
  {
    const char input[] = "\xF0\x90\x80";
    const char* p = input;
    const char* e = input + 3;
    EXPECT_EQ(utf8_traits::decode(p, e), incomplete);
  }
}

TEST(DetailUTF8, Decode_InvalidTrailBytes) {
  // Lead byte followed by ASCII byte (not a valid trail byte)
  // trail_length for ASCII (0x00-0x7F) returns 0, and !0 == true => illegal
  {
    const char input[] = "\xC2\x41";  // 0xC2 followed by 'A' (0x41)
    const char* p = input;
    const char* e = input + 2;
    EXPECT_EQ(utf8_traits::decode(p, e), illegal);
  }
  // 3-byte lead followed by ASCII in second position
  {
    const char input[] = "\xE0\x41\x80";  // 0xE0 + 'A' + 0x80
    const char* p2 = input;
    const char* e2 = input + 3;
    EXPECT_EQ(utf8_traits::decode(p2, e2), illegal);
  }
  // Lead byte followed by a byte in 0x80-0xBF range IS a valid trail byte,
  // but bytes like 0xC0 (which has trail_length == -1) still pass the
  // !trail_length check since -1 is non-zero. The real invalid case is
  // an ASCII byte (trail_length == 0).
}

TEST(DetailUTF8, Decode_OverlongSequences) {
  // Overlong 2-byte: U+0000 encoded as C0 80 (should be 00)
  {
    const char input[] = "\xC0\x80";
    const char* p = input;
    const char* e = input + 2;
    EXPECT_EQ(utf8_traits::decode(p, e), illegal);
  }
  // Overlong 2-byte: U+007F encoded as C1 BF (should be 7F)
  {
    const char input[] = "\xC1\xBF";
    const char* p = input;
    const char* e = input + 2;
    EXPECT_EQ(utf8_traits::decode(p, e), illegal);
  }
  // Overlong 3-byte: U+07FF encoded as E0 9F BF
  {
    const char input[] = "\xE0\x9F\xBF";
    const char* p = input;
    const char* e = input + 3;
    EXPECT_EQ(utf8_traits::decode(p, e), illegal);
  }
  // Overlong 4-byte: U+FFFF encoded as F0 8F BF BF
  {
    const char input[] = "\xF0\x8F\xBF\xBF";
    const char* p = input;
    const char* e = input + 4;
    EXPECT_EQ(utf8_traits::decode(p, e), illegal);
  }
}

TEST(DetailUTF8, Decode_SurrogateCodepoints) {
  // U+D800 encoded as ED A0 80
  {
    const char input[] = "\xED\xA0\x80";
    const char* p = input;
    const char* e = input + 3;
    EXPECT_EQ(utf8_traits::decode(p, e), illegal);
  }
  // U+DFFF encoded as ED BF BF
  {
    const char input[] = "\xED\xBF\xBF";
    const char* p = input;
    const char* e = input + 3;
    EXPECT_EQ(utf8_traits::decode(p, e), illegal);
  }
}

TEST(DetailUTF8, Decode_BeyondMaxCodepoint) {
  // U+110000 encoded as F4 90 80 80
  {
    const char input[] = "\xF4\x90\x80\x80";
    const char* p = input;
    const char* e = input + 4;
    EXPECT_EQ(utf8_traits::decode(p, e), illegal);
  }
  // U+13FFFF as F4 BF BF BF
  {
    const char input[] = "\xF4\xBF\xBF\xBF";
    const char* p = input;
    const char* e = input + 4;
    EXPECT_EQ(utf8_traits::decode(p, e), illegal);
  }
}

TEST(DetailUTF8, Decode_MultipleSequences) {
  // Mixed ASCII and multi-byte: "A" + U+00A2 + "B" + U+4F60
  const char input[] =
      "A\xC2\xA2"
      "B\xE4\xBD\xA0";
  const char* p = input;
  const char* e = input + sizeof(input) - 1;
  EXPECT_EQ(utf8_traits::decode(p, e), 'A');
  EXPECT_EQ(utf8_traits::decode(p, e), 0xA2u);
  EXPECT_EQ(utf8_traits::decode(p, e), 'B');
  EXPECT_EQ(utf8_traits::decode(p, e), 0x4F60u);
  EXPECT_EQ(p, e);
}

TEST(DetailUTF8, DecodeValid_Basic) {
  // decode_valid skips validation - should still correctly decode valid input
  {
    const char input[] = "\xC2\xA2";
    const char* p = input;
    EXPECT_EQ(utf8_traits::decode_valid(p), 0xA2u);
    EXPECT_EQ(p, input + 2);
  }
  {
    const char input[] = "\xE4\xBD\xA0";
    const char* p = input;
    EXPECT_EQ(utf8_traits::decode_valid(p), 0x4F60u);
    EXPECT_EQ(p, input + 3);
  }
  {
    const char input[] = "\xF0\x9F\x98\x80";
    const char* p = input;
    EXPECT_EQ(utf8_traits::decode_valid(p), 0x1F600u);
    EXPECT_EQ(p, input + 4);
  }
  // ASCII
  {
    const char input[] = "A";
    const char* p = input;
    EXPECT_EQ(utf8_traits::decode_valid(p), 'A');
  }
}

TEST(DetailUTF8, Encode_ASCII) {
  char buf[4] = {};
  char* p = utf8_traits::encode(0x41, buf);
  EXPECT_EQ(p - buf, 1);
  EXPECT_EQ(buf[0], 'A');
}

TEST(DetailUTF8, Encode_TwoByte) {
  char buf[4] = {};
  // U+00A2 = C2 A2
  char* p = utf8_traits::encode(0xA2, buf);
  EXPECT_EQ(p - buf, 2);
  EXPECT_EQ(static_cast<unsigned char>(buf[0]), 0xC2);
  EXPECT_EQ(static_cast<unsigned char>(buf[1]), 0xA2);

  // U+07FF = DF BF
  char buf2[4] = {};
  p = utf8_traits::encode(0x7FF, buf2);
  EXPECT_EQ(p - buf2, 2);
  EXPECT_EQ(static_cast<unsigned char>(buf2[0]), 0xDF);
  EXPECT_EQ(static_cast<unsigned char>(buf2[1]), 0xBF);
}

TEST(DetailUTF8, Encode_ThreeByte) {
  char buf[4] = {};
  // U+0800 = E0 A0 80
  char* p = utf8_traits::encode(0x800, buf);
  EXPECT_EQ(p - buf, 3);
  EXPECT_EQ(static_cast<unsigned char>(buf[0]), 0xE0);
  EXPECT_EQ(static_cast<unsigned char>(buf[1]), 0xA0);
  EXPECT_EQ(static_cast<unsigned char>(buf[2]), 0x80);

  // U+FFFF = EF BF BF
  char buf2[4] = {};
  p = utf8_traits::encode(0xFFFF, buf2);
  EXPECT_EQ(p - buf2, 3);
  EXPECT_EQ(static_cast<unsigned char>(buf2[0]), 0xEF);
  EXPECT_EQ(static_cast<unsigned char>(buf2[1]), 0xBF);
  EXPECT_EQ(static_cast<unsigned char>(buf2[2]), 0xBF);
}

TEST(DetailUTF8, Encode_FourByte) {
  char buf[4] = {};
  // U+10000 = F0 90 80 80
  char* p = utf8_traits::encode(0x10000, buf);
  EXPECT_EQ(p - buf, 4);
  EXPECT_EQ(static_cast<unsigned char>(buf[0]), 0xF0);
  EXPECT_EQ(static_cast<unsigned char>(buf[1]), 0x90);
  EXPECT_EQ(static_cast<unsigned char>(buf[2]), 0x80);
  EXPECT_EQ(static_cast<unsigned char>(buf[3]), 0x80);

  // U+10FFFF = F4 8F BF BF
  char buf2[4] = {};
  p = utf8_traits::encode(0x10FFFF, buf2);
  EXPECT_EQ(p - buf2, 4);
  EXPECT_EQ(static_cast<unsigned char>(buf2[0]), 0xF4);
  EXPECT_EQ(static_cast<unsigned char>(buf2[1]), 0x8F);
  EXPECT_EQ(static_cast<unsigned char>(buf2[2]), 0xBF);
  EXPECT_EQ(static_cast<unsigned char>(buf2[3]), 0xBF);
}

TEST(DetailUTF8, EncodeDecode_Roundtrip) {
  // Test that encode + decode roundtrips correctly for various codepoints
  const uint32_t codepoints[] = {0x00,  0x7F,   0x80,    0xA2,    0x7FF,
                                 0x800, 0xFFFF, 0x10000, 0x1F600, 0x10FFFF};
  for (auto cp : codepoints) {
    char buf[4] = {};
    char* end = utf8_traits::encode(cp, buf);
    char* p = buf;
    auto decoded = utf8_traits::decode(p, end);
    EXPECT_EQ(decoded, cp) << "Roundtrip failed for U+" << std::hex << cp;
    EXPECT_EQ(p, end);
  }
}

// ============================================================================
// utf_traits<char16_t, 2> (UTF-16) tests
// ============================================================================

using utf16_traits = utf_traits<char16_t, 2>;

TEST(DetailUTF16, IsFirstSurrogate) {
  EXPECT_TRUE(utf16_traits::is_first_surrogate(0xD800));
  EXPECT_TRUE(utf16_traits::is_first_surrogate(0xDBFF));
  EXPECT_FALSE(utf16_traits::is_first_surrogate(0xD7FF));
  EXPECT_FALSE(utf16_traits::is_first_surrogate(0xDC00));
  EXPECT_FALSE(utf16_traits::is_first_surrogate(0x0000));
  EXPECT_FALSE(utf16_traits::is_first_surrogate(0xFFFF));
}

TEST(DetailUTF16, IsSecondSurrogate) {
  EXPECT_TRUE(utf16_traits::is_second_surrogate(0xDC00));
  EXPECT_TRUE(utf16_traits::is_second_surrogate(0xDFFF));
  EXPECT_FALSE(utf16_traits::is_second_surrogate(0xDBFF));
  EXPECT_FALSE(utf16_traits::is_second_surrogate(0xE000));
  EXPECT_FALSE(utf16_traits::is_second_surrogate(0x0000));
}

TEST(DetailUTF16, CombineSurrogate) {
  // Minimum surrogate pair: (D800, DC00) -> U+10000
  EXPECT_EQ(utf16_traits::combine_surrogate(0xD800, 0xDC00), 0x10000u);
  // Maximum surrogate pair: (DBFF, DFFF) -> U+10FFFF
  EXPECT_EQ(utf16_traits::combine_surrogate(0xDBFF, 0xDFFF), 0x10FFFFu);
  // Some emoji: U+1F600 = (D83D, DE00)
  EXPECT_EQ(utf16_traits::combine_surrogate(0xD83D, 0xDE00), 0x1F600u);
}

TEST(DetailUTF16, TrailLength) {
  EXPECT_EQ(utf16_traits::trail_length(0xD800), 1);   // First surrogate
  EXPECT_EQ(utf16_traits::trail_length(0xDBFF), 1);   // First surrogate
  EXPECT_EQ(utf16_traits::trail_length(0xDC00), -1);  // Second surrogate alone
  EXPECT_EQ(utf16_traits::trail_length(0xDFFF), -1);  // Second surrogate alone
  EXPECT_EQ(utf16_traits::trail_length(0x0041), 0);   // BMP 'A'
  EXPECT_EQ(utf16_traits::trail_length(0xD7FF), 0);   // BMP max before surr
  EXPECT_EQ(utf16_traits::trail_length(0xE000), 0);   // BMP min after surr
}

TEST(DetailUTF16, IsTrail) {
  EXPECT_TRUE(utf16_traits::is_trail(0xDC00));
  EXPECT_TRUE(utf16_traits::is_trail(0xDFFF));
  EXPECT_FALSE(utf16_traits::is_trail(0xD800));
  EXPECT_FALSE(utf16_traits::is_trail(0x0041));
  EXPECT_FALSE(utf16_traits::is_trail(0x0000));
}

TEST(DetailUTF16, IsLead) {
  EXPECT_FALSE(utf16_traits::is_lead(0xDC00));
  EXPECT_FALSE(utf16_traits::is_lead(0xDFFF));
  EXPECT_TRUE(utf16_traits::is_lead(0xD800));
  EXPECT_TRUE(utf16_traits::is_lead(0x0041));
  EXPECT_TRUE(utf16_traits::is_lead(0x0000));
}

TEST(DetailUTF16, Width) {
  EXPECT_EQ(utf16_traits::width(0x0041), 1);    // BMP
  EXPECT_EQ(utf16_traits::width(0xFFFF), 1);    // BMP max
  EXPECT_EQ(utf16_traits::width(0x10000), 2);   // Suppl min
  EXPECT_EQ(utf16_traits::width(0x10FFFF), 2);  // Suppl max
  EXPECT_EQ(utf16_traits::width(0x1F600), 2);   // Emoji
}

TEST(DetailUTF16, Decode_BMP) {
  const char16_t input[] = u"ABC";
  const char16_t* p = input;
  const char16_t* e = input + 3;
  EXPECT_EQ(utf16_traits::decode(p, e), 'A');
  EXPECT_EQ(utf16_traits::decode(p, e), 'B');
  EXPECT_EQ(utf16_traits::decode(p, e), 'C');
  EXPECT_EQ(p, e);
}

TEST(DetailUTF16, Decode_SurrogatePair) {
  // U+1F600 grinning face = D83D DE00
  const char16_t input[] = {0xD83D, 0xDE00, 0x0000};
  const char16_t* p = input;
  const char16_t* e = input + 2;
  EXPECT_EQ(utf16_traits::decode(p, e), 0x1F600u);
  EXPECT_EQ(p, e);
}

TEST(DetailUTF16, Decode_UnpairedFirstSurrogate) {
  // First surrogate at end of input
  const char16_t input[] = {0xD83D, 0x0000};
  const char16_t* p = input;
  const char16_t* e = input + 1;
  EXPECT_EQ(utf16_traits::decode(p, e), incomplete);
}

TEST(DetailUTF16, Decode_UnpairedSecondSurrogate) {
  // Lone second surrogate
  const char16_t input[] = {0xDE00, 0x0000};
  const char16_t* p = input;
  const char16_t* e = input + 1;
  EXPECT_EQ(utf16_traits::decode(p, e), illegal);
}

TEST(DetailUTF16, Decode_FirstSurrogateWithInvalidSecond) {
  // First surrogate followed by non-surrogate
  const char16_t input[] = {0xD83D, 0x0041, 0x0000};
  const char16_t* p = input;
  const char16_t* e = input + 2;
  EXPECT_EQ(utf16_traits::decode(p, e), illegal);
}

TEST(DetailUTF16, Decode_FirstSurrogateWithFirstSurrogate) {
  // Two first surrogates in a row
  const char16_t input[] = {0xD83D, 0xD83D, 0x0000};
  const char16_t* p = input;
  const char16_t* e = input + 2;
  EXPECT_EQ(utf16_traits::decode(p, e), illegal);
}

TEST(DetailUTF16, Decode_EmptyInput) {
  const char16_t empty[] = {0x0000};
  const char16_t* p = empty;
  const char16_t* e = empty;
  EXPECT_EQ(utf16_traits::decode(p, e), incomplete);
}

TEST(DetailUTF16, Decode_Endian) {
  // On a little-endian machine, big-endian U+1F600 (D83D DE00) is stored
  // as bytes [D8,3D, DE,00], which as LE u16 values = {0x3DD8, 0x00DE}.
  // Read with endian::big byte-swaps each u16 back to {0xD83D, 0xDE00}.
  // On a big-endian machine, native endian already matches.
  const uint16_t raw[] = {0x3DD8, 0x00DE};
  const char16_t* p = reinterpret_cast<const char16_t*>(raw);
  const char16_t* e = p + 2;

  if (utfx::endian::native == utfx::endian::big) {
    // On BE machine, the raw values ARE the BE values
    EXPECT_EQ(utf16_traits::decode(p, e, utfx::endian::native), 0x1F600u);
  } else {
    // On LE machine, specify big endian to byte-swap
    EXPECT_EQ(utf16_traits::decode(p, e, utfx::endian::big), 0x1F600u);
  }
}

TEST(DetailUTF16, DecodeValid_Basic) {
  // BMP
  const char16_t input[] = u"Z";
  const char16_t* p = input;
  EXPECT_EQ(utf16_traits::decode_valid(p), 'Z');

  // Surrogate pair
  const char16_t input2[] = {0xD83D, 0xDE00, 0x0000};
  p = input2;
  EXPECT_EQ(utf16_traits::decode_valid(p), 0x1F600u);
  EXPECT_EQ(p, input2 + 2);
}

TEST(DetailUTF16, Encode_BMP) {
  char16_t buf[2] = {};
  char16_t* p = utf16_traits::encode(0x41, buf);
  EXPECT_EQ(p - buf, 1);
  EXPECT_EQ(buf[0], u'A');
}

TEST(DetailUTF16, Encode_Supplementary) {
  char16_t buf[3] = {};
  // U+1F600 -> D83D DE00
  char16_t* p = utf16_traits::encode(0x1F600, buf);
  EXPECT_EQ(p - buf, 2);
  EXPECT_EQ(static_cast<uint16_t>(buf[0]), 0xD83Du);
  EXPECT_EQ(static_cast<uint16_t>(buf[1]), 0xDE00u);
}

TEST(DetailUTF16, Encode_Endian) {
  char16_t buf[3] = {};
  // Big-endian encoding of U+1F600 on LE: each u16 is byte-swapped
  char16_t* p = utf16_traits::encode(0x1F600, buf, utfx::endian::big);
  EXPECT_EQ(p - buf, 2);

  if (utfx::endian::native == utfx::endian::big) {
    // On BE machine: native = big, no swapping needed
    EXPECT_EQ(static_cast<uint16_t>(buf[0]), 0xD83Du);
    EXPECT_EQ(static_cast<uint16_t>(buf[1]), 0xDE00u);
  } else {
    // On LE machine: bytes are swapped for big-endian output
    EXPECT_EQ(static_cast<uint16_t>(buf[0]), 0x3DD8u);
    EXPECT_EQ(static_cast<uint16_t>(buf[1]), 0x00DEu);
  }
}

TEST(DetailUTF16, EncodeDecode_Roundtrip) {
  const uint32_t codepoints[] = {0x00,  0x41,   0x7F,    0x80,    0x7FF,
                                 0x800, 0xFFFF, 0x10000, 0x1F600, 0x10FFFF};
  for (auto cp : codepoints) {
    char16_t buf[3] = {};
    char16_t* end = utf16_traits::encode(cp, buf);
    char16_t* p = buf;
    auto decoded = utf16_traits::decode(p, end);
    EXPECT_EQ(decoded, cp) << "Roundtrip failed for U+" << std::hex << cp;
    EXPECT_EQ(p, end);
  }
}

// ============================================================================
// utf_traits<char32_t, 4> (UTF-32) tests
// ============================================================================

using utf32_traits = utf_traits<char32_t, 4>;

TEST(DetailUTF32, TrailLength) {
  EXPECT_EQ(utf32_traits::trail_length(0x00), 0);
  EXPECT_EQ(utf32_traits::trail_length(0x10FFFF), 0);
  EXPECT_EQ(utf32_traits::trail_length(0x110000), -1);    // Beyond max
  EXPECT_EQ(utf32_traits::trail_length(0xD800), -1);      // Surrogate
  EXPECT_EQ(utf32_traits::trail_length(0xDFFF), -1);      // Surrogate
  EXPECT_EQ(utf32_traits::trail_length(0xFFFFFFFF), -1);  // Invalid
}

TEST(DetailUTF32, IsTrail) {
  // UTF-32 never has trail bytes
  EXPECT_FALSE(utf32_traits::is_trail(0x00));
  EXPECT_FALSE(utf32_traits::is_trail(0xFFFF));
}

TEST(DetailUTF32, IsLead) {
  // UTF-32 always lead
  EXPECT_TRUE(utf32_traits::is_lead(0x00));
  EXPECT_TRUE(utf32_traits::is_lead(0xFFFF));
}

TEST(DetailUTF32, Width) {
  EXPECT_EQ(utf32_traits::width(0x00), 1);
  EXPECT_EQ(utf32_traits::width(0x10FFFF), 1);
}

TEST(DetailUTF32, Decode_Valid) {
  const char32_t input[] = U"AB";
  const char32_t* p = input;
  const char32_t* e = input + 2;
  EXPECT_EQ(utf32_traits::decode(p, e), U'A');
  EXPECT_EQ(utf32_traits::decode(p, e), U'B');
  EXPECT_EQ(p, e);
}

TEST(DetailUTF32, Decode_Surrogate) {
  const char32_t input[] = {0xD800, 0x0000};
  const char32_t* p = input;
  const char32_t* e = input + 1;
  EXPECT_EQ(utf32_traits::decode(p, e), illegal);
}

TEST(DetailUTF32, Decode_BeyondMax) {
  const char32_t input[] = {0x110000, 0x0000};
  const char32_t* p = input;
  const char32_t* e = input + 1;
  EXPECT_EQ(utf32_traits::decode(p, e), illegal);
}

TEST(DetailUTF32, Decode_EmptyInput) {
  const char32_t empty[] = {0x0000};
  const char32_t* p = empty;
  const char32_t* e = empty;
  EXPECT_EQ(utf32_traits::decode(p, e), incomplete);
}

TEST(DetailUTF32, Decode_Endian) {
  // Big endian representation of U+0041 = 0x41000000 on LE machine
  const uint32_t input_be[] = {0x41000000};
  const char32_t* p = reinterpret_cast<const char32_t*>(input_be);
  const char32_t* e = p + 1;

  if (utfx::endian::native == utfx::endian::big) {
    EXPECT_EQ(utf32_traits::decode(p, e, utfx::endian::native), U'A');
  } else {
    EXPECT_EQ(utf32_traits::decode(p, e, utfx::endian::big), U'A');
  }
}

TEST(DetailUTF32, DecodeValid_Basic) {
  const char32_t input[] = U"X";
  const char32_t* p = input;
  EXPECT_EQ(utf32_traits::decode_valid(p), U'X');

  // decode_valid does not validate codepoints!
  const char32_t input2[] = {0xD800, 0x0000};
  p = input2;
  EXPECT_EQ(utf32_traits::decode_valid(p), 0xD800u);
}

TEST(DetailUTF32, Encode_Basic) {
  char32_t buf[1] = {};
  char32_t* p = utf32_traits::encode(U'A', buf);
  EXPECT_EQ(p - buf, 1);
  EXPECT_EQ(buf[0], U'A');
}

TEST(DetailUTF32, Encode_Endian) {
  char32_t buf[1] = {};
  char32_t* p = utf32_traits::encode(0x41, buf, utfx::endian::big);
  EXPECT_EQ(p - buf, 1);

  if (utfx::endian::native == utfx::endian::big) {
    EXPECT_EQ(buf[0], U'A');
  } else {
    EXPECT_EQ(buf[0], 0x41000000u);
  }
}

TEST(DetailUTF32, EncodeDecode_Roundtrip) {
  const uint32_t codepoints[] = {0x00,     0x41,   0xFFFF, 0x10000,
                                 0x10FFFF, 0xE000, 0x1F600};
  for (auto cp : codepoints) {
    char32_t buf[1] = {};
    char32_t* end = utf32_traits::encode(cp, buf);
    char32_t* p = buf;
    auto decoded = utf32_traits::decode(p, end);
    EXPECT_EQ(decoded, cp) << "Roundtrip failed for U+" << std::hex << cp;
    EXPECT_EQ(p, end);
  }
}
