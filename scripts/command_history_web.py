#!/usr/bin/env python3
"""Enhanced Web UI with Auto-Fill Terminal Input"""

from flask import Flask, jsonify, request, render_template_string
import sqlite3
from pathlib import Path

app = Flask(__name__)
DB_PATH = Path.home() / ".WindTerm" / "extensions"
SESSION_DB = DB_PATH / "session_manager.db"
HISTORY_DB = DB_PATH / "command_history.db"

def get_db(db_path):
    if not db_path.exists():
        return None
    conn = sqlite3.connect(str(db_path))
    conn.row_factory = sqlite3.Row
    return conn

@app.route('/api/sessions')
def get_sessions():
    limit = int(request.args.get('limit', 50))
    conn = get_db(SESSION_DB)
    if not conn:
        return jsonify([])
    cur = conn.cursor()
    cur.execute("SELECT * FROM sessions ORDER BY last_active_at DESC LIMIT ?", (limit,))
    sessions = []
    for r in cur.fetchall():
        session = dict(r)
        cur.execute("SELECT var_name, var_value FROM session_environment WHERE session_id = ?", (session['session_id'],))
        session['environment'] = {row['var_name']: row['var_value'] for row in cur.fetchall()}
        cur.execute("SELECT command FROM command_history WHERE session_id = ? ORDER BY timestamp DESC LIMIT 10", (session['session_id'],))
        session['recent_commands'] = [row['command'] for row in cur.fetchall()]
        sessions.append(session)
    conn.close()
    return jsonify(sessions)

@app.route('/api/session/<session_id>')
def get_session(session_id):
    conn = get_db(SESSION_DB)
    if not conn:
        return jsonify({'error': 'Database not found'}), 404
    cur = conn.cursor()
    cur.execute("SELECT * FROM sessions WHERE session_id = ?", (session_id,))
    row = cur.fetchone()
    if not row:
        return jsonify({'error': 'Session not found'}), 404
    session = dict(row)
    cur.execute("SELECT var_name, var_value FROM session_environment WHERE session_id = ?", (session_id,))
    session['environment'] = {row['var_name']: row['var_value'] for row in cur.fetchall()}
    cur.execute("SELECT * FROM command_history WHERE session_id = ? ORDER BY timestamp DESC LIMIT 50", (session_id,))
    session['commands'] = [dict(r) for r in cur.fetchall()]
    conn.close()
    return jsonify(session)

@app.route('/api/fill-command/<session_id>')
def fill_command(session_id):
    """获取会话的最后成功命令，用于填充到终端"""
    conn = get_db(SESSION_DB)
    if not conn:
        return jsonify({'error': 'Database not found'}), 404
    cur = conn.cursor()
    cur.execute("SELECT working_directory, last_successful_command, session_name FROM sessions WHERE session_id = ?", (session_id,))
    row = cur.fetchone()
    if not row:
        return jsonify({'error': 'Session not found'}), 404
    conn.close()
    
    workdir = row[0] or ''
    cmd = row[2] or ''
    
    # 构建填充命令
    fill_text = ""
    if workdir:
        fill_text = f"cd {workdir}"
    if cmd:
        if fill_text:
            fill_text += " && " + cmd
        else:
            fill_text = cmd
    
    return jsonify({
        'session_id': session_id,
        'session_name': row[2] or '',
        'working_directory': workdir,
        'last_successful_command': cmd,
        'fill_text': fill_text,
        'instructions': f"""
## 自动填充到终端（不执行）

### 方式 1: 快捷键
在 WindTerm 终端中按:
- **Ctrl+Shift+R**: 恢复上次会话
- **Ctrl+Shift+L**: 填充最后一次成功命令

### 方式 2: 复制粘贴
```bash
{fill_text or 'echo "No command to restore"'}
```

### 方式 3: 插件调用
如果你开发了 WindTerm 插件，可以调用:
```cpp
plugin->sendTextToInput("{fill_text}");
```

**注意**: 命令会自动填充到终端输入行，但**不会自动执行**，需要用户手动按回车确认。
"""
    })

