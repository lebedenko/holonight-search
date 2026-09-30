#include <holonight/search/index.h>

int main() {
  HolonightSearch::Index index;
  index.replace("sample", {{"one", {{"name", "lambda", 1}}, 0}});
  return index.search("lambda").size() == 1 ? 0 : 1;
}
