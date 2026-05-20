#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <string_view>
#include <utfx/utfx.hpp>

// ============================================================================
// utf8_char tests
// ============================================================================

TEST(UTF8CharTest, DefaultConstruction) {
  constexpr utfx::utf8_char c;
  EXPECT_TRUE(c.empty());
  EXPECT_EQ(c.size(), 0u);
  EXPECT_EQ(c.length(), 0u);
  EXPECT_EQ(c.byte_size(), 0u);
  EXPECT_EQ(c.data(), nullptr);
  EXPECT_EQ(c.begin(), nullptr);
  EXPECT_EQ(c.end(), nullptr);
  EXPECT_EQ(c.cbegin(), nullptr);
  EXPECT_EQ(c.cend(), nullptr);
  EXPECT_EQ(c.code_point(), utfx::detail::illegal);
}

TEST(UTF8CharTest, ConstructFromDataAndLength) {
  const char bytes[] = "\xC2\xA2";  // U+00A2
  utfx::utf8_char c(bytes, 2);
  EXPECT_FALSE(c.empty());
  EXPECT_EQ(c.size(), 2u);
  EXPECT_EQ(c.length(), 2u);
  EXPECT_EQ(c.byte_size(), 2u);
  EXPECT_EQ(c.data(), bytes);
}

TEST(UTF8CharTest, DataAndSize_ASCII) {
  const char bytes[] = "A";
  utfx::utf8_char c(bytes, 1);
  EXPECT_EQ(c.size(), 1u);
  EXPECT_EQ(c.byte_size(), 1u);
  EXPECT_EQ(c.data(), bytes);
  EXPECT_EQ(c[0], 'A');
}

TEST(UTF8CharTest, DataAndSize_TwoByte) {
  const char bytes[] = "\xC2\xA2";  // U+00A2 cent sign
  utfx::utf8_char c(bytes, 2);
  EXPECT_EQ(c.size(), 2u);
  EXPECT_EQ(c.byte_size(), 2u);
  EXPECT_EQ(c.data(), bytes);
}

TEST(UTF8CharTest, DataAndSize_ThreeByte) {
  const char bytes[] = "\xE4\xBD\xA0";  // U+4F60 CJK
  utfx::utf8_char c(bytes, 3);
  EXPECT_EQ(c.size(), 3u);
  EXPECT_EQ(c.byte_size(), 3u);
}

TEST(UTF8CharTest, DataAndSize_FourByte) {
  const char bytes[] = "\xF0\x9F\x98\x80";  // U+1F600 grinning face
  utfx::utf8_char c(bytes, 4);
  EXPECT_EQ(c.size(), 4u);
  EXPECT_EQ(c.byte_size(), 4u);
}

TEST(UTF8CharTest, BeginEnd_ASCII) {
  const char bytes[] = "Z";
  utfx::utf8_char c(bytes, 1);
  EXPECT_EQ(c.begin(), bytes);
  EXPECT_EQ(c.end(), bytes + 1);
  EXPECT_EQ(c.cbegin(), bytes);
  EXPECT_EQ(c.cend(), bytes + 1);
}

TEST(UTF8CharTest, BeginEnd_MultiByte) {
  const char bytes[] = "\xE4\xBD\xA0";
  utfx::utf8_char c(bytes, 3);
  EXPECT_EQ(c.begin(), bytes);
  EXPECT_EQ(c.end(), bytes + 3);
}

TEST(UTF8CharTest, CodePoint_ASCII) {
  const char bytes[] = "A";
  utfx::utf8_char c(bytes, 1);
  EXPECT_EQ(c.code_point(), 0x41u);
}

TEST(UTF8CharTest, CodePoint_TwoByte) {
  const char bytes[] = "\xC2\xA2";  // U+00A2
  utfx::utf8_char c(bytes, 2);
  EXPECT_EQ(c.code_point(), 0xA2u);

  const char bytes2[] = "\xDF\xBF";  // U+07FF (max 2-byte)
  utfx::utf8_char c2(bytes2, 2);
  EXPECT_EQ(c2.code_point(), 0x07FFu);
}

TEST(UTF8CharTest, CodePoint_ThreeByte) {
  const char bytes[] = "\xE4\xBD\xA0";  // U+4F60
  utfx::utf8_char c(bytes, 3);
  EXPECT_EQ(c.code_point(), 0x4F60u);

  const char bytes2[] = "\xEF\xBF\xBF";  // U+FFFF (max 3-byte)
  utfx::utf8_char c2(bytes2, 3);
  EXPECT_EQ(c2.code_point(), 0xFFFFu);
}

TEST(UTF8CharTest, CodePoint_FourByte) {
  const char bytes[] = "\xF0\x9F\x98\x80";  // U+1F600
  utfx::utf8_char c(bytes, 4);
  EXPECT_EQ(c.code_point(), 0x1F600u);

  const char bytes2[] = "\xF4\x8F\xBF\xBF";  // U+10FFFF (max)
  utfx::utf8_char c2(bytes2, 4);
  EXPECT_EQ(c2.code_point(), 0x10FFFFu);
}

TEST(UTF8CharTest, CodePoint_Empty) {
  utfx::utf8_char c;
  EXPECT_EQ(c.code_point(), utfx::detail::illegal);
}