@app.route('/api/restore/<session_id>')
def restore_session(session_id):
    """获取完整恢复命令（带 python 脚本）"""
    conn = get_db(SESSION_DB)
    if not conn:
        return jsonify({'error': 'Database not found'}), 404
    cur = conn.cursor()
    cur.execute("SELECT * FROM sessions WHERE session_id = ?", (session_id,))
    row = cur.fetchone()
    if not row:
        return jsonify({'error': 'Session not found'}), 404
    session = dict(row)
    conn.close()
    
    restore_cmd = f"python3 session_restore.py restore {session_id}"
    
    return jsonify({
        'session_id': session_id,
        'session_name': session.get('session_name', ''),
        'connection_type': session.get('connection_type', 'ssh'),
        'host': session.get('host', 'localhost'),
        'port': session.get('port', 22),
        'username': session.get('username', ''),
        'working_directory': session.get('working_directory', ''),
        'last_successful_command': session.get('last_successful_command', ''),
        'restore_command': restore_cmd,
        'fill_command_url': f'/api/fill-command/{session_id}'
    })

@app.route('/api/history')
def get_history():
    session_id = request.args.get('session', '')
    limit = int(request.args.get('limit', 200))
    conn = get_db(HISTORY_DB)
    if not conn:
        return jsonify([])
    cur = conn.cursor()
    if session_id:
        cur.execute("SELECT * FROM command_history WHERE session_id = ? ORDER BY timestamp DESC LIMIT ?", (session_id, limit))
    else:
        cur.execute("SELECT * FROM command_history ORDER BY timestamp DESC LIMIT ?", (limit,))
    rows = [dict(r) for r in cur.fetchall()]
    conn.close()
    return jsonify(rows)

@app.route('/api/quick-stats')
def get_stats():
    stats = {'total_sessions': 0, 'active_sessions': 0, 'total_commands': 0}
    conn = get_db(SESSION_DB)
    if conn:
        cur = conn.cursor()
        cur.execute("SELECT COUNT(*) as cnt FROM sessions")
        stats['total_sessions'] = cur.fetchone()['cnt']
        cur.execute("SELECT COUNT(*) as cnt FROM sessions WHERE is_active = 1")
        stats['active_sessions'] = cur.fetchone()['cnt']
        conn.close()
    conn = get_db(HISTORY_DB)
    if conn:
        cur = conn.cursor()
        cur.execute("SELECT COUNT(*) as cnt FROM command_history")
        stats['total_commands'] = cur.fetchone()['cnt']
        conn.close()
    return jsonify(stats)

