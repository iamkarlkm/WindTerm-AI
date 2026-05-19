#!/usr/bin/env python3
"""会话恢复工具 - 支持多种恢复模式"""

import sys
import subprocess
import sqlite3
import os
from pathlib import Path
from datetime import datetime
import argparse

DB_PATH = Path.home() / ".WindTerm" / "extensions" / "session_manager.db"

def get_session(session_id):
    if not DB_PATH.exists():
        print(f"Error: Database not found at {DB_PATH}")
        return None
    conn = sqlite3.connect(str(DB_PATH))
    conn.row_factory = sqlite3.Row
    cur = conn.cursor()
    cur.execute("SELECT * FROM sessions WHERE session_id = ?", (session_id,))
    session = dict(cur.fetchone())
    if not session:
        print(f"Error: Session {session_id} not found")
        conn.close()
        return None
    cur.execute("SELECT var_name, var_value FROM session_environment WHERE session_id = ?", (session_id,))
    session['environment'] = {row['var_name']: row['var_value'] for row in cur.fetchall()}
    cur.execute("SELECT command FROM command_history WHERE session_id = ? ORDER BY timestamp DESC LIMIT 10", (session_id,))
    session['recent_commands'] = [row['command'] for row in cur.fetchall()]
    conn.close()
    return session

def restore_ssh_tab(session):
    """恢复 SSH 会话到新标签页 (使用 gnome-terminal 或 tmux)"""
    host = session['host']
    port = session['port'] or 22
    username = session['username']
    workdir = session['working_directory']
    cmd = session['last_successful_command']
    
    commands = []
    if workdir:
        commands.append(f"cd {workdir}")
    if cmd:
        commands.append(cmd)
    
    ssh_cmd = f"ssh {'-l ' + username + ' ' if username else ''}{host} -p {port}"
    
    # 检测终端类型
    terminal = os.environ.get('TERM_PROGRAM', 'unknown')
    
    if 'Apple_Terminal' in terminal or 'iTerm' in terminal:
        # macOS
        script = f'''
        tell application "Terminal"
            activate
            do script "{ssh_cmd}"
        end tell
        '''
        subprocess.run(['osascript', '-e', script])
    elif 'gnome' in terminal.lower() or 'GNOME' in os.environ.get('DESKTOP_SESSION', ''):
        # GNOME Terminal
        subprocess.Popen(['gnome-terminal', '--tab', '-e', 'bash', '-c', ssh_cmd])
    else:
        # 默认：在当前终端打开新标签（使用 tmux 或 screen）
        print(f"🔌 Opening SSH in new tmux pane: {ssh_cmd}")
        subprocess.Popen(['tmux', 'split-window', '-h', ssh_cmd])
    
    print(f"✅ Session opened in new tab/pane")

def restore_ssh_current(session):
    """在当前终端执行 SSH（替换当前会话）"""
    host = session['host']
    port = session['port'] or 22
    username = session['username']
    workdir = session['working_directory']
    cmd = session['last_successful_command']
    
    ssh_cmd = ['ssh', '-p', str(port)]
    if username:
        ssh_cmd.extend(['-l', username])
    ssh_cmd.append(host)
    
    print(f"\n🔌 Connecting to SSH: {username+'@' if username else ''}{host}:{port}")
    if workdir:
        print(f"📁 Will cd to: {workdir}")
    if cmd:
        print(f"⚡ Will execute: {cmd}")
    
    subprocess.run(ssh_cmd)

