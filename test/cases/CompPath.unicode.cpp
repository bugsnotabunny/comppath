#include "comppath/CompPath.hpp"

#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

namespace {

template <typename... CONSTANTS, typename... TO_COMPARE>
consteval bool constants_eq(std::tuple<CONSTANTS...>,
                            std::tuple<TO_COMPARE...> to_compare) noexcept {
  return std::tuple{CONSTANTS::value...} == to_compare;
}

} // namespace

TEST_CASE("to_utf8 from u16string_view", "[utf][to_utf8][u16]") {
  SECTION("Empty string") {
    std::u16string_view empty;
    auto result = comppath::detail::to_utf8(empty);
    CHECK(result.empty());
    CHECK(result == u8"");
  }

  SECTION("ASCII only") {
    std::u16string_view ascii = u"Hello";
    auto result = comppath::detail::to_utf8(ascii);
    CHECK(result == u8"Hello");
  }

  SECTION("Basic Multilingual Plane (BMP) – Latin-1 supplement") {
    std::u16string_view bmp = u"\u00E9"; // é
    auto result = comppath::detail::to_utf8(bmp);
    CHECK(result == u8"\xC3\xA9");
  }

  SECTION("BMP – currency symbol") {
    std::u16string_view bmp = u"\u20AC"; // €
    auto result = comppath::detail::to_utf8(bmp);
    CHECK(result == u8"\xE2\x82\xAC");
  }

  SECTION("BMP – Greek") {
    std::u16string_view bmp = u"\u03A9"; // Ω
    auto result = comppath::detail::to_utf8(bmp);
    CHECK(result == u8"\xCE\xA9");
  }

  SECTION("Surrogate pair (non-BMP)") {
    // U+1F600 😀 (UTF-16: 0xD83D 0xDE00)
    std::u16string_view non_bmp = u"\U0001F600";
    auto result = comppath::detail::to_utf8(non_bmp);
    CHECK(result == u8"\xF0\x9F\x98\x80");
  }

  SECTION("Multiple code points with surrogate pair") {
    std::u16string_view mixed = u"a\u00E9\u20AC\U0001F600";
    auto result = comppath::detail::to_utf8(mixed);
    CHECK(result == u8"a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80");
  }

  SECTION("Constexpr evaluation") {
    constexpr std::u16string_view INPUT = u"\U0001F600";
    STATIC_CHECK(comppath::detail::to_utf8(INPUT) == u8"\xF0\x9F\x98\x80");
  }
}

TEST_CASE("to_utf8 from u32string_view", "[utf][to_utf8][u32]") {
  SECTION("Empty string") {
    std::u32string_view empty;
    auto result = comppath::detail::to_utf8(empty);
    CHECK(result.empty());
    CHECK(result == u8"");
  }

  SECTION("ASCII only") {
    std::u32string_view ascii = U"Hello";
    auto result = comppath::detail::to_utf8(ascii);
    CHECK(result == u8"Hello");
  }

  SECTION("BMP – é") {
    std::u32string_view bmp = U"\u00E9";
    auto result = comppath::detail::to_utf8(bmp);
    CHECK(result == u8"\xC3\xA9");
  }

  SECTION("BMP – €") {
    std::u32string_view bmp = U"\u20AC";
    auto result = comppath::detail::to_utf8(bmp);
    CHECK(result == u8"\xE2\x82\xAC");
  }

  SECTION("Non-BMP – 😀") {
    std::u32string_view non_bmp = U"\U0001F600";
    auto result = comppath::detail::to_utf8(non_bmp);
    CHECK(result == u8"\xF0\x9F\x98\x80");
  }

  SECTION("Mixed content") {
    std::u32string_view mixed = U"a\u00E9\u20AC\U0001F600";
    auto result = comppath::detail::to_utf8(mixed);
    CHECK(result == u8"a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80");
  }

  SECTION("Constexpr evaluation") {
    constexpr std::u32string_view INPUT = U"\U0001F600";
    STATIC_CHECK(comppath::detail::to_utf8(INPUT) == u8"\xF0\x9F\x98\x80");
  }
}

TEST_CASE("to_utf16 from u8string_view", "[utf][to_utf16]") {
  SECTION("Empty string") {
    std::u8string_view empty;
    auto result = comppath::detail::to_utf16(empty);
    CHECK(result.empty());
    CHECK(result == u"");
  }

  SECTION("ASCII only") {
    std::u8string_view ascii = u8"Hello";
    auto result = comppath::detail::to_utf16(ascii);
    CHECK(result == u"Hello");
  }

  SECTION("2‑byte UTF‑8 sequence – é") {
    std::u8string_view input = u8"\xC3\xA9";
    auto result = comppath::detail::to_utf16(input);
    CHECK(result == u"\u00E9");
  }

  SECTION("3‑byte UTF‑8 sequence – €") {
    std::u8string_view input = u8"\xE2\x82\xAC";
    auto result = comppath::detail::to_utf16(input);
    CHECK(result == u"\u20AC");
  }

  SECTION("4‑byte UTF‑8 sequence – 😀") {
    std::u8string_view input = u8"\xF0\x9F\x98\x80";
    auto result = comppath::detail::to_utf16(input);
    // Expected UTF-16 surrogate pair: 0xD83D, 0xDE00
    CHECK(result == u"\U0001F600");
  }

  SECTION("Mixed UTF‑8 input") {
    std::u8string_view mixed = u8"a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80";
    auto result = comppath::detail::to_utf16(mixed);
    CHECK(result == u"a\u00E9\u20AC\U0001F600");
  }

  SECTION("Constexpr evaluation") {
    constexpr std::u8string_view INPUT = u8"\xF0\x9F\x98\x80";
    STATIC_CHECK(comppath::detail::to_utf16(INPUT) == u"\U0001F600");
  }
}

TEST_CASE("Round‑trip conversions", "[utf][roundtrip]") {
  SECTION("UTF‑8 → UTF‑16 → UTF‑8") {
    std::u8string_view original = u8"a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80";
    auto utf16 = comppath::detail::to_utf16(original);
    auto roundtrip = comppath::detail::to_utf8(utf16);
    CHECK(roundtrip == original);
  }

  SECTION("UTF‑16 → UTF‑8 → UTF‑16") {
    std::u16string_view original = u"a\u00E9\u20AC\U0001F600";
    auto utf8 = comppath::detail::to_utf8(original);
    auto roundtrip = comppath::detail::to_utf16(utf8);
    CHECK(roundtrip == original);
  }

  SECTION("UTF‑32 → UTF‑8 → UTF‑32 (via UTF‑16 intermediate)") {
    std::u32string_view original_u32 = U"a\u00E9\u20AC\U0001F600";
    auto utf8 = comppath::detail::to_utf8(original_u32);
    auto utf16 = comppath::detail::to_utf16(utf8);
    CHECK(utf16 == u"a\u00E9\u20AC\U0001F600");
  }
}
