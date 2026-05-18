#!/usr/bin/env python3
"""Command History & Session Manager Web UI"""

from flask import Flask, jsonify, request, render_template_string
import sqlite3, os
from pathlib import Path
from datetime import datetime

app = Flask(__name__)
DB_PATH = Path.home() / ".WindTerm" / "extensions"
HISTORY_DB = DB_PATH / "command_history.db"
SESSION_DB = DB_PATH / "session_manager.db"

def get_db(db_path):
    if not db_path.exists(): return None
    conn = sqlite3.connect(str(db_path))
    conn.row_factory = sqlite3.Row
    return conn

@app.route('/api/history')
def get_history():
    working_dir = request.args.get('dir', '')
    session_id = request.args.get('session', '')
    limit = int(request.args.get('limit', 100))
    conn = get_db(HISTORY_DB)
    if not conn: return jsonify([])
    cur = conn.cursor()
    if working_dir:
        cur.execute("SELECT * FROM command_history WHERE working_directory LIKE ? ORDER BY timestamp DESC LIMIT ?", (f'%{working_dir}%', limit))
    elif session_id:
        cur.execute("SELECT * FROM command_history WHERE session_id = ? ORDER BY timestamp DESC LIMIT ?", (session_id, limit))
    else:
        cur.execute("SELECT * FROM command_history ORDER BY timestamp DESC LIMIT ?", (limit,))
    rows = [dict(r) for r in cur.fetchall()]
    conn.close()
    return jsonify(rows)

@app.route('/api/sessions')
def get_sessions():
    limit = int(request.args.get('limit', 50))
    conn = get_db(SESSION_DB)
    if not conn: return jsonify([])
    cur = conn.cursor()
    cur.execute("SELECT * FROM sessions ORDER BY last_active_at DESC LIMIT ?", (limit,))
    sessions = []
    for r in cur.fetchall():
        session = dict(r)
        # Get environment variables
        cur.execute("SELECT var_name, var_value FROM session_environment WHERE session_id = ?", (session['session_id'],))
        session['environment'] = {row['var_name']: row['var_value'] for row in cur.fetchall()}
        sessions.append(session)
    conn.close()
    return jsonify(sessions)

@app.route('/api/session/<session_id>')
def get_session(session_id):
    conn = get_db(SESSION_DB)
    if not conn: return jsonify({})
    cur = conn.cursor()
    cur.execute("SELECT * FROM sessions WHERE session_id = ?", (session_id,))
    row = cur.fetchone()
    if not row: return jsonify({})
    session = dict(row)
    cur.execute("SELECT var_name, var_value FROM session_environment WHERE session_id = ?", (session_id,))
    session['environment'] = {row['var_name']: row['var_value'] for row in cur.fetchall()}
    cur.execute("SELECT * FROM command_history WHERE session_id = ? ORDER BY timestamp DESC LIMIT 50", (session_id,))
    session['commands'] = [dict(r) for r in cur.fetchall()]
    conn.close()
    return jsonify(session)

@app.route('/api/restore/<session_id>')
def restore_session(session_id):
    conn = get_db(SESSION_DB)
    if not conn: return jsonify({'error': 'Session not found'})
    cur = conn.cursor()
    cur.execute("SELECT working_directory, last_successful_command FROM sessions WHERE session_id = ?", (session_id,))
    row = cur.fetchone()
    conn.close()
    if not row: return jsonify({'error': 'Session not found'})
    return jsonify({
        'working_directory': row[0],
        'last_command': row[1],
        'session_id': session_id
    })

