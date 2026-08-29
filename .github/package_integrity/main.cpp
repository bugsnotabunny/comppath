#include "comppath/CompPath.hpp"

#include <cstdlib>

int main(int, char **) {
  static_assert(comppath::LEXICALLY_NORMAL<"some/cool/../dir"> ==
                comppath::CompPath{"some"} / "dir");
  return EXIT_SUCCESS;
}
