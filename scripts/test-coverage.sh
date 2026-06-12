#!/bin/bash
# 测试覆盖率脚本

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

echo "=== WindTerm-Extensions 测试覆盖率 ==="

# 创建构建目录
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 配置带覆盖率的项目
echo "配置 CMake (带覆盖率)..."
cmake .. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCODE_COVERAGE=ON \
  -DBUILD_TESTS=ON

# 构建
echo "构建项目..."
cmake --build . -j$(nproc)

# 运行测试
echo "运行所有测试..."
ctest --output-on-failure

# 生成覆盖率报告 (如果有 gcov)
if command -v gcov &> /dev/null; then
    echo "生成覆盖率报告..."
    
    # 使用 lcov 生成报告
    if command -v lcov &> /dev/null; then
        lcov --capture \
             --directory . \
             --output-file coverage.info \
             --gcov-tool gcov
        
        # 过滤系统文件
        lcov --remove coverage.info \
             '/usr/*' \
             '*/tests/*' \
             '*/Qt/*' \
             --output-file coverage.filtered.info
        
        # 生成 HTML 报告
        genhtml coverage.filtered.info --output-directory coverage_report
        
        echo "覆盖率报告已生成：$BUILD_DIR/coverage_report/index.html"
    else
        echo "警告：lcov 未安装，跳过 HTML 报告生成"
        echo "安装：sudo apt-get install lcov"
    fi
else
    echo "警告：gcov 未安装，无法生成覆盖率报告"
fi

echo "=== 测试完成 ==="
