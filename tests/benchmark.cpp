#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>

#include <algorithm>
#include <holonight/search/index.h>
#include <iostream>

using namespace HolonightSearch;

int main(int argc, char** argv) {
  const int count = argc > 1 ? QByteArray(argv[1]).toInt() : 2'000'000;
  const QString listPath = argc > 2 ? QString::fromLocal8Bit(argv[2]) : QString{};
  QFile list(listPath);
  if (!listPath.isEmpty() && !list.open(QIODevice::WriteOnly | QIODevice::Text)) {
    return 1;
  }
  QTextStream stream(&list);
  Index index;
  QElapsedTimer timer;
  timer.start();
  constexpr int batchSize = 4096;
  QVector<Record> batch;
  batch.reserve(batchSize);
  for (int i = 0; i < count; ++i) {
    const QString folder = QStringLiteral("home/projects/repository-%1/module-%2/").arg(i / 400).arg((i / 40) % 10);
    const QString name = i % 251 == 0 ? QStringLiteral("lambda-audit-%1.cpp").arg(i)
                                      : QStringLiteral("document-%1-%2.cpp").arg(i % 997).arg(i);
    const QString path = folder + name;
    if (list.isOpen()) {
      stream << path << '\n';
    }
    batch.append({path, {{"name", name, 0}, {"path", path, 0}}, 0});
    if (batch.size() == batchSize) {
      index.append("files", std::move(batch));
      batch.clear();
      batch.reserve(batchSize);
    }
  }
  index.append("files", std::move(batch));
  stream.flush();
  std::cout << "records=" << index.size() << " index_ms=" << timer.elapsed() << '\n';
  QFile status(QStringLiteral("/proc/self/status"));
  if (status.open(QIODevice::ReadOnly)) {
    for (const QByteArray& line : status.readAll().split('\n')) {
      if (line.startsWith("VmRSS:") || line.startsWith("VmHWM:")) {
        std::cout << line.constData() << '\n';
      }
    }
  }
  for (const QString& query : {QStringLiteral("lambda"), QStringLiteral("lambda audit"), QStringLiteral("audit lambda"),
                               QStringLiteral("document 42")}) {
    QVector<qint64> durations;
    for (int sample = 0; sample < 20; ++sample) {
      timer.restart();
      const auto hits = index.search(query, {.limit = 80, .profile = Profile::Path});
      durations.append(timer.nsecsElapsed() / 1'000'000);
      if (sample == 0) {
        std::cout << "query=" << query.toStdString() << " hits=" << hits.size() << ' ';
      }
    }
    std::sort(durations.begin(), durations.end());
    std::cout << "p50_ms=" << durations[durations.size() / 2] << " p95_ms=" << durations[18] << '\n';
  }
}
