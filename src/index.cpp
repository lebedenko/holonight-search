#include <QReadLocker>
#include <QRegularExpression>
#include <QWriteLocker>
#include <queue>

#include <algorithm>
#include <holonight/search/index.h>
#include <limits>
#include <thread>
#include <utility>

namespace HolonightSearch {
namespace {

quint64 characterMask(const QString& text) {
  quint64 mask = 0;
  for (const QChar character : text) {
    const ushort code = character.toCaseFolded().unicode();
    if (code >= 'a' && code <= 'z') {
      mask |= quint64{1} << (code - 'a');
    } else if (code >= '0' && code <= '9') {
      mask |= quint64{1} << (26 + code - '0');
    }
  }
  return mask;
}

QString fold(const QString& text) {
  QString result;
  result.reserve(text.size());
  bool changed = false;
  for (const QChar character : text) {
    const QChar lowered = character.toCaseFolded();
    changed |= lowered != character;
    result.append(lowered);
  }
  return changed ? result : text;
}

bool boundary(const QString& text, int position) {
  if (position == 0) {
    return true;
  }
  const QChar previous = text[position - 1];
  const QChar current = text[position];
  return previous == u'/' || previous == u'.' || previous == u'_' || previous == u'-' || previous == u' ' ||
         (previous.isLower() && current.isUpper());
}

struct Term {
  QString text;
  quint64 mask = 0;
};

struct Match {
  int score = std::numeric_limits<int>::min();
  QVector<int> positions;
};

// Dynamic programming retains the best predecessor for each character. The
// score pass avoids position storage; only selected results reconstruct it.
// Preserve the established dynamic-programming scoring and tie order.
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
Match matchTerm(const Term& term, const QString& original, const QString& folded, bool caseSensitive,
                bool positionsNeeded) {
  if (term.text.size() > original.size()) {
    return {};
  }
  const QString& haystack = caseSensitive ? original : folded;
  const int width = static_cast<int>(haystack.size());
  // A boundary-aligned contiguous match already receives the strongest local
  // bonuses. Handle this common case without allocating DP rows for every hit.
  const int contiguous = static_cast<int>(haystack.indexOf(term.text));
  if (contiguous >= 0 && boundary(original, contiguous)) {
    Match direct;
    direct.score = 16 + 12 - contiguous;
    for (int i = 1; i < term.text.size(); ++i) {
      direct.score += 28 + (boundary(original, contiguous + i) ? 12 : 0);
    }
    direct.score -= width / 8;
    if (positionsNeeded) {
      direct.positions.reserve(term.text.size());
      for (int i = 0; i < term.text.size(); ++i) {
        direct.positions.append(contiguous + i);
      }
    }
    return direct;
  }
  constexpr int impossible = std::numeric_limits<int>::min() / 4;
  QVector<int> previous(width, impossible);
  QVector<int> current(width, impossible);
  QVector<QVector<int>> parents;
  if (positionsNeeded) {
    parents.resize(term.text.size());
  }
  for (int i = 0; i < term.text.size(); ++i) {
    std::ranges::fill(current, impossible);
    if (positionsNeeded) {
      parents[i].resize(width);
      std::ranges::fill(parents[i], -1);
    }
    int bestGapScore = impossible;
    int bestGapPosition = -1;
    for (int j = i; j < width; ++j) {
      if (i > 0 && previous[j - 1] != impossible) {
        const int value = previous[j - 1] + ((j - 1) * 2);
        if (value > bestGapScore) {
          bestGapScore = value;
          bestGapPosition = j - 1;
        }
      }
      if (haystack[j] != term.text[i]) {
        continue;
      }
      const int bonus = boundary(original, j) ? 12 : 0;
      if (i == 0) {
        current[j] = 16 + bonus - j;
        continue;
      }
      if (bestGapPosition < 0) {
        continue;
      }
      int score = bestGapScore + 16 + bonus - ((j - 1) * 2);
      int parent = bestGapPosition;
      if (previous[j - 1] != impossible) {
        const int consecutive = previous[j - 1] + 28 + bonus;
        if (consecutive >= score) {
          score = consecutive;
          parent = j - 1;
        }
      }
      current[j] = score;
      if (positionsNeeded) {
        parents[i][j] = parent;
      }
    }
    previous.swap(current);
  }
  const auto best = std::ranges::max_element(std::as_const(previous));
  if (best == previous.cend() || *best == impossible) {
    return {};
  }
  Match result;
  result.score = *best - (width / 8);
  if (positionsNeeded) {
    int position = static_cast<int>(best - previous.cbegin());
    result.positions.resize(term.text.size());
    for (int i = static_cast<int>(term.text.size()) - 1; i >= 0; --i) {
      result.positions[i] = position;
      position = parents[i][position];
    }
  }
  return result;
}

bool better(const Result& left, const Result& right) {
  if (left.score != right.score) {
    return left.score > right.score;
  }
  if (left.source != right.source) {
    return left.source < right.source;
  }
  return left.id < right.id;
}

}  // namespace

void Index::replace(const QString& source, QVector<Record> records) {
  auto batch = std::make_shared<Batch>();
  batch->reserve(records.size());
  for (Record& record : records) {
    IndexedRecord entry;
    entry.id = std::move(record.id);
    entry.boost = record.boost;
    entry.fields.reserve(record.fields.size());
    for (Field& field : record.fields) {
      IndexedField indexed;
      indexed.value = std::move(field);
      indexed.folded = fold(indexed.value.text);
      indexed.character_mask = characterMask(indexed.folded);
      entry.character_mask |= indexed.character_mask;
      entry.fields.append(std::move(indexed));
    }
    batch->append(std::move(entry));
  }
  QWriteLocker guard(&lock_);
  sources_[source] = {std::move(batch)};
}

void Index::append(const QString& source, QVector<Record> records) {
  if (records.isEmpty()) {
    return;
  }
  auto batch = std::make_shared<Batch>();
  batch->reserve(records.size());
  for (Record& record : records) {
    IndexedRecord entry;
    entry.id = std::move(record.id);
    entry.boost = record.boost;
    entry.fields.reserve(record.fields.size());
    for (Field& field : record.fields) {
      IndexedField indexed;
      indexed.value = std::move(field);
      indexed.folded = fold(indexed.value.text);
      indexed.character_mask = characterMask(indexed.folded);
      entry.character_mask |= indexed.character_mask;
      entry.fields.append(std::move(indexed));
    }
    batch->append(std::move(entry));
  }
  QWriteLocker guard(&lock_);
  sources_[source].append(std::move(batch));
}

void Index::remove(const QString& source) {
  QWriteLocker guard(&lock_);
  sources_.remove(source);
}

qsizetype Index::size() const {
  QReadLocker guard(&lock_);
  qsizetype count = 0;
  for (const auto& batches : sources_) {
    for (const auto& batch : batches) {
      count += batch->size();
    }
  }
  return count;
}

// Keep ranking, cancellation and bounded highlight reconstruction in their established order.
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
QVector<Result> Index::search(const QString& query, Options options) const {
  if (options.limit <= 0 || query.trimmed().isEmpty()) {
    return {};
  }
  const bool caseSensitive = std::ranges::any_of(query, [](QChar character) { return character.isUpper(); });
  QVector<Term> terms;
  for (const QString& part : query.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts)) {
    const QString text = caseSensitive ? part : fold(part);
    terms.append({.text = text, .mask = characterMask(text)});
  }
  QHash<QString, QVector<std::shared_ptr<const Batch>>> snapshot;
  {
    QReadLocker guard(&lock_);
    snapshot = sources_;
  }
  struct Ranked {
    Result result;
    const IndexedRecord* record;
  };
  auto rankedBetter = [](const Ranked& left, const Ranked& right) { return better(left.result, right.result); };
  struct Group {
    QString source;
    std::shared_ptr<const Batch> batch;
  };
  QVector<Group> groups;
  for (auto source = snapshot.cbegin(); source != snapshot.cend(); ++source) {
    if (!options.source.isEmpty() && source.key() != options.source) {
      continue;
    }
    for (const auto& batch : source.value()) {
      groups.append({.source = source.key(), .batch = batch});
    }
  }
  const int workers = groups.size() < 16 ? 1 : std::min(8, static_cast<int>(groups.size()));
  QVector<QVector<Ranked>> partial(workers);
  // The worker retains the established scoring loop and bounded heap ordering.
  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  auto searchGroups = [&](int worker) {
    std::priority_queue<Ranked, std::vector<Ranked>, decltype(rankedBetter)> best(rankedBetter);
    for (int groupIndex = worker; groupIndex < groups.size(); groupIndex += workers) {
      const auto& [source, batch] = groups[groupIndex];
      for (const IndexedRecord& record : *batch) {
        if (options.cancelled != nullptr && options.cancelled->load(std::memory_order_relaxed)) {
          return;
        }
        bool possible = true;
        for (const Term& term : terms) {
          if ((record.character_mask & term.mask) != term.mask) {
            possible = false;
            break;
          }
        }
        if (!possible) {
          continue;
        }
        int score = record.boost;
        for (const Term& term : terms) {
          int bestField = std::numeric_limits<int>::min();
          for (const IndexedField& field : record.fields) {
            if ((field.character_mask & term.mask) != term.mask) {
              continue;
            }
            const Match match = matchTerm(term, field.value.text, field.folded, caseSensitive, false);
            if (match.score == std::numeric_limits<int>::min()) {
              continue;
            }
            const int pathBonus =
                options.profile == Profile::Path && field.value.name == QStringLiteral("name") ? 60 : 0;
            bestField = std::max(bestField, match.score + field.value.weight + pathBonus);
            if (pathBonus != 0) {
              break;  // A filename hit wins this term in the path profile.
            }
          }
          if (bestField == std::numeric_limits<int>::min()) {
            possible = false;
            break;
          }
          score += bestField;
        }
        if (!possible) {
          continue;
        }
        Ranked result{.result = {.source = source, .id = record.id, .score = score, .highlights = {}},
                      .record = &record};
        if (best.size() < static_cast<size_t>(options.limit)) {
          best.push(std::move(result));
        } else if (better(result.result, best.top().result)) {
          best.pop();
          best.push(std::move(result));
        }
      }
    }
    auto& output = partial[worker];
    output.reserve(static_cast<qsizetype>(best.size()));
    while (!best.empty()) {
      output.append(best.top());
      best.pop();
    }
  };
  std::vector<std::thread> threads;
  for (int worker = 1; worker < workers; ++worker) {
    threads.emplace_back(searchGroups, worker);
  }
  searchGroups(0);
  for (auto& thread : threads) {
    thread.join();
  }
  if (options.cancelled != nullptr && options.cancelled->load(std::memory_order_relaxed)) {
    return {};
  }
  std::priority_queue<Ranked, std::vector<Ranked>, decltype(rankedBetter)> best(rankedBetter);
  for (const auto& output : partial) {
    for (const Ranked& result : output) {
      if (best.size() < static_cast<size_t>(options.limit)) {
        best.push(result);
      } else if (better(result.result, best.top().result)) {
        best.pop();
        best.push(result);
      }
    }
  }
  QVector<Ranked> selected;
  selected.reserve(static_cast<qsizetype>(best.size()));
  while (!best.empty()) {
    selected.append(best.top());
    best.pop();
  }
  std::ranges::sort(selected, rankedBetter);
  QVector<Result> results;
  results.reserve(selected.size());
  // Highlight positions are computed only for the bounded result set.
  for (Ranked& ranked : selected) {
    Result& result = ranked.result;
    for (const Term& term : terms) {
      int bestScore = std::numeric_limits<int>::min();
      QString bestName;
      QVector<int> positions;
      for (const IndexedField& field : ranked.record->fields) {
        const Match match = matchTerm(term, field.value.text, field.folded, caseSensitive, true);
        const int pathBonus = options.profile == Profile::Path && field.value.name == QStringLiteral("name") ? 60 : 0;
        const int fieldScore =
            match.score == std::numeric_limits<int>::min() ? match.score : match.score + field.value.weight + pathBonus;
        if (fieldScore > bestScore) {
          bestScore = fieldScore;
          bestName = field.value.name;
          positions = match.positions;
        }
        if (pathBonus != 0 && match.score != std::numeric_limits<int>::min()) {
          break;
        }
      }
      auto existing = std::ranges::find_if(result.highlights,
                                           [&](const Highlight& highlight) { return highlight.field == bestName; });
      if (existing == result.highlights.end()) {
        result.highlights.append({.field = bestName, .positions = positions});
      } else {
        existing->positions += positions;
      }
    }
    results.append(std::move(result));
  }
  if (options.cancelled != nullptr && options.cancelled->load(std::memory_order_relaxed)) {
    return {};
  }
  return results;
}

}  // namespace HolonightSearch