@app.route('/')
def index():
    return render_template_string('''
<!DOCTYPE html>
<html><head>
<title>WindTerm Session Manager</title>
<meta charset="utf-8">
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:"Segoe UI","Microsoft YaHei",monospace,sans-serif;background:#1e1e1e;color:#d4d4d4;padding:20px;line-height:1.6}
h1,h2,h3{margin-bottom:15px;color:#569cd6}
.container{max-width:1600px;margin:0 auto}
.stats{display:grid;grid-template-columns:repeat(3,1fr);gap:15px;margin-bottom:30px}
.stat-card{background:#2d2d30;padding:20px;border-radius:8px;border-left:4px solid #569cd6}
.stat-value{font-size:32px;font-weight:bold;color:#4ec9b0}
.stat-label{color:#808080;font-size:14px;margin-top:5px}
.tabs{display:flex;margin-bottom:20px;border-bottom:2px solid #3e3e42}
.tab{padding:12px 24px;cursor:pointer;border:none;background:none;color:#d4d4d4;font-size:14px;font-weight:500;transition:all 0.2s}
.tab.active{border-bottom:2px solid #569cd6;color:#569cd6}
.tab:hover{color:#569cd6;background:#252526}
.panel{display:none}
.panel.active{display:block}
table{width:100%;border-collapse:collapse;background:#252526;margin-bottom:20px;border-radius:4px;overflow:hidden}
td,th{border:1px solid #3e3e42;padding:12px;text-align:left}
th{background:#2d2d30;color:#569cd6;font-weight:600;text-transform:uppercase;font-size:12px}
tr:nth-child(even){background:#2a2a2b}
tr:hover{background:#37373d}
.badge{display:inline-block;padding:3px 10px;border-radius:4px;font-size:11px;font-weight:600;margin-right:5px}
.badge-ssh{background:#0e639c;color:white}
.badge-telnet{background:#ce9178;color:#1e1e1e}
.badge-serial{background:#b5cea8;color:#1e1e1e}
.badge-local{background:#4ec9b0;color:#1e1e1e}
.btn{padding:8px 16px;border:none;border-radius:4px;cursor:pointer;font-size:13px;font-weight:500;transition:all 0.2s;text-decoration:none;display:inline-block}
.btn-fill{background:#4ec9b0;color:#1e1e1e}
.btn-fill:hover{background:#5dd4b9;transform:translateY(-1px)}
.btn-restore{background:#ce9178;color:#1e1e1e}
.btn-restore:hover{background:#d49d85}
.btn-details{background:#0e639c;color:white}
.btn-details:hover{background:#1177bb}
.btn-copy{background:#569cd6;color:white;padding:4px 10px;font-size:11px}
.btn-copy:hover{background:#6a9cd6}
.input-group{margin-bottom:20px;display:flex;gap:10px;align-items:center}
input[type="text"]{flex:1;padding:12px;background:#3c3c3c;border:1px solid #3e3e42;color:#d4d4d4;border-radius:4px;font-size:14px}
input[type="text"]:focus{outline:none;border-color:#569cd6;box-shadow:0 0 0 2px rgba(86,156,214,0.2)}
.modal{display:none;position:fixed;top:0;left:0;width:100%;height:100%;background:rgba(0,0,0,0.7);z-index:1000;overflow-y:auto}
.modal.active{display:block}
.modal-content{background:#252526;margin:5% auto;padding:30px;border-radius:8px;max-width:900px;border:1px solid #3e3e42}
.modal-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:20px;border-bottom:1px solid #3e3e42;padding-bottom:15px}
.modal-close{background:none;border:none;color:#808080;font-size:28px;cursor:pointer}
.modal-close:hover{color:#d4d4d4}
.env-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(250px,1fr));gap:10px;margin:15px 0}
.env-item{background:#2d2d30;padding:12px;border-radius:4px;border-left:3px solid #569cd6}
.env-name{color:#9cdcfe;font-weight:600;font-size:13px;margin-bottom:5px}
.env-value{color:#ce9178;font-size:12px;word-break:break-all;max-height:60px;overflow-y:auto}
.command-list{max-height:400px;overflow-y:auto;background:#1e1e1e;border-radius:4px;padding:15px}
.command-item{padding:10px;border-bottom:1px solid #3e3e42;font-family:monospace;font-size:13px}
.command-item:last-child{border-bottom:none}
.session-info{background:#2d2d30;padding:20px;border-radius:4px;margin-bottom:20px}
.info-row{display:flex;margin-bottom:10px}
.info-label{color:#808080;width:150px;font-size:13px}
.info-value{color:#d4d4d4;font-size:13px}
.fill-box{background:#1e3a1e;border:1px solid #4ec9b0;border-radius:4px;padding:20px;margin-top:20px}
.fill-box h3{color:#4ec9b0;margin-bottom:15px}
.code-block{background:#1e1e1e;padding:15px;border-radius:4px;overflow-x:auto;font-family:Consolas,monospace;font-size:13px;color:#d4d4d4;margin:10px 0}
.code-block code{color:#ce9178}
.action-buttons{display:flex;gap:10px;margin-top:15px;flex-wrap:wrap}
.notice{background:#264f26;border-left:4px solid #4ec9b0;padding:15px;border-radius:4px;margin:15px 0}
.notice-title{color:#4ec9b0;font-weight:600;margin-bottom:10px}
.notice p{color:#808080;font-size:13px}
.shortcut{display:inline-block;background:#3e3e42;padding:4px 10px;border-radius:4px;font-family:monospace;font-size:12px;margin:0 5px}
.refresh-btn{position:fixed;bottom:20px;right:20px;width:50px;height:50px;border-radius:50%;background:#0e639c;color:white;border:none;cursor:pointer;font-size:20px;box-shadow:0 2px 10px rgba(0,0,0,0.3)}
.refresh-btn:hover{background:#1177bb;transform:scale(1.1)}
</style>
</head>
<body><div class="container">
<h1>📦 WindTerm Session Manager</h1>

<div class="stats">
    <div class="stat-card"><div class="stat-value" id="stat-total">-</div><div class="stat-label">Total Sessions</div></div>
    <div class="stat-card"><div class="stat-value" id="stat-active">-</div><div class="stat-label">Active Sessions</div></div>
    <div class="stat-card"><div class="stat-value" id="stat-commands">-</div><div class="stat-label">Commands Recorded</div></div>
</div>

<div class="tabs">
    <div class="tab active" onclick="showTab('sessions')">📋 Sessions</div>
    <div class="tab" onclick="showTab('history')">📜 Command History</div>
</div>

<div id="sessions" class="panel active">
    <div class="input-group">
        <input type="text" id="filter-session" placeholder="Filter by host or name...">
        <button class="btn btn-details" onclick="loadSessions()">Search</button>
    </div>
    <table>
        <thead><tr><th>Status</th><th>Session</th><th>Host</th><th>Type</th><th>Working Directory</th><th>Last Command</th><th>Last Active</th><th>Actions</th></tr></thead>
        <tbody id="sessions-data"></tbody>
    </table>
</div>

<div id="history" class="panel">
    <div class="input-group">
        <input type="text" id="filter-dir" placeholder="Filter by working directory...">
        <button class="btn btn-details" onclick="loadHistory()">Search</button>
    </div>
    <table>
        <thead><tr><th>Time</th><th>Session</th><th>Directory</th><th>Command</th></tr></thead>
        <tbody id="history-data"></tbody>
    </table>
</div>

<div id="session-modal" class="modal">
    <div class="modal-content">
        <div class="modal-header">
            <h2 id="modal-title">Session Details</h2>
            <button class="modal-close" onclick="closeModal()">&times;</button>
        </div>
        <div id="modal-body"></div>
    </div>
</div>

<button class="refresh-btn" onclick="loadStats()" title="Refresh">⟳</button>

<script>
let sessionsCache = [];

function showTab(name) {
    document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
    document.querySelectorAll('.panel').forEach(p => p.classList.remove('active'));
    document.querySelector(`.tab[onclick="showTab('${name}')"]`).classList.add('active');
    document.getElementById(name).classList.add('active');
    if (name === 'sessions') loadSessions();
    else loadHistory();
}

async function loadStats() {
    const res = await fetch('/api/quick-stats');
    const stats = await res.json();
    document.getElementById('stat-total').textContent = stats.total_sessions || 0;
    document.getElementById('stat-active').textContent = stats.active_sessions || 0;
    document.getElementById('stat-commands').textContent = stats.total_commands || 0;
}

async function loadSessions() {
    const res = await fetch('/api/sessions?limit=50');
    sessionsCache = await res.json();
    const filter = document.getElementById('filter-session').value.toLowerCase();
    const filtered = filter ? sessionsCache.filter(s => (s.host||'').toLowerCase().includes(filter) || (s.session_name||'').toLowerCase().includes(filter)) : sessionsCache;
    
    document.getElementById('sessions-data').innerHTML = filtered.map(s => {
        const statusBadge = s.is_active ? '🟢' : '⚪';
        const typeClass = (s.connection_type||'ssh').toLowerCase();
        return `
        <tr class="${s.last_successful_command ? 'success' : ''}">
            <td>${statusBadge}</td>
            <td><div style="font-weight:600">${s.session_name||'Unnamed'}</div><div style="font-size:11px;color:#808080">${s.session_id.substring(0,8)}...</div></td>
            <td>${s.host||'Local'}:${s.port||'-'}</td>
            <td><span class="badge badge-${typeClass}">${(s.connection_type||'ssh').toUpperCase()}</span></td>
            <td class="working-dir">${s.working_directory||'N/A'}</td>
            <td style="max-width:250px;overflow:hidden;text-overflow:ellipsis" title="${s.last_command||''}">${s.last_command||'<span style="color:#808080">None</span>'}</td>
            <td style="font-size:12px;color:#808080">${s.last_active_at||'-'}</td>
            <td>
                <button class="btn btn-fill" onclick="fillCommand('${s.session_id}')" title="Fill to terminal input (not execute)">📝 Fill</button>
                <button class="btn btn-restore" onclick="restoreSession('${s.session_id}')" style="margin-left:5px">🚀 Restore</button>
                <button class="btn btn-details" onclick="showSessionDetails('${s.session_id}')" style="margin-left:5px">Details</button>
            </td>
        </tr>`;
    }).join('') || '<tr><td colspan="8" style="text-align:center;color:#808080;padding:40px">No sessions found</td></tr>';
}

async function loadHistory() {
    const dir = document.getElementById('filter-dir').value;
    const url = '/api/history?limit=200' + (dir ? '&dir='+encodeURIComponent(dir) : '');
    const res = await fetch(url);
    const data = await res.json();
    document.getElementById('history-data').innerHTML = data.map(r => `
        <tr>
            <td class="timestamp">${r.timestamp||'-'}</td>
            <td><span class="badge badge-ssh">${(r.session_id||'N/A').substring(0,8)}...</span></td>
            <td class="working-dir">${r.working_directory||'N/A'}</td>
            <td style="font-family:monospace;max-width:600px;word-break:break-all">${r.command||'-'}</td>
        </tr>
    `).join('') || '<tr><td colspan="4" style="text-align:center;color:#808080;padding:40px">No commands found</td></tr>';
}

async function fillCommand(sessionId) {
    const res = await fetch(`/api/fill-command/${sessionId}`);
    const data = await res.json();
    
    if (data.error) { alert('Error: ' + data.error); return; }
    
    const html = `
<div class="notice">
    <div class="notice-title">⚡ Auto-Fill to Terminal (Not Execute)</div>
    <p>The command will be automatically filled to WindTerm terminal input line, but <strong>NOT executed</strong>. You need to press <span class="shortcut">Enter</span> manually to run it.</p>
</div>

<div class="session-info">
    <div class="info-row"><span class="info-label">Session:</span><span class="info-value">${data.session_name||sessionId}</span></div>
    <div class="info-row"><span class="info-label">Working Dir:</span><span class="info-value">${data.working_directory||'N/A'}</span></div>
</div>

<div class="fill-box">
    <h3>📝 Command to Fill</h3>
    <div class="code-block"><code>${data.fill_text||'No command to restore'}</code></div>
    <div class="action-buttons">
        <button class="btn btn-copy" onclick="navigator.clipboard.writeText('${data.fill_text||''}')">📋 Copy</button>
    </div>
</div>

<div class="notice">
    <div class="notice-title">⌨️ Keyboard Shortcuts in WindTerm</div>
    <p>
        <span class="shortcut">Ctrl+Shift+R</span> Restore last session<br>
        <span class="shortcut">Ctrl+Shift+L</span> Fill last successful command
    </p>
</div>

<div style="margin-top:20px">
    <h3>📋 Manual Commands</h3>
    <div class="code-block"><strong>Change Directory:</strong><br><code>cd ${data.working_directory||'/'}</code></div>
    <div class="code-block"><strong>Last Successful Command:</strong><br><code>${data.last_successful_command||'echo "No command saved"'}</code></div>
</div>
`;
    
    showModal('Fill Command: ' + (data.session_name||sessionId), html);
}

async function restoreSession(sessionId) {
    const res = await fetch(`/api/restore/${sessionId}`);
    const data = await res.json();
    if (data.error) { alert('Error: ' + data.error); return; }
    
    const html = `
<div class="session-info">
    <div class="info-row"><span class="info-label">Session:</span><span class="info-value">${data.session_name||sessionId}</span></div>
    <div class="info-row"><span class="info-label">Host:</span><span class="info-value">${data.host||'Local'}:${data.port||'-'}</span></div>
    <div class="info-row"><span class="info-label">Type:</span><span class="info-value">${(data.connection_type||'ssh').toUpperCase()}</span></div>
    <div class="info-row"><span class="info-label">Working Dir:</span><span class="info-value">${data.working_directory||'N/A'}</span></div>
</div>

<div class="fill-box">
    <h3>🚀 Full Restore (Auto-Execute)</h3>
    <p style="color:#808080;margin-bottom:15px">Run this command to auto-connect and execute:</p>
    <div class="code-block"><code>${data.restore_command}</code></div>
    <div class="action-buttons">
        <button class="btn btn-copy" onclick="navigator.clipboard.writeText('${data.restore_command}')">📋 Copy Command</button>
        <button class="btn btn-fill" onclick="fillCommand('${sessionId}')">📝 Just Fill (Don't Execute)</button>
    </div>
</div>
`;
    showModal('Restore Session: ' + (data.session_name||sessionId), html);
}

async function showSessionDetails(sessionId) {
    const res = await fetch(`/api/session/${sessionId}`);
    const s = await res.json();
    if (!s.session_id) { alert('Session not found'); return; }
    
    const envHtml = Object.entries(s.environment||{}).map(([k,v]) => 
        `<div class="env-item"><div class="env-name">${k}</div><div class="env-value">${v}</div></div>`
    ).join('') || '<p style="color:#808080">No environment variables saved</p>';
    
    const cmdHtml = (s.commands||[]).slice(0,30).map(c => 
        `<div class="command-item"><span class="timestamp">${c.timestamp||'-'}</span><span class="working-dir">${c.working_directory||'N/A'}</span><div style="font-family:monospace;margin-top:5px">${c.command||'-'}</div></div>`
    ).join('') || '<p style="color:#808080">No commands recorded</p>';
    
    const html = `
<div class="session-info">
    <div class="info-row"><span class="info-label">Session ID:</span><span class="info-value">${s.session_id}</span></div>
    <div class="info-row"><span class="info-label">Name:</span><span class="info-value">${s.session_name||'Unnamed'}</span></div>
    <div class="info-row"><span class="info-label">Host:</span><span class="info-value">${s.host||'Local'}:${s.port||'-'}</span></div>
    <div class="info-row"><span class="info-label">Type:</span><span class="info-value">${(s.connection_type||'ssh').toUpperCase()}</span></div>
    <div class="info-row"><span class="info-label">Working Directory:</span><span class="info-value">${s.working_directory||'N/A'}</span></div>
    <div class="info-row"><span class="info-label">Last Successful:</span><span class="info-value" style="color:#4ec9b0">${s.last_successful_command||'None'}</span></div>
</div>

<h3>🔧 Environment Variables</h3>
<div class="env-grid">${envHtml}</div>

<h3 style="margin-top:20px">📜 Recent Commands</h3>
<div class="command-list">${cmdHtml}</div>
`;
    showModal('Session Details: ' + (s.session_name||sessionId), html);
}

function showModal(title, content) {
    document.getElementById('modal-title').textContent = title;
    document.getElementById('modal-body').innerHTML = content;
    document.getElementById('session-modal').classList.add('active');
}

function closeModal() {
    document.getElementById('session-modal').classList.remove('active');
}

document.getElementById('session-modal').addEventListener('click', function(e) {
    if (e.target === this) closeModal();
});

document.addEventListener('keydown', function(e) {
    if (e.key === 'Escape') closeModal();
});

loadStats();
loadSessions();
setInterval(loadStats, 30000);
</script>
</body></html>
''')

if __name__ == "__main__":
    DB_PATH.mkdir(parents=True, exist_ok=True)
    print("Starting WindTerm Session Manager Web UI...")
    print("Access: http://localhost:8767")
    app.run(host="127.0.0.1", port=8767, debug=False)
