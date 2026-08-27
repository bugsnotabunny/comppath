#ifndef COMPPATH_COMPATH_HPP
#define COMPPATH_COMPATH_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <climits>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

#ifdef _WIN32
#define COMPPATH_PLATFORM_POSIX 0
#define COMPPATH_PLATFORM_WINDOWS 1
#else
#define COMPPATH_PLATFORM_POSIX 1
#define COMPPATH_PLATFORM_WINDOWS 0
#endif

namespace comppath {

namespace detail {

static_assert(CHAR_BIT == 8, "Wide-char platforms are not supported");

template <typename CHAR>
  requires(sizeof(CHAR) == 1)
constexpr bool is_valid_utf8 // NOLINT(readability-function-cognitive-complexity)
    (std::basic_string_view<CHAR> str) noexcept {
  size_t i = 0;
  while (i < str.size()) {
    unsigned char c = str[i];

    // ----- 1-byte sequence (ASCII): 0xxxxxxx -----
    if (c <= 0x7F) {
      ++i;
      continue;
    }

    int trailing = 0;
    uint32_t codepoint = 0;

    // ----- Determine sequence length and extract leading bits -----
    if ((c & 0xE0) == 0xC0) { // 2-byte: 110xxxxx
      trailing = 1;
      codepoint = c & 0x1F;
    } else if ((c & 0xF0) == 0xE0) { // 3-byte: 1110xxxx
      trailing = 2;
      codepoint = c & 0x0F;
    } else if ((c & 0xF8) == 0xF0) { // 4-byte: 11110xxx
      trailing = 3;
      codepoint = c & 0x07;
    } else {
      return false; // Invalid leading byte (e.g., 11111xxx)
    }

    // ----- Ensure there are enough bytes left -----
    if (i + trailing >= str.size()) {
      return false; // Truncated sequence at end of string
    }

    // ----- Read the trailing continuation bytes (10xxxxxx) -----
    for (int j = 1; j <= trailing; ++j) {
      unsigned char next = str[i + j];
      if ((next & 0xC0) != 0x80) { // Must start with 10
        return false;
      }
      codepoint = (codepoint << 6) | (next & 0x3F);
    }

    // ----- Check for overlong encodings -----
    // A character must be encoded using the shortest possible byte sequence.
    if (trailing == 1 && codepoint < 0x80) {
      return false; // C0, C1 are invalid
    }
    if (trailing == 2 && codepoint < 0x800) {
      return false; // E0 invalid without A0
    }
    if (trailing == 3 && codepoint < 0x10000) {
      return false; // F0 invalid without 90
    }

    // ----- Surrogate range check (forbidden in UTF-8) -----
    // Surrogates (U+D800 to U+DFFF) are only valid in UTF-16.
    if (codepoint >= 0xD800 && codepoint <= 0xDFFF) {
      return false;
    }

    // ----- Maximum valid Unicode codepoint -----
    if (codepoint > 0x10FFFF) {
      return false;
    }

    i += (trailing + 1);
  }

  return true;
}

[[noreturn]] inline void throw_invalid_utf8() {
  throw std::out_of_range{"str is not valid UTF-8"};
};

[[noreturn]] inline void throw_invalid_utf16() {
  throw std::out_of_range{"str is not valid UTF-16"};
};

[[noreturn]] inline void throw_invalid_utf32() {
  throw std::out_of_range{"str is not valid UTF-32"};
};

constexpr void push_utf8(std::u8string &out, char32_t codepoint) {
  if (codepoint <= 0x7F) {
    out.push_back(static_cast<char>(codepoint));
  } else if (codepoint <= 0x7FF) {
    out.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else if (codepoint <= 0xFFFF) {
    out.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else if (codepoint <= 0x10FFFF) {
    out.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
  } else {
    throw std::out_of_range{"codepoint is not valid unicode"};
  }
}

constexpr void push_utf16(std::u16string &out, char32_t codepoint) {
  if (codepoint <= 0xFFFF) {
    out.push_back(static_cast<char16_t>(codepoint));
  } else if (codepoint <= 0x10FFFF) {
    // Surrogate pair
    uint32_t temp = codepoint - 0x10000;
    auto codepoints = std::to_array({
        static_cast<char16_t>((temp >> 10) + 0xD800),
        static_cast<char16_t>((temp & 0x3FF) + 0xDC00),
    });
    out.append(std::u16string_view{codepoints.data(), codepoints.size()});
  } else {
    throw std::out_of_range{"codepoint is not valid unicode"};
  }
}

constexpr std::u8string_view to_utf8(std::u8string_view utf8) {
  if (!is_valid_utf8(utf8)) {
    throw_invalid_utf8();
  }

  return utf8;
}

constexpr std::u8string to_utf8(std::u16string_view utf16_str) {
  std::u8string result;
  result.reserve(utf16_str.size() * 3);

  for (size_t i = 0; i < utf16_str.size();) {
    char16_t ch = utf16_str[i];
    if (ch >= 0xD800 && ch <= 0xDBFF) { // High surrogate
      if (i + 1 < utf16_str.size()) {
        char16_t low = utf16_str[i + 1];
        if (low >= 0xDC00 && low <= 0xDFFF) {
          uint32_t codepoint = 0x10000 + ((ch - 0xD800) << 10) + (low - 0xDC00);
          push_utf8(result, codepoint);
          i += 2;
          continue;
        }

        throw_invalid_utf16();
      }
      throw_invalid_utf16();
    } else if (ch >= 0xDC00 && ch <= 0xDFFF) { // Low surrogate without high
      throw_invalid_utf16();
    } else {
      push_utf8(result, static_cast<uint32_t>(ch));
      ++i;
    }
  }
  return result;
}

constexpr std::u8string to_utf8(std::u32string_view utf32) {
  std::u8string result;
  result.reserve(utf32.size() * 3);

  for (char32_t codepoint : utf32) {
    if (codepoint >= 0xD800 && codepoint <= 0xDFFF) {
      throw_invalid_utf32();
    }
    push_utf8(result, codepoint);
  }

  return result;
}

constexpr std::u16string to_utf16(std::u8string_view utf8) {
  std::u16string result;
  result.reserve(utf8.size());

  for (size_t i = 0; i < utf8.size();) {
    char32_t codepoint = 0;
    auto c = static_cast<unsigned char>(utf8[i]);

    int trailing = 0;
    if (c < 0x80) {
      codepoint = c;
      trailing = 0;
    } else if ((c & 0xE0) == 0xC0) {
      codepoint = c & 0x1F;
      trailing = 1;
    } else if ((c & 0xF0) == 0xE0) {
      codepoint = c & 0x0F;
      trailing = 2;
    } else if ((c & 0xF8) == 0xF0) {
      codepoint = c & 0x07;
      trailing = 3;
    } else {
      throw_invalid_utf8();
    }

    // Check that we have enough bytes left
    if (i + trailing >= utf8.size()) {
      throw_invalid_utf8();
    }

    // Process continuation bytes
    bool valid = true;
    for (int j = 0; j < trailing; ++j) {
      auto cont = static_cast<unsigned char>(utf8[++i]);
      if ((cont & 0xC0) != 0x80) {
        valid = false;
        break;
      }
      codepoint = (codepoint << 6) | (cont & 0x3F);
    }
    if (!valid) {
      throw_invalid_utf8();
    }
    ++i;

    push_utf16(result, codepoint);
  }
  return result;
}

constexpr std::u16string_view to_utf16(std::u16string_view utf16) {
  (void)to_utf8(utf16);
  return utf16;
}

constexpr std::u16string to_utf16(std::u32string_view utf32) {
  return to_utf16(to_utf8(utf32));
}

template <typename DATA, typename FROM, typename TO, typename CHAR, size_t N>
  requires(std::unsigned_integral<FROM> && std::unsigned_integral<TO> &&
           sizeof(CHAR) == sizeof(FROM))
consteval DATA make_data(CHAR const (&s)[N]) {
  std::array<FROM, N> as_unsigned{};
  std::ranges::copy(s | std::views::transform([](CHAR c) { return static_cast<FROM>(c); }),
                    as_unsigned.begin());

  DATA result;

  auto str = [&]() {
    if constexpr (std::same_as<char8_t, TO>) {
      return to_utf8(std::basic_string_view{as_unsigned.data()});
    } else if constexpr (std::same_as<char16_t, TO>) {
      return to_utf16(std::basic_string_view{as_unsigned.data()});
    } else {
      static_assert(false, "Unsupported target encoding");
    }
  }();
  std::ranges::copy(str, result.cstr);
  result.size = str.size();
  return result;
}

template <template <size_t> typename COMP_PATH>
consteval auto append_paths(auto const &lhs, auto const &rhs) noexcept {
  assert(!rhs.is_absolute());

  using l_type = std::decay_t<decltype(lhs)>;
  using r_type = std::decay_t<decltype(rhs)>;

  using value_type = l_type::value_type;
  static_assert(std::same_as<value_type, typename r_type::value_type>);

  constexpr size_t SEP = 1;
  constexpr size_t NULL_TERM = 1;
  constexpr size_t RETURN_CAP = l_type::capacity() + SEP + r_type::capacity() + NULL_TERM;
  value_type data[RETURN_CAP]{};

  if (lhs.empty()) {
    std::ranges::copy(std::basic_string_view<value_type>{rhs}, data);
    return COMP_PATH{data};
  }

  if (std::ranges::find(l_type::possible_separators,
                        std::basic_string_view<value_type>{lhs}.back()) !=
      std::ranges::end(l_type::possible_separators)) {
    auto *out = std::ranges::copy(std::basic_string_view<value_type>{lhs}, data).out;
    std::ranges::copy(std::basic_string_view<value_type>{rhs}, out);
    return COMP_PATH{data};
  }

  auto *out = std::ranges::copy(std::basic_string_view<value_type>{lhs}, data).out;
  *out++ = l_type::preferred_separator;
  std::ranges::copy(std::basic_string_view<value_type>{rhs}, out);

  return COMP_PATH{data};
}

template <template <size_t> typename COMP_PATH>
consteval auto concat_paths(auto const &lhs, auto const &rhs) noexcept {
  using l_type = std::decay_t<decltype(lhs)>;
  using r_type = std::decay_t<decltype(rhs)>;

  using value_type = l_type::value_type;
  static_assert(std::same_as<value_type, typename r_type::value_type>);

  constexpr size_t NULL_TERM = 1;
  constexpr size_t RETURN_CAP = l_type::capacity() + r_type::capacity() + NULL_TERM;
  value_type data[RETURN_CAP]{};

  auto *out = std::ranges::copy(std::basic_string_view<value_type>{lhs}, data).out;
  std::ranges::copy(std::basic_string_view<value_type>{rhs}, out);
  return COMP_PATH{data};
}

} // namespace detail

namespace posix {

/// @brief: Character type used to store POSIX paths
using PathChar = char8_t;

/// @brief: View over POSIX path string
using PathStringView = std::basic_string_view<PathChar>;

/// @brief: A separator which POSIX prefers
static constexpr PathChar OS_PREFERRED_SEPARATOR = '/';

/// @brief: List of all separators allowed in POSIX paths
static constexpr std::array OS_SEPARATORS = std::to_array<PathChar>({OS_PREFERRED_SEPARATOR});

/// @brief: Compile-time POSIX path storage. Underlying string is encoded in UTF-8
/// @warn:  N may be bigger than the actual size of a path. Extra capacity can accumulate due to how
///         operator/ and constructors are implemented. See more detailed answer in their
///         description. Use SHRINK<PATH> to get efficiently stored paths
template <size_t N>
struct [[nodiscard]] CompPath {
  /// @brief: Character type used to store POSIX paths
  using value_type = PathChar;

  /// @brief: A separator which POSIX prefers
  static constexpr value_type preferred_separator = // NOLINT(readability-identifier-naming) case is
                                                    // chosen for better std-compatibility
      OS_PREFERRED_SEPARATOR;

  /// @brief: List of all separators allowed in POSIX paths
  static constexpr std::span<value_type const>
      possible_separators = // NOLINT(readability-identifier-naming) case is
                            // chosen for better std-compatibility
      OS_SEPARATORS;

  /// @brief: Construct an empty path
  consteval CompPath() noexcept
    requires(N == 1)
  = default;

  /// @brief: Construct a path from utf-8 string literal. If given string is not a valid utf-8, then
  ///         program is ill-formed
  template <typename CHAR>
    requires(sizeof(CHAR) == 1)
  consteval CompPath(CHAR const (&s)[N]) noexcept
      : data{detail::make_data<Data, char8_t, PathChar>(s)} {
  }

  /// @brief: Construct a path from utf-16 string literal. If given string is not a valid utf-16,
  ///         then program is ill-formed
  /// @warn:  This ctor performs conversion to utf-8 and may produce bloated strings since we have
  ///         to allocate capacity for worst-case scenario
  template <typename CHAR, size_t M>
    requires(sizeof(CHAR) == 2)
  consteval CompPath(CHAR const (&s)[M]) noexcept
      : data{detail::make_data<Data, char16_t, PathChar>(s)} {
  }

  /// @brief: Construct a path from utf-32 string literal. If given string is not a valid utf-32,
  ///         then program is ill-formed
  /// @warn:  This ctor performs conversion to utf-8 and may produce bloated strings since we have
  ///         to allocate capacity for worst-case scenario
  template <typename CHAR, size_t M>
    requires(sizeof(CHAR) == 4)
  consteval CompPath(CHAR const (&s)[M]) noexcept
      : data{detail::make_data<Data, char32_t, PathChar>(s)} {
  }

  /// @brief: Convert path into string view
  constexpr operator PathStringView() const noexcept {
    return PathStringView{data.cstr, size()};
  }

  /// @brief: Convert path into c-string which may be passes into syscalls. This method is only
  ///         available if target platform is POSIX
  constexpr char const *os_str() const noexcept
    requires(COMPPATH_PLATFORM_POSIX == 1)
  {
    return reinterpret_cast<char const *>(std::ranges::begin(data.cstr));
  }

  /// @brief: Append one path to another using os-preferred separator
  /// @warn:  May produce string which capacity is 1 or 2 bytes bigger than needed
  template <size_t M>
  consteval auto operator/(CompPath<M> const &rhs) const noexcept {
    return detail::append_paths<CompPath>(*this, rhs);
  }

  /// @brief: Append one path to another using os-preferred separator
  /// @warn:  May produce string which capacity is 1 or 2 bytes bigger than needed
  template <typename CHAR, size_t M>
  consteval auto operator/(CHAR const (&rhs)[M]) const noexcept {
    return *this / CompPath<M>{rhs};
  }

  /// @brief: Concatenate one path with another without inserting a separator
  /// @warn:  Any extra capacities inside paths sum and persist further
  template <size_t M>
  consteval auto operator+(CompPath<M> const &rhs) const noexcept {
    return detail::concat_paths<CompPath>(*this, rhs);
  }

  /// @brief: Concatenate one path with another without inserting a separator
  /// @warn:  Any extra capacities inside paths sum and persist further
  template <typename CHAR, size_t M>
  consteval auto operator+(CHAR const (&rhs)[M]) const noexcept {
    return *this + CompPath<M>{rhs};
  }

  /// @brief: Tell if path is an absolute path
  consteval bool is_absolute() const noexcept {
    return PathStringView{*this}.starts_with('/');
  }

  /// @brief: Tell if path is just root path and nothing else
  consteval bool is_root_path() const noexcept {
    return !empty() &&
           std::ranges::all_of(PathStringView{data.cstr}, [](char c) { return c == '/'; });
  }

  /// @brief: Tell if path is a relative path
  consteval bool is_relative() const noexcept {
    return !is_absolute();
  }

  /// @brief: Tell if path is empty
  consteval bool empty() const noexcept {
    return size() == 0;
  }

  /// @brief: Get path's actual size
  [[nodiscard]] consteval size_t size() const noexcept {
    return data.size;
  }

  /// @brief: Get path's maximum size
  [[nodiscard]] static consteval size_t capacity() noexcept {
    return N - 1;
  }

  /// @brief: Get path's maximum size
  [[nodiscard]] static consteval size_t max_size() noexcept {
    return capacity();
  }

  /// @brief: Tell if path is equal to a path constructed from given string literal
  template <typename CHAR, size_t M>
  [[nodiscard]] consteval bool operator==(CHAR const (&rhs)[M]) const noexcept {
    return *this == CompPath<M>{rhs};
  }

  /// @brief: Compare path with a path constructed from given string literal
  template <typename CHAR, size_t M>
  [[nodiscard]] consteval auto operator<=>(CHAR const (&rhs)[M]) const noexcept {
    return *this <=> CompPath<M>{rhs};
  }

  /// @brief: Tell if path is equal to given object convertible into PathStringView
  template <std::convertible_to<PathStringView> S>
  [[nodiscard]] constexpr bool operator==(S const &rhs) const noexcept {
    return PathStringView{*this} == PathStringView{rhs};
  }

  /// @brief: Compare path with given object convertible into PathStringView
  template <std::convertible_to<PathStringView> S>
  [[nodiscard]] constexpr auto operator<=>(S const &rhs) const noexcept {
    return PathStringView{*this} <=> PathStringView{rhs};
  }

  struct Data {
    /// @brief: Path's actual size
    size_t size = N - 1;

    /// @brief: Null-terminated at cstr[size] string of utf-8 characters
    PathChar cstr[N]{}; // C array used for better compiler messages
  };

  /// @brief: Path's actual size
  Data const data;
};

/// @brief: Deduction guide for default construction. Deduces CompPath<1> which represents an empty
///         path
CompPath() -> CompPath<1>;

/// @brief: Deduction guide for construction from utf-16 string literal. Reserves M * 2 capacity
///         which is enough for worst-case conversion into utf-8
template <typename CHAR, size_t M>
  requires(sizeof(CHAR) == 2)
CompPath(CHAR const (&)[M]) noexcept -> CompPath<M * 2>;

/// @brief: Deduction guide for construction from utf-32 string literal. Reserves M * 4 capacity
///         which is enough for worst-case conversion into utf-8
template <typename CHAR, size_t M>
  requires(sizeof(CHAR) == 4)
CompPath(CHAR const (&s)[M]) noexcept -> CompPath<M * 4>;

} // namespace posix

namespace win {

namespace detail {

using namespace ::comppath::detail;

// Helper: checks if a character is a path separator on Windows
consteval bool is_sep(char16_t c) noexcept {
  return c == u'\\' || c == u'/';
}

consteval char8_t to_lower(char8_t c) noexcept {
  if (c >= 'A' && c <= 'Z') {
    return static_cast<char>(c + ('a' - 'A'));
  }
  return c;
}

consteval bool comp_ascii_insensitive(std::u16string_view lhs, std::u16string_view rhs) noexcept {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  return std::equal(lhs.begin(), lhs.end(), rhs.begin(), [](char16_t a, char16_t b) {
    return a <= 127 && b <= 127 &&
           to_lower(static_cast<char8_t>(a)) == to_lower(static_cast<char8_t>(b));
  });
}

consteval std::u16string_view parse_win_root_name(std::u16string_view path) noexcept {
  // 1. Check for a local drive letter: C: or c:
  if (path.size() > 1 &&
      ((path[0] >= u'A' && path[0] <= u'Z') || (path[0] >= u'a' && path[0] <= u'z')) &&
      path[1] == u':') {
    return path.substr(0, 2);
  }

  // 2. Check for network paths (UNC or device paths starting with \\ or //)
  if (path.size() <= 1 || !is_sep(path[0]) || !is_sep(path[1])) {
    return {};
  }

  // 2a. Extended-length device path: "\\?\" or "\\.\"
  if (path.size() > 3 && is_sep(path[0]) && is_sep(path[1]) &&
      (path[2] == u'?' || path[2] == u'.') && is_sep(path[3])) {

    // \\?\ or \\.\"
    constexpr size_t PREFIX_LEN = 4;
    size_t sep1 = path.find_first_of(u"\\/", PREFIX_LEN);

    // Segment after prefix (e.g., "C:" or "UNC")
    std::u16string_view seg1 = path.substr(
        PREFIX_LEN,
        (sep1 == std::u16string_view::npos) ? std::u16string_view::npos : sep1 - PREFIX_LEN);

    // If the segment is "UNC" (case-insensitive), the server name is part of the root name
    if (comp_ascii_insensitive(seg1, u"UNC")) {
      // Find the next separator after the "UNC\" part
      size_t sep2 = path.find_first_of(u"\\/", sep1 + 1);

      // root_name = \\?\UNC\Server (everything up to the separator after the server name)
      if (sep2 == std::u16string_view::npos) {
        // No trailing slash → whole path is the root name
        return path;
      }

      size_t sep3 = path.find_first_of(u"\\/", std::min(path.size() - 1, sep2 + 1));
      return path.substr(0, std::min(sep3, path.size()));
    }
    // Device path to a local drive: \\?\C: (or \\.\C:)
    // root_name stops at the separator after the drive specifier
    if (sep1 == std::u16string_view::npos) {
      return path; // e.g., "\\?\C:" (invalid but parse it)
    }
    return path.substr(0, sep1);
  }
  // 2b. Standard UNC path: \\Server\Share or //Server/Share
  size_t sep = path.find_first_of(u"\\/", 2);
  if (sep == std::u16string_view::npos) {
    return path; // e.g., "\\Server" (no trailing slash)
  }

  sep = path.find_first_of(u"\\/", std::min(path.size() - 1, sep + 1));
  return path.substr(0, std::min(sep, path.size()));
}

} // namespace detail

/// @brief: Character type used to store Windows paths
using PathChar = char16_t;

/// @brief: View over Windows path string
using PathStringView = std::basic_string_view<PathChar>;

/// @brief: A separator which Windows prefers
static constexpr PathChar OS_PREFERRED_SEPARATOR = '\\';

/// @brief: List of all separators allowed in Windows paths
static constexpr std::array OS_SEPARATORS = std::to_array<PathChar>({OS_PREFERRED_SEPARATOR, '/'});

/// @brief: Compile-time Windows path storage. Underlying string is encoded in UTF-16
/// @warn:  N may be bigger than the actual size of a path. Extra capacity can accumulate due to how
///         operator/ and constructors are implemented. See more detailed answer in their
///         description. Use SHRINK<PATH> to get efficiently stored paths
template <size_t N>
struct [[nodiscard]] CompPath {
  /// @brief: Character type used to store Windows paths
  using value_type = PathChar;

  /// @brief: A separator which Windows prefers
  static constexpr value_type preferred_separator = // NOLINT(readability-identifier-naming) case is
                                                    // chosen for better std-compatibility
      OS_PREFERRED_SEPARATOR;

  /// @brief: List of all separators allowed in Windows paths
  static constexpr std::span<value_type const>
      possible_separators = // NOLINT(readability-identifier-naming) case is
                            // chosen for better std-compatibility
      OS_SEPARATORS;

  /// @brief: Construct an empty path
  consteval CompPath() noexcept
    requires(N == 1)
  = default;

  /// @brief: Construct a path from utf-8 string literal. If given string is not a valid utf-8, then
  ///         program is ill-formed
  /// @warn:  This ctor performs conversion to utf-16 and may produce bloated strings since we have
  ///         to allocate capacity for worst-case scenario
  template <typename CHAR>
    requires(sizeof(CHAR) == 1)
  consteval CompPath(CHAR const (&s)[N]) noexcept
      : data{detail::make_data<Data, char8_t, PathChar>(s)} {
  }

  /// @brief: Construct a path from utf-16 string literal. If given string is not a valid utf-16,
  ///         then program is ill-formed
  template <typename CHAR>
    requires(sizeof(CHAR) == 2)
  consteval CompPath(CHAR const (&s)[N]) noexcept
      : data{detail::make_data<Data, char16_t, PathChar>(s)} {
  }

  /// @brief: Construct a path from utf-32 string literal. If given string is not a valid utf-32,
  ///         then program is ill-formed
  /// @warn:  This ctor performs conversion to utf-16 and may produce bloated strings since we have
  ///         to allocate capacity for worst-case scenario
  template <typename CHAR, size_t M>
    requires(sizeof(CHAR) == 4)
  consteval CompPath(CHAR const (&s)[M]) noexcept
      : data{detail::make_data<Data, char32_t, PathChar>(s)} {
  }

  /// @brief: Convert path into string view
  constexpr operator PathStringView() const noexcept {
    return PathStringView{data.cstr, size()};
  }

  /// @brief: Convert path into c-string which may be passes into syscalls. This method is only
  ///         available if target platform is Windows
  constexpr wchar_t const *os_str() const noexcept
    requires(COMPPATH_PLATFORM_WINDOWS == 1)
  {
    return reinterpret_cast<wchar_t const *>(std::ranges::begin(data.cstr));
  }

  /// @brief: Append one path to another using os-preferred separator
  /// @warn:  May produce string which capacity is 1 or 2 bytes bigger than needed
  template <size_t M>
  consteval auto operator/(CompPath<M> const &rhs) const noexcept {
    return detail::append_paths<CompPath>(*this, rhs);
  }

  /// @brief: Append one path to another using os-preferred separator
  /// @warn:  May produce string which capacity is 1 or 2 bytes bigger than needed
  template <typename CHAR, size_t M>
  consteval auto operator/(CHAR const (&rhs)[M]) const noexcept {
    return *this / CompPath<M>{rhs};
  }

  /// @brief: Concatenate one path with another without inserting a separator
  /// @warn:  Any extra capacities inside paths sum and persist further
  template <size_t M>
  consteval auto operator+(CompPath<M> const &rhs) const noexcept {
    return detail::concat_paths<CompPath>(*this, rhs);
  }

  /// @brief: Concatenate one path with another without inserting a separator
  /// @warn:  Any extra capacities inside paths sum and persist further
  template <typename CHAR, size_t M>
  consteval auto operator+(CHAR const (&rhs)[M]) const noexcept {
    return *this + CompPath<M>{rhs};
  }

  /// @brief: Tell if path is empty
  consteval bool empty() const noexcept {
    return size() == 0;
  }

  /// @brief: Tell if path is an absolute path. Path is absolute if it has a root name (drive letter
  ///         like C: or UNC path like \\server\share) or starts with a separator
  consteval bool is_absolute() const noexcept {
    auto root_name = detail::parse_win_root_name(*this);
    auto this_sv = PathStringView{*this};
    if (root_name.empty()) {
      return this_sv.starts_with('/') || this_sv.starts_with('\\');
    }
    return true;
  }

  /// @brief: Tell if path consists only of a root name (if present) and/or a single root directory
  ///         separator and nothing else
  consteval bool is_root_path() const noexcept {
    auto root_name = detail::parse_win_root_name(*this);
    auto this_sv = PathStringView{*this};
    this_sv.remove_prefix(root_name.size());
    return this_sv == u"\\" || this_sv == u"/" || (!root_name.empty() && this_sv.empty());
  }

  /// @brief: Tell if path is a relative path
  consteval bool is_relative() const noexcept {
    return !is_absolute();
  }

  /// @brief: Get path's actual size
  [[nodiscard]] consteval size_t size() const noexcept {
    return data.size;
  }

  /// @brief: Get path's maximum size
  [[nodiscard]] static consteval size_t capacity() noexcept {
    return N - 1;
  }

  /// @brief: Get path's maximum size
  [[nodiscard]] static consteval size_t max_size() noexcept {
    return capacity();
  }

  /// @brief: Tell if path is equal to a path constructed from given string literal
  template <typename CHAR, size_t M>
  [[nodiscard]] consteval bool operator==(CHAR const (&rhs)[M]) const noexcept {
    return *this == CompPath<M>{rhs};
  }

  /// @brief: Compare path with a path constructed from given string literal
  template <typename CHAR, size_t M>
  [[nodiscard]] consteval auto operator<=>(CHAR const (&rhs)[M]) const noexcept {
    return *this <=> CompPath<M>{rhs};
  }

  /// @brief: Tell if path is equal to given object convertible into PathStringView
  template <std::convertible_to<PathStringView> S>
  [[nodiscard]] constexpr bool operator==(S const &rhs) const noexcept {
    return PathStringView{*this} == PathStringView{rhs};
  }

  /// @brief: Compare path with given object convertible into PathStringView
  template <std::convertible_to<PathStringView> S>
  [[nodiscard]] constexpr auto operator<=>(S const &rhs) const noexcept {
    return PathStringView{*this} <=> PathStringView{rhs};
  }

  struct Data {
    /// @brief: Path's actual size
    size_t size = N - 1;

    /// @brief: Null-terminated at cstr[size] string of utf-16 characters
    PathChar cstr[N]{}; // C array used for better compiler messages
  };

  /// @brief: Path's underlying data
  Data const data;
};

/// @brief: Deduction guide for default construction. Deduces CompPath<1> which represents an empty
///         path
CompPath() -> CompPath<1>;

/// @brief: Deduction guide for construction from utf-32 string literal. Reserves M * 2 capacity
///         which is enough for worst-case conversion into utf-16
template <typename CHAR, size_t M>
  requires(sizeof(CHAR) == 4)
CompPath(CHAR const (&s)[M]) noexcept -> CompPath<M * 2>;

} // namespace win

/// @brief: Helper which produces a compile-time error when instantiated. Intended to be used in
///         if-constexpr branches which handle unsupported path types
template <auto...>
consteval auto ill_form() noexcept {
  struct Nothing {};
  static_assert(false, "Unsupported path type");
  return Nothing{};
}

namespace detail {

template <template <size_t> typename COMP_PATH, auto PATH, size_t POS, size_t N = PATH.size()>
consteval auto path_substr() noexcept {
  using value_type = decltype(PATH)::value_type;
  value_type data[N + 1]{};
  auto sv = std::basic_string_view<value_type>{PATH}.substr(POS, N);
  std::ranges::copy(sv, data);
  return COMP_PATH{data};
}

template <template <size_t> typename COMP_PATH, auto PATH>
consteval auto shrink_impl() noexcept {
  using value_type = decltype(PATH)::value_type;
  constexpr auto SIZE = PATH.size() + 1;
  value_type data[SIZE]{};
  std::ranges::copy(std::basic_string_view<value_type>{PATH}, data);
  return COMP_PATH{data};
}

template <template <size_t> typename COMP_PATH, auto PATH>
constexpr COMP_PATH SHRINK = shrink_impl<COMP_PATH, PATH>();

template <posix::CompPath PATH>
consteval auto root_name_impl() noexcept {
  if constexpr (PATH.is_absolute()) {
    constexpr posix::PathStringView SV = []() {
      posix::PathStringView sv = PATH;
      if (sv.starts_with('/')) {
        sv.remove_prefix(1);
      }
      posix::PathStringView sv_before = sv;
      size_t slashes = 0;
      while (sv.starts_with('/')) {
        sv.remove_prefix(1);
        ++slashes;
      }
      return sv_before.substr(0, slashes);
    }();

    char name[SV.size() + 1]{};
    std::ranges::copy(SV, name);
    return posix::CompPath{name};
  } else {
    return posix::CompPath{};
  }
}

template <win::CompPath PATH>
consteval auto root_name_impl() noexcept {
  constexpr win::PathStringView SV = win::detail::parse_win_root_name(PATH);
  win::PathChar data[SV.size() + 1]{};
  std::ranges::copy(SV, data);
  return win::CompPath{data};
}

template <template <size_t> typename COMP_PATH, auto PATH>
constexpr COMP_PATH ROOT_NAME = SHRINK<COMP_PATH, root_name_impl<PATH>()>;

template <template <size_t> typename COMP_PATH, auto PATH>
consteval auto root_directory_impl() noexcept {
  if constexpr (PATH.is_absolute()) {
    constexpr auto SEP_POS = root_name_impl<PATH>().size();
    if constexpr (SEP_POS < PATH.size()) {
      using value_type = decltype(PATH)::value_type;
      value_type data[2]{PATH.data.cstr[SEP_POS], value_type('\0')};
      return COMP_PATH{data};
    } else {
      return COMP_PATH{};
    }
  } else {
    return COMP_PATH{};
  }
}

template <template <size_t> typename COMP_PATH, auto PATH>
constexpr COMP_PATH ROOT_DIRECTORY = SHRINK<COMP_PATH, root_directory_impl<COMP_PATH, PATH>()>;

template <template <size_t> typename COMP_PATH, auto PATH>
consteval auto root_path_impl() noexcept {
  return root_name_impl<PATH>() + root_directory_impl<COMP_PATH, PATH>();
}

template <template <size_t> typename COMP_PATH, auto PATH>
constexpr COMP_PATH ROOT_PATH = SHRINK<COMP_PATH, root_path_impl<COMP_PATH, PATH>()>;

template <template <size_t> typename COMP_PATH, auto PATH>
consteval auto remove_trailing_seps_impl() noexcept {
  using value_type = decltype(PATH)::value_type;

  if constexpr (PATH.empty() || PATH.is_root_path()) {
    return PATH;
  } else {
    constexpr std::basic_string_view<value_type> SV = []() {
      std::basic_string_view<value_type> sv = PATH;
      while (!sv.empty() && std::ranges::find(PATH.possible_separators, sv.back()) !=
                                PATH.possible_separators.end()) {
        sv.remove_suffix(1);
      }
      return sv;
    }();

    typename decltype(PATH)::value_type name[SV.size() + 1]{};
    std::ranges::copy(SV, name);
    return COMP_PATH{name};
  }
}

template <template <size_t> typename COMP_PATH, auto PATH>
constexpr COMP_PATH REMOVE_TRAILING_SEPS =
    SHRINK<COMP_PATH, remove_trailing_seps_impl<COMP_PATH, PATH>()>;

template <template <size_t> typename COMP_PATH, auto PATH>
consteval auto relative_path_impl() noexcept {
  return path_substr<COMP_PATH, PATH, ROOT_PATH<COMP_PATH, PATH>.size()>();
}

template <template <size_t> typename COMP_PATH, auto PATH>
constexpr COMP_PATH RELATIVE_PATH = SHRINK<COMP_PATH, relative_path_impl<COMP_PATH, PATH>()>;

template <template <size_t> typename COMP_PATH, auto PATH>
consteval auto parent_path_impl() noexcept {
  using value_type = decltype(PATH)::value_type;

  if constexpr (PATH.empty() || PATH.is_root_path()) {
    return PATH;
  } else {
    constexpr std::basic_string_view<value_type> SV = PATH;
    constexpr size_t SEP = SV.find_last_of(
        std::basic_string_view{PATH.possible_separators.data(), PATH.possible_separators.size()});
    constexpr auto const &ROOT_P = ROOT_PATH<COMP_PATH, PATH>;

    if constexpr (SEP == decltype(SV)::npos) {
      return COMP_PATH{};
    } else if constexpr (SEP + 1 <= SV.size() && SV.substr(0, SEP + 1) == ROOT_P) {
      return COMP_PATH{ROOT_P};
    } else {
      constexpr auto RESULT = []<size_t SEP>(std::integral_constant<size_t, SEP>, auto SV) {
        char data[SEP + 1]{};
        std::ranges::copy(SV.substr(0, SEP), data);
        return COMP_PATH{data};
      }(std::integral_constant<size_t, SEP>{}, SV);
      return remove_trailing_seps_impl<COMP_PATH, RESULT>();
    }
  }
}

template <template <size_t> typename COMP_PATH, auto PATH>
constexpr COMP_PATH PARENT_PATH = SHRINK<COMP_PATH, parent_path_impl<COMP_PATH, PATH>()>;

template <template <size_t> typename COMP_PATH, auto PATH>
consteval auto filename_impl() noexcept {
  using value_type = decltype(PATH)::value_type;

  if constexpr (PATH.empty()) {
    return PATH;
  } else {
    constexpr auto const &REL = RELATIVE_PATH<COMP_PATH, PATH>;
    constexpr std::basic_string_view<value_type> SV = REL;
    constexpr size_t SEP = SV.find_last_of(
        std::basic_string_view{PATH.possible_separators.data(), PATH.possible_separators.size()});
    if constexpr (SEP != std::basic_string_view<value_type>::npos) {
      return path_substr<COMP_PATH, REL, SEP + 1>();
    } else {
      return REL;
    }
  }
}

template <template <size_t> typename COMP_PATH, auto PATH>
constexpr COMP_PATH FILENAME = SHRINK<COMP_PATH, filename_impl<COMP_PATH, PATH>()>;

template <template <size_t> typename COMP_PATH, auto PATH>
consteval auto stem_impl() noexcept {
  using value_type = decltype(PATH)::value_type;

  constexpr auto const &FNAME = FILENAME<COMP_PATH, PATH>;
  if constexpr (FNAME.empty()) {
    return COMP_PATH{};
  } else {
    if constexpr (FNAME == "." || FNAME == "..") {
      return FNAME;
    } else {
      constexpr std::basic_string_view<value_type> SV = FNAME;
      constexpr size_t POS = SV.rfind('.');

      if constexpr (POS == std::basic_string_view<value_type>::npos || POS == 0 ||
                    (SV[0] == '.' && POS == 1)) {
        return FNAME;
      } else {
        return path_substr<COMP_PATH, FNAME, 0, POS>();
      }
    }
  }
}

template <template <size_t> typename COMP_PATH, auto PATH>
constexpr COMP_PATH STEM = SHRINK<COMP_PATH, stem_impl<COMP_PATH, PATH>()>;

template <template <size_t> typename COMP_PATH, auto PATH>
consteval auto extension_impl() noexcept {
  constexpr auto const &FNAME = FILENAME<COMP_PATH, PATH>;
  constexpr auto const &FSTEM = STEM<COMP_PATH, FNAME>;
  return path_substr<COMP_PATH, FNAME, FSTEM.size(), FNAME.size() - FSTEM.size()>();
}

template <template <size_t> typename COMP_PATH, auto PATH>
constexpr COMP_PATH EXTENSION = SHRINK<COMP_PATH, extension_impl<COMP_PATH, PATH>()>;

template <template <size_t> typename COMP_PATH, auto PATH>
consteval auto remove_filename_impl() noexcept {
  constexpr auto INTERMEDIATE =
      path_substr<COMP_PATH, PATH, 0, PATH.size() - filename_impl<COMP_PATH, PATH>().size()>();

  return remove_trailing_seps_impl<COMP_PATH, INTERMEDIATE>();
}

template <template <size_t> typename COMP_PATH, auto PATH>
constexpr COMP_PATH REMOVE_FILENAME = SHRINK<COMP_PATH, remove_filename_impl<COMP_PATH, PATH>()>;

template <template <size_t> typename COMP_PATH, auto PATH>
consteval auto remove_extension_impl() noexcept {
  return path_substr<COMP_PATH, PATH, 0, PATH.size() - extension_impl<COMP_PATH, PATH>().size()>();
}

template <template <size_t> typename COMP_PATH, auto PATH>
constexpr COMP_PATH REMOVE_EXTENSION = SHRINK<COMP_PATH, remove_extension_impl<COMP_PATH, PATH>()>;

template <template <size_t> typename COMP_PATH, auto PATH, auto REPLACEMENT>
consteval auto replace_filename_impl() noexcept {
  return remove_filename_impl<COMP_PATH, PATH>() / REPLACEMENT;
}

template <template <size_t> typename COMP_PATH, auto PATH, auto REPLACEMENT>
constexpr COMP_PATH REPLACE_FILENAME =
    SHRINK<COMP_PATH, replace_filename_impl<COMP_PATH, PATH, REPLACEMENT>()>;

template <template <size_t> typename COMP_PATH, auto PATH, auto REPLACEMENT>
consteval auto replace_extension_impl() noexcept {
  if constexpr (!REPLACEMENT.empty() && REPLACEMENT.data.cstr[0] != '.') {
    return replace_extension_impl<COMP_PATH, PATH, COMP_PATH{"."} + REPLACEMENT>();
  } else {
    return remove_extension_impl<COMP_PATH, PATH>() + REPLACEMENT;
  }
}

template <template <size_t> typename COMP_PATH, auto PATH, auto REPLACEMENT>
constexpr COMP_PATH REPLACE_EXTENSION =
    SHRINK<COMP_PATH, replace_extension_impl<COMP_PATH, PATH, REPLACEMENT>()>;

template <auto VALUE>
consteval auto make_integral_constant() noexcept {
  return std::integral_constant<std::decay_t<decltype(VALUE)>, VALUE>{};
}

template <template <size_t> typename COMP_PATH, auto PATH, typename... ACCUMULATED_PATHS>
consteval auto tokens_impl_impl(std::tuple<ACCUMULATED_PATHS...> accumulated) noexcept {
  using value_type = decltype(PATH)::value_type;
  if constexpr (PATH.empty()) {
    return accumulated;
  } else {
    constexpr std::basic_string_view<value_type> SV = PATH;
    constexpr size_t FIRST_SEP = SV.find_first_of(
        std::basic_string_view{PATH.possible_separators.data(), PATH.possible_separators.size()});

    constexpr auto FIRST_COMPONENT =
        path_substr<COMP_PATH, PATH, 0, std::min(PATH.size(), FIRST_SEP)>();

    if constexpr (FIRST_SEP == SV.size() - 1) {
      return std::tuple_cat(accumulated,
                            std::tuple{
                                make_integral_constant<shrink_impl<COMP_PATH, FIRST_COMPONENT>()>(),
                                make_integral_constant<COMP_PATH{}>(),
                            });
    } else {
      constexpr auto OTHER_COMPONENTS =
          path_substr<COMP_PATH, PATH, std::min(SV.size(), FIRST_COMPONENT.size() + 1)>();

      if constexpr (!FIRST_COMPONENT.empty()) {
        return tokens_impl_impl<COMP_PATH, OTHER_COMPONENTS>(std::tuple_cat(
            accumulated,
            std::tuple{make_integral_constant<shrink_impl<COMP_PATH, FIRST_COMPONENT>()>()}));
      } else {
        return tokens_impl_impl<COMP_PATH, OTHER_COMPONENTS>(accumulated);
      }
    }
  }
}

template <template <size_t> typename COMP_PATH, auto PATH>
consteval auto tokens_impl() noexcept {
  if constexpr (PATH.empty()) {
    return std::tuple<>{};
  } else if constexpr (PATH.is_absolute()) {
    constexpr auto const &ROOT_DIR = ROOT_PATH<COMP_PATH, PATH>;
    return tokens_impl_impl<COMP_PATH, COMP_PATH{relative_path_impl<COMP_PATH, PATH>()}>(
        std::tuple{make_integral_constant<shrink_impl<COMP_PATH, ROOT_DIR>()>()});
  } else {
    return tokens_impl_impl<COMP_PATH, PATH>(std::tuple<>{});
  }
}

template <template <size_t> typename COMP_PATH, auto PATH>
constexpr std::tuple TOKENS = tokens_impl<COMP_PATH, PATH>();

template <template <size_t> typename COMP_PATH, COMP_PATH ACCUMULATOR>
consteval auto lexically_normal_impl_impl() noexcept {
  if constexpr (ACCUMULATOR.empty()) {
    return COMP_PATH{"."};
  } else {
    return ACCUMULATOR;
  }
}

template <template <size_t> typename COMP_PATH, auto ACCUMULATOR, auto TOKEN, auto... TOKENS>
consteval auto lexically_normal_impl_impl() noexcept {
  using value_type = decltype(ACCUMULATOR)::value_type;

  if constexpr (TOKEN.empty() || TOKEN == "." || (TOKEN == ".." && ACCUMULATOR.is_root_path())) {
    return lexically_normal_impl_impl<COMP_PATH, ACCUMULATOR, TOKENS...>();
  } else if constexpr (TOKEN == ".." && !ACCUMULATOR.empty() &&
                       !std::basic_string_view<value_type>{ACCUMULATOR}.ends_with(
                           COMP_PATH{".."})) {
    return lexically_normal_impl_impl<COMP_PATH,
                                      remove_filename_impl<COMP_PATH, ACCUMULATOR>(),
                                      TOKENS...>();
  } else {
    return lexically_normal_impl_impl<COMP_PATH, ACCUMULATOR / TOKEN, TOKENS...>();
  }
}

template <template <size_t> typename COMP_PATH, auto ACCUMULATOR, typename... TOKENS>
consteval auto call_lexically_normal_impl_impl(std::tuple<TOKENS...>) noexcept {
  return lexically_normal_impl_impl<COMP_PATH, ACCUMULATOR, TOKENS::value...>();
}

template <template <size_t> typename COMP_PATH, auto PATH>
consteval auto lexically_normal_impl() noexcept {
  constexpr auto ROOT = root_path_impl<COMP_PATH, PATH>();
  constexpr auto REL = relative_path_impl<COMP_PATH, PATH>();
  return call_lexically_normal_impl_impl<COMP_PATH, ROOT>(tokens_impl<COMP_PATH, REL>());
}

template <template <size_t> typename COMP_PATH, auto PATH>
constexpr COMP_PATH LEXICALLY_NORMAL = SHRINK<COMP_PATH, lexically_normal_impl<COMP_PATH, PATH>()>;

template <typename NORMALIZED_PATH_TOKENS_TYPE, typename NORMALIZED_BASE_TOKENS_TYPE, size_t I = 0>
consteval auto lexically_relative_impl_get_mismatch_idx() noexcept {
  constexpr size_t ITER_UP_TO = std::min(std::tuple_size_v<NORMALIZED_PATH_TOKENS_TYPE>,
                                         std::tuple_size_v<NORMALIZED_BASE_TOKENS_TYPE>);

  if constexpr (I == ITER_UP_TO) {
    return I;
  } else {
    constexpr auto PATH_TOKEN = std::tuple_element_t<I, NORMALIZED_PATH_TOKENS_TYPE>::value;
    constexpr auto BASE_TOKEN = std::tuple_element_t<I, NORMALIZED_BASE_TOKENS_TYPE>::value;
    if constexpr (PATH_TOKEN != BASE_TOKEN) {
      return I;
    } else {
      return lexically_relative_impl_get_mismatch_idx<NORMALIZED_PATH_TOKENS_TYPE,
                                                      NORMALIZED_BASE_TOKENS_TYPE,
                                                      I + 1>();
    }
  }
}

template <template <size_t> typename COMP_PATH, typename TOKENS, size_t FROM, auto ACCUMULATOR>
consteval auto lexically_relative_impl_append_remaining_dirs() noexcept {
  if constexpr (FROM == std::tuple_size_v<TOKENS>) {
    return ACCUMULATOR;
  } else {
    constexpr COMP_PATH TO_APPEND = std::tuple_element_t<FROM, TOKENS>::value;
    return lexically_relative_impl_append_remaining_dirs<COMP_PATH,
                                                         TOKENS,
                                                         FROM + 1,
                                                         ACCUMULATOR / TO_APPEND>();
  }
}

template <template <size_t> typename COMP_PATH, size_t N, auto ACCUMULATED>
consteval auto lexically_relative_impl_append_dotdots() noexcept {
  if constexpr (N == 0) {
    return ACCUMULATED;
  } else {
    return lexically_relative_impl_append_dotdots<COMP_PATH, N - 1, ACCUMULATED / "..">();
  }
}

template <template <size_t> typename COMP_PATH, auto PATH, auto BASE>
consteval auto lexically_relative_impl() noexcept {
  if constexpr (BASE.empty()) {
    return PATH;
  } else {
    constexpr COMP_PATH NORMALIZED_PATH =
        shrink_impl<COMP_PATH, lexically_normal_impl<COMP_PATH, PATH>()>();
    constexpr COMP_PATH NORMALIZED_BASE =
        shrink_impl<COMP_PATH, lexically_normal_impl<COMP_PATH, BASE>()>();

    if constexpr (root_path_impl<COMP_PATH, NORMALIZED_BASE>() !=
                  root_path_impl<COMP_PATH, NORMALIZED_PATH>()) {
      return COMP_PATH{};
    } else {
      constexpr auto NORMALIZED_PATH_TOKENS = tokens_impl<COMP_PATH, NORMALIZED_PATH>();
      constexpr auto NORMALIZED_BASE_TOKENS = tokens_impl<COMP_PATH, NORMALIZED_BASE>();

      using normalized_path_tokens_type = std::decay_t<decltype(NORMALIZED_PATH_TOKENS)>;
      using normalized_base_tokens_type = std::decay_t<decltype(NORMALIZED_BASE_TOKENS)>;

      constexpr auto MISMATCH_IDX =
          lexically_relative_impl_get_mismatch_idx<normalized_path_tokens_type,
                                                   normalized_base_tokens_type>();

      constexpr auto ACCUMULATED =
          lexically_relative_impl_append_dotdots<COMP_PATH,
                                                 std::tuple_size_v<normalized_base_tokens_type> -
                                                     MISMATCH_IDX,
                                                 COMP_PATH{}>();

      constexpr auto ACCUMULATED2 =
          lexically_relative_impl_append_remaining_dirs<COMP_PATH,
                                                        normalized_path_tokens_type,
                                                        MISMATCH_IDX,
                                                        ACCUMULATED>();

      if constexpr (ACCUMULATED2.empty()) {
        return COMP_PATH{"."};
      } else {
        return ACCUMULATED2;
      }
    }
  }
}

template <template <size_t> typename COMP_PATH, auto PATH, auto BASE>
constexpr COMP_PATH LEXICALLY_RELATIVE =
    SHRINK<COMP_PATH, lexically_relative_impl<COMP_PATH, PATH, BASE>()>;

template <template <size_t> typename COMP_PATH, auto PATH, COMP_PATH BASE>
consteval auto lexically_proximate_impl() noexcept {
  constexpr auto RELATIVE = lexically_relative_impl<COMP_PATH, PATH, BASE>();
  if constexpr (RELATIVE.empty()) {
    return PATH;
  } else {
    return RELATIVE;
  }
}

template <template <size_t> typename COMP_PATH, auto PATH, auto BASE>
constexpr COMP_PATH LEXICALLY_PROXIMATE =
    SHRINK<COMP_PATH, lexically_proximate_impl<COMP_PATH, PATH, BASE>()>;

template <win::CompPath PATH>
consteval auto make_preferred_impl() noexcept {
  win::PathChar data[PATH.size() + 1];
  std::ranges::copy(PATH.data.cstr | std::views::transform([](win::PathChar c) {
                      if (c == u'/') {
                        return PATH.preferred_separator;
                      }
                      return c;
                    }),
                    data);
  return win::CompPath{data};
}

} // namespace detail

namespace posix {

/// @brief: Reconstruct given path shrinking its capacity to the bare minimum required to store
///         path's characters
template <CompPath PATH>
constexpr CompPath SHRINK = detail::SHRINK<CompPath, PATH>;

/// @brief: Root path. May be given any root path, e.g. "/", "//" or "///" on POSIX
template <CompPath PATH = "/">
  requires(PATH.is_root_path())
constexpr CompPath ROOT = SHRINK<PATH>;

/// @brief: Append path B to path A inserting os-preferred separator if needed and shrink the result
template <CompPath A, CompPath B>
constexpr CompPath APPEND = SHRINK<A / B>;

/// @brief: Concatenate path A with path B without inserting a separator and shrink the result
template <CompPath A, CompPath B>
constexpr CompPath CONCAT = SHRINK<A + B>;

/// @brief: Get root name if it exists, otherwise get empty path. POSIX paths have no root names,
///         but leading separators beyond the first one are treated as root name, e.g.
///         ROOT_NAME<"//usr"> == "/" and ROOT_NAME<"///usr"> == "//"
template <CompPath PATH>
constexpr CompPath ROOT_NAME = detail::ROOT_NAME<CompPath, PATH>;

/// @brief: Get root directory if it exists, otherwise get empty path. Root directory is a single
///         separator which goes right after root name if it is present
template <CompPath PATH>
constexpr CompPath ROOT_DIRECTORY = detail::ROOT_DIRECTORY<CompPath, PATH>;

/// @brief: Get root path which consists of root name and root directory if they exist. On POSIX it
///         is "/" for absolute paths and empty for relative paths
template <CompPath PATH>
constexpr CompPath ROOT_PATH = detail::ROOT_PATH<CompPath, PATH>;

/// @brief: Get path with all trailing separators removed. Root paths are returned unchanged
template <CompPath PATH>
constexpr CompPath REMOVE_TRAILING_SEPS = detail::REMOVE_TRAILING_SEPS<CompPath, PATH>;

/// @brief: Get path relative to it's root. If path has no root part, the whole path is returned
///         unchanged
template <CompPath PATH>
constexpr CompPath RELATIVE_PATH = detail::RELATIVE_PATH<CompPath, PATH>;

/// @brief: Get parent path which is a path without it's last component. Trailing separators are
///         removed first. Root path, empty path and single-component relative path have no parent:
///         root path is returned unchanged while the latter two become empty
template <CompPath PATH>
constexpr CompPath PARENT_PATH = detail::PARENT_PATH<CompPath, PATH>;

/// @brief: Get last path component. Root path and paths ending with a separator have no filename
///         which means an empty path is returned
template <CompPath PATH>
constexpr CompPath FILENAME = detail::FILENAME<CompPath, PATH>;

/// @brief: Get filename without it's extension. Leading dots are not treated as extension
///         separators which means dotfiles like ".profile" have no extension
template <CompPath PATH>
constexpr CompPath STEM = detail::STEM<CompPath, PATH>;

/// @brief: Get extension of a path's filename starting from the last dot. Empty path is returned
///         if filename has no extension
template <CompPath PATH>
constexpr CompPath EXTENSION = detail::EXTENSION<CompPath, PATH>;

/// @brief: Get path with all separators converted into os-preferred ones. POSIX has only one
///         allowed separator, so the path is simply shrunk
template <CompPath PATH>
constexpr CompPath MAKE_PREFERRED = SHRINK<PATH>;

/// @brief: Get path without it's filename which is everything before the last path component
template <CompPath PATH>
constexpr CompPath REMOVE_FILENAME = detail::REMOVE_FILENAME<CompPath, PATH>;

/// @brief: Get path without it's extension
template <CompPath PATH>
constexpr CompPath REMOVE_EXTENSION = detail::REMOVE_EXTENSION<CompPath, PATH>;

/// @brief: Get path with it's filename replaced by given replacement path. If path has no filename,
///         replacement is appended to it's parent
template <CompPath PATH, CompPath REPLACEMENT>
constexpr CompPath REPLACE_FILENAME = detail::REPLACE_FILENAME<CompPath, PATH, REPLACEMENT>;

/// @brief: Get path with it's extension replaced by given replacement path. Replacement starting
///         without a dot gets one prepended. Empty replacement removes the extension. Dotfiles like
///         ".profile" have no extension, so replacement is appended to them
template <CompPath PATH, CompPath REPLACEMENT>
constexpr CompPath REPLACE_EXTENSION = detail::REPLACE_EXTENSION<CompPath, PATH, REPLACEMENT>;

/// @brief: Decompose path into a tuple of it's components as integral constants. Absolute paths
///         start with a root-directory token. Empty components produced by trailing separators are
///         preserved
template <CompPath PATH>
constexpr std::tuple TOKENS = detail::TOKENS<CompPath, PATH>;

/// @brief: Get path in normal form: dot components are removed, dot-dot components resolve
///         preceding components, redundant separators are collapsed and leading root path is
///         preserved. Root path eats all dot-dots, e.g. "/.." resolves to "/". Empty path and path
///         consisting only of dot-dots resolve to "." and leading ".." respectively
template <CompPath PATH>
constexpr CompPath LEXICALLY_NORMAL = detail::LEXICALLY_NORMAL<CompPath, PATH>;

/// @brief: Get path relative to given base path. Both paths are normalized first. Returns empty
///         path if paths have different root paths. Empty base returns the path itself. Equal paths
///         yield "."
template <CompPath PATH, CompPath BASE>
constexpr CompPath LEXICALLY_RELATIVE = detail::LEXICALLY_RELATIVE<CompPath, PATH, BASE>;

/// @brief: Get path relative to given base path if possible, otherwise return the path itself.
///         Unlike LEXICALLY_RELATIVE it never returns an empty path when paths have different roots
template <CompPath PATH, CompPath BASE>
constexpr CompPath LEXICALLY_PROXIMATE = detail::LEXICALLY_PROXIMATE<CompPath, PATH, BASE>;

} // namespace posix

namespace win {

/// @brief: Reconstruct given path shrinking its capacity to the bare minimum required to store
///         path's characters
template <CompPath PATH>
constexpr CompPath SHRINK = detail::SHRINK<CompPath, PATH>;

/// @brief: Root path. May be given any root path, e.g. "\\", "/", "C:\\" or "//server/share"
template <CompPath PATH = "/">
  requires(PATH.is_root_path())
constexpr CompPath ROOT = SHRINK<PATH>;

/// @brief: Append path B to path A inserting os-preferred separator if needed and shrink the result
template <CompPath A, CompPath B>
constexpr CompPath APPEND = SHRINK<A / B>;

/// @brief: Concatenate path A with path B without inserting a separator and shrink the result
template <CompPath A, CompPath B>
constexpr CompPath CONCAT = SHRINK<A + B>;

/// @brief: Get root name if it exists, otherwise get empty path. Windows root name is a drive
///         letter like "C:", a UNC path like "\\server\share" or an extended-length path prefix
///         like "\\?\C:" or "\\?\UNC\server"
template <CompPath PATH>
constexpr CompPath ROOT_NAME = detail::ROOT_NAME<CompPath, PATH>;

/// @brief: Get root directory if it exists, otherwise get empty path. Root directory is a single
///         separator which goes right after root name if it is present. Paths which consist only
///         of a root name, like "C:", have no root directory
template <CompPath PATH>
constexpr CompPath ROOT_DIRECTORY = detail::ROOT_DIRECTORY<CompPath, PATH>;

/// @brief: Get root path which consists of root name and root directory if they exist. If root
///         directory is missing, root path equals root name, e.g. ROOT_PATH<"C:"> == "C:"
template <CompPath PATH>
constexpr CompPath ROOT_PATH = detail::ROOT_PATH<CompPath, PATH>;

/// @brief: Get path with all trailing separators removed. Root paths are returned unchanged
template <CompPath PATH>
constexpr CompPath REMOVE_TRAILING_SEPS = detail::REMOVE_TRAILING_SEPS<CompPath, PATH>;

/// @brief: Get path relative to it's root. If path has no root part, the whole path is returned
///         unchanged
template <CompPath PATH>
constexpr CompPath RELATIVE_PATH = detail::RELATIVE_PATH<CompPath, PATH>;

/// @brief: Get parent path which is a path without it's last component. Trailing separators are
///         removed first. Root path, empty path and single-component relative path have no parent:
///         root path is returned unchanged while the latter two become empty
template <CompPath PATH>
constexpr CompPath PARENT_PATH = detail::PARENT_PATH<CompPath, PATH>;

/// @brief: Get last path component. Root path and paths ending with a separator have no filename
///         which means an empty path is returned
template <CompPath PATH>
constexpr CompPath FILENAME = detail::FILENAME<CompPath, PATH>;

/// @brief: Get filename without it's extension. Leading dots are not treated as extension
///         separators which means dotfiles like ".profile" have no extension
template <CompPath PATH>
constexpr CompPath STEM = detail::STEM<CompPath, PATH>;

/// @brief: Get extension of a path's filename starting from the last dot. Empty path is returned
///         if filename has no extension
template <CompPath PATH>
constexpr CompPath EXTENSION = detail::EXTENSION<CompPath, PATH>;

/// @brief: Get path with all separators converted into os-preferred ones, e.g. "foo/bar" becomes
///         "foo\bar". Existing os-preferred separators are left in place which means repeated
///         separators are not collapsed
template <CompPath PATH>
constexpr CompPath MAKE_PREFERRED = SHRINK<detail::make_preferred_impl<PATH>()>;

/// @brief: Get path without it's filename which is everything before the last path component
template <CompPath PATH>
constexpr CompPath REMOVE_FILENAME = detail::REMOVE_FILENAME<CompPath, PATH>;

/// @brief: Get path without it's extension
template <CompPath PATH>
constexpr CompPath REMOVE_EXTENSION = detail::REMOVE_EXTENSION<CompPath, PATH>;

/// @brief: Get path with it's filename replaced by given replacement path. If path has no filename,
///         replacement is appended to it's parent
template <CompPath PATH, CompPath REPLACEMENT>
constexpr CompPath REPLACE_FILENAME = detail::REPLACE_FILENAME<CompPath, PATH, REPLACEMENT>;

/// @brief: Get path with it's extension replaced by given replacement path. Replacement starting
///         without a dot gets one prepended. Empty replacement removes the extension. Dotfiles like
///         ".profile" have no extension, so replacement is appended to them
template <CompPath PATH, CompPath REPLACEMENT>
constexpr CompPath REPLACE_EXTENSION = detail::REPLACE_EXTENSION<CompPath, PATH, REPLACEMENT>;

/// @brief: Decompose path into a tuple of it's components as integral constants. Absolute paths
///         start with a root-path token. Empty components produced by trailing separators are
///         preserved
template <CompPath PATH>
constexpr std::tuple TOKENS = detail::TOKENS<CompPath, PATH>;

/// @brief: Get path in normal form: dot components are removed, dot-dot components resolve
///         preceding components, redundant separators are collapsed and leading root path is
///         preserved. Root path eats all dot-dots, e.g. "C:\.." resolves to "C:\". Empty path and
///         path consisting only of dot-dots resolve to "." and leading ".." respectively
template <CompPath PATH>
constexpr CompPath LEXICALLY_NORMAL = detail::LEXICALLY_NORMAL<CompPath, PATH>;

/// @brief: Get path relative to given base path. Both paths are normalized first. Returns empty
///         path if paths have different root paths, e.g. "C:" and "D:" drives. Empty base returns
///         the path itself. Equal paths yield "."
template <CompPath PATH, CompPath BASE>
constexpr CompPath LEXICALLY_RELATIVE = detail::LEXICALLY_RELATIVE<CompPath, PATH, BASE>;

/// @brief: Get path relative to given base path if possible, otherwise return the path itself.
///         Unlike LEXICALLY_RELATIVE it never returns an empty path when paths have different roots
template <CompPath PATH, CompPath BASE>
constexpr CompPath LEXICALLY_PROXIMATE = detail::LEXICALLY_PROXIMATE<CompPath, PATH, BASE>;

} // namespace win

#if COMPPATH_PLATFORM_POSIX
using namespace posix;
#elif COMPPATH_PLATFORM_WINDOWS
using namespace win;
#endif

} // namespace comppath

#undef COMPPATH_PLATFORM_POSIX
#undef COMPPATH_PLATFORM_WINDOWS

#endif