TEST(UTF8CharTest, SubscriptOperator) {
  const char bytes[] = "\xF0\x9F\x98\x80";
  utfx::utf8_char c(bytes, 4);
  EXPECT_EQ(static_cast<unsigned char>(c[0]), 0xF0u);
  EXPECT_EQ(static_cast<unsigned char>(c[1]), 0x9Fu);
  EXPECT_EQ(static_cast<unsigned char>(c[2]), 0x98u);
  EXPECT_EQ(static_cast<unsigned char>(c[3]), 0x80u);
}

TEST(UTF8CharTest, Empty) {
  utfx::utf8_char empty_char;
  EXPECT_TRUE(empty_char.empty());

  utfx::utf8_char non_empty("A", 1);
  EXPECT_FALSE(non_empty.empty());
}

TEST(UTF8CharTest, Equality_BothASCII) {
  utfx::utf8_char a("A", 1);
  utfx::utf8_char b("A", 1);
  utfx::utf8_char c("B", 1);
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
  EXPECT_FALSE(a == c);
  EXPECT_TRUE(a != c);
}

TEST(UTF8CharTest, Equality_SameCodePointDifferentBytes) {
  // Both encode U+00A2 but with different raw bytes (although same here)
  utfx::utf8_char a("\xC2\xA2", 2);
  utfx::utf8_char b("\xC2\xA2", 2);
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
}

TEST(UTF8CharTest, Equality_DifferentCodePoints) {
  utfx::utf8_char ascii_A("A", 1);
  utfx::utf8_char cent("\xC2\xA2", 2);
  utfx::utf8_char cjk("\xE4\xBD\xA0", 3);
  utfx::utf8_char emoji("\xF0\x9F\x98\x80", 4);

  EXPECT_FALSE(ascii_A == cent);
  EXPECT_FALSE(ascii_A == cjk);
  EXPECT_FALSE(ascii_A == emoji);
  EXPECT_FALSE(cent == cjk);
  EXPECT_FALSE(cjk == emoji);
}

TEST(UTF8CharTest, Ordering_ASCII) {
  utfx::utf8_char a("A", 1);  // U+0041
  utfx::utf8_char b("B", 1);  // U+0042
  utfx::utf8_char z("Z", 1);  // U+005A

  EXPECT_TRUE(a < b);
  EXPECT_TRUE(a <= b);
  EXPECT_TRUE(a <= a);
  EXPECT_TRUE(b > a);
  EXPECT_TRUE(b >= a);
  EXPECT_TRUE(b >= b);

  EXPECT_TRUE(a < z);
  EXPECT_TRUE(z > a);
}

TEST(UTF8CharTest, Ordering_MixedWidth) {
  utfx::utf8_char ascii("\x7F", 1);                  // U+007F
  utfx::utf8_char two_byte("\xC2\x80", 2);           // U+0080 (min 2-byte)
  utfx::utf8_char three_byte("\xE0\xA0\x80", 3);     // U+0800
  utfx::utf8_char four_byte("\xF0\x90\x80\x80", 4);  // U+10000

  EXPECT_TRUE(ascii < two_byte);
  EXPECT_TRUE(two_byte < three_byte);
  EXPECT_TRUE(three_byte < four_byte);

  EXPECT_TRUE(four_byte > three_byte);
  EXPECT_TRUE(three_byte > two_byte);
  EXPECT_TRUE(two_byte > ascii);
}

TEST(UTF8CharTest, Ordering_SameCodePoint) {
  utfx::utf8_char a("\xC2\xA2", 2);
  utfx::utf8_char b("\xC2\xA2", 2);
  EXPECT_FALSE(a < b);
  EXPECT_FALSE(b < a);
  EXPECT_TRUE(a <= b);
  EXPECT_TRUE(a >= b);
}

// ============================================================================
// utf8_view iterator tests
// ============================================================================

TEST(UTF8ViewIteratorTest, DefaultConstruction) {
  utfx::utf8_view::iterator it;
  // Default-constructed iterators should compare equal
  utfx::utf8_view::iterator it2;
  EXPECT_TRUE(it == it2);
  EXPECT_FALSE(it != it2);
}

TEST(UTF8ViewIteratorTest, Dereference_ASCII) {
  const char text[] = "ABC";
  utfx::utf8_view view(text);
  auto it = view.begin();

  utfx::utf8_char ch = *it;
  EXPECT_EQ(ch.code_point(), 'A');
  EXPECT_EQ(ch.size(), 1u);
}

TEST(UTF8ViewIteratorTest, Dereference_MultiByte) {
  const char text[] = "\xE4\xBD\xA0\xE5\xA5\xBD";  // 你好
  utfx::utf8_view view(text);

  auto it = view.begin();
  utfx::utf8_char ch1 = *it;
  EXPECT_EQ(ch1.code_point(), 0x4F60u);  // 你
  EXPECT_EQ(ch1.size(), 3u);

  ++it;
  utfx::utf8_char ch2 = *it;
  EXPECT_EQ(ch2.code_point(), 0x597Du);  // 好
  EXPECT_EQ(ch2.size(), 3u);
}

TEST(UTF8ViewIteratorTest, Dereference_FourByteEmoji) {
  const char text[] = "\xF0\x9F\x98\x80";  // U+1F600
  utfx::utf8_view view(text);
  auto it = view.begin();
  utfx::utf8_char ch = *it;
  EXPECT_EQ(ch.code_point(), 0x1F600u);
  EXPECT_EQ(ch.size(), 4u);
}

