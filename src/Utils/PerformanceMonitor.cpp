#include "PerformanceMonitor.h"
#include <QElapsedTimer>
#include <QDateTime>
#include <QDebug>

#ifdef Q_OS_LINUX
#include <sys/resource.h>
#endif

PerformanceMonitor* PerformanceMonitor::s_instance = nullptr;

PerformanceMonitor::PerformanceMonitor(QObject* parent)
    : QObject(parent)
    , m_peakMemory(0)
    , m_fpsThreshold(30.0)
    , m_renderTimeThreshold(16.7)  // 60 FPS target
    , m_memoryThreshold(512 * 1024 * 1024)  // 512MB
    , m_monitoring(true)
    , m_maxHistorySeconds(60)
    , m_lastFrameTime(0)
    , m_frameCount(0) {
}

PerformanceMonitor* PerformanceMonitor::instance() {
    if (!s_instance) {
        s_instance = new PerformanceMonitor();
    }
    return s_instance;
}

void PerformanceMonitor::recordFrameRendered(double renderTimeMs) {
    if (!m_monitoring) return;
    
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    
    if (m_lastFrameTime > 0) {
        qint64 frameInterval = now - m_lastFrameTime;
        if (frameInterval > 0) {
            m_currentMetrics.fps = 1000.0 / frameInterval;
        }
    }
    
    m_currentMetrics.renderTimeMs = renderTimeMs;
    m_currentMetrics.timestamp = now;
    
    m_frameCount++;
    m_lastFrameTime = now;
    
    m_metricsHistory.enqueue(m_currentMetrics);
    cleanupOldMetrics();
    checkThresholds();
    
    emit metricsUpdated(m_currentMetrics);
}

void PerformanceMonitor::recordParseCompleted(double parseTimeMs, int charsParsed) {
    m_currentMetrics.parseTimeMs = parseTimeMs;
}

void PerformanceMonitor::updateMemoryUsage() {
#ifdef Q_OS_LINUX
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        m_currentMetrics.memoryUsageBytes = usage.ru_maxrss * 1024;  // Convert KB to bytes
        if (m_currentMetrics.memoryUsageBytes > m_peakMemory) {
            m_peakMemory = m_currentMetrics.memoryUsageBytes;
        }
    }
#else
    m_currentMetrics.memoryUsageBytes = 0;
#endif
}

void PerformanceMonitor::updateConnectionCount(int count) {
    m_currentMetrics.activeConnections = count;
}

PerformanceMetrics PerformanceMonitor::currentMetrics() const {
    return m_currentMetrics;
}

double PerformanceMonitor::averageFps(int lastSeconds) const {
    if (m_metricsHistory.isEmpty()) return 0;
    
    qint64 cutoff = QDateTime::currentMSecsSinceEpoch() - (lastSeconds * 1000);
    double sum = 0;
    int count = 0;
    
    for (const PerformanceMetrics& m : m_metricsHistory) {
        if (m.timestamp >= cutoff) {
            sum += m.fps;
            count++;
        }
    }
    
    return count > 0 ? sum / count : 0;
}

double PerformanceMonitor::averageRenderTime(int lastSeconds) const {
    if (m_metricsHistory.isEmpty()) return 0;
    
    qint64 cutoff = QDateTime::currentMSecsSinceEpoch() - (lastSeconds * 1000);
    double sum = 0;
    int count = 0;
    
    for (const PerformanceMetrics& m : m_metricsHistory) {
        if (m.timestamp >= cutoff) {
            sum += m.renderTimeMs;
            count++;
        }
    }
    
    return count > 0 ? sum / count : 0;
}

quint64 PerformanceMonitor::peakMemoryUsage() const {
    return m_peakMemory;
}

PerformanceMonitor::PerformanceReport PerformanceMonitor::generateReport(int lastSeconds) const {
    PerformanceReport report;
    report.timeRangeSeconds = lastSeconds;
    
    if (m_metricsHistory.isEmpty()) {
        return report;
    }
    
    qint64 cutoff = QDateTime::currentMSecsSinceEpoch() - (lastSeconds * 1000);
    
    report.avgFps = 0;
    report.minFps = 1e9;
    report.maxFps = 0;
    report.avgRenderTime = 0;
    report.maxRenderTime = 0;
    report.avgMemory = 0;
    report.maxMemory = 0;
    report.totalFrames = 0;
    
    double fpsSum = 0, renderSum = 0, memorySum = 0;
    
    for (const PerformanceMetrics& m : m_metricsHistory) {
        if (m.timestamp >= cutoff) {
            report.totalFrames++;
            
            fpsSum += m.fps;
            if (m.fps < report.minFps) report.minFps = m.fps;
            if (m.fps > report.maxFps) report.maxFps = m.fps;
            
            renderSum += m.renderTimeMs;
            if (m.renderTimeMs > report.maxRenderTime) report.maxRenderTime = m.renderTimeMs;
            
            memorySum += m.memoryUsageBytes;
            if (m.memoryUsageBytes > report.maxMemory) report.maxMemory = m.memoryUsageBytes;
        }
    }
    
    if (report.totalFrames > 0) {
        report.avgFps = fpsSum / report.totalFrames;
        report.avgRenderTime = renderSum / report.totalFrames;
        report.avgMemory = memorySum / report.totalFrames;
    }
    
    return report;
}

void PerformanceMonitor::setFpsThreshold(double minFps) {
    m_fpsThreshold = minFps;
}

void PerformanceMonitor::setRenderTimeThreshold(double maxMs) {
    m_renderTimeThreshold = maxMs;
}

void PerformanceMonitor::setMemoryThreshold(quint64 maxBytes) {
    m_memoryThreshold = maxBytes;
}

void PerformanceMonitor::startMonitoring() {
    m_monitoring = true;
}

void PerformanceMonitor::stopMonitoring() {
    m_monitoring = false;
}

void PerformanceMonitor::resetMetrics() {
    m_metricsHistory.clear();
    m_currentMetrics = PerformanceMetrics();
    m_peakMemory = 0;
    m_frameCount = 0;
    m_lastFrameTime = 0;
}

void PerformanceMonitor::checkThresholds() {
    if (m_currentMetrics.fps > 0 && m_currentMetrics.fps < m_fpsThreshold) {
        emit fpsDropped(m_currentMetrics.fps);
        qDebug() << "[PerformanceMonitor] FPS dropped:" << m_currentMetrics.fps;
    }
    
    if (m_currentMetrics.renderTimeMs > m_renderTimeThreshold) {
        emit renderTimeExceeded(m_currentMetrics.renderTimeMs);
        qDebug() << "[PerformanceMonitor] Render time exceeded:" << m_currentMetrics.renderTimeMs << "ms";
    }
    
    if (m_currentMetrics.memoryUsageBytes > m_memoryThreshold) {
        emit memoryLimitExceeded(m_currentMetrics.memoryUsageBytes);
        qDebug() << "[PerformanceMonitor] Memory limit exceeded:" << m_currentMetrics.memoryUsageBytes;
    }
}

void PerformanceMonitor::cleanupOldMetrics() {
    qint64 cutoff = QDateTime::currentMSecsSinceEpoch() - (m_maxHistorySeconds * 1000);
    
    while (!m_metricsHistory.isEmpty() && m_metricsHistory.head().timestamp < cutoff) {
        m_metricsHistory.dequeue();
    }
}

#include "PerformanceMonitor.moc"
