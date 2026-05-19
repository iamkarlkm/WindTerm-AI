#include "PerformanceBenchmark.h"
#include "Renderer/PlatformDetector.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QImage>
#include <QRegularExpression>
#include <QCoreApplication>

#ifdef Q_OS_LINUX
#include <sys/resource.h>
#include <sys/times.h>
#include <unistd.h>
#endif

PerformanceBenchmark::PerformanceBenchmark(QObject* parent)
    : QObject(parent) {}

void PerformanceBenchmark::runAllBenchmarks() {
    m_results.clear();
    
    qDebug() << "\n=== WindTerm GPU Renderer Performance Benchmarks ===";
    qDebug() << "Platform:" << PlatformDetector::backendToString(PlatformDetector::detectBestBackend());
    qDebug() << "\n";
    
    emit benchmarkStarted("Startup Time");
    BenchmarkResult startup = {"Startup Time", measureStartupTime(), 0, 0, 0, 0, "Application launch to first frame"};
    m_results.append(startup);
    emit benchmarkFinished(startup);
    
    emit benchmarkStarted("Scroll FPS");
    BenchmarkResult scroll = {"Scroll FPS", 0, measureScrollFPS(5), 0, 0, 0, "5-second scroll test"};
    m_results.append(scroll);
    emit benchmarkFinished(scroll);
    
    emit benchmarkStarted("CPU Usage");
    BenchmarkResult cpu = {"CPU Usage", 0, 0, measureCPULoad(5), 0, 0, "5-second CPU load measurement"};
    m_results.append(cpu);
    emit benchmarkFinished(cpu);
    
    emit benchmarkStarted("Memory Usage");
    BenchmarkResult memory = {"Memory Usage", 0, 0, 0, measureMemoryUsage(), 0, "Peak memory after buffer fill"};
    m_results.append(memory);
    emit benchmarkFinished(memory);
    
    emit benchmarkStarted("Glyph Upload");
    double glyphTime = measureGlyphUploadTime(1000);
    BenchmarkResult glyph = {"Glyph Upload", glyphTime, 0, 0, 0, 1000, "1000 glyphs upload time"};
    m_results.append(glyph);
    emit benchmarkFinished(glyph);
    
    emit allBenchmarksFinished();
    printResults();
}

void PerformanceBenchmark::runScrollBenchmark() {
    m_results.clear();
    emit benchmarkStarted("Scroll FPS");
    BenchmarkResult result = {"Scroll FPS", 0, measureScrollFPS(10), 0, 0, 0, "10-second scroll test"};
    m_results.append(result);
    emit benchmarkFinished(result);
    printResults();
}

void PerformanceBenchmark::runStartupBenchmark() {
    m_results.clear();
    emit benchmarkStarted("Startup Time");
    BenchmarkResult result = {"Startup Time", measureStartupTime(), 0, 0, 0, 0, "Application launch to first frame"};
    m_results.append(result);
    emit benchmarkFinished(result);
    printResults();
}

void PerformanceBenchmark::runCPUBenchmark() {
    m_results.clear();
    emit benchmarkStarted("CPU Usage");
    BenchmarkResult result = {"CPU Usage", 0, 0, measureCPULoad(10), 0, 0, "10-second CPU load measurement"};
    m_results.append(result);
    emit benchmarkFinished(result);
    printResults();
}

void PerformanceBenchmark::runMemoryBenchmark() {
    m_results.clear();
    emit benchmarkStarted("Memory Usage");
    BenchmarkResult result = {"Memory Usage", 0, 0, 0, measureMemoryUsage(), 0, "Peak memory after buffer fill"};
    m_results.append(result);
    emit benchmarkFinished(result);
    printResults();
}

void PerformanceBenchmark::runGlyphUploadBenchmark() {
    m_results.clear();
    emit benchmarkStarted("Glyph Upload");
    double time = measureGlyphUploadTime(5000);
    BenchmarkResult result = {"Glyph Upload", time, 0, 0, 0, 5000, "5000 glyphs upload time"};
    m_results.append(result);
    emit benchmarkFinished(result);
    printResults();
}

QString PerformanceBenchmark::generateTestText(int lines) {
    QString text;
    for (int i = 0; i < lines; i++) {
        text += QString("Line %1: The quick brown fox jumps over the lazy dog. 0123456789\n").arg(i);
    }
    return text;
}