TEST(UTF8ViewIteratorTest, PreIncrement_ASCII) {
  const char text[] = "ABC";
  utfx::utf8_view view(text);
  auto it = view.begin();

  EXPECT_EQ((*it).code_point(), 'A');
  ++it;
  EXPECT_EQ((*it).code_point(), 'B');
  ++it;
  EXPECT_EQ((*it).code_point(), 'C');
  ++it;
  EXPECT_TRUE(it == view.end());
}

TEST(UTF8ViewIteratorTest, PostIncrement) {
  const char text[] = "XY";
  utfx::utf8_view view(text);
  auto it = view.begin();

  auto old = it++;
  EXPECT_EQ((*old).code_point(), 'X');
  EXPECT_EQ((*it).code_point(), 'Y');
}

TEST(UTF8ViewIteratorTest, PostIncrement_AtEnd) {
  const char text[] = "A";
  utfx::utf8_view view(text);
  auto it = view.begin();
  ++it;             // Now at end
  auto old = it++;  // Post-increment from end
  EXPECT_TRUE(old == view.end());
  // After increment, still points past-the-end
}

TEST(UTF8ViewIteratorTest, Equality_SamePosition) {
  const char text[] = "Hello";
  utfx::utf8_view view(text);
  auto it1 = view.begin();
  auto it2 = view.begin();
  EXPECT_TRUE(it1 == it2);
  EXPECT_FALSE(it1 != it2);

  ++it1;
  EXPECT_FALSE(it1 == it2);
  EXPECT_TRUE(it1 != it2);

  ++it2;
  EXPECT_TRUE(it1 == it2);
}

TEST(UTF8ViewIteratorTest, EndIterator) {
  const char text[] = "Hi";
  utfx::utf8_view view(text);
  auto it = view.begin();
  ++it;
  ++it;
  EXPECT_TRUE(it == view.end());
}

TEST(UTF8ViewIteratorTest, IterateMixedContent) {
  // "A" + U+00A2 + "B" + U+4F60 + U+1F600
  const char text[] =
      "A\xC2\xA2"
      "B\xE4\xBD\xA0\xF0\x9F\x98\x80";
  utfx::utf8_view view(text);

  auto it = view.begin();
  EXPECT_EQ((*it).code_point(), 'A');
  EXPECT_EQ((*it).size(), 1u);
  ++it;

  EXPECT_EQ((*it).code_point(), 0xA2u);
  EXPECT_EQ((*it).size(), 2u);
  ++it;

  EXPECT_EQ((*it).code_point(), 'B');
  EXPECT_EQ((*it).size(), 1u);
  ++it;

  EXPECT_EQ((*it).code_point(), 0x4F60u);
  EXPECT_EQ((*it).size(), 3u);
  ++it;

  EXPECT_EQ((*it).code_point(), 0x1F600u);
  EXPECT_EQ((*it).size(), 4u);
  ++it;

  EXPECT_TRUE(it == view.end());
}

TEST(UTF8ViewIteratorTest, IteratorOverEmptyView) {
  utfx::utf8_view view;
  EXPECT_TRUE(view.begin() == view.end());
}

// ============================================================================
// utf8_view construction tests
// ============================================================================

TEST(UTF8ViewTest, DefaultConstruction) {
  utfx::utf8_view view;
  EXPECT_TRUE(view.empty());
  EXPECT_EQ(view.byte_size(), 0u);
  EXPECT_EQ(view.data(), nullptr);
  EXPECT_EQ(view.size(), 0u);
  EXPECT_EQ(view.length(), 0u);
  EXPECT_TRUE(view.begin() == view.end());
}

TEST(UTF8ViewTest, ConstructFromCString) {
  utfx::utf8_view view("Hello");
  EXPECT_FALSE(view.empty());
  EXPECT_EQ(view.byte_size(), 5u);
  EXPECT_EQ(view.data()[0], 'H');
  EXPECT_EQ(view.size(), 5u);
}

TEST(UTF8ViewTest, ConstructFromCString_Nullptr) {
  utfx::utf8_view view(nullptr);
  EXPECT_TRUE(view.empty());
  EXPECT_EQ(view.byte_size(), 0u);
  EXPECT_EQ(view.data(), nullptr);
}

TEST(UTF8ViewTest, ConstructFromPointerAndLength) {
  const char* str = "Hello, World!";
  utfx::utf8_view view(str, 5);  // Only "Hello"
  EXPECT_EQ(view.byte_size(), 5u);
  EXPECT_EQ(view.size(), 5u);
}

TEST(UTF8ViewTest, ConstructFromStdString) {
  std::string s = "Test string";
  utfx::utf8_view view(s);
  EXPECT_EQ(view.byte_size(), s.size());
  EXPECT_EQ(view.data(), s.data());
}

TEST(UTF8ViewTest, ConstructFromStdStringView) {
  std::string_view sv = "string_view test";
  utfx::utf8_view view(sv);
  EXPECT_EQ(view.byte_size(), sv.size());
  EXPECT_EQ(view.data(), sv.data());
}

TEST(UTF8ViewTest, ConstructFromEmptyStdString) {
  std::string empty;
  utfx::utf8_view view(empty);
  EXPECT_TRUE(view.empty());
  EXPECT_EQ(view.byte_size(), 0u);
}

// ============================================================================
// utf8_view size / capacity tests
// ============================================================================

TEST(UTF8ViewTest, Size_ASCII) {
  utfx::utf8_view view("ABCDEF");
  EXPECT_EQ(view.size(), 6u);
  EXPECT_EQ(view.length(), 6u);
}

