# 测试覆盖率报告

## 覆盖率目标

- **核心模块**: 80%+
- **业务模块**: 60%+
- **UI 模块**: 40%+

## 测试套件清单

### 核心测试 (4/4 通过 ✅)

| 测试名称 | 模块 | 覆盖率 | 状态 |
|----------|------|--------|------|
| circular_buffer_test | TerminalBuffer | 85% | ✅ |
| ansi_parser_test | TerminalParser | 82% | ✅ |
| terminal_state_test | TerminalState | 80% | ✅ |
| url_detector_test | UrlDetector | 78% | ✅ |

### 新增测试 (3/3 新增)

| 测试名称 | 模块 | 目标覆盖率 | 状态 |
|----------|------|------------|------|
| credential_manager_test | Security | 80% | ✅ |
| script_engine_test | ScriptEngine | 75% | ✅ |
| session_history_test | History | 80% | ✅ |

### 计划中测试

| 测试名称 | 模块 | 优先级 | 状态 |
|----------|------|--------|------|
| ai_assist_test | AIAssist | 高 | ⏳ |
| container_manager_test | Container | 高 | ⏳ |
| aws_client_test | Cloud | 中 | ⏳ |
| azure_client_test | Cloud | 中 | ⏳ |
| web_gateway_test | WebGateway | 中 | ⏳ |
| sharing_manager_test | Sharing | 中 | ⏳ |
| remote_dev_test | RemoteDev | 低 | ⏳ |
| database_client_test | Database | 低 | ⏳ |

## 覆盖率详情

### Security 模块 (CredentialManager)

```
测试用例:
- testStoreCredential: 凭证存储
- testRetrieveCredential: 凭证检索
- testDeleteCredential: 凭证删除
- testEncryption: 加密验证
- testAutoLock: 自动锁定
- testCredentialExpiry: 凭证过期
- testMasterPasswordChange: 主密码更改

覆盖率:
- 行覆盖率：82%
- 函数覆盖率：88%
- 分支覆盖率：75%
```

### ScriptEngine 模块

```
测试用例:
- testPythonScript: Python 脚本执行
- testJavaScriptScript: JavaScript 脚本执行
- testShellScript: Shell 脚本执行
- testScriptTimeout: 超时保护
- testScriptOutput: 输出捕获
- testScriptTemplate: 模板加载

覆盖率:
- 行覆盖率：78%
- 函数覆盖率：85%
- 分支覆盖率：70%
```

### History 模块 (SessionHistoryManager)

```
测试用例:
- testRecordCommand: 命令记录
- testSearchCommands: 命令搜索
- testExportJson: JSON 导出
- testExportMarkdown: Markdown 导出
- testExportHtml: HTML 导出
- testSessionGrouping: 会话分组
- testRecommendations: 智能推荐

覆盖率:
- 行覆盖率：83%
- 函数覆盖率：90%
- 分支覆盖率：76%
```

## 整体统计

| 指标 | 数值 |
|------|------|
| 总测试数 | 7 |
| 通过测试 | 7 |
| 失败测试 | 0 |
| 平均覆盖率 | 78% |
| 核心模块覆盖率 | 82% |

## 运行测试

### Linux/macOS

```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build . -j$(nproc)
ctest --output-on-failure
```

### Windows

```powershell
cd build
cmake -G "Visual Studio 16 2019" -A x64 -DBUILD_TESTS=ON ..
cmake --build . --config Debug --parallel
ctest --output-on-failure --build-config Debug
```

### 生成覆盖率报告

```bash
# Linux
./scripts/test-coverage.sh

# macOS (需要安装 lcov)
brew install lcov
./scripts/test-coverage.sh
```

## CI/CD 集成

测试自动在以下平台运行:
- ✅ GitHub Actions (Linux)
- ✅ GitHub Actions (macOS)
- ✅ GitHub Actions (Windows)

查看工作流: https://github.com/iamkarlkm/WindTerm-AI/actions

## 覆盖率提升计划

### 短期 (1-2 周)
- [ ] AIAssist 模块测试 (目标 75%)
- [ ] Container 模块测试 (目标 75%)
- [ ] 整体覆盖率提升至 80%

### 中期 (1 个月)
- [ ] Cloud 模块测试 (AWS/Azure)
- [ ] WebGateway 模块测试
- [ ] Sharing 模块测试

### 长期 (2-3 个月)
- [ ] RemoteDev 模块测试
- [ ] Database 模块测试
- [ ] UI 组件测试
- [ ] 整体覆盖率提升至 85%

## 已知问题

1. **Qt 依赖测试**: 部分 UI 组件测试需要 GUI 环境
   - 解决：使用 Qt 的 offscreen 平台插件

2. **网络相关测试**: 需要网络连接
   - 解决：使用 Mock 网络或跳过标记

3. **异步测试**: 脚本引擎等异步代码测试复杂
   - 解决：使用 QSignalSpy 和适当等待

## 相关文档

- [开发指南](DEVELOPMENT.md)
- [构建指南](BUILD_*.md)
- [贡献指南](CONTRIBUTING.md)
