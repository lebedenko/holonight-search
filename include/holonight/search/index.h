#pragma once

#include <QHash>
#include <QReadWriteLock>
#include <QString>
#include <QVarLengthArray>
#include <QVector>

#include <atomic>
#include <memory>

namespace HolonightSearch {

struct Field {
  QString name;
  QString text;
  int weight = 0;
};

struct Record {
  QString id;
  QVector<Field> fields;
  int boost = 0;
};

// Preserve the public enum representation.
// NOLINTNEXTLINE(performance-enum-size)
enum class Profile { General, Path };

struct Options {
  int limit = 80;
  Profile profile = Profile::General;
  const std::atomic_bool* cancelled = nullptr;
  QString source;  // Empty searches every source.
};

struct Highlight {
  QString field;
  QVector<int> positions;
};

struct Result {
  QString source;
  QString id;
  int score = 0;
  QVector<Highlight> highlights;
};

// Each batch is immutable after publication. Search takes a coherent snapshot of
// batch pointers, so append and replacement do not invalidate an active query.
class Index {
 public:
  void replace(const QString& source, QVector<Record> records);
  void append(const QString& source, QVector<Record> records);
  void remove(const QString& source);
  QVector<Result> search(const QString& query, Options options = {}) const;
  qsizetype size() const;

 private:
  struct IndexedField {
    Field value;
    QString folded;
    quint64 character_mask = 0;
  };
  struct IndexedRecord {
    QString id;
    QVarLengthArray<IndexedField, 2> fields;
    int boost = 0;
    quint64 character_mask = 0;
  };
  using Batch = QVector<IndexedRecord>;
  QHash<QString, QVector<std::shared_ptr<const Batch>>> sources_;
  mutable QReadWriteLock lock_;
};

}  // namespace HolonightSearch
