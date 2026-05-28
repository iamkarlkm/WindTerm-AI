# WindTerm-AI Extensions

**完全独立的 AI 增强型 WindTerm** - 基于 Qt5 的跨平台 GPU 加速终端模拟器，一次克隆，一次编译，直接使用。

## 快速开始

```bash
git clone https://github.com/iamkarlkm/WindTerm-AI.git
cd WindTerm-Extensions
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
./windterm-terminal
```

## 系统要求

- **编译器**: C++17 兼容 (GCC 7+, Clang 5+, MSVC 2017+)
- **Qt5**: 5.12+ (Core, Gui, Widgets, OpenGL, Sql)
- **libssh**: SSH2 协议支持
- **CMake**: 3.16+

## 功能特性

### GPU 渲染引擎
- **多后端支持**: OpenGL 3.3 (跨平台), Metal (macOS), DirectX12 (Windows), Vulkan (Linux, 预留)
- **SDF 字体渲染**: 基于 Signed Distance Field 的高质量抗锯齿字体
- **高性能渲染**: GPU 加速的终端内容绘制，支持大量文本流畅滚动
- **逐单元格着色**: 精确的字符/背景/前景色独立渲染

### 终端核心
- **ANSI 解析**: 完整的 ANSI 转义序列解析，支持光标控制、颜色设置、窗口操作
- **环形缓冲区**: 高效的滚动回滚历史管理
- **PTY 管理**: 跨平台伪终端支持 (Linux/Mac/Windows)
- **终端会话**: 完整的终端状态管理

### SSH 远程连接 (Feature 1)
- **快捷键**: `Ctrl+Shift+S`
- **完整 SSH2 支持**: 基于 libssh，支持密码和公钥认证
- **连接管理**: 创建、编辑、删除连接配置
- **会话列表**: 主窗口左侧显示已保存的连接
- **数据持久化**: SQLite 存储连接配置

### 主题系统 (Feature 2)
- **快捷键**: `Ctrl+Shift+T`
- **内置主题**: Solarized Dark, Solarized Light, Monokai, GitHub Dark
- **ANSI 16 色定制**: 每个主题独立配置完整的 ANSI 调色板
- **自定义主题**: 用户可创建和编辑自定义主题
- **配置项**: 字体、字号、光标样式、光标/选区颜色
- **数据持久化**: SQLite 存储主题配置

### Tab 管理 (Feature 3)
- **多 Tab 支持**: 自定义 TabWidget/TabBar 实现
- **双击重命名**: 双击 Tab 标题进行内联编辑
- **右键菜单**: 关闭、关闭其他、新建 Tab
- **状态持久化**: 启动时自动恢复上次会话的 Tab 列表和名称
- **快捷键**: `Ctrl+T` 新建 Tab

### 命令历史 (Feature 4)
- **快捷键**: `Ctrl+Shift+R`
- **SQLite 存储**: `~/.local/share/WindTerm-AI/command_history.db`
- **工作目录关联**: 每条记录关联执行时的当前目录
- **搜索过滤**: 按关键词搜索历史命令
- **快速重放**: 双击命令直接发送到终端执行

### 书签管理 (Feature 5)
- **快捷键**: `Ctrl+Shift+B`
- **分类管理**: 支持按类别组织书签 (开发、配置、文档等)
- **自动 cd**: 跳转到书签时自动切换工作目录
- **SQLite 存储**: `~/.local/share/WindTerm-AI/bookmarks.db`
- **搜索过滤**: 按名称搜索书签

### 设置导入/导出 (Feature 6)
- **JSON 格式**: 统一的 JSON 配置导出，便于跨设备迁移
- **全量导出**: 包含 SSH 连接、主题、历史、书签、终端设置
- **选择性导入**: 可选择性地导入部分配置
- **数据验证**: 导入前验证 JSON 结构和数据完整性

### 记忆碎片 (Memory Fragments)
- **上下文标记**: 为重要的终端会话片段添加标签和上下文
- **SQLite 存储**: 持久化存储记忆碎片
- **查看/编辑**: 完整的记忆碎片管理对话框

