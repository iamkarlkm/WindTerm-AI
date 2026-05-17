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