def restore_local_tab(session):
    """恢复本地会话到新标签页"""
    workdir = session['working_directory']
    cmd = session['last_successful_command']
    
    terminal = os.environ.get('TERM_PROGRAM', 'unknown')
    
    # 构建命令
    commands = []
    if workdir:
        commands.append(f"cd {workdir}")
        print(f"📁 Working directory: {workdir}")
    for key, value in session.get('environment', {}).items():
        if key in ['PATH', 'LD_LIBRARY_PATH', 'PYTHONPATH']:
            commands.append(f"export {key}={value}")
    if cmd:
        commands.append(cmd)
        print(f"⚡ Command: {cmd}")
    
    full_cmd = " && ".join(commands) if commands else "exec $SHELL"
    
    if 'gnome' in terminal.lower():
        subprocess.Popen(['gnome-terminal', '--tab', '-e', 'bash', '-c', full_cmd])
    else:
        # 使用 tmux 新窗口
        subprocess.Popen(['tmux', 'new-window', '-n', 'Session', full_cmd])
    
    print(f"✅ Session opened in new tab")

def restore_local_current(session):
    """在当前终端恢复本地会话"""
    workdir = session['working_directory']
    cmd = session['last_successful_command']
    
    print(f"\n📁 Restoring local session")
    
    if workdir:
        print(f"Working directory: {workdir}")
        os.chdir(workdir)
    
    for key, value in session.get('environment', {}).items():
        if key in ['PATH', 'LD_LIBRARY_PATH']:
            os.environ[key] = value
            print(f"Environment: {key}={value[:50]}...")
    
    if cmd:
        print(f"Executing: {cmd}")
        print("-" * 60)
        subprocess.run(cmd, shell=True)
    else:
        print("Starting interactive shell...")
        subprocess.run(os.environ.get('SHELL', 'bash'))

def restore_all_tabs(sessions):
    """一次性恢复所有会话到独立标签页"""
    print(f"\n🚀 Restoring {len(sessions)} sessions to separate tabs...\n")
    
    for i, session in enumerate(sessions, 1):
        conn_type = (session['connection_type'] or 'ssh').lower()
        print(f"[{i}/{len(sessions)}] Restoring {session['session_name'] or session['session_id'][:8]}...")
        
        if conn_type == 'ssh':
            restore_ssh_tab(session)
        elif conn_type == 'telnet':
            subprocess.Popen(['gnome-terminal', '--tab', '-e', 'telnet', session['host'], str(session['port'] or 23)])
        else:  # local
            restore_local_tab(session)
        
        import time
        time.sleep(0.5)  # 避免打开太快
    
    print(f"\n✅ All {len(sessions)} sessions restored!")

def restore_batch_script(sessions):
    """生成批量恢复脚本"""
    script = "#!/bin/bash\n# WindTerm Session Batch Restore\n\n"
    
    for session in sessions:
        session_id = session['session_id']
        name = session['session_name'] or session_id[:8]
        conn_type = (session['connection_type'] or 'ssh').lower()
        
        script += f"# Session: {name}\n"
        
        if conn_type == 'ssh':
            host = session['host']
            port = session['port'] or 22
            username = session['username']
            workdir = session['working_directory']
            cmd = session['last_successful_command']
            
            ssh_part = f"ssh {'-l ' + username + ' ' if username else ''}{host} -p {port}"
            if workdir or cmd:
                cmds = []
                if workdir:
                    cmds.append(f"cd {workdir}")
                if cmd:
                    cmds.append(cmd)
                ssh_part += f" '{' && '.join(cmds)}'"
            
            script += f"gnome-terminal --tab -e 'bash -c \"{ssh_part}\"'\n\n"
    
    return script

def list_sessions():
    if not DB_PATH.exists():
        print("No session database found.")
        return
    conn = sqlite3.connect(str(DB_PATH))
    conn.row_factory = sqlite3.Row
    cur = conn.cursor()
    cur.execute("""
        SELECT session_id, session_name, host, port, connection_type, 
               working_directory, last_successful_command, last_active_at, is_active
        FROM sessions ORDER BY last_active_at DESC LIMIT 20
    """)
    sessions = cur.fetchall()
    conn.close()
    
    if not sessions:
        print("No sessions found.")
        return
    
    print("\n📦 Saved Sessions:")
    print("=" * 100)
    print(f"{'#':<3} {'Session ID':<35} {'Host':<20} {'Type':<8} {'Working Dir':<25} {'Last Active':<20}")
    print("=" * 100)
    
    for i, s in enumerate(sessions, 1):
        sid = s['session_id'][:35]
        host = (s['host'] or 'Local')[:18]
        wdir = (s['working_directory'] or '/')[:23]
        active = s['last_active_at'] or ''
        status = "🟢" if s['is_active'] else "⚪"
        print(f"{status} {i:<2} {sid:<35} {host:<20} {s['connection_type'] or 'ssh':<8} {wdir:<25} {active:<20}")
    
    print("=" * 100)