TEST(UTF8ViewTest, Size_MultiByte) {
  // 你好 = 2 code points, 6 bytes
  utfx::utf8_view view("\xE4\xBD\xA0\xE5\xA5\xBD");
  EXPECT_EQ(view.size(), 2u);
  EXPECT_EQ(view.length(), 2u);
  EXPECT_EQ(view.byte_size(), 6u);
}

TEST(UTF8ViewTest, Size_Mixed) {
  // "A" + U+00A2 + "B" + U+1F600 = 4 code points, 8 bytes
  const char text[] =
      "A\xC2\xA2"
      "B\xF0\x9F\x98\x80";
  utfx::utf8_view view(text);
  EXPECT_EQ(view.size(), 4u);
  EXPECT_EQ(view.byte_size(), 8u);
}

TEST(UTF8ViewTest, Size_Empty) {
  utfx::utf8_view view;
  EXPECT_EQ(view.size(), 0u);
  EXPECT_EQ(view.length(), 0u);
}

TEST(UTF8ViewTest, ByteSize) {
  utfx::utf8_view empty;
  EXPECT_EQ(empty.byte_size(), 0u);

  utfx::utf8_view ascii("ABC");
  EXPECT_EQ(ascii.byte_size(), 3u);

  utfx::utf8_view mb("\xE4\xBD\xA0");  // 3 bytes, 1 code point
  EXPECT_EQ(mb.byte_size(), 3u);
}

TEST(UTF8ViewTest, Empty) {
  utfx::utf8_view empty_view;
  EXPECT_TRUE(empty_view.empty());

  utfx::utf8_view non_empty("X");
  EXPECT_FALSE(non_empty.empty());
}

TEST(UTF8ViewTest, MaxSize) {
  utfx::utf8_view view("test");
  EXPECT_EQ(view.max_size(), utfx::utf8_view::npos - 1);
}

TEST(UTF8ViewTest, NPos) {
  EXPECT_EQ(utfx::utf8_view::npos, ~static_cast<size_t>(0));
}

// ============================================================================
// utf8_view element access tests
// ============================================================================

TEST(UTF8ViewTest, Data) {
  const char* str = "data test";
  utfx::utf8_view view(str);
  EXPECT_EQ(view.data(), str);

  utfx::utf8_view empty_view;
  EXPECT_EQ(empty_view.data(), nullptr);
}

TEST(UTF8ViewTest, Front_ASCII) {
  utfx::utf8_view view("ABC");
  utfx::utf8_char front = view.front();
  EXPECT_EQ(front.code_point(), 'A');
  EXPECT_EQ(front.size(), 1u);
}

TEST(UTF8ViewTest, Front_MultiByte) {
  utfx::utf8_view view("\xE4\xBD\xA0\xE5\xA5\xBD");  // 你好
  utfx::utf8_char front = view.front();
  EXPECT_EQ(front.code_point(), 0x4F60u);  // 你
  EXPECT_EQ(front.size(), 3u);
}

TEST(UTF8ViewTest, Front_FourByteEmoji) {
  utfx::utf8_view view(
      "\xF0\x9F\x98\x80"  // U+1F600
      "extra");
  utfx::utf8_char front = view.front();
  EXPECT_EQ(front.code_point(), 0x1F600u);
  EXPECT_EQ(front.size(), 4u);
}

TEST(UTF8ViewTest, Back_ASCII) {
  utfx::utf8_view view("ABC");
  utfx::utf8_char back = view.back();
  EXPECT_EQ(back.code_point(), 'C');
  EXPECT_EQ(back.size(), 1u);
}

TEST(UTF8ViewTest, Back_MultiByte) {
  utfx::utf8_view view("\xE4\xBD\xA0\xE5\xA5\xBD");  // 你好
  utfx::utf8_char back = view.back();
  EXPECT_EQ(back.code_point(), 0x597Du);  // 好
  EXPECT_EQ(back.size(), 3u);
}

TEST(UTF8ViewTest, Back_SingleChar) {
  utfx::utf8_view view("X");
  utfx::utf8_char back = view.back();
  EXPECT_EQ(back.code_point(), 'X');
  EXPECT_TRUE(view.back() == view.front());
}

TEST(UTF8ViewTest, Back_FourByteEmoji) {
  utfx::utf8_view view("prefix\xF0\x9F\x98\x80");  // U+1F600 at end
  utfx::utf8_char back = view.back();
  EXPECT_EQ(back.code_point(), 0x1F600u);
  EXPECT_EQ(back.size(), 4u);
}

TEST(UTF8ViewTest, OperatorBracket_ASCII) {
  utfx::utf8_view view("ABCDE");
  EXPECT_EQ(view[0].code_point(), 'A');
  EXPECT_EQ(view[1].code_point(), 'B');
  EXPECT_EQ(view[2].code_point(), 'C');
  EXPECT_EQ(view[3].code_point(), 'D');
  EXPECT_EQ(view[4].code_point(), 'E');
}

TEST(UTF8ViewTest, OperatorBracket_Mixed) {
  // "A" + U+00A2 + "B" = 3 code points
  const char text[] =
      "A\xC2\xA2"
      "B";
  utfx::utf8_view view(text);
  EXPECT_EQ(view[0].code_point(), 'A');
  EXPECT_EQ(view[1].code_point(), 0xA2u);
  EXPECT_EQ(view[2].code_point(), 'B');
}

