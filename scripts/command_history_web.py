#!/usr/bin/env python3
from flask import Flask, jsonify, request, render_template_string
import sqlite3, os
from pathlib import Path

app = Flask(__name__)
DB_PATH = Path.home() / ".WindTerm" / "extensions" / "command_history.db"

def get_db():
    conn = sqlite3.connect(str(DB_PATH)); conn.row_factory = sqlite3.Row
    return conn

@app.route('/api/history')
def get_history():
    working_dir = request.args.get('dir', '')
    limit = int(request.args.get('limit', 100))
    conn = get_db()
    cur = conn.cursor()
    if working_dir: cur.execute("SELECT * FROM command_history WHERE working_directory=? ORDER BY timestamp DESC LIMIT ?", (working_dir, limit))
    else: cur.execute("SELECT * FROM command_history ORDER BY timestamp DESC LIMIT ?", (limit,))
    rows = [dict(r) for r in cur.fetchall()]
    conn.close()
    return jsonify(rows)

@app.route('/')
def index():
    return render_template_string('''
<!DOCTYPE html><html><head><title>Command History</title>
<style>body{font-family:monospace;margin:20px}table{width:100%;border-collapse:collapse}td,th{border:1px solid #ddd;padding:8px}</style>
</head><body><h1>WindTerm Command History</h1>
<input type="text" id="dir" placeholder="Working directory" style="width:400px;padding:8px">
<button onclick="load()" style="padding:8px 16px">Query</button>
<table><thead><tr><th>Time</th><th>Directory</th><th>Command</th><th>Host</th></tr></thead><tbody id="data"></tbody></table>
<script>
function load() {
    const dir = document.getElementById('dir').value;
    fetch('/api/history?dir='+encodeURIComponent(dir)).then(r=>r.json()).then(data=>{
        document.getElementById('data').innerHTML = data.map(r=>'<tr><td>'+r.timestamp+'</td><td>'+r.working_directory+'</td><td>'+r.command+'</td><td>'+r.hostname+'</td></tr>').join('');
    });
}
load();
</script></body></html>
''')

if __name__ == "__main__":
    Path(DB_PATH).parent.mkdir(parents=True, exist_ok=True)
    app.run(host="127.0.0.1", port=8767)
