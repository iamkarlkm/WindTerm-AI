#include <QCoreApplication>
#include <QDebug>
#include "PerformanceBenchmark.h"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("WindTerm Performance Benchmark");
    app.setApplicationVersion("0.2.0");
    
    PerformanceBenchmark benchmark;
    benchmark.runAllBenchmarks();
    
    return 0;
}