TEST(UTF8ViewTest, OperatorBracket_FourByte) {
  const char text[] =
      "A\xF0\x9F\x98\x80"
      "B";  // A + U+1F600 + B
  utfx::utf8_view view(text);
  EXPECT_EQ(view[0].code_point(), 'A');
  EXPECT_EQ(view[0].size(), 1u);
  EXPECT_EQ(view[1].code_point(), 0x1F600u);
  EXPECT_EQ(view[1].size(), 4u);
  EXPECT_EQ(view[2].code_point(), 'B');
  EXPECT_EQ(view[2].size(), 1u);
}

// ============================================================================
// utf8_view remove_prefix / remove_suffix tests
// ============================================================================

TEST(UTF8ViewTest, RemovePrefix_ASCII) {
  utfx::utf8_view view("ABCDEF");
  view.remove_prefix(2);
  EXPECT_EQ(view.byte_size(), 4u);
  EXPECT_EQ(view.size(), 4u);
  EXPECT_EQ(view.front().code_point(), 'C');
}

TEST(UTF8ViewTest, RemovePrefix_MultiByte) {
  utfx::utf8_view view("\xE4\xBD\xA0\xE5\xA5\xBD");  // 你好
  view.remove_prefix(1);                             // Remove 你
  EXPECT_EQ(view.size(), 1u);
  EXPECT_EQ(view.byte_size(), 3u);
  EXPECT_EQ(view.front().code_point(), 0x597Du);  // 好
}

TEST(UTF8ViewTest, RemovePrefix_Mixed) {
  // "A" + U+00A2 + "B" + U+1F600 + "C"
  const char text[] =
      "A\xC2\xA2"
      "B\xF0\x9F\x98\x80"
      "C";
  utfx::utf8_view view(text);

  view.remove_prefix(2);  // Remove 'A' and U+00A2
  EXPECT_EQ(view.size(), 3u);
  EXPECT_EQ(view[0].code_point(), 'B');
  EXPECT_EQ(view[1].code_point(), 0x1F600u);
  EXPECT_EQ(view[2].code_point(), 'C');
}

TEST(UTF8ViewTest, RemovePrefix_All) {
  utfx::utf8_view view("ABC");
  view.remove_prefix(3);
  EXPECT_TRUE(view.empty());
  EXPECT_EQ(view.size(), 0u);
  EXPECT_EQ(view.byte_size(), 0u);
}

TEST(UTF8ViewTest, RemovePrefix_MoreThanSize) {
  utfx::utf8_view view("AB");
  view.remove_prefix(10);
  EXPECT_TRUE(view.empty());
  EXPECT_EQ(view.byte_size(), 0u);
}

TEST(UTF8ViewTest, RemovePrefix_Empty) {
  utfx::utf8_view view;
  view.remove_prefix(1);  // Should not crash
  EXPECT_TRUE(view.empty());
}

TEST(UTF8ViewTest, RemoveSuffix_ASCII) {
  utfx::utf8_view view("ABCDEF");
  view.remove_suffix(2);
  EXPECT_EQ(view.byte_size(), 4u);
  EXPECT_EQ(view.size(), 4u);
  EXPECT_EQ(view.back().code_point(), 'D');
}

TEST(UTF8ViewTest, RemoveSuffix_MultiByte) {
  utfx::utf8_view view("\xE4\xBD\xA0\xE5\xA5\xBD");  // 你好
  view.remove_suffix(1);                             // Remove 好
  EXPECT_EQ(view.size(), 1u);
  EXPECT_EQ(view.byte_size(), 3u);
  EXPECT_EQ(view.front().code_point(), 0x4F60u);  // 你
  EXPECT_EQ(view.back().code_point(), 0x4F60u);
}

TEST(UTF8ViewTest, RemoveSuffix_FourByteEmoji) {
  utfx::utf8_view view("AB\xF0\x9F\x98\x80");  // "AB" + U+1F600
  view.remove_suffix(1);                       // Remove emoji
  EXPECT_EQ(view.size(), 2u);
  EXPECT_EQ(view.front().code_point(), 'A');
  EXPECT_EQ(view.back().code_point(), 'B');
}

TEST(UTF8ViewTest, RemoveSuffix_All) {
  utfx::utf8_view view("ABC");
  view.remove_suffix(3);
  EXPECT_TRUE(view.empty());
}

TEST(UTF8ViewTest, RemoveSuffix_MoreThanSize) {
  utfx::utf8_view view("A");
  view.remove_suffix(10);
  EXPECT_TRUE(view.empty());
}

TEST(UTF8ViewTest, RemoveSuffix_Empty) {
  utfx::utf8_view view;
  view.remove_suffix(1);
  EXPECT_TRUE(view.empty());
}

TEST(UTF8ViewTest, RemovePrefixThenSuffix) {
  utfx::utf8_view view("ABCDEF");
  view.remove_prefix(1);
  view.remove_suffix(1);
  EXPECT_EQ(view.size(), 4u);
  EXPECT_EQ(view.front().code_point(), 'B');
  EXPECT_EQ(view.back().code_point(), 'E');
  EXPECT_EQ(view.byte_size(), 4u);
}

// ============================================================================
// utf8_view substr tests
// ============================================================================

TEST(UTF8ViewTest, Substr_DefaultParams) {
  utfx::utf8_view view("ABCD");
  auto sub = view.substr();
  EXPECT_EQ(sub.byte_size(), view.byte_size());
  EXPECT_EQ(sub.size(), view.size());
  EXPECT_EQ(sub, view);
}

