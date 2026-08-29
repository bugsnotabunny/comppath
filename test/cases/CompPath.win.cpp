#include "comppath/CompPath.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("windows CompPath") {
  using namespace comppath::win;

  STATIC_CHECK(SHRINK<"">.empty());
  STATIC_CHECK(SHRINK<"foo"> == "foo");
  STATIC_CHECK(SHRINK<"foo/bar"> == "foo/bar");
  STATIC_CHECK(SHRINK<"C:\\foo\\bar"> == "C:\\foo\\bar");

  STATIC_CHECK(ROOT<"\\"> == "\\");
  STATIC_CHECK(ROOT<"/"> == "/");
  STATIC_CHECK(ROOT<"C:\\"> == "C:\\");
  STATIC_CHECK(ROOT<"C:/"> == "C:/");
  STATIC_CHECK(ROOT<"//server/share"> == "//server/share"); // UNC root path
  STATIC_CHECK(ROOT<R"(\\?\C:\)"> == R"(\\?\C:\)");         // UNC root path

  STATIC_CHECK(APPEND<"foo", "bar"> == "foo\\bar");   // no trailing sep -> insert '\'
  STATIC_CHECK(APPEND<"foo\\", "bar"> == "foo\\bar"); // already trailing sep
  STATIC_CHECK(APPEND<"foo/", "bar"> == "foo/bar");   // forward slash retained
  STATIC_CHECK(APPEND<"foo", "bar/"> == "foo\\bar/"); // rhs with trailing sep
  STATIC_CHECK(APPEND<"", "bar"> == "bar");
  STATIC_CHECK(APPEND<"foo", ""> == "foo\\");
  STATIC_CHECK(APPEND<"C:\\foo", "bar"> == "C:\\foo\\bar");
  STATIC_CHECK(APPEND<"C:\\foo\\", "bar"> == "C:\\foo\\bar");
  STATIC_CHECK(APPEND<"C:/foo", "bar"> == "C:/foo\\bar");
  STATIC_CHECK(APPEND<"C:/foo/", "bar"> == "C:/foo/bar");

  STATIC_CHECK(CONCAT<"foo", "bar"> == "foobar");
  STATIC_CHECK(CONCAT<"foo\\", "bar"> == "foo\\bar");
  STATIC_CHECK(CONCAT<"foo/", "bar"> == "foo/bar");
  STATIC_CHECK(CONCAT<"", "bar"> == "bar");
  STATIC_CHECK(CONCAT<"foo", ""> == "foo");

  STATIC_CHECK(ROOT_NAME<"C:\\foo\\bar"> == "C:");
  STATIC_CHECK(ROOT_NAME<"C:/foo/bar"> == "C:");
  STATIC_CHECK(ROOT_NAME<"C:"> == "C:");
  STATIC_CHECK(ROOT_NAME<"\\foo">.empty());
  STATIC_CHECK(ROOT_NAME<"/foo">.empty());
  STATIC_CHECK(ROOT_NAME<"//server/share/foo"> == "//server/share");
  STATIC_CHECK(ROOT_NAME<R"(\\server\share\foo)"> == R"(\\server\share)");
  STATIC_CHECK(ROOT_NAME<"//?\\UNC\\server/share/foo"> == "//?\\UNC\\server/share");
  STATIC_CHECK(ROOT_NAME<"\\\\?\\uNc/server/share/foo"> == "\\\\?\\uNc/server/share");
  STATIC_CHECK(ROOT_NAME<R"(\\server\share\foo)"> == R"(\\server\share)");
  STATIC_CHECK(ROOT_NAME<"foo/bar">.empty());

  STATIC_CHECK(ROOT_DIRECTORY<"C:\\foo\\bar"> == "\\");
  STATIC_CHECK(ROOT_DIRECTORY<"C:/foo/bar"> == "/"); // root directory is '/'
  STATIC_CHECK(ROOT_DIRECTORY<"\\foo"> == "\\");
  STATIC_CHECK(ROOT_DIRECTORY<"/foo"> == "/");
  STATIC_CHECK(ROOT_DIRECTORY<"C:">.empty()); // no root directory
  STATIC_CHECK(ROOT_DIRECTORY<"C:\\"> == "\\");
  STATIC_CHECK(ROOT_DIRECTORY<"//server/share/foo"> == "/"); // root directory is '/'
  STATIC_CHECK(ROOT_DIRECTORY<"foo/bar">.empty());

  STATIC_CHECK(ROOT_PATH<"C:\\foo\\bar"> == "C:\\");
  STATIC_CHECK(ROOT_PATH<"C:/foo/bar"> == "C:/");
  STATIC_CHECK(ROOT_PATH<"\\foo"> == "\\");
  STATIC_CHECK(ROOT_PATH<"/foo"> == "/");

  STATIC_CHECK(ROOT_PATH<"C:"> == "C:"); // no root directory -> root_path = root_name
  STATIC_CHECK(ROOT_PATH<"//server/share/foo"> == "//server/share/");
  STATIC_CHECK(ROOT_PATH<"foo/bar">.empty());

  STATIC_CHECK(REMOVE_TRAILING_SEPS<"foo\\"> == "foo");
  STATIC_CHECK(REMOVE_TRAILING_SEPS<"foo/"> == "foo");
  STATIC_CHECK(REMOVE_TRAILING_SEPS<"foo\\\\"> == "foo");
  STATIC_CHECK(REMOVE_TRAILING_SEPS<"foo//"> == "foo");
  STATIC_CHECK(REMOVE_TRAILING_SEPS<"C:\\foo\\"> == "C:\\foo");
  STATIC_CHECK(REMOVE_TRAILING_SEPS<"C:\\"> == "C:\\"); // root path unchanged
  STATIC_CHECK(REMOVE_TRAILING_SEPS<"\\"> == "\\");
  STATIC_CHECK(REMOVE_TRAILING_SEPS<"">.empty());

  STATIC_CHECK(RELATIVE_PATH<"C:\\foo\\bar"> == "foo\\bar");
  STATIC_CHECK(RELATIVE_PATH<"C:/foo/bar"> == "foo/bar");
  STATIC_CHECK(RELATIVE_PATH<"\\foo\\bar"> == "foo\\bar"); // absolute without root-name
  STATIC_CHECK(RELATIVE_PATH<"/foo/bar"> == "foo/bar");
  STATIC_CHECK(RELATIVE_PATH<"foo/bar"> == "foo/bar"); // already relative
  STATIC_CHECK(RELATIVE_PATH<"C:">.empty());           // no relative part
  STATIC_CHECK(RELATIVE_PATH<"C:\\">.empty());

  STATIC_CHECK(PARENT_PATH<"C:\\foo\\bar"> == "C:\\foo");
  STATIC_CHECK(PARENT_PATH<"C:/foo/bar"> == "C:/foo");
  STATIC_CHECK(PARENT_PATH<"\\foo\\bar"> == "\\foo");
  STATIC_CHECK(PARENT_PATH<"/foo/bar"> == "/foo");
  STATIC_CHECK(PARENT_PATH<"foo\\bar"> == "foo");
  STATIC_CHECK(PARENT_PATH<"foo/bar"> == "foo");
  STATIC_CHECK(PARENT_PATH<"foo">.empty());    // no parent
  STATIC_CHECK(PARENT_PATH<"foo\\"> == "foo"); // trailing sep removed before parent
  STATIC_CHECK(PARENT_PATH<"C:\\"> == "C:\\"); // root path -> itself
  STATIC_CHECK(PARENT_PATH<"\\"> == "\\");
  STATIC_CHECK(PARENT_PATH<"C:"> == "C:");
  STATIC_CHECK(PARENT_PATH<"//server/share/foo"> == "//server/share/");
  STATIC_CHECK(PARENT_PATH<"//server/share/"> == "//server/share/");
  STATIC_CHECK(PARENT_PATH<"//server/share"> == "//server/share");

  STATIC_CHECK(FILENAME<"C:\\foo\\bar"> == "bar");
  STATIC_CHECK(FILENAME<"C:/foo/bar"> == "bar");
  STATIC_CHECK(FILENAME<"foo\\bar"> == "bar");
  STATIC_CHECK(FILENAME<"foo/bar"> == "bar");
  STATIC_CHECK(FILENAME<"foo"> == "foo");
  STATIC_CHECK(FILENAME<"foo\\">.empty()); // trailing sep -> no filename
  STATIC_CHECK(FILENAME<"foo/">.empty());
  STATIC_CHECK(FILENAME<"C:\\">.empty()); // root directory
  STATIC_CHECK(FILENAME<"C:">.empty());   // just root-name
  STATIC_CHECK(FILENAME<"">.empty());

  STATIC_CHECK(STEM<"foo.txt"> == "foo");
  STATIC_CHECK(STEM<"foo"> == "foo");
  STATIC_CHECK(STEM<".profile"> == ".profile");  // leading dot, no extension
  STATIC_CHECK(STEM<"foo.tar.gz"> == "foo.tar"); // only last dot considered
  STATIC_CHECK(STEM<"C:\\foo\\bar.txt"> == "bar");
  STATIC_CHECK(STEM<"foo/bar."> == "bar"); // trailing dot -> extension empty, stem is "bar"
  STATIC_CHECK(STEM<"foo.."> == "foo.");
  STATIC_CHECK(STEM<".foo"> == ".foo");
  STATIC_CHECK(STEM<"">.empty());

  STATIC_CHECK(EXTENSION<"foo.txt"> == ".txt");
  STATIC_CHECK(EXTENSION<"foo">.empty());
  STATIC_CHECK(EXTENSION<".profile">.empty());
  STATIC_CHECK(EXTENSION<"foo.tar.gz"> == ".gz");
  STATIC_CHECK(EXTENSION<"foo."> == ".");
  STATIC_CHECK(EXTENSION<"foo.."> == ".");
  STATIC_CHECK(EXTENSION<"C:\\foo\\bar.txt"> == ".txt");

  STATIC_CHECK(MAKE_PREFERRED<"foo/bar"> == "foo\\bar");
  STATIC_CHECK(MAKE_PREFERRED<"C:\\foo\\bar"> == "C:\\foo\\bar");
  STATIC_CHECK(MAKE_PREFERRED<"C:/foo//bar"> == "C:\\foo\\\\bar");

  STATIC_CHECK(REMOVE_FILENAME<"C:\\foo\\bar"> == "C:\\foo");
  STATIC_CHECK(REMOVE_FILENAME<"foo/bar"> == "foo");
  STATIC_CHECK(REMOVE_FILENAME<"foo">.empty());
  STATIC_CHECK(REMOVE_FILENAME<"foo\\"> == "foo");
  STATIC_CHECK(REMOVE_FILENAME<"C:"> == "C:");
  STATIC_CHECK(REMOVE_FILENAME<"foo/bar/"> == "foo/bar");
  STATIC_CHECK(REMOVE_FILENAME<"foo\\bar.txt"> == "foo");

  STATIC_CHECK(REMOVE_EXTENSION<"foo.txt"> == "foo");
  STATIC_CHECK(REMOVE_EXTENSION<"foo.tar.gz"> == "foo.tar");
  STATIC_CHECK(REMOVE_EXTENSION<"foo"> == "foo");
  STATIC_CHECK(REMOVE_EXTENSION<".profile"> == ".profile");
  STATIC_CHECK(REMOVE_EXTENSION<"foo."> == "foo");
  STATIC_CHECK(REMOVE_EXTENSION<"foo.."> == "foo.");
  STATIC_CHECK(REMOVE_EXTENSION<"C:\\foo\\bar.txt"> == "C:\\foo\\bar");

  STATIC_CHECK(REPLACE_FILENAME<"foo/bar", "baz"> == "foo\\baz");
  STATIC_CHECK(REPLACE_FILENAME<"foo/bar.txt", "baz"> == "foo\\baz");
  STATIC_CHECK(REPLACE_FILENAME<"foo/", "baz"> == "foo\\baz");
  STATIC_CHECK(REPLACE_FILENAME<"foo", "baz"> == "baz");
  STATIC_CHECK(REPLACE_FILENAME<"C:\\foo\\bar", "baz"> == "C:\\foo\\baz");
  STATIC_CHECK(REPLACE_FILENAME<"C:\\", "baz"> == "C:\\baz");

  STATIC_CHECK(REPLACE_EXTENSION<"foo.txt", ".csv"> == "foo.csv");
  STATIC_CHECK(REPLACE_EXTENSION<"foo.txt", "csv"> == "foo.csv"); // adds dot if missing
  STATIC_CHECK(REPLACE_EXTENSION<"foo", ".txt"> == "foo.txt");
  STATIC_CHECK(REPLACE_EXTENSION<"foo", "txt"> == "foo.txt"); // adds dot
  STATIC_CHECK(REPLACE_EXTENSION<"foo.tar.gz", ".bz2"> == "foo.tar.bz2");
  STATIC_CHECK(REPLACE_EXTENSION<".profile", ".bak"> == ".profile.bak"); // leading dot file
  STATIC_CHECK(REPLACE_EXTENSION<"foo.", ".txt"> == "foo.txt"); // removes trailing dot and adds new
  STATIC_CHECK(REPLACE_EXTENSION<"foo..", ".txt"> == "foo..txt");
  STATIC_CHECK(REPLACE_EXTENSION<"C:\\foo\\bar.txt", ".bak"> == "C:\\foo\\bar.bak");

  constexpr auto TOKENS1 = TOKENS<"C:\\foo\\bar">;
  STATIC_CHECK(std::tuple_size_v<decltype(TOKENS1)> == 3);
  STATIC_CHECK(std::get<0>(TOKENS1).value == "C:\\");
  STATIC_CHECK(std::get<1>(TOKENS1).value == "foo");
  STATIC_CHECK(std::get<2>(TOKENS1).value == "bar");

  constexpr auto TOKENS2 = TOKENS<"foo/bar/baz">;
  STATIC_CHECK(std::tuple_size_v<decltype(TOKENS2)> == 3);
  STATIC_CHECK(std::get<0>(TOKENS2).value == "foo");
  STATIC_CHECK(std::get<1>(TOKENS2).value == "bar");
  STATIC_CHECK(std::get<2>(TOKENS2).value == "baz");

  constexpr auto TOKENS3 = TOKENS<"\\foo\\bar">;
  STATIC_CHECK(std::tuple_size_v<decltype(TOKENS3)> == 3);
  STATIC_CHECK(std::get<0>(TOKENS3).value == "\\");
  STATIC_CHECK(std::get<1>(TOKENS3).value == "foo");
  STATIC_CHECK(std::get<2>(TOKENS3).value == "bar");

  constexpr auto TOKENS4 = TOKENS<"C:">;
  STATIC_CHECK(std::tuple_size_v<decltype(TOKENS4)> == 1);
  STATIC_CHECK(std::get<0>(TOKENS4).value == "C:");

  constexpr auto TOKENS5 = TOKENS<"">;
  STATIC_CHECK(std::tuple_size_v<decltype(TOKENS5)> == 0);

  constexpr auto TOKENS6 = TOKENS<"a\\b\\">;
  STATIC_CHECK(std::tuple_size_v<decltype(TOKENS6)> == 3);
  STATIC_CHECK(std::get<0>(TOKENS6).value == "a");
  STATIC_CHECK(std::get<1>(TOKENS6).value == "b");
  STATIC_CHECK(std::get<2>(TOKENS6).value.empty());

  constexpr auto TOKENS7 = TOKENS<"a\\/b\\">;
  STATIC_CHECK(std::tuple_size_v<decltype(TOKENS7)> == 3);
  STATIC_CHECK(std::get<0>(TOKENS7).value == "a");
  STATIC_CHECK(std::get<1>(TOKENS7).value == "b");
  STATIC_CHECK(std::get<2>(TOKENS7).value.empty());

  STATIC_CHECK(LEXICALLY_NORMAL<"foo/./bar"> == "foo\\bar");
  STATIC_CHECK(LEXICALLY_NORMAL<"foo/../bar"> == "bar");
  STATIC_CHECK(LEXICALLY_NORMAL<"foo/bar/.."> == "foo");
  STATIC_CHECK(LEXICALLY_NORMAL<"foo/bar/../baz"> == "foo\\baz");
  STATIC_CHECK(LEXICALLY_NORMAL<"foo/../../bar"> == "..\\bar");
  STATIC_CHECK(LEXICALLY_NORMAL<"foo/bar/../../baz"> == "baz");
  STATIC_CHECK(LEXICALLY_NORMAL<"C:\\foo\\.\\bar"> == "C:\\foo\\bar");
  STATIC_CHECK(LEXICALLY_NORMAL<"C:\\foo\\..\\bar"> == "C:\\bar");
  STATIC_CHECK(LEXICALLY_NORMAL<"C:\\foo\\..\\..\\bar"> == "C:\\bar");
  STATIC_CHECK(LEXICALLY_NORMAL<"\\foo\\..\\bar"> == "\\bar");
  STATIC_CHECK(LEXICALLY_NORMAL<"\\foo\\..\\..\\bar"> == "\\bar");
  STATIC_CHECK(LEXICALLY_NORMAL<"foo/."> == "foo");
  STATIC_CHECK(LEXICALLY_NORMAL<"foo/.."> == ".");
  STATIC_CHECK(LEXICALLY_NORMAL<"foo/../.."> == "..");
  STATIC_CHECK(LEXICALLY_NORMAL<"//server/share/foo/.."> == "//server/share/");
  STATIC_CHECK(LEXICALLY_NORMAL<"//server/share/foo/../.."> == "//server/share/");

  STATIC_CHECK(LEXICALLY_RELATIVE<"foo/bar", "foo"> == "bar");
  STATIC_CHECK(LEXICALLY_RELATIVE<"foo/bar/baz", "foo/bar"> == "baz");
  STATIC_CHECK(LEXICALLY_RELATIVE<"foo/bar", "foo/baz"> == "..\\bar");
  STATIC_CHECK(LEXICALLY_RELATIVE<"foo/bar", "foo/bar/baz"> == "..");
  STATIC_CHECK(LEXICALLY_RELATIVE<"foo/bar", "foo/bar/baz/qux"> == "..\\..");
  STATIC_CHECK(LEXICALLY_RELATIVE<"foo/bar", "foo"> == "bar");
  STATIC_CHECK(LEXICALLY_RELATIVE<"foo/bar", "foo/bar"> == ".");
  STATIC_CHECK(LEXICALLY_RELATIVE<"foo/bar", "foo/bar/."> == ".");
  STATIC_CHECK(LEXICALLY_RELATIVE<"C:\\foo\\bar", "C:\\foo"> == "bar");
  STATIC_CHECK(LEXICALLY_RELATIVE<"C:\\foo\\bar", "D:\\foo">.empty());
  STATIC_CHECK(LEXICALLY_RELATIVE<"\\foo\\bar", "\\foo"> == "bar");
  STATIC_CHECK(LEXICALLY_RELATIVE<"\\foo\\bar", "\\baz"> == "..\\foo\\bar");
  STATIC_CHECK(LEXICALLY_RELATIVE<"foo/bar", ""> == "foo/bar");

  STATIC_CHECK(LEXICALLY_PROXIMATE<"foo/bar", "foo"> == "bar");
  STATIC_CHECK(LEXICALLY_PROXIMATE<"C:\\foo\\bar", "D:\\foo"> == "C:\\foo\\bar");
  STATIC_CHECK(LEXICALLY_PROXIMATE<"foo/bar", "baz"> == "..\\foo\\bar");

  STATIC_CHECK(comppath::win::CompPath{u"😷😷/перкеля/bear/💃💃💃"} ==
               u8"😷😷/перкеля/bear/💃💃💃");
  STATIC_CHECK(comppath::win::CompPath{U"😷😷/перкеля/bear/💃💃💃"} ==
               u8"😷😷/перкеля/bear/💃💃💃");
}
