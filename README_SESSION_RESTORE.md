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