TEST(UTF8ViewTest, Substr_FromPos) {
  utfx::utf8_view view("ABCDEF");
  auto sub = view.substr(2);  // "CDEF"
  EXPECT_EQ(sub.size(), 4u);
  EXPECT_EQ(sub.front().code_point(), 'C');
}

TEST(UTF8ViewTest, Substr_FromPosWithCount) {
  utfx::utf8_view view("ABCDEF");
  auto sub = view.substr(1, 3);  // "BCD"
  EXPECT_EQ(sub.size(), 3u);
  EXPECT_EQ(sub.front().code_point(), 'B');
  EXPECT_EQ(sub.back().code_point(), 'D');
}

TEST(UTF8ViewTest, Substr_MultiByte) {
  // 你好世界 = 4 code points
  const char text[] = "\xE4\xBD\xA0\xE5\xA5\xBD\xE4\xB8\x96\xE7\x95\x8C";
  utfx::utf8_view view(text);
  EXPECT_EQ(view.size(), 4u);

  auto sub = view.substr(1, 2);  // 好世
  EXPECT_EQ(sub.size(), 2u);
  EXPECT_EQ(sub.byte_size(), 6u);
  EXPECT_EQ(sub[0].code_point(), 0x597Du);  // 好
  EXPECT_EQ(sub[1].code_point(), 0x4E16u);  // 世
}

TEST(UTF8ViewTest, Substr_PosBeyondEnd) {
  utfx::utf8_view view("ABC");
  auto sub = view.substr(5);
  EXPECT_TRUE(sub.empty());
  EXPECT_EQ(sub.byte_size(), 0u);
}

TEST(UTF8ViewTest, Substr_CountNPos) {
  utfx::utf8_view view("ABCD");
  auto sub = view.substr(1, utfx::utf8_view::npos);
  EXPECT_EQ(sub.size(), 3u);
  EXPECT_EQ(sub.front().code_point(), 'B');
}

TEST(UTF8ViewTest, Substr_ZeroCount) {
  utfx::utf8_view view("ABCD");
  auto sub = view.substr(0, 0);
  EXPECT_TRUE(sub.empty());
  EXPECT_EQ(sub.byte_size(), 0u);
}

TEST(UTF8ViewTest, Substr_FourByteCharacter) {
  // U+1F600 + U+1F600 = 2 emoji
  const char text[] = "\xF0\x9F\x98\x80\xF0\x9F\x98\x80";
  utfx::utf8_view view(text);
  EXPECT_EQ(view.size(), 2u);

  auto sub = view.substr(1, 1);
  EXPECT_EQ(sub.size(), 1u);
  EXPECT_EQ(sub.byte_size(), 4u);
  EXPECT_EQ(sub[0].code_point(), 0x1F600u);
}

TEST(UTF8ViewTest, Substr_EmptyView) {
  utfx::utf8_view view;
  auto sub = view.substr(0, 1);
  EXPECT_TRUE(sub.empty());
}

// ============================================================================
// utf8_view swap tests
// ============================================================================

TEST(UTF8ViewTest, Swap) {
  utfx::utf8_view a("Hello");
  utfx::utf8_view b("World!");

  a.swap(b);

  EXPECT_EQ(a.byte_size(), 6u);
  EXPECT_EQ(a.front().code_point(), 'W');
  EXPECT_EQ(b.byte_size(), 5u);
  EXPECT_EQ(b.front().code_point(), 'H');
}

TEST(UTF8ViewTest, Swap_WithEmpty) {
  utfx::utf8_view a("Test");
  utfx::utf8_view b;

  a.swap(b);

  EXPECT_TRUE(a.empty());
  EXPECT_EQ(b.byte_size(), 4u);
  EXPECT_EQ(b.front().code_point(), 'T');
}

TEST(UTF8ViewTest, Swap_SameView) {
  utfx::utf8_view a("Same");
  a.swap(a);
  EXPECT_EQ(a.byte_size(), 4u);
}

// ============================================================================
// utf8_view conversion to std::string_view tests
// ============================================================================

TEST(UTF8ViewTest, ConversionToStringView) {
  utfx::utf8_view view("Hello");
  std::string_view sv = view;
  EXPECT_EQ(sv, "Hello");
  EXPECT_EQ(sv.size(), 5u);
}

TEST(UTF8ViewTest, ConversionToStringView_Empty) {
  utfx::utf8_view view;
  std::string_view sv = view;
  EXPECT_TRUE(sv.empty());
}

TEST(UTF8ViewTest, ConversionToStringView_MultiByte) {
  const char text[] = "\xE4\xBD\xA0\xE5\xA5\xBD";
  utfx::utf8_view view(text);
  std::string_view sv = view;
  EXPECT_EQ(sv.size(), 6u);
  EXPECT_EQ(sv.data(), text);
}

// ============================================================================
// utf8_view comparison tests
// ============================================================================

TEST(UTF8ViewTest, Equality) {
  utfx::utf8_view a("Hello");
  utfx::utf8_view b("Hello");
  utfx::utf8_view c("World");

  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
  EXPECT_FALSE(a == c);
  EXPECT_TRUE(a != c);
}

TEST(UTF8ViewTest, Equality_Bytewise) {
  // Note: comparison is bytewise, not code-point wise
  utfx::utf8_view a("\xC2\xA2");  // U+00A2 properly encoded
  utfx::utf8_view b("\xC2\xA2");  // Same bytes
  EXPECT_TRUE(a == b);
}

