# WindTerm-AI 使用说明

## 编译

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

## 启动 AI 服务

```bash
cd scripts
python3 context_aware_ai.py      # WebSocket 服务 (端口 8766)
python3 command_history_web.py   # Web UI (端口 8767)
```

## 功能

- **AI 面板**: Ctrl+Shift+A
- **终端触发**: `/ai 你的问题`, `!! 解释`
- **历史导航**: ↑/↓ 箭头（按工作目录过滤）

## 配置

`~/.WindTerm/extensions/config.json`:
```json
{"ai": {"provider": "openai", "api_key": "sk-xxx", "model": "gpt-4"}}
```

## 会话管理功能

### 自动保存
- **工作目录**: 实时跟踪并保存当前工作目录
- **执行的命令**: 记录所有执行的命令
- **成功命令**: 特别标记最后一次成功执行的命令
- **环境变量**: 保存关键环境变量（PATH, HOME, USER, SHELL, PWD 等）

### 会话恢复
启动时显示最近会话，一键恢复到上次工作状态：
```bash
cd /path/to/project  # 自动进入上次工作目录
# 按 ↑ 可以看到上次执行的命令，回车即可重新执行
```

### Web UI 查看
访问 http://localhost:8767 查看：
- 所有活动会话列表
- 每个会话的详细信息（工作目录、命令历史、环境变量）
- 按工作目录过滤命令历史
- 恢复会话状态
