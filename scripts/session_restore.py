#!/usr/bin/env python3
"""一键会话恢复工具 - 自动连接并恢复终端状态"""

import sys
import subprocess
import sqlite3
import os
from pathlib import Path
from datetime import datetime

DB_PATH = Path.home() / ".WindTerm" / "extensions" / "session_manager.db"

def get_session(session_id):
    """获取会话信息"""
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

def restore_ssh(session):
    """恢复 SSH 会话"""
    host = session['host']
    port = session['port'] or 22
    username = session['username']
    workdir = session['working_directory']
    cmd = session['last_successful_command']
    
    print(f"\n🔌 Connecting to SSH: {username}@{host}:{port}")
    
    ssh_cmd = ["ssh", "-p", str(port)]
    if username:
        ssh_cmd = ["ssh", "-l", username, "-p", str(port)]
    
    commands = []
    if workdir:
        commands.append(f"cd {workdir}")
        print(f"📁 Working directory: {workdir}")
    
    for key, value in session.get('environment', {}).items():
        if key in ['PATH', 'LD_LIBRARY_PATH', 'PYTHONPATH']:
            commands.append(f"export {key}={value}")
            print(f"🔧 Environment: {key}={value[:50]}...")
    
    if cmd:
        commands.append(cmd)
        print(f"⚡ Last command: {cmd}")
    
    if commands:
        script = " && ".join(commands)
        ssh_cmd.extend([host, script])
    else:
        ssh_cmd.append(host)
    
    print(f"\nExecuting: {' '.join(ssh_cmd)}\n")
    subprocess.run(ssh_cmd)

def restore_local(session):
    """恢复本地会话"""
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

def restore_telnet(session):
    """恢复 Telnet 会话"""
    host = session['host']
    port = session['port'] or 23
    
    print(f"\n🔌 Connecting to Telnet: {host}:{port}")
    subprocess.run(["telnet", host, str(port)])

def list_sessions():
    """列出所有会话"""
    if not DB_PATH.exists():
        print("No session database found.")
        return
    
    conn = sqlite3.connect(str(DB_PATH))
    conn.row_factory = sqlite3.Row
    cur = conn.cursor()
    
    cur.execute("""
        SELECT session_id, session_name, host, port, connection_type, 
               working_directory, last_successful_command, last_active_at, is_active
        FROM sessions 
        ORDER BY last_active_at DESC 
        LIMIT 20
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
    print(f"\nUsage: python3 {sys.argv[0]} restore <session_id>")
    print(f"       python3 {sys.argv[0]} list")

def main():
    if len(sys.argv) < 2:
        print("Usage:")
        print(f"  python3 {sys.argv[0]} list                  - List all sessions")
        print(f"  python3 {sys.argv[0]} restore <session_id>  - Restore a session")
        print(f"  python3 {sys.argv[0]} show <session_id>     - Show session details")
        sys.exit(1)
    
    action = sys.argv[1]
    
    if action == "list":
        list_sessions()
    
    elif action == "restore" and len(sys.argv) > 2:
        session = get_session(sys.argv[2])
        if not session:
            sys.exit(1)
        
        conn_type = (session['connection_type'] or 'ssh').lower()
        
        if conn_type == 'ssh':
            restore_ssh(session)
        elif conn_type == 'telnet':
            restore_telnet(session)
        elif conn_type == 'local':
            restore_local(session)
        else:
            print(f"Unknown connection type: {conn_type}")
            restore_ssh(session)
    
    elif action == "show" and len(sys.argv) > 2:
        session = get_session(sys.argv[2])
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
        print(f"Unknown action: {action}")
        sys.exit(1)

if __name__ == "__main__":
    main()