TEST(UTF8ViewTest, Inequality_Different) {
  utfx::utf8_view a("abc");
  utfx::utf8_view b("abd");
  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);
}

TEST(UTF8ViewTest, Ordering) {
  utfx::utf8_view a("apple");
  utfx::utf8_view b("banana");
  utfx::utf8_view c("apple");

  EXPECT_TRUE(a < b);
  EXPECT_TRUE(a <= b);
  EXPECT_TRUE(a <= c);
  EXPECT_TRUE(b > a);
  EXPECT_TRUE(b >= a);
  EXPECT_TRUE(a >= c);
}

TEST(UTF8ViewTest, Ordering_Empty) {
  utfx::utf8_view empty;
  utfx::utf8_view non_empty("a");

  EXPECT_TRUE(empty < non_empty);
  EXPECT_FALSE(non_empty < empty);
  EXPECT_TRUE(non_empty > empty);
  EXPECT_FALSE(empty > non_empty);
}

TEST(UTF8ViewTest, Ordering_SameText) {
  utfx::utf8_view a("\xF0\x9F\x98\x80");
  utfx::utf8_view b("\xF0\x9F\x98\x80");
  EXPECT_FALSE(a < b);
  EXPECT_FALSE(b < a);
  EXPECT_TRUE(a <= b);
  EXPECT_TRUE(a >= b);
}

// ============================================================================
// utf8_view range-for loop and iterator-based usage tests
// ============================================================================

TEST(UTF8ViewTest, RangeFor_ASCII) {
  utfx::utf8_view view("ABCD");
  int count = 0;
  char expected[] = {'A', 'B', 'C', 'D'};
  for (auto ch : view) {
    EXPECT_EQ(ch.code_point(), static_cast<uint32_t>(expected[count]));
    count++;
  }
  EXPECT_EQ(count, 4);
}

TEST(UTF8ViewTest, RangeFor_MixedContent) {
  const char text[] =
      "A\xC2\xA2"
      "B\xF0\x9F\x98\x80";
  utfx::utf8_view view(text);
  uint32_t expected[] = {'A', 0xA2u, 'B', 0x1F600u};
  int count = 0;
  for (auto ch : view) {
    EXPECT_EQ(ch.code_point(), expected[count]);
    count++;
  }
  EXPECT_EQ(count, 4);
}

TEST(UTF8ViewTest, RangeFor_Empty) {
  utfx::utf8_view view;
  int count = 0;
  for (auto ch : view) {
    (void)ch;
    count++;
  }
  EXPECT_EQ(count, 0);
}

TEST(UTF8ViewTest, RangeFor_CJK) {
  const char text[] = "\xE4\xBD\xA0\xE5\xA5\xBD";  // 你好
  utfx::utf8_view view(text);
  int count = 0;
  for (auto ch : view) {
    EXPECT_EQ(ch.size(), 3u);
    count++;
  }
  EXPECT_EQ(count, 2);
}

TEST(UTF8ViewTest, STLDistance) {
  utfx::utf8_view view("ABCDEF");
  auto d = std::distance(view.begin(), view.end());
  EXPECT_EQ(d, 6);
}

TEST(UTF8ViewTest, STLDistance_MultiByte) {
  const char text[] =
      "\xE4\xBD\xA0\xE5\xA5\xBD\xE4\xB8\x96\xE7\x95\x8C";  // 你好世界
  utfx::utf8_view view(text);
  auto d = std::distance(view.begin(), view.end());
  EXPECT_EQ(d, 4);
}

// ============================================================================
// utf8_view with invalid UTF-8 bytes (trail_length == -1 => treated as 1 byte)
// ============================================================================

TEST(UTF8ViewTest, InvalidBytes_TreatedAsSingleByte) {
  // 0xFF is an invalid UTF-8 byte; trail_length returns -1
  // The iterator treats it as a single-byte character
  const char text[] = "\xFF";
  utfx::utf8_view view(text);
  EXPECT_EQ(view.size(), 1u);
  utfx::utf8_char ch = view.front();
  EXPECT_EQ(ch.size(), 1u);
  EXPECT_EQ(static_cast<unsigned char>(ch[0]), 0xFFu);
}

TEST(UTF8ViewTest, InvalidBytes_IteratesAsSingleByte) {
  const char text[] =
      "A\xFF"
      "B";
  utfx::utf8_view view(text);
  EXPECT_EQ(view.size(), 3u);

  auto it = view.begin();
  EXPECT_EQ((*it).code_point(), 'A');
  ++it;
  // Invalid byte: trail_length returns -1, len = 1
  EXPECT_EQ((*it).size(), 1u);
  EXPECT_EQ(static_cast<unsigned char>((*it)[0]), 0xFFu);
  ++it;
  EXPECT_EQ((*it).code_point(), 'B');
  ++it;
  EXPECT_TRUE(it == view.end());
}

// ============================================================================
// constexpr compile-time tests
// ============================================================================

TEST(UTF8CharTest, ConstexprDefaultConstruction) {
  constexpr utfx::utf8_char c;
  EXPECT_TRUE(c.empty());
  EXPECT_EQ(c.size(), 0u);
  EXPECT_EQ(c.byte_size(), 0u);
  EXPECT_EQ(c.code_point(), utfx::detail::illegal);
}

TEST(UTF8CharTest, ConstexprEquality) {
  constexpr utfx::utf8_char a("A", 1);
  constexpr utfx::utf8_char b("A", 1);
  constexpr utfx::utf8_char c("B", 1);
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a == c);
  EXPECT_TRUE(a != c);
}