@app.route('/')
def index():
    return render_template_string('''
<!DOCTYPE html>
<html><head>
<title>WindTerm Session Manager</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:"Segoe UI",monospace,sans-serif;background:#1e1e1e;color:#d4d4d4;padding:20px}
h1,h2{margin-bottom:15px;color:#569cd6}
.container{max-width:1400px;margin:0 auto}
.tabs{display:flex;margin-bottom:20px;border-bottom:2px solid #3e3e42}
.tab{padding:10px 20px;cursor:pointer;border:none;background:none;color:#d4d4d4;font-size:14px}
.tab.active{border-bottom:2px solid #569cd6;color:#569cd6}
.tab:hover{color:#569cd6}
.panel{display:none}
.panel.active{display:block}
table{width:100%;border-collapse:collapse;background:#252526;margin-bottom:20px}
td,th{border:1px solid #3e3e42;padding:10px;text-align:left}
th{background:#2d2d30;color:#569cd6;font-weight:600}
tr:nth-child(even){background:#2a2a2b}
tr:hover{background:#37373d}
.input-group{margin-bottom:15px;display:flex;gap:10px}
input[type="text"]{flex:1;padding:10px;background:#3c3c3c;border:1px solid #3e3e42;color:#d4d4d4;border-radius:4px;font-size:14px}
input[type="text"]:focus{outline:none;border-color:#569cd6}
button{padding:10px 20px;background:#0e639c;color:white;border:none;border-radius:4px;cursor:pointer;font-size:14px}
button:hover{background:#1177bb}
.badge{display:inline-block;padding:2px 8px;border-radius:3px;font-size:12px;margin-right:5px}
.badge-ssh{background:#0e639c}
.badge-telnet{background:#ce9178}
.badge-serial{background:#b5cea8}
.env-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(300px,1fr));gap:10px;margin-top:10px}
.env-item{background:#2d2d30;padding:10px;border-radius:4px;border-left:3px solid #569cd6}
.env-name{color:#9cdcfe;font-weight:600}
.env-value{color:#ce9178;margin-top:5px;word-break:break-all}
.session-details{background:#252526;padding:15px;border-radius:4px;margin-top:10px}
.command-list{max-height:300px;overflow-y:auto}
.timestamp{color:#6a9955;font-size:12px}
.dir{color:#4ec9b0;font-size:13px}
.restore-btn{padding:5px 10px;font-size:12px;background:#4ec9b0}
.restore-btn:hover{background:#5dd4b9}
</style>
</head>
<body><div class="container">
<h1>📦 WindTerm Session Manager</h1>
<div class="tabs">
    <div class="tab active" onclick="showTab('sessions')">Sessions</div>
    <div class="tab" onclick="showTab('history')">Command History</div>
</div>
<div id="sessions" class="panel active">
    <h2>Active Sessions</h2>
    <table><thead><tr><th>Session ID</th><th>Host</th><th>Protocol</th><th>Working Directory</th><th>Last Command</th><th>Last Active</th><th>Actions</th></tr></thead>
    <tbody id="sessions-data"></tbody></table>
</div>
<div id="history" class="panel">
    <h2>Command History</h2>
    <div class="input-group">
        <input type="text" id="filter-dir" placeholder="Filter by working directory...">
        <button onclick="loadHistory()">Filter</button>
    </div>
    <table><thead><tr><th>Time</th><th>Directory</th><th>Command</th><th>Session</th></tr></thead>
    <tbody id="history-data"></tbody></table>
</div>
</div>
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
async function loadSessions() {
    const res = await fetch('/api/sessions?limit=50');
    sessionsCache = await res.json();
    document.getElementById('sessions-data').innerHTML = sessionsCache.map(s => `
        <tr>
            <td><code style="font-size:11px">${s.session_id.substring(0,8)}...</code></td>
            <td>${s.host || 'Local'}</td>
            <td><span class="badge badge-${(s.protocol||'ssh').toLowerCase()}">${s.protocol||'SSH'}</span></td>
            <td class="dir">${s.working_directory || 'N/A'}</td>
            <td style="max-width:300px;overflow:hidden;text-overflow:ellipsis">${s.last_command || 'None'}</td>
            <td class="timestamp">${s.last_active_at}</td>
            <td><button class="restore-btn" onclick="showSession('${s.session_id}')">View Details</button></td>
        </tr>
    `).join('');
}
async function showSession(sessionId) {
    const res = await fetch(`/api/session/${sessionId}`);
    const s = await res.json();
    if (!s.session_id) return alert('Session not found');
    const envHtml = Object.entries(s.environment||{}).map(([k,v]) => 
        `<div class="env-item"><div class="env-name">${k}</div><div class="env-value">${v}</div></div>`
    ).join('');
    const cmdHtml = (s.commands||[]).slice(0,20).map(c => 
        `<tr><td class="timestamp">${c.timestamp}</td><td class="dir">${c.working_directory||'N/A'}</td><td>${c.command}</td></tr>`
    ).join('');
    const html = `
        <div class="session-details">
            <h3>Session: ${s.session_id.substring(0,8)}...</h3>
            <p><strong>Host:</strong> ${s.host||'Local'} | <strong>Protocol:</strong> ${s.protocol||'SSH'} | <strong>Port:</strong> ${s.port||22}</p>
            <p><strong>Working Directory:</strong> ${s.working_directory||'N/A'}</p>
            <p><strong>Last Command:</strong> ${s.last_command||'None'}</p>
            <p><strong>Last Successful Command:</strong> <code style="background:#2d2d30;padding:2px 6px">${s.last_successful_command||'N/A'}</code></p>
            <p><strong>Created:</strong> ${s.created_at} | <strong>Last Active:</strong> ${s.last_active_at}</p>
            <h4 style="margin:15px 0 10px">Environment Variables</h4>
            <div class="env-grid">${envHtml || '<p>No environment variables saved</p>'}</div>
            <h4 style="margin:15px 0 10px">Recent Commands</h4>
            <table class="command-list"><thead><tr><th>Time</th><th>Directory</th><th>Command</th></tr></thead><tbody>${cmdHtml || '<tr><td colspan="3">No commands recorded</td></tr>'}</tbody></table>
        </div>
    `;
    document.getElementById('sessions').insertAdjacentHTML('afterbegin', html);
}
async function loadHistory() {
    const dir = document.getElementById('filter-dir').value;
    const res = await fetch(`/api/history?dir=${encodeURIComponent(dir)}&limit=200`);
    const data = await res.json();
    document.getElementById('history-data').innerHTML = data.map(r => `
        <tr>
            <td class="timestamp">${r.timestamp}</td>
            <td class="dir">${r.working_directory||'N/A'}</td>
            <td style="max-width:600px">${r.command}</td>
            <td><code style="font-size:11px">${r.session_id ? r.session_id.substring(0,8)+'...' : 'N/A'}</code></td>
        </tr>
    `).join('');
}
loadSessions();
</script></body></html>
''')

if __name__ == "__main__":
    DB_PATH.mkdir(parents=True, exist_ok=True)
    app.run(host="127.0.0.1", port=8767, debug=False)
