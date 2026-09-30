#include <gtest/gtest.h>
#include <holonight/search/index.h>

using namespace HolonightSearch;

namespace {
Record path(const QString& value, int boost = 0) {
  const QString name = value.mid(value.lastIndexOf(u'/') + 1);
  return {value, {{"name", name, 0}, {"path", value, 0}}, boost};
}
}  // namespace

TEST(SearchIndex, UnorderedTermsFindFilenameAndAncestor) {
  Index index;
  index.replace("files", {path("src/lambda/audit.cpp"), path("src/audit/lambda.cpp"), path("notes/lambda.txt")});
  for (const QString& query : {"lambda", "lambda func", "lambda audit", "audit lambda"}) {
    if (query == "lambda func") {
      index.append("files", {path("src/lambda/functions.cpp")});
    }
    const auto hits = index.search(query, {.profile = Profile::Path});
    ASSERT_FALSE(hits.isEmpty()) << query.toStdString();
    EXPECT_TRUE(hits.first().id.contains("lambda") || hits.first().id.contains("functions"));
  }
  const auto forward = index.search("lambda audit", {.profile = Profile::Path});
  const auto reverse = index.search("audit lambda", {.profile = Profile::Path});
  ASSERT_EQ(forward.size(), 2);
  ASSERT_EQ(reverse.size(), 2);
  EXPECT_EQ(forward.first().id, reverse.first().id);
}

TEST(SearchIndex, PathProfilePrefersFilenameAndReturnsFieldHighlights) {
  Index index;
  index.replace("files", {path("lambda/archive.cpp"), path("docs/lambda.cpp")});
  const auto hits = index.search("lambda", {.profile = Profile::Path});
  ASSERT_EQ(hits.size(), 2);
  EXPECT_EQ(hits.first().id, "docs/lambda.cpp");
  ASSERT_EQ(hits.first().highlights.size(), 1);
  EXPECT_EQ(hits.first().highlights.first().field, "name");
  EXPECT_EQ(hits.first().highlights.first().positions, (QVector<int>{0, 1, 2, 3, 4, 5}));
}

TEST(SearchIndex, FuzzyCharactersPreferConsecutiveAndBoundaryMatches) {
  Index index;
  index.replace("launcher", {{"split", {{"title", "lxxxmxxbxxd", 0}}, 0}, {"compact", {{"title", "lambda", 0}}, 0}});
  const auto hits = index.search("lmbd");
  ASSERT_EQ(hits.size(), 2);
  EXPECT_EQ(hits.first().id, "compact");
  EXPECT_EQ(hits.first().highlights.first().positions, (QVector<int>{0, 2, 3, 4}));
}

TEST(SearchIndex, SmartCaseAndCrossFieldTermsWorkForNonPathSources) {
  Index index;
  index.replace("launcher", {{"editor", {{"title", "Lambda Editor", 20}, {"category", "Development", 5}}, 3}});
  index.replace("packages", {{"pkg-1", {{"name", "lambda-tools", 20}, {"summary", "Audit utilities", 5}}, 0}});
  EXPECT_EQ(index.search("lambda audit").first().source, "packages");
  EXPECT_TRUE(index.search("audit", {.source = "launcher"}).isEmpty());
  EXPECT_EQ(index.search("audit", {.source = "packages"}).first().id, "pkg-1");
  EXPECT_EQ(index.search("Lambda audit").size(), 0);
  EXPECT_EQ(index.search("Lambda Editor").first().id, "editor");
}

TEST(SearchIndex, ReplacementRemovalAndCancellationAreCoherent) {
  Index index;
  index.replace("files", {path("old.txt")});
  index.append("files", {path("new.txt")});
  EXPECT_EQ(index.size(), 2);
  index.replace("files", {path("fresh.txt")});
  EXPECT_TRUE(index.search("old").isEmpty());
  EXPECT_EQ(index.search("fresh").first().id, "fresh.txt");
  std::atomic_bool cancelled{true};
  EXPECT_TRUE(index.search("fresh", {.cancelled = &cancelled}).isEmpty());
  index.remove("files");
  EXPECT_EQ(index.size(), 0);
}

TEST(SearchIndex, BoundedResultsRespectBoostAndStableIds) {
  Index index;
  index.replace("packages", {{"a", {{"name", "lambda", 0}}, 0}, {"b", {{"name", "lambda", 0}}, 10}});
  const auto hits = index.search("lambda", {.limit = 1});
  ASSERT_EQ(hits.size(), 1);
  EXPECT_EQ(hits.first().id, "b");
}
