# WindTerm AI - Web Terminal Client

基于 Tauri 的现代化 Web 终端客户端，连接 WindTerm Web Terminal Gateway。

## 功能特性

- **多标签终端** - 同时管理多个 SSH 会话
- **WebSocket 连接** - 通过 Web 终端网关连接远程主机
- **现代化 UI** - 深色主题，Catppuccin 风格配色
- **碎片笔记** - 富媒体编辑器，支持代码高亮、格式化和本地持久化
- **右键菜单** - 复制终端内容、保存选中文本为笔记
- **键盘快捷键** - 完整的快捷键支持

## 快捷键

| 快捷键 | 功能 |
|--------|------|
| `Ctrl+T` | 新建标签 |
| `Ctrl+W` | 关闭当前标签 |
| `Ctrl+1-9` | 切换标签 |
| `Ctrl+B` | 切换笔记面板 |
| `Ctrl+S` | 保存笔记 (笔记编辑时) |
| `Ctrl+Shift+C` | 复制终端选中内容 |
| `Ctrl+Shift+V` | 粘贴到终端 |

## 快速开始

### 浏览器直接使用

```bash
cd webclient
python3 -m http.server 3000
```

打开 http://localhost:3000

### Tauri 桌面应用

```bash
cd webclient

# 安装 Tauri CLI
cargo install tauri-cli --version "^2.0"

# 开发模式
cargo tauri dev

# 构建发布版
cargo tauri build
```

## 连接配置

1. 确保 Web 终端网关已启动 (参考项目根目录)
2. 在顶部输入目标主机地址
3. 输入认证令牌
4. 点击"连接"

## 架构

```
浏览器/桌面窗口
    │
    ├── xterm.js (终端渲染)
    ├── WebSocket (连接网关)
    │
    ▼
WebTerminalGateway (Qt/C++)
    │
    ▼
SSH/TCP 连接 (远程主机)
```

## 技术栈

- **前端**: HTML/CSS/JavaScript + xterm.js
- **桌面**: Tauri 2 (Rust)
- **终端**: xterm.js + addon-fit + addon-web-links
- **富文本**: contenteditable + execCommand
- **存储**: localStorage (Web) / Tauri FS (Desktop)