### AI 集成 (Phase 5)
- **快捷键**: `Ctrl+Shift+A`
- **终端触发**: `/ai 你的问题` 或 `!! 解释`
- **上下文感知**: 自动分析工作目录和命令历史
- **多后端支持**: OpenAI, Claude, Ollama, 自定义 API
- **流式响应**: 支持 SSE 流式输出，实时显示 AI 回复
- **WebSocket 本地**: 支持本地 Python AI 服务器 (端口 8766)
- **对话 UI**: 完整的聊天界面，支持历史记录
- **API 配置**: 内置 API Key 管理，支持多 Provider 切换

### 文件传输 (Phase 6)
- **快捷键**: `Ctrl+Shift+F`
- **SCP 协议**: 基于 libssh 的安全文件传输
- **文件上传/下载**: 支持单文件和目录传输
- **远程文件浏览**: 可视化远程目录结构
- **进度显示**: 实时传输进度条
- **拖拽支持**: 支持从本地拖拽文件到终端上传
- **权限管理**: 自动设置文件权限 (644)

### 终端录制与回放 (Phase 7)
- **快捷键**: `Ctrl+Shift+R` (录制)
- **Asciinema v2 格式**: 标准 JSON 录制文件，兼容外部播放工具
- **实时录制**: 捕获终端输出的 ANSI 转义序列和时间戳
- **可变速度回放**: 支持 1x-4x 速度调节
- **播放控制**: 播放/暂停/停止完整控制
- **进度显示**: 实时回放进度条
- **自动加载**: 录制结束后自动加载到播放器

### Bug 修复与改进
- **字体设置修复**: `calculateCharDimensions()` 现在正确使用主题字体配置
- **插件构建集成**: 示例插件自动编译到 `build/plugins/` 目录
- **PluginLoader 修复**: JSON 元数据路径解析 bug 已修复
- **AI 触发实现**: `/ai <问题>` 和 `!! 解释` 命令现在可以正常工作
- **终端内搜索**: `Ctrl+Shift+G` 在当前终端缓冲区搜索文本并高亮匹配
- **拖拽上传**: 支持从文件管理器拖拽文件到终端，自动插入转义后的路径
- **滚动条集成**: 终端有回滚历史时自动显示右侧滚动条
- **Bracketed Paste Mode**: 支持现代 Shell 的括号粘贴模式，区分粘贴输入和手动输入
- **URL 检测与点击**: 自动检测 URL 和文件路径，`Ctrl+Click` 打开链接或文件
- **OSC 8 超链接**: 支持现代终端 OSC 8 标准超链接，蓝色下划线标识
- **Bell 通知**: 收到 bell 字符时发出声音并闪烁屏幕
- **单元测试**: 添加 AnsiParser 和 TerminalState 核心模块单元测试 (4 个测试全部通过)

### 插件系统 (Phase 4)
- **快捷键**: `Ctrl+Shift+P`
- **动态加载**: 基于 QPluginLoader 的动态库加载
- **生命周期管理**: 加载/初始化/启动/停止/卸载完整生命周期
- **依赖管理**: 自动检查插件依赖关系
- **事件广播**: 终端输出、命令执行、目录变化等事件广播
- **键盘拦截**: 插件可拦截键盘事件实现自定义快捷键
- **插件上下文**: 向插件暴露终端写入、设置读写、通知发送等能力
- **示例插件**: TerminalInfo (会话统计), ThemeSwitcher (快速切换主题)
- **管理界面**: PluginManagerDialog 提供完整的插件管理 UI

## 快捷键汇总

| 快捷键 | 功能 |
|--------|------|
| `Ctrl+Shift+S` | SSH 连接管理器 |
| `Ctrl+Shift+T` | 主题选择器 |
| `Ctrl+Shift+B` | 书签管理 |
| `Ctrl+Shift+I` | 设置导入/导出 |
| `Ctrl+Shift+P` | 插件管理器 |
| `Ctrl+Shift+A` | AI 助手 |
| `Ctrl+Shift+F` | 文件传输 (SCP) |
| `Ctrl+Shift+G` | 终端内搜索 |
| `Ctrl+Shift+R` | 终端录制与回放 |
| `Ctrl+Shift+H` | 水平分割终端 |
| `Ctrl+T` | 新建 Tab |
| `Tab 双击` | 重命名 Tab |
| `/ai <问题>` | 终端内触发 AI 问答 |
| `!! 解释` | 上一条命令 AI 解释 |

### 测试

