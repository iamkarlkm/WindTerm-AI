#ifndef PERFORMANCE_MONITOR_H
#define PERFORMANCE_MONITOR_H

#include <QObject>
#include <QMap>
#include <QQueue>

struct PerformanceMetrics {
    double fps;
    double renderTimeMs;
    double parseTimeMs;
    quint64 memoryUsageBytes;
    double cpuUsagePercent;
    int activeConnections;
    int sessionCount;
    qint64 timestamp;
    
    PerformanceMetrics() 
        : fps(0), renderTimeMs(0), parseTimeMs(0), 
          memoryUsageBytes(0), cpuUsagePercent(0),
          activeConnections(0), sessionCount(0), timestamp(0) {}
};

class PerformanceMonitor : public QObject {
    Q_OBJECT
public:
    explicit PerformanceMonitor(QObject* parent = nullptr);
    
    static PerformanceMonitor* instance();
    
    // 指标记录
    void recordFrameRendered(double renderTimeMs);
    void recordParseCompleted(double parseTimeMs, int charsParsed);
    void updateMemoryUsage();
    void updateConnectionCount(int count);
    
    // 指标查询
    PerformanceMetrics currentMetrics() const;
    double averageFps(int lastSeconds = 5) const;
    double averageRenderTime(int lastSeconds = 5) const;
    quint64 peakMemoryUsage() const;
    
    // 性能分析
    struct PerformanceReport {
        double avgFps;
        double minFps;
        double maxFps;
        double avgRenderTime;
        double maxRenderTime;
        quint64 avgMemory;
        quint64 maxMemory;
        int totalFrames;
        qint64 timeRangeSeconds;
    };
    
    PerformanceReport generateReport(int lastSeconds = 60) const;
    
    // 阈值告警
    void setFpsThreshold(double minFps);
    void setRenderTimeThreshold(double maxMs);
    void setMemoryThreshold(quint64 maxBytes);
    
    // 监控控制
    void startMonitoring();
    void stopMonitoring();
    void resetMetrics();
    
signals:
    void fpsDropped(double fps);
    void renderTimeExceeded(double ms);
    void memoryLimitExceeded(quint64 bytes);
    void metricsUpdated(const PerformanceMetrics& metrics);

private:
    void checkThresholds();
    void cleanupOldMetrics();
    
    static PerformanceMonitor* s_instance;
    
    QQueue<PerformanceMetrics> m_metricsHistory;
    PerformanceMetrics m_currentMetrics;
    quint64 m_peakMemory;
    
    double m_fpsThreshold;
    double m_renderTimeThreshold;
    quint64 m_memoryThreshold;
    
    bool m_monitoring;
    int m_maxHistorySeconds;
    
    qint64 m_lastFrameTime;
    int m_frameCount;
};

#endif
