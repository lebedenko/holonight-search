#include <holonight/search/index.h>

int main() {
  HolonightSearch::Index index;
  index.replace("sample", {{.id = "one", .fields = {{.name = "name", .text = "lambda", .weight = 1}}, .boost = 0}});
  return index.search("lambda").size() == 1 ? 0 : 1;
}
