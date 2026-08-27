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

TEST_CASE("posix CompPath") {
  using namespace comppath::posix;

  constexpr comppath::posix::CompPath EMPTY = "";
  constexpr comppath::posix::CompPath ROOT_SLASH = "/";
  constexpr comppath::posix::CompPath USR = "/usr";
  constexpr comppath::posix::CompPath USR_BIN = "/usr/bin";
  constexpr comppath::posix::CompPath USR_BIN_TRAIL = "/usr/bin/";
  constexpr comppath::posix::CompPath REL_USR = "usr";
  constexpr comppath::posix::CompPath REL_USR_BIN = "usr/bin";
  constexpr comppath::posix::CompPath DOT = ".";
  constexpr comppath::posix::CompPath DOTDOT = "..";
  constexpr comppath::posix::CompPath FILE_TXT = " file.txt ";
  constexpr comppath::posix::CompPath HIDDEN = ".hidden";
  constexpr comppath::posix::CompPath A_B_C = "a/b/c.txt";
  constexpr comppath::posix::CompPath A_B_C_D = "a/b/c.d.txt";
  constexpr comppath::posix::CompPath A_B_DOTC = "a/b/.c";
  constexpr comppath::posix::CompPath A_B_DOTDOT = "a/b/..";
  constexpr comppath::posix::CompPath A_B_DOTDOT_C = "a/b/../c";
  constexpr comppath::posix::CompPath ABS_DOTDOT_C = "/a/b/../c";
  constexpr comppath::posix::CompPath DOUBLE_SLASH = "//";
  constexpr comppath::posix::CompPath DOUBLE_SEP = "a//b";
  constexpr comppath::posix::CompPath TRAIL_REL = "a/b/";
  constexpr comppath::posix::CompPath DOUBLE_SLASH_ROOT = "//";
  constexpr comppath::posix::CompPath DOUBLE_SLASH_USR = "//usr";
  constexpr comppath::posix::CompPath DOUBLE_SLASH_USR_BIN = "//usr/bin";

  STATIC_CHECK(APPEND<REL_USR, "bin"> == "usr/bin");
  STATIC_CHECK(APPEND<USR, "bin"> == "/usr/bin");
  STATIC_CHECK(APPEND<ROOT<>, "usr"> == "/usr");

  STATIC_CHECK(CONCAT<REL_USR, "bin"> == "usrbin");
  STATIC_CHECK(CONCAT<USR, "bin"> == "/usrbin");
  STATIC_CHECK(CONCAT<REL_USR, "/bin"> == "usr/bin");

  STATIC_CHECK(EMPTY.empty());
  STATIC_CHECK(!ROOT_SLASH.empty());
  STATIC_CHECK(!USR.empty());

  STATIC_CHECK(ROOT_SLASH.is_absolute());
  STATIC_CHECK(USR.is_absolute());
  STATIC_CHECK(!REL_USR.is_absolute());
  STATIC_CHECK(!EMPTY.is_absolute());

  STATIC_CHECK(!ROOT_SLASH.is_relative());
  STATIC_CHECK(!USR.is_relative());
  STATIC_CHECK(REL_USR.is_relative());
  STATIC_CHECK(EMPTY.is_relative()); // empty path is relative

  STATIC_CHECK(USR == "/usr");
  STATIC_CHECK(USR != "/usr/");
  STATIC_CHECK(USR == comppath::posix::CompPath{"/usr"});
  STATIC_CHECK((USR <=> "/usr") == 0);
  STATIC_CHECK(USR < "/usr/bin");
  STATIC_CHECK(EMPTY == "");

  // operator/  (inserts separator)
  STATIC_CHECK(REL_USR / "bin" == "usr/bin");
  STATIC_CHECK(USR / "bin" == "/usr/bin");
  STATIC_CHECK(ROOT<> / "usr" == "/usr");
  STATIC_CHECK(USR_BIN / "" == "/usr/bin/"); // appends trailing slash
  STATIC_CHECK(REL_USR / "" == "usr/");

  // operator+  (no separator)
  STATIC_CHECK(REL_USR + "bin" == "usrbin");
  STATIC_CHECK(USR + "bin" == "/usrbin");
  STATIC_CHECK(REL_USR + "/bin" == "usr/bin");
  STATIC_CHECK(EMPTY + "foo" == "foo");

  STATIC_CHECK(ROOT_NAME<EMPTY> == "");
  STATIC_CHECK(ROOT_NAME<ROOT_SLASH> == "");
  STATIC_CHECK(ROOT_NAME<USR> == "");

  STATIC_CHECK(ROOT_DIRECTORY<EMPTY> == "");
  STATIC_CHECK(ROOT_DIRECTORY<ROOT_SLASH> == "/");
  STATIC_CHECK(ROOT_DIRECTORY<USR> == "/");
  STATIC_CHECK(ROOT_DIRECTORY<REL_USR> == "");

  STATIC_CHECK(ROOT_PATH<EMPTY> == "");
  STATIC_CHECK(ROOT_PATH<ROOT_SLASH> == "/");
  STATIC_CHECK(ROOT_PATH<USR> == "/");
  STATIC_CHECK(ROOT_PATH<REL_USR> == "");

  STATIC_CHECK(RELATIVE_PATH<EMPTY> == "");
  STATIC_CHECK(RELATIVE_PATH<ROOT_SLASH> == "");
  STATIC_CHECK(RELATIVE_PATH<USR> == "usr");
  STATIC_CHECK(RELATIVE_PATH<USR_BIN> == "usr/bin");
  STATIC_CHECK(RELATIVE_PATH<REL_USR> == "usr");
  STATIC_CHECK(RELATIVE_PATH<DOUBLE_SLASH> == ""); // root only

  STATIC_CHECK(PARENT_PATH<EMPTY> == "");
  STATIC_CHECK(PARENT_PATH<ROOT_SLASH> == "/");
  STATIC_CHECK(PARENT_PATH<USR> == "/");
  STATIC_CHECK(PARENT_PATH<USR_BIN> == "/usr");
  STATIC_CHECK(PARENT_PATH<USR_BIN_TRAIL> == "/usr/bin"); // removes trailing slash
  STATIC_CHECK(PARENT_PATH<REL_USR> == "");
  STATIC_CHECK(PARENT_PATH<REL_USR_BIN> == "usr");
  STATIC_CHECK(PARENT_PATH<DOT> == "");
  STATIC_CHECK(PARENT_PATH<DOTDOT> == "");
  STATIC_CHECK(PARENT_PATH<A_B_DOTDOT> == "a/b");
  STATIC_CHECK(PARENT_PATH<A_B_DOTDOT_C> == "a/b/.."); // does not resolve ".."
  STATIC_CHECK(PARENT_PATH<ABS_DOTDOT_C> == "/a/b/..");
  STATIC_CHECK(PARENT_PATH<DOUBLE_SLASH> == "//"); // root with double slash?

  STATIC_CHECK(FILENAME<EMPTY> == "");
  STATIC_CHECK(FILENAME<ROOT_SLASH> == "");
  STATIC_CHECK(FILENAME<USR> == "usr");
  STATIC_CHECK(FILENAME<USR_BIN> == "bin");
  STATIC_CHECK(FILENAME<USR_BIN_TRAIL> == ""); // trailing slash => empty filename
  STATIC_CHECK(FILENAME<REL_USR> == "usr");
  STATIC_CHECK(FILENAME<REL_USR_BIN> == "bin");
  STATIC_CHECK(FILENAME<DOT> == ".");
  STATIC_CHECK(FILENAME<DOTDOT> == "..");
  STATIC_CHECK(FILENAME<FILE_TXT> == " file.txt ");
  STATIC_CHECK(FILENAME<HIDDEN> == ".hidden");
  STATIC_CHECK(FILENAME<A_B_C> == "c.txt");
  STATIC_CHECK(FILENAME<A_B_C_D> == "c.d.txt");
  STATIC_CHECK(FILENAME<A_B_DOTC> == ".c");
  STATIC_CHECK(FILENAME<A_B_DOTDOT_C> == "c");
  STATIC_CHECK(FILENAME<DOUBLE_SEP> == "b"); // last component "b"
  STATIC_CHECK(FILENAME<TRAIL_REL> == "");   // trailing slash

  STATIC_CHECK(STEM<EMPTY> == "");
  STATIC_CHECK(EXTENSION<EMPTY> == "");

  STATIC_CHECK(STEM<FILE_TXT> == " file");
  STATIC_CHECK(EXTENSION<FILE_TXT> == ".txt ");

  STATIC_CHECK(STEM<HIDDEN> == ".hidden"); // leading dot does not count as extension
  STATIC_CHECK(EXTENSION<HIDDEN> == "");

  STATIC_CHECK(STEM<A_B_C> == "c");
  STATIC_CHECK(EXTENSION<A_B_C> == ".txt");

  STATIC_CHECK(STEM<A_B_C_D> == "c.d"); // last dot separates extension

  STATIC_CHECK(EXTENSION<A_B_C_D> == ".txt");

  STATIC_CHECK(STEM<A_B_DOTC> == ".c"); // leading dot → no extension
  STATIC_CHECK(EXTENSION<A_B_DOTC> == "");

  STATIC_CHECK(STEM<DOT> == ".");
  STATIC_CHECK(EXTENSION<DOT> == "");
  STATIC_CHECK(STEM<DOTDOT> == "..");
  STATIC_CHECK(EXTENSION<DOTDOT> == "");

  STATIC_CHECK(STEM<"..file"> == "..file");
  STATIC_CHECK(EXTENSION<"..file"> == "");

  STATIC_CHECK(STEM<"file."> == "file");
  STATIC_CHECK(EXTENSION<"file."> == ".");

  STATIC_CHECK(STEM<"f."> == "f");
  STATIC_CHECK(EXTENSION<"f."> == ".");

  STATIC_CHECK(STEM<".profile"> == ".profile");
  STATIC_CHECK(EXTENSION<".profile"> == "");

  STATIC_CHECK(STEM<"archive.tar.gz"> == "archive.tar");
  STATIC_CHECK(EXTENSION<"archive.tar.gz"> == ".gz");

  // remove_filename
  STATIC_CHECK(REMOVE_FILENAME<USR_BIN> == "/usr");
  STATIC_CHECK(REMOVE_FILENAME<REL_USR_BIN> == "usr");
  STATIC_CHECK(REMOVE_FILENAME<A_B_C> == "a/b");
  STATIC_CHECK(REMOVE_FILENAME<ROOT_SLASH> == "/"); // root unchanged

  // remove_extension
  STATIC_CHECK(REMOVE_EXTENSION<FILE_TXT> == " file");
  STATIC_CHECK(REMOVE_EXTENSION<A_B_C> == "a/b/c");
  STATIC_CHECK(REMOVE_EXTENSION<A_B_C_D> == "a/b/c.d");
  STATIC_CHECK(REMOVE_EXTENSION<HIDDEN> == ".hidden"); // no extension
  STATIC_CHECK(REMOVE_EXTENSION<DOT> == ".");
  STATIC_CHECK(REMOVE_EXTENSION<"file."> == "file");

  // replace_filename
  STATIC_CHECK(REPLACE_FILENAME<USR_BIN, "new"> == "/usr/new");
  STATIC_CHECK(REPLACE_FILENAME<REL_USR_BIN, "new"> == "usr/new");
  STATIC_CHECK(REPLACE_FILENAME<A_B_C, "new"> == "a/b/new");
  STATIC_CHECK(REPLACE_FILENAME<ROOT_SLASH, "new"> == "/new"); // root + replacement

  STATIC_CHECK(REPLACE_EXTENSION<FILE_TXT, "bak"> == " file.bak");
  STATIC_CHECK(REPLACE_EXTENSION<A_B_C, "bak"> == "a/b/c.bak");
  STATIC_CHECK(REPLACE_EXTENSION<A_B_C_D, "bak"> == "a/b/c.d.bak");
  STATIC_CHECK(REPLACE_EXTENSION<HIDDEN, "bak"> == ".hidden.bak"); // adds extension
  STATIC_CHECK(REPLACE_EXTENSION<"file.", "bak"> == "file.bak");
  STATIC_CHECK(REPLACE_EXTENSION<"file", "bak"> == "file.bak");
  STATIC_CHECK(REPLACE_EXTENSION<"file.a", "bak"> == "file.bak");

  STATIC_CHECK(constants_eq(TOKENS<EMPTY>, std::tuple{}));
  STATIC_CHECK(constants_eq(TOKENS<ROOT_SLASH>, std::tuple{u8"/"}));
  STATIC_CHECK(constants_eq(TOKENS<USR_BIN>, std::tuple{u8"/", u8"usr", u8"bin"}));
  STATIC_CHECK(constants_eq(TOKENS<REL_USR_BIN>, std::tuple{u8"usr", u8"bin"}));
  STATIC_CHECK(constants_eq(TOKENS<A_B_DOTDOT_C>, std::tuple{u8"a", u8"b", u8"..", u8"c"}));
  STATIC_CHECK(constants_eq(TOKENS<DOUBLE_SLASH>, std::tuple{u8"//"}));
  STATIC_CHECK(constants_eq(TOKENS<DOUBLE_SEP>, std::tuple{u8"a", u8"b"}));
  STATIC_CHECK(constants_eq(TOKENS<"a/b/">, std::tuple{u8"a", u8"b", u8""}));

  STATIC_CHECK(LEXICALLY_NORMAL<EMPTY> == ".");
  STATIC_CHECK(LEXICALLY_NORMAL<ROOT_SLASH> == "/");
  STATIC_CHECK(LEXICALLY_NORMAL<USR_BIN> == "/usr/bin");
  STATIC_CHECK(LEXICALLY_NORMAL<A_B_DOTDOT_C> == "a/c");
  STATIC_CHECK(LEXICALLY_NORMAL<ABS_DOTDOT_C> == "/a/c");
  STATIC_CHECK(LEXICALLY_NORMAL<DOUBLE_SLASH> == "//");
  STATIC_CHECK(LEXICALLY_NORMAL<DOUBLE_SEP> == "a/b");

  STATIC_CHECK(LEXICALLY_NORMAL<"a/b/"> == "a/b");
  STATIC_CHECK(LEXICALLY_NORMAL<"/a/.."> == "/");
  STATIC_CHECK(LEXICALLY_NORMAL<"/../a"> == "/a");
  STATIC_CHECK(LEXICALLY_NORMAL<"a/.."> == ".");
  STATIC_CHECK(LEXICALLY_NORMAL<"a/../.."> == "..");

  STATIC_CHECK(ROOT_NAME<DOUBLE_SLASH_ROOT> == "/");
  STATIC_CHECK(ROOT_DIRECTORY<DOUBLE_SLASH_ROOT> == "/");
  STATIC_CHECK(ROOT_PATH<DOUBLE_SLASH_ROOT> == "//");

  STATIC_CHECK(ROOT_NAME<DOUBLE_SLASH_USR> == "/");
  STATIC_CHECK(ROOT_DIRECTORY<DOUBLE_SLASH_USR> == "/");
  STATIC_CHECK(ROOT_PATH<DOUBLE_SLASH_USR> == "//");

  STATIC_CHECK(RELATIVE_PATH<DOUBLE_SLASH_ROOT> == "");
  STATIC_CHECK(RELATIVE_PATH<DOUBLE_SLASH_USR> == "usr");
  STATIC_CHECK(RELATIVE_PATH<DOUBLE_SLASH_USR_BIN> == "usr/bin");

  STATIC_CHECK(FILENAME<DOUBLE_SLASH_USR> == "usr");
  STATIC_CHECK(FILENAME<DOUBLE_SLASH_USR_BIN> == "bin");

  STATIC_CHECK(PARENT_PATH<DOUBLE_SLASH_USR_BIN> == "//usr");
  STATIC_CHECK(PARENT_PATH<DOUBLE_SLASH_USR> == "//");
  STATIC_CHECK(PARENT_PATH<DOUBLE_SLASH_ROOT> == "//");

  STATIC_CHECK(constants_eq(TOKENS<DOUBLE_SLASH_ROOT>, std::tuple{u8"//"}));
  STATIC_CHECK(constants_eq(TOKENS<DOUBLE_SLASH_USR>, std::tuple{u8"//", u8"usr"}));
  STATIC_CHECK(constants_eq(TOKENS<DOUBLE_SLASH_USR_BIN>, std::tuple{u8"//", u8"usr", u8"bin"}));

  STATIC_CHECK(LEXICALLY_NORMAL<DOUBLE_SLASH_ROOT> == "//");
  STATIC_CHECK(LEXICALLY_NORMAL<DOUBLE_SLASH_USR> == "//usr");
  STATIC_CHECK(LEXICALLY_NORMAL<DOUBLE_SLASH_USR_BIN> == "//usr/bin");
  STATIC_CHECK(LEXICALLY_NORMAL<"//usr/.."> == "//");
  STATIC_CHECK(LEXICALLY_NORMAL<"//a/b/../c"> == "//a/c");
  STATIC_CHECK(LEXICALLY_NORMAL<"/a/b"> == "/a/b");
  STATIC_CHECK(LEXICALLY_NORMAL<"/a"> == "/a");

  constexpr comppath::posix::CompPath A_SLASH_SLASH_B = "a//b";
  constexpr comppath::posix::CompPath A_SLASH_SLASH_SLASH_B = "a///b";
  constexpr comppath::posix::CompPath SLASH_A_SLASH_SLASH_B = "/a//b";
  constexpr comppath::posix::CompPath A_SLASH_SLASH_B_SLASH = "a//b/";

  STATIC_CHECK(FILENAME<A_SLASH_SLASH_B> == "b");
  STATIC_CHECK(FILENAME<SLASH_A_SLASH_SLASH_B> == "b");
  STATIC_CHECK(FILENAME<A_SLASH_SLASH_B_SLASH> == "");

  STATIC_CHECK(PARENT_PATH<A_SLASH_SLASH_B> == "a");
  STATIC_CHECK(PARENT_PATH<SLASH_A_SLASH_SLASH_B> == "/a");
  STATIC_CHECK(PARENT_PATH<A_SLASH_SLASH_B_SLASH> == "a//b"); // trailing slash removed

  STATIC_CHECK(REMOVE_FILENAME<A_SLASH_SLASH_B> == "a");
  STATIC_CHECK(REMOVE_FILENAME<SLASH_A_SLASH_SLASH_B> == "/a");
  STATIC_CHECK(REMOVE_FILENAME<A_SLASH_SLASH_B_SLASH> == "a//b"); // unchanged (filename empty)

  STATIC_CHECK(constants_eq(TOKENS<A_SLASH_SLASH_SLASH_B>, std::tuple{u8"a", u8"b"}));
  STATIC_CHECK(constants_eq(TOKENS<SLASH_A_SLASH_SLASH_B>, std::tuple{u8"/", u8"a", u8"b"}));
  STATIC_CHECK(constants_eq(TOKENS<A_SLASH_SLASH_B_SLASH>, std::tuple{u8"a", u8"b", u8""}));

  STATIC_CHECK(LEXICALLY_NORMAL<A_SLASH_SLASH_SLASH_B> == "a/b");
  STATIC_CHECK(LEXICALLY_NORMAL<SLASH_A_SLASH_SLASH_B> == "/a/b");
  STATIC_CHECK(LEXICALLY_NORMAL<A_SLASH_SLASH_B_SLASH> == "a/b");

  STATIC_CHECK(RELATIVE_PATH<"/usr/"> == "usr/");
  STATIC_CHECK(RELATIVE_PATH<"//usr/"> == "usr/");

  STATIC_CHECK(PARENT_PATH<"/a/b/"> == "/a/b");
  STATIC_CHECK(PARENT_PATH<"a/b/"> == "a/b");
  STATIC_CHECK(PARENT_PATH<"///"> == "///");
  STATIC_CHECK(PARENT_PATH<"///usr"> == "///");

  STATIC_CHECK(REPLACE_FILENAME<A_SLASH_SLASH_B, "new"> == "a/new");
  STATIC_CHECK(REPLACE_FILENAME<SLASH_A_SLASH_SLASH_B, "new"> == "/a/new");
  STATIC_CHECK(REPLACE_EXTENSION<A_SLASH_SLASH_B, "bak"> == "a//b.bak");

  STATIC_CHECK(LEXICALLY_RELATIVE<"/a/b", "/a"> == "b");
  STATIC_CHECK(LEXICALLY_RELATIVE<"/a/b", "/a/b"> == ".");
  STATIC_CHECK(LEXICALLY_RELATIVE<"/a/b", "/a/c"> == "../b");
  STATIC_CHECK(LEXICALLY_RELATIVE<"/a/b", "/a/b/c"> == "..");
  STATIC_CHECK(LEXICALLY_RELATIVE<"/a/b", "//c"> == ""); // different root
  STATIC_CHECK(LEXICALLY_RELATIVE<"/a/b", ""> == "/a/b");
  STATIC_CHECK(LEXICALLY_RELATIVE<"a/b", "a"> == "b");
  STATIC_CHECK(LEXICALLY_RELATIVE<"a/b", "b"> == "../a/b"); // not same prefix
  STATIC_CHECK(LEXICALLY_RELATIVE<"a/b", "a/b"> == ".");

  STATIC_CHECK(LEXICALLY_PROXIMATE<"/a/b", "//c"> == "/a/b");
  STATIC_CHECK(LEXICALLY_PROXIMATE<"/a/b", "/a"> == "b");

  STATIC_CHECK(comppath::posix::CompPath{u"😷😷/перкеля/bear/💃💃💃"} ==
               u8"😷😷/перкеля/bear/💃💃💃");
  STATIC_CHECK(comppath::posix::CompPath{U"😷😷/перкеля/bear/💃💃💃"} ==
               u8"😷😷/перкеля/bear/💃💃💃");

  STATIC_CHECK(SHRINK<""> == "");
  STATIC_CHECK(SHRINK<"foo"> == "foo");
  STATIC_CHECK(SHRINK<"foo/bar"> == "foo/bar");
  STATIC_CHECK(SHRINK<"/foo/bar"> == "/foo/bar");

  STATIC_CHECK(ROOT<"/"> == "/");
  STATIC_CHECK(ROOT<"//"> == "//");   // POSIX allows multiple leading slashes as root
  STATIC_CHECK(ROOT<"///"> == "///"); // also root

  STATIC_CHECK(APPEND<"foo", "bar"> == "foo/bar");
  STATIC_CHECK(APPEND<"foo/", "bar"> == "foo/bar");  // trailing sep already
  STATIC_CHECK(APPEND<"foo", "bar/"> == "foo/bar/"); // rhs trailing sep
  STATIC_CHECK(APPEND<"", "bar"> == "bar");
  STATIC_CHECK(APPEND<"foo", ""> == "foo/");
  STATIC_CHECK(APPEND<"/foo", "bar"> == "/foo/bar");
  STATIC_CHECK(APPEND<"/foo/", "bar"> == "/foo/bar");

  STATIC_CHECK(CONCAT<"foo", "bar"> == "foobar");
  STATIC_CHECK(CONCAT<"foo/", "bar"> == "foo/bar");
  STATIC_CHECK(CONCAT<"", "bar"> == "bar");
  STATIC_CHECK(CONCAT<"foo", ""> == "foo");

  STATIC_CHECK(ROOT_NAME<"/foo/bar"> == "");
  STATIC_CHECK(ROOT_NAME<"foo/bar"> == "");
  STATIC_CHECK(ROOT_NAME<""> == "");

  STATIC_CHECK(ROOT_DIRECTORY<"/foo/bar"> == "/");
  STATIC_CHECK(ROOT_DIRECTORY<"//foo/bar"> == "/");
  STATIC_CHECK(ROOT_DIRECTORY<"/foo"> == "/");
  STATIC_CHECK(ROOT_DIRECTORY<"//foo"> == "/");
  STATIC_CHECK(ROOT_DIRECTORY<"foo/bar"> == ""); // relative
  STATIC_CHECK(ROOT_DIRECTORY<""> == "");

  STATIC_CHECK(ROOT_PATH<"/foo/bar"> == "/");
  STATIC_CHECK(ROOT_PATH<"//foo/bar"> == "//");
  STATIC_CHECK(ROOT_PATH<"foo/bar"> == "");
  STATIC_CHECK(ROOT_PATH<""> == "");

  STATIC_CHECK(REMOVE_TRAILING_SEPS<"foo/"> == "foo");
  STATIC_CHECK(REMOVE_TRAILING_SEPS<"foo//"> == "foo");
  STATIC_CHECK(REMOVE_TRAILING_SEPS<"/foo/"> == "/foo");
  STATIC_CHECK(REMOVE_TRAILING_SEPS<"/"> == "/"); // root unchanged
  STATIC_CHECK(REMOVE_TRAILING_SEPS<""> == "");

  STATIC_CHECK(RELATIVE_PATH<"/foo/bar"> == "foo/bar");
  STATIC_CHECK(RELATIVE_PATH<"//foo/bar"> == "foo/bar"); // root stripped
  STATIC_CHECK(RELATIVE_PATH<"foo/bar"> == "foo/bar");   // already relative
  STATIC_CHECK(RELATIVE_PATH<"/"> == "");                // root only -> empty relative
  STATIC_CHECK(RELATIVE_PATH<""> == "");

  STATIC_CHECK(PARENT_PATH<"/foo/bar"> == "/foo");
  STATIC_CHECK(PARENT_PATH<"/foo/bar/"> == "/foo/bar"); // trailing sep removed
  STATIC_CHECK(PARENT_PATH<"foo/bar"> == "foo");
  STATIC_CHECK(PARENT_PATH<"foo"> == "");     // no parent
  STATIC_CHECK(PARENT_PATH<"foo/"> == "foo"); // trailing sep -> parent is "foo"
  STATIC_CHECK(PARENT_PATH<"/foo"> == "/");
  STATIC_CHECK(PARENT_PATH<"/"> == "/"); // root -> itself
  STATIC_CHECK(PARENT_PATH<""> == "");

  STATIC_CHECK(FILENAME<"/foo/bar"> == "bar");
  STATIC_CHECK(FILENAME<"foo/bar"> == "bar");
  STATIC_CHECK(FILENAME<"foo"> == "foo");
  STATIC_CHECK(FILENAME<"foo/"> == ""); // trailing sep -> no filename
  STATIC_CHECK(FILENAME<"/"> == "");    // root -> no filename
  STATIC_CHECK(FILENAME<""> == "");

  STATIC_CHECK(STEM<"foo.txt"> == "foo");
  STATIC_CHECK(STEM<"foo"> == "foo");
  STATIC_CHECK(STEM<".profile"> == ".profile");
  STATIC_CHECK(STEM<"foo.tar.gz"> == "foo.tar");
  STATIC_CHECK(STEM<"/foo/bar.txt"> == "bar");
  STATIC_CHECK(STEM<"foo."> == "foo");
  STATIC_CHECK(STEM<"foo.."> == "foo.");
  STATIC_CHECK(STEM<""> == "");

  STATIC_CHECK(EXTENSION<"foo.txt"> == ".txt");
  STATIC_CHECK(EXTENSION<"foo"> == "");
  STATIC_CHECK(EXTENSION<".profile"> == "");
  STATIC_CHECK(EXTENSION<"foo.tar.gz"> == ".gz");
  STATIC_CHECK(EXTENSION<"foo."> == ".");
  STATIC_CHECK(EXTENSION<"foo.."> == ".");
  STATIC_CHECK(EXTENSION<"/foo/bar.txt"> == ".txt");

  STATIC_CHECK(MAKE_PREFERRED<"foo/bar"> == "foo/bar");
  STATIC_CHECK(MAKE_PREFERRED<"foo\\bar"> == "foo\\bar");

  STATIC_CHECK(REMOVE_FILENAME<"/foo/bar"> == "/foo");
  STATIC_CHECK(REMOVE_FILENAME<"foo/bar"> == "foo");
  STATIC_CHECK(REMOVE_FILENAME<"foo"> == "");
  STATIC_CHECK(REMOVE_FILENAME<"foo/"> == "foo");
  STATIC_CHECK(REMOVE_FILENAME<"/"> == "/");
  STATIC_CHECK(REMOVE_FILENAME<""> == "");

  STATIC_CHECK(REMOVE_EXTENSION<"foo.txt"> == "foo");
  STATIC_CHECK(REMOVE_EXTENSION<"foo.tar.gz"> == "foo.tar");
  STATIC_CHECK(REMOVE_EXTENSION<"foo"> == "foo");
  STATIC_CHECK(REMOVE_EXTENSION<".profile"> == ".profile");
  STATIC_CHECK(REMOVE_EXTENSION<"foo."> == "foo");
  STATIC_CHECK(REMOVE_EXTENSION<"foo.."> == "foo.");
  STATIC_CHECK(REMOVE_EXTENSION<"/foo/bar.txt"> == "/foo/bar");

  STATIC_CHECK(REPLACE_FILENAME<"foo/bar", "baz"> == "foo/baz");
  STATIC_CHECK(REPLACE_FILENAME<"foo/bar.txt", "baz"> == "foo/baz");
  STATIC_CHECK(REPLACE_FILENAME<"foo/", "baz"> ==
               "foo/baz"); // remove_filename returns "foo/", then / "baz" -> "foo/baz"
  STATIC_CHECK(REPLACE_FILENAME<"foo", "baz"> == "baz"); // no parent -> just new filename
  STATIC_CHECK(REPLACE_FILENAME<"/foo/bar", "baz"> == "/foo/baz");
  STATIC_CHECK(REPLACE_FILENAME<"/", "baz"> == "/baz"); // root path + new filename

  STATIC_CHECK(REPLACE_EXTENSION<"foo.txt", ".csv"> == "foo.csv");
  STATIC_CHECK(REPLACE_EXTENSION<"foo.txt", "csv"> == "foo.csv"); // adds dot
  STATIC_CHECK(REPLACE_EXTENSION<"foo", ".txt"> == "foo.txt");
  STATIC_CHECK(REPLACE_EXTENSION<"foo", "txt"> == "foo.txt");
  STATIC_CHECK(REPLACE_EXTENSION<"foo.tar.gz", ".bz2"> == "foo.tar.bz2");
  STATIC_CHECK(REPLACE_EXTENSION<".profile", ".bak"> == ".profile.bak");
  STATIC_CHECK(REPLACE_EXTENSION<"foo.", ".txt"> == "foo.txt");
  STATIC_CHECK(REPLACE_EXTENSION<"foo..", ".txt"> == "foo..txt");
  STATIC_CHECK(REPLACE_EXTENSION<"/foo/bar.txt", ".bak"> == "/foo/bar.bak");

  constexpr auto TOKENS1 = TOKENS<"/foo/bar">;
  STATIC_CHECK(std::tuple_size_v<decltype(TOKENS1)> == 3);
  STATIC_CHECK(std::get<0>(TOKENS1).value == "/"); // root-directory token
  STATIC_CHECK(std::get<1>(TOKENS1).value == "foo");
  STATIC_CHECK(std::get<2>(TOKENS1).value == "bar");

  constexpr auto TOKENS2 = TOKENS<"foo/bar/baz">;
  STATIC_CHECK(std::tuple_size_v<decltype(TOKENS2)> == 3);
  STATIC_CHECK(std::get<0>(TOKENS2).value == "foo");
  STATIC_CHECK(std::get<1>(TOKENS2).value == "bar");
  STATIC_CHECK(std::get<2>(TOKENS2).value == "baz");

  constexpr auto TOKENS3 = TOKENS<"//foo/bar">;
  STATIC_CHECK(std::tuple_size_v<decltype(TOKENS3)> == 3);
  STATIC_CHECK(std::get<0>(TOKENS3).value == "//"); // root-directory (since root-name empty)
  STATIC_CHECK(std::get<1>(TOKENS3).value == "foo");
  STATIC_CHECK(std::get<2>(TOKENS3).value == "bar");

  constexpr auto TOKENS4 = TOKENS<"">;
  STATIC_CHECK(std::tuple_size_v<decltype(TOKENS4)> == 0);

  STATIC_CHECK(LEXICALLY_NORMAL<"foo/./bar"> == "foo/bar");
  STATIC_CHECK(LEXICALLY_NORMAL<"foo/../bar"> == "bar");
  STATIC_CHECK(LEXICALLY_NORMAL<"foo/bar/.."> == "foo");
  STATIC_CHECK(LEXICALLY_NORMAL<"foo/bar/../baz"> == "foo/baz");
  STATIC_CHECK(LEXICALLY_NORMAL<"foo/../../bar"> == "../bar");
  STATIC_CHECK(LEXICALLY_NORMAL<"foo/bar/../../baz"> == "baz");
  STATIC_CHECK(LEXICALLY_NORMAL<"/foo/./bar"> == "/foo/bar");
  STATIC_CHECK(LEXICALLY_NORMAL<"/foo/../bar"> == "/bar");
  STATIC_CHECK(LEXICALLY_NORMAL<"/foo/../../bar"> == "/bar"); // root stays
  STATIC_CHECK(LEXICALLY_NORMAL<"foo/."> == "foo");
  STATIC_CHECK(LEXICALLY_NORMAL<"foo/.."> == ".");
  STATIC_CHECK(LEXICALLY_NORMAL<"foo/../.."> == "..");
  STATIC_CHECK(LEXICALLY_NORMAL<"//foo/.."> == "//"); // //foo/.. -> / (root)
  STATIC_CHECK(LEXICALLY_NORMAL<"//foo/../bar"> == "//bar");

  STATIC_CHECK(LEXICALLY_RELATIVE<"foo/bar", "foo"> == "bar");
  STATIC_CHECK(LEXICALLY_RELATIVE<"foo/bar/baz", "foo/bar"> == "baz");
  STATIC_CHECK(LEXICALLY_RELATIVE<"foo/bar", "foo/baz"> == "../bar");
  STATIC_CHECK(LEXICALLY_RELATIVE<"foo/bar", "foo/bar/baz"> == "..");
  STATIC_CHECK(LEXICALLY_RELATIVE<"foo/bar", "foo/bar/baz/qux"> == "../..");
  STATIC_CHECK(LEXICALLY_RELATIVE<"foo/bar", "foo"> == "bar");
  STATIC_CHECK(LEXICALLY_RELATIVE<"foo/bar", "foo/bar"> == ".");
  STATIC_CHECK(LEXICALLY_RELATIVE<"/foo/bar", "/foo"> == "bar");
  STATIC_CHECK(LEXICALLY_RELATIVE<"/foo/bar", "/baz"> == "../foo/bar");
  STATIC_CHECK(LEXICALLY_RELATIVE<"/foo/bar", "/foo/bar"> == ".");
  STATIC_CHECK(LEXICALLY_RELATIVE<"/foo/bar", "/foo/bar/baz"> == "..");
  STATIC_CHECK(LEXICALLY_RELATIVE<"/foo/bar", "/baz/qux"> == "../../foo/bar");
  STATIC_CHECK(LEXICALLY_RELATIVE<"/foo", "/bar"> == "../foo");
}
