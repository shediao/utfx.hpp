#include <gtest/gtest.h>

#include <utfx/utfx.hpp>

#include "emoji_sequences.h"
TEST(TestUTFX, EmojiTest) {
  ASSERT_EQ(utfx::utf16_to_utf8(utfx::utf8_to_utf16(emoji_sequences)),
            emoji_sequences);
}
