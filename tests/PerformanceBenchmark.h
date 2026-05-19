#ifndef PERFORMANCE_BENCHMARK_H
#define PERFORMANCE_BENCHMARK_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QString>
#include <QVector>

struct BenchmarkResult {
    QString name;
    double durationMs;
    double fps;
    double cpuUsage;
    double memoryMB;
    int framesRendered;
    QString notes;
};

class PerformanceBenchmark : public QObject {
    Q_OBJECT
public:
    explicit PerformanceBenchmark(QObject* parent = nullptr);
    
    void runAllBenchmarks();
    void runScrollBenchmark();
    void runStartupBenchmark();
    void runCPUBenchmark();
    void runMemoryBenchmark();
    void runGlyphUploadBenchmark();
    
    const QVector<BenchmarkResult>& results() const { return m_results; }
    void printResults() const;
    void saveResults(const QString& filePath) const;
    
signals:
    void benchmarkStarted(const QString& name);
    void benchmarkFinished(const BenchmarkResult& result);
    void allBenchmarksFinished();
    
private:
    QString generateTestText(int lines);
    double measureStartupTime();
    double measureScrollFPS(int durationSeconds);
    double measureCPULoad(int durationSeconds);
    double measureMemoryUsage();
    double measureGlyphUploadTime(int glyphCount);
    
    QVector<BenchmarkResult> m_results;
};

#endif