```bash
# 运行所有单元测试
cd build && QT_QPA_PLATFORM=offscreen ctest --output-on-failure

# 测试覆盖
# - CircularTextBufferTest: 环形文本缓冲区
# - AnsiParserTest: ANSI 转义序列解析
# - TerminalStateTest: 终端状态管理
# - UrlDetectorTest: URL 和文件路径检测
# - UnicodeUtilTest: Unicode 字符宽度/Emoji 检测
# - PerformanceBenchmark: 渲染性能基准

# 测试结果：6/6 测试通过
```

## 构建选项

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_RENDERER=ON \
  -DBUILD_BUFFER=ON \
  -DBUILD_THEME=ON \
  -DBUILD_WIDGET=ON \
  -DBUILD_APP=ON \
  -DBUILD_PLUGINS=ON \
  -DBUILD_TESTS=OFF \
  -DWITH_OPENGL=ON \
  -DWITH_METAL=OFF \
  -DWITH_DIRECTX=OFF \
  -DWITH_VULKAN=OFF
```

## 插件开发

### 插件目录结构
```
plugins/
├── terminal_info.json          # 插件元数据
├── theme_switcher.json         # 插件元数据
├── libterminal_info_plugin.so  # 插件动态库
└── libtheme_switcher_plugin.so # 插件动态库
```

### 创建自定义插件
1. 继承 `PluginInterface` 基类
2. 实现 `metadata()` 返回插件元数据
3. 实现 `initialize()` 和 `shutdown()` 生命周期方法
4. 可选实现事件回调 (onCommandExecuted, interceptKeyEvent 等)
5. 使用 `Q_PLUGIN_METADATA` 宏声明插件
6. 编译为 MODULE 类型的动态库
7. 将 .so 文件和 .json 元数据文件放入 plugins 目录

### 插件元数据格式 (JSON)
```json
{
    "id": "my-plugin",
    "name": "My Plugin",
    "version": "1.0.0",
    "description": "Plugin description",
    "author": "Author Name",
    "type": "TerminalHook",
    "dependencies": ["other-plugin-id"]
}
```

### 插件类型
- `TerminalHook`: 终端事件钩子
- `ThemeProvider`: 主题提供者
- `CommandExtension`: 命令扩展
- `UISupplement`: UI 补充
- `ProtocolHandler`: 协议处理器

## 项目结构

```
WindTerm-Extensions/
├── CMakeLists.txt                    # 主构建配置
├── README.md                         # 本文档
├── src/
│   ├── Renderer/                     # GPU 渲染引擎
│   ├── Buffer/                       # 环形文本缓冲区
│   ├── Terminal/                     # 终端核心 (ANSI, PTY, Session)
│   ├── Widget/                       # UI 控件 (主窗口, Tab, 对话框)
│   ├── Theme/                        # 主题系统
│   ├── Ssh/                          # SSH 远程连接
│   ├── CommandHistory/               # 命令历史
│   ├── Bookmarks/                    # 书签管理
│   ├── MemoryFragment/               # 记忆碎片
│   ├── Settings/                     # 设置管理
│   ├── plugins/                      # 插件系统
│   │   ├── PluginInterface.h         # 插件接口
│   │   ├── PluginContext.h           # 插件上下文
│   │   ├── PluginLoader.h            # 动态库加载器
│   │   ├── PluginManager.h           # 插件管理器
│   │   ├── TerminalInfoPlugin/       # 示例插件：会话统计
│   │   └── ThemeSwitcherPlugin/      # 示例插件：主题切换
│   ├── Protocols/                    # 协议层
│   ├── Pty/                          # PTY 跨平台实现
│   ├── AiIntegration/                # AI 集成
│   ├── Utility/                      # 工具模块
│   ├── libssh/                       # libssh 第三方库
│   └── Onigmo/                       # Onigmo 正则表达式库
├── tests/                            # 单元测试
└── scripts/                          # Python 辅助脚本
```

## 数据存储路径

| 模块 | 数据库文件 | 路径 |
|------|-----------|------|
| SSH 连接 | `ssh_profiles.db` | `~/.local/share/WindTerm-AI/` |
| 命令历史 | `command_history.db` | `~/.local/share/WindTerm-AI/` |
| 书签 | `bookmarks.db` | `~/.local/share/WindTerm-AI/` |
| 主题 | 内建 + SQLite | - |
| 设置 | QSettings | - |

## 许可证

Apache-2.0