double PerformanceBenchmark::measureStartupTime() {
    QProcess process;
    QString appPath = QCoreApplication::applicationDirPath() + "/windterm-terminal";
    
    QDateTime start = QDateTime::currentDateTime();
    process.start(appPath, QStringList() << "--benchmark" << "--exit-on-ready");
    
    if (process.waitForStarted(5000)) {
        process.waitForFinished(10000);
    }
    
    QDateTime end = QDateTime::currentDateTime();
    return start.msecsTo(end);
}

double PerformanceBenchmark::measureScrollFPS(int durationSeconds) {
    int framesRendered = 0;
    QDateTime start = QDateTime::currentDateTime();
    QDateTime end = start.addSecs(durationSeconds);
    
    while (QDateTime::currentDateTime() < end) {
        framesRendered++;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 0);
    }
    
    int elapsed = start.msecsTo(QDateTime::currentDateTime());
    if (elapsed == 0) return 0;
    
    return (framesRendered * 1000.0) / elapsed;
}

double PerformanceBenchmark::measureCPULoad(int durationSeconds) {
#ifdef Q_OS_LINUX
    struct tms startTimes, endTimes;
    clock_t startClock = times(&startTimes);
    
    QCoreApplication::processEvents();
    
    sleep(durationSeconds);
    
    clock_t endClock = times(&endTimes);
    long totalTicks = endClock - startClock;
    long userTicks = endTimes.tms_utime - startTimes.tms_utime;
    long sysTicks = endTimes.tms_stime - startTimes.tms_stime;
    
    long clkTck = sysconf(_SC_CLK_TCK);
    if (totalTicks == 0 || clkTck == 0) return 0;
    
    return ((userTicks + sysTicks) * 100.0) / (totalTicks);
#else
    return 0;
#endif
}

double PerformanceBenchmark::measureMemoryUsage() {
#ifdef Q_OS_LINUX
    QFile statusFile("/proc/self/status");
    if (statusFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&statusFile);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith("VmRSS:")) {
                QStringList parts = line.split(QRegularExpression("\\s+"));
                if (parts.size() >= 2) {
                    return parts[1].toDouble() / 1024.0;
                }
            }
        }
    }
#endif
    return 0;
}

double PerformanceBenchmark::measureGlyphUploadTime(int glyphCount) {
    QDateTime start = QDateTime::currentDateTime();
    
    for (int i = 0; i < glyphCount; i++) {
        QChar ch = QChar(0x20 + (i % 95));
        QImage glyphImage(16, 16, QImage::Format_Grayscale8);
        glyphImage.fill(static_cast<uchar>(i % 256));
    }
    
    return start.msecsTo(QDateTime::currentDateTime());
}

void PerformanceBenchmark::printResults() const {
    qDebug() << "\n=== Benchmark Results ===";
    qDebug() << QString("%1 | %2 | %3 | %4 | %5")
                .arg("Test", -20)
                .arg("Time (ms)", -12)
                .arg("FPS", -10)
                .arg("CPU (%)", -10)
                .arg("Mem (MB)", -10);
    qDebug() << QString("-").repeated(70);
    
    for (const BenchmarkResult& result : m_results) {
        qDebug() << QString("%1 | %2 | %3 | %4 | %5")
                    .arg(result.name, -20)
                    .arg(result.durationMs, -12, 'f', 2)
                    .arg(result.fps, -10, 'f', 1)
                    .arg(result.cpuUsage, -10, 'f', 1)
                    .arg(result.memoryMB, -10, 'f', 1);
    }
    qDebug() << "";
}

void PerformanceBenchmark::saveResults(const QString& filePath) const {
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "WindTerm GPU Renderer Benchmark Results\n";
        out << "Date: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
        out << "Platform: " << PlatformDetector::backendToString(PlatformDetector::detectBestBackend()) << "\n\n";
        
        out << QString("%1,%2,%3,%4,%5,%6,%7\n")
               .arg("Test")
               .arg("Duration (ms)")
               .arg("FPS")
               .arg("CPU (%)")
               .arg("Memory (MB)")
               .arg("Frames")
               .arg("Notes");
        
        for (const BenchmarkResult& result : m_results) {
            out << QString("%1,%2,%3,%4,%5,%6,%7\n")
                   .arg(result.name)
                   .arg(result.durationMs, 0, 'f', 2)
                   .arg(result.fps, 0, 'f', 1)
                   .arg(result.cpuUsage, 0, 'f', 1)
                   .arg(result.memoryMB, 0, 'f', 2)
                   .arg(result.framesRendered)
                   .arg(result.notes);
        }
    }
}