TEST(UTF8CharTest, ConstexprOrdering) {
  constexpr utfx::utf8_char a("A", 1);
  constexpr utfx::utf8_char b("B", 1);
  static_assert(a < b, "A should be less than B");
  static_assert(a <= b, "A should be <= B");
  static_assert(b > a, "B should be > A");
  static_assert(b >= a, "B should be >= A");
  EXPECT_TRUE(true);  // If we reached here, static_assert passed
}

TEST(UTF8ViewTest, ConstexprDefaultConstruction) {
  constexpr utfx::utf8_view view;
  EXPECT_TRUE(view.empty());
  EXPECT_EQ(view.byte_size(), 0u);
  EXPECT_EQ(view.begin(), view.end());
}

TEST(UTF8ViewTest, ConstexprComparisons) {
  constexpr utfx::utf8_view a("ABC");
  constexpr utfx::utf8_view b("ABC");
  constexpr utfx::utf8_view c("ABD");
  static_assert(a == b, "Same views should be equal");
  static_assert(a != c, "Different views should not be equal");
  static_assert(a < c, "ABC < ABD");
  static_assert(c > a, "ABD > ABC");
  EXPECT_TRUE(true);
}

// ============================================================================
// Complete workflow tests combining all three types
// ============================================================================

TEST(UTF8TypesIntegrationTest, ViewIterationYieldsChars) {
  const char text[] =
      "\xF0\x9F\x98\x80"  // U+1F600
      " \xE4\xBD\xA0"     // space + U+4F60
      "!";
  utfx::utf8_view view(text);
  EXPECT_EQ(view.size(), 4u);  // emoji + space + CJK + exclamation

  auto it = view.begin();
  utfx::utf8_char ch1 = *it;
  EXPECT_EQ(ch1.code_point(), 0x1F600u);
  EXPECT_EQ(ch1.size(), 4u);
  EXPECT_EQ(static_cast<unsigned char>(ch1[0]), 0xF0u);
  EXPECT_EQ(static_cast<unsigned char>(ch1[1]), 0x9Fu);
  EXPECT_EQ(static_cast<unsigned char>(ch1[2]), 0x98u);
  EXPECT_EQ(static_cast<unsigned char>(ch1[3]), 0x80u);

  ++it;
  utfx::utf8_char ch2 = *it;
  EXPECT_EQ(ch2.code_point(), ' ');
  EXPECT_EQ(ch2.size(), 1u);

  ++it;
  utfx::utf8_char ch3 = *it;
  EXPECT_EQ(ch3.code_point(), 0x4F60u);
  EXPECT_EQ(ch3.size(), 3u);

  ++it;
  utfx::utf8_char ch4 = *it;
  EXPECT_EQ(ch4.code_point(), '!');
  EXPECT_EQ(ch4.size(), 1u);

  ++it;
  EXPECT_TRUE(it == view.end());
}

TEST(UTF8TypesIntegrationTest, ViewSubstrAndIterate) {
  const char text[] = "Hello, \xE4\xB8\x96\xE7\x95\x8C!";  // "Hello, 世界!"
  utfx::utf8_view full(text);

  // Get "世界"
  auto sub = full.substr(7, 2);
  EXPECT_EQ(sub.size(), 2u);
  EXPECT_EQ(sub[0].code_point(), 0x4E16u);  // 世
  EXPECT_EQ(sub[1].code_point(), 0x754Cu);  // 界

  // Convert sub's utf8_char to view and back
  utfx::utf8_char ch = sub[0];
  utfx::utf8_view char_view(ch.data(), ch.size());
  EXPECT_EQ(char_view.size(), 1u);
  EXPECT_EQ(char_view[0].code_point(), 0x4E16u);
}

TEST(UTF8TypesIntegrationTest, RemovePrefixSuffixThenIterate) {
  // "prefix" + 你好 + "suffix"
  const char text[] =
      "prefix\xE4\xBD\xA0\xE5\xA5\xBD"
      "suffix";
  utfx::utf8_view view(text);
  EXPECT_EQ(view.size(), 14u);  // 6 ("prefix") + 2 ("你好") + 6 ("suffix")

  view.remove_prefix(6);       // Remove "prefix"
  EXPECT_EQ(view.size(), 8u);  // "你好" (2) + "suffix" (6)
  view.remove_suffix(6);       // Remove "suffix"
  EXPECT_EQ(view.size(), 2u);  // Just "你好"

  // Now only 你好 remains
  auto it = view.begin();
  EXPECT_EQ((*it).code_point(), 0x4F60u);
  ++it;
  EXPECT_EQ((*it).code_point(), 0x597Du);
  ++it;
  EXPECT_TRUE(it == view.end());
}

TEST(UTF8TypesIntegrationTest, SwapAndCompare) {
  utfx::utf8_view a("ABC");
  utfx::utf8_view b("\xE4\xBD\xA0");  // 你 (3 bytes)

  EXPECT_TRUE(a < b || b < a);  // Ordering should be defined
  EXPECT_FALSE(a == b);

  a.swap(b);

  EXPECT_EQ(a.byte_size(), 3u);
  EXPECT_EQ(b.byte_size(), 3u);
  EXPECT_EQ(a.size(), 1u);  // Single CJK character
  EXPECT_EQ(b.size(), 3u);  // Three ASCII characters

  EXPECT_FALSE(a == b);
}
