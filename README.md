# WindTerm-AI Extensions

**完全独立的 AI 增强型 WindTerm** - 一次克隆，一次编译，直接使用。

## 快速开始

```bash
git clone https://github.com/iamkarlkm/WindTerm-AI.git
cd WindTerm-Extensions
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

## 新增功能

### AI 智能推荐
- **快捷键**: Ctrl+Shift+A
- **终端触发**: `/ai 你的问题` 或 `!! 解释`
- **上下文感知**: 自动分析工作目录和命令历史

### 命令历史导航
- **↑/↓ 箭头**: 按工作目录过滤浏览历史
- **SQLite 存储**: `~/.WindTerm/extensions/command_history.db`
- **Web UI**: http://localhost:8767

### Python AI 服务
```bash
cd scripts
python3 context_aware_ai.py      # WebSocket (端口 8766)
python3 command_history_web.py   # Web UI (端口 8767)
```

## 配置

`~/.WindTerm/extensions/config.json`:
```json
{
  "ai": {
    "provider": "openai",
    "api_key": "sk-xxx",
    "model": "gpt-4"
  }
}
```

## 项目结构

```
WindTerm-Extensions/
├── README.md                    # 本文档
├── README_AI.md                 # AI 功能说明
├── scripts/                     # Python AI 服务
└── src/                         # WindTerm 源码 + AI 扩展
    ├── AiIntegration/           # AI 客户端
    ├── plugins/
    │   ├── interfaces/          # 插件接口
    │   └── TerminalHistoryPlugin/  # 历史导航
    └── ...
```

## 许可证

Apache-2.0
