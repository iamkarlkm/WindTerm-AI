# 会话恢复功能使用指南

## 一键恢复终端状态

### 方法 1: Web UI 恢复（推荐）

1. 启动 Web UI:
```bash
cd scripts
python3 command_history_web.py
```

2. 访问 http://localhost:8767

3. 在 Sessions 标签页找到目标会话

4. 点击 **🚀 Restore** 按钮

5. 复制恢复命令并执行:
```bash
python3 session_restore.py restore <session_id>
```

### 方法 2: 命令行恢复

列出所有会话:
```bash
python3 session_restore.py list
```

恢复指定会话:
```bash
python3 session_restore.py restore <session_id>
```

查看会话详情:
```bash
python3 session_restore.py show <session_id>
```

### 恢复内容

恢复工具会自动:

1. **SSH/Telnet 会话**:
   - 自动连接到目标主机
   - 进入上次工作目录
   - 设置环境变量（PATH 等）
   - 执行最后一次成功的命令

2. **本地会话**:
   - 切换到工作目录
   - 恢复环境变量
   - 执行命令或启动交互 shell

3. **Serial 会话**:
   - 连接串口设备
   - 恢复上次状态

### 示例

```bash
# 列出所有会话
$ python3 session_restore.py list

📦 Saved Sessions:
====================================================================================================
#   Session ID                          Host                 Type     Working Dir               Last Active         
====================================================================================================
🟢 1  abc12345-6789-...               192.168.1.100       ssh      /home/user/project        2026-05-17 10:30    
🟢 2  def67890-1234-...               localhost           local    /var/log                  2026-05-17 09:15    

# 恢复会话
$ python3 session_restore.py restore abc12345-6789-...

🔌 Connecting to SSH: user@192.168.1.100:22
📁 Working directory: /home/user/project
🔧 Environment: PATH=/usr/local/bin:...
⚡ Last command: docker-compose up -d

# 自动执行 SSH 连接和命令...
```

## Web UI 功能

### Sessions 标签页
- 查看所有保存的会话
- 显示连接类型、主机、工作目录
- 一键恢复按钮
- 会话详情查看

### Command History 标签页
- 按工作目录过滤命令
- 按会话查看命令历史
- 搜索命令内容

### 会话详情
点击 "Details" 查看:
- 完整会话信息
- 所有环境变量
- 最近 50 条命令
- 创建和活跃时间

## 自动捕获

插件会自动保存:

1. **连接信息**: 主机、端口、协议类型、用户名
2. **工作目录**: 实时跟踪 cd 命令
3. **执行的命令**: 所有输入的命令
4. **成功命令**: 最后一次成功执行的命令（用于恢复）
5. **环境变量**: PATH, HOME, USER, SHELL, PWD, LANG 等

## 数据库位置

```bash
~/.WindTerm/extensions/session_manager.db  # 会话信息
~/.WindTerm/extensions/command_history.db  # 命令历史
```

## 清理旧会话

Web UI 会自动清理 7 天前的非活跃会话。

手动清理:
```sql
DELETE FROM sessions WHERE last_active_at < datetime('now', '-7 days') AND is_active = 0;
```

## 自动填充命令（不执行）

### 方式 1: 快捷键（推荐）

在 WindTerm 终端窗口中：

| 快捷键 | 功能 |
|--------|------|
| **Ctrl+Shift+R** | 恢复上次会话的命令（cd + 最后成功命令） |
| **Ctrl+Shift+L** | 填充最后一次成功执行的命令 |

**重要**: 命令会自动填充到终端输入行，但**不会自动执行**，需要用户手动按回车确认。

### 方式 2: Web UI

1. 访问 http://localhost:8767
2. 找到目标会话
3. 点击 **📝 Fill** 按钮
4. 查看将填充的命令
5. 在终端中手动输入或使用快捷键

### 填充内容

- **Ctrl+Shift+R**: `cd /工作目录 && 最后成功的命令`
- **Ctrl+Shift+L**: `最后成功的命令`

### 示例

假设上次会话在 `/home/user/project` 目录执行了 `docker-compose up -d`：

```bash
# 按 Ctrl+Shift+R 后，终端输入行自动显示：
cd "/home/user/project" && docker-compose up -d

# 用户确认无误后，按 Enter 执行
```

### 插件 API

如果你开发 WindTerm 插件，可以使用：

```cpp
// 发送文本到终端输入行（不执行）
plugin->sendTextToInput("cd /path && command");

// 清空输入行
plugin->clearInput();

// 获取当前输入内容
QString current = plugin->getCurrentInput();
```

### 优势

✅ **用户控制**: 用户可以查看命令后再决定是否执行  
✅ **安全性**: 避免自动执行可能的危险命令  
✅ **可修改**: 填充后可以编辑命令再执行  
✅ **便捷**: 一键恢复，无需手动输入

## 多会话恢复模式

### 恢复单个会话

```bash
# 在新标签页恢复（默认）
python3 session_restore.py restore <session_id>

# 在当前终端恢复（替换当前会话）
python3 session_restore.py restore <session_id> --mode current

# 在新窗口恢复
python3 session_restore.py restore <session_id> --mode window
```

### 恢复多个会话

```bash
# 恢复最近 10 个会话到独立标签页
python3 session_restore.py restore-tabs --limit 10

# 恢复最近 5 个会话
python3 session_restore.py restore-tabs --limit 5
```

### 生成批量恢复脚本

```bash
# 生成恢复最近会话的脚本
python3 session_restore.py batch --limit 5 > restore_sessions.sh
chmod +x restore_sessions.sh
./restore_sessions.sh
```

### 恢复模式对比

| 模式 | 行为 | 适用场景 |
|------|------|----------|
| **tab** (默认) | 每个会话一个新标签页 | 同时管理多个会话 |
| **window** | 每个会话一个新窗口 | 多显示器或需要独立窗口 |
| **current** | 在当前终端执行 | 快速切换，不占用额外标签 |

### 示例

```bash
# 查看会话列表
$ python3 session_restore.py list

# 恢复最近 5 个会话到独立标签页
$ python3 session_restore.py restore-tabs --limit 5
🚀 Restoring 5 sessions to separate tabs...

[1/5] Restoring production-server...
✅ Session opened in new tab
[2/5] Restoring db-server...
✅ Session opened in new tab
...

✅ All 5 sessions restored!

# 在当前终端恢复指定会话
$ python3 session_restore.py restore abc123... --mode current
🔌 Connecting to SSH: user@192.168.1.100:22
# 直接连接，不打开新标签
```

### tmux/screen 支持

如果使用 tmux，新会话会在新 pane 中打开：

```bash
# tmux 环境自动使用 split-window
python3 session_restore.py restore-tabs --limit 3
# 会在 3 个垂直分割的 pane 中打开会话
```