def main():
    parser = argparse.ArgumentParser(description='WindTerm Session Restore Tool')
    parser.add_argument('action', choices=['list', 'restore', 'restore-all', 'restore-tabs', 'show', 'batch'],
                       help='Action to perform')
    parser.add_argument('session_id', nargs='?', help='Session ID to restore')
    parser.add_argument('--mode', choices=['current', 'tab', 'window'], default='tab',
                       help='Restore mode: current terminal, new tab, or new window')
    parser.add_argument('--limit', type=int, default=10, help='Number of sessions to restore')
    
    args = parser.parse_args()
    
    if args.action == 'list':
        list_sessions()
    
    elif args.action == 'restore' and args.session_id:
        session = get_session(args.session_id)
        if not session:
            sys.exit(1)
        
        conn_type = (session['connection_type'] or 'ssh').lower()
        
        if args.mode == 'current':
            if conn_type == 'ssh':
                restore_ssh_current(session)
            else:
                restore_local_current(session)
        else:  # tab or window
            if conn_type == 'ssh':
                restore_ssh_tab(session)
            else:
                restore_local_tab(session)
    
    elif args.action == 'restore-tabs':
        # 恢复最近 N 个会话到独立标签页
        sessions = []
        conn = sqlite3.connect(str(DB_PATH))
        conn.row_factory = sqlite3.Row
        cur = conn.cursor()
        cur.execute("SELECT * FROM sessions WHERE is_active = 1 ORDER BY last_active_at DESC LIMIT ?", (args.limit,))
        sessions = [dict(s) for s in cur.fetchall()]
        conn.close()
        
        if not sessions:
            print("No active sessions to restore.")
            return
        
        restore_all_tabs(sessions)
    
    elif args.action == 'batch':
        # 生成批量恢复脚本
        conn = sqlite3.connect(str(DB_PATH))
        conn.row_factory = sqlite3.Row
        cur = conn.cursor()
        cur.execute("SELECT * FROM sessions WHERE is_active = 1 ORDER BY last_active_at DESC LIMIT ?", (args.limit,))
        sessions = [dict(s) for s in cur.fetchall()]
        conn.close()
        
        script = restore_batch_script(sessions)
        print(script)
    
    elif args.action == 'show' and args.session_id:
        session = get_session(args.session_id)
        if not session:
            sys.exit(1)
        
        print("\n📦 Session Details:")
        print("=" * 60)
        print(f"Session ID:   {session['session_id']}")
        print(f"Name:         {session['session_name'] or 'N/A'}")
        print(f"Host:         {session['host'] or 'Local'}:{session['port'] or 'N/A'}")
        print(f"Type:         {session['connection_type'] or 'ssh'}")
        print(f"Username:     {session['username'] or 'N/A'}")
        print(f"Working Dir:  {session['working_directory'] or 'N/A'}")
        print(f"Last Command: {session['last_successful_command'] or 'None'}")
        print(f"Created:      {session['created_at']}")
        print(f"Last Active:  {session['last_active_at']}")
        
        if session.get('environment'):
            print("\n🔧 Environment Variables:")
            for k, v in session['environment'].items():
                print(f"  {k}={v[:60]}{'...' if len(v) > 60 else ''}")
        
        if session.get('recent_commands'):
            print("\n📜 Recent Commands:")
            for cmd in session['recent_commands'][:5]:
                print(f"  - {cmd}")
        
        print("=" * 60)
    else:
        parser.print_help()
        sys.exit(1)

if __name__ == "__main__":
    main()
