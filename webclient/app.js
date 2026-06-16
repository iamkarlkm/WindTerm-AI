/* WindTerm AI - Web Terminal Client (Production) */
/* P0: 安全加固 + 断线重连 + 心跳 + Token 管理 + 错误诊断 */
'use strict';

// =========================== 安全工具 ===========================
const ALLOWED_TAGS = ['b', 'i', 'u', 's', 'pre', 'code', 'a', 'ul', 'ol', 'li',
  'span', 'br', 'strong', 'em', 'h1', 'h2', 'h3', 'p', 'img', 'blockquote', 'table', 'tr', 'td', 'th'];
const ALLOWED_ATTR = ['href', 'src', 'alt', 'style', 'class', 'title', 'target'];

function sanitizeHtml(dirty) {
  if (typeof DOMPurify === 'undefined') {
    const div = document.createElement('div'); div.textContent = dirty; return div.innerHTML;
  }
  return DOMPurify.sanitize(dirty, { ALLOWED_TAGS, ALLOWED_ATTR });
}

function sanitizeSnippetContent(dirty) {
  if (typeof DOMPurify === 'undefined') {
    const div = document.createElement('div'); div.textContent = dirty; return div.innerHTML;
  }
  return DOMPurify.sanitize(dirty, {
    ALLOWED_TAGS: [...ALLOWED_TAGS, 'font'],
    ALLOWED_ATTR: [...ALLOWED_ATTR, 'color'],
    ALLOW_DATA_ATTR: false
  });
}

// =========================== Token 管理 ===========================
function storeToken(token) {
  sessionStorage.setItem('windterm_token', btoa(String(token)));
}
function getToken() {
  try { return atob(sessionStorage.getItem('windterm_token') || ''); } catch(e) { return ''; }
}
function clearToken() {
  sessionStorage.removeItem('windterm_token');
}

// =========================== 连接配置持久化 ===========================
function loadConnections() {
  try {
    const raw = localStorage.getItem('windterm_connections');
    return raw ? JSON.parse(raw) : [];
  } catch(e) { return []; }
}
function saveConnections(list) {
  localStorage.setItem('windterm_connections', JSON.stringify(list.slice(0, 50)));
}
function addConnection(host, port, username, token) {
  const list = loadConnections();
  const idx = list.findIndex(c => c.host === host && c.port === port && c.username === username);
  if (idx >= 0) { list.splice(idx, 1); }
  list.unshift({ host, port, username, token, time: Date.now() });
  saveConnections(list);
}

// =========================== 状态管理 ===========================
const state = {
  currentTab: null,
  tabs: new Map(),
  snippets: [],
  currentSnippetId: null,
  token: getToken(),
  autosaveTimer: null
};

// =========================== 常量 ===========================
const RECONNECT_MAX_RETRIES = 10;
const RECONNECT_BASE_DELAY = 1000;
const RECONNECT_MAX_DELAY = 30000;
const HEARTBEAT_INTERVAL = 30000;
const HEARTBEAT_TIMEOUT = 10000;

// =========================== 连接错误诊断 ===========================
function diagnoseError(event, tabData) {
  const host = tabData?.host || '';
  const port = tabData?.port || '';

  if (!navigator.onLine) {
    return { level: 'error', title: '网络断开', detail: '设备未连接到网络，请检查网络设置。' };
  }

  if (event && event.code === 1006) {
    return { level: 'error', title: '连接异常关闭', detail: `无法连接到 ${host}:${port}。请检查:</br>1. 服务器是否运行</br>2. 防火墙是否放行端口 ${port}</br>3. 主机地址是否正确` };
  }

  if (event && event.code === 1001) {
    return { level: 'warn', title: '服务端关闭连接', detail: '服务器主动关闭了连接，可能是会话超时。' };
  }

  const url = tabData?.ws?.url || '';
  if (url.startsWith('ws://') && location.protocol === 'https:') {
    return { level: 'error', title: '协议不匹配', detail: `当前页面使用 HTTPS，但 WebSocket 目标为 ${url}。浏览器会阻止不安全连接。请使用 wss:// 协议。` };
  }

  return { level: 'error', title: '连接失败', detail: `无法建立 WebSocket 连接到 ${host}:${port}。<br/>请检查主机地址和端口是否正确。` };
}

function showConnectionError(error) {
  const container = document.getElementById('terminalContainer');
  const existing = document.getElementById('errorOverlay');
  if (existing) existing.remove();

  const overlay = document.createElement('div');
  overlay.id = 'errorOverlay';
  overlay.style.cssText = 'position:absolute;inset:0;display:flex;align-items:center;justify-content:center;background:rgba(0,0,0,0.7);z-index:100;';
  overlay.innerHTML = `
    <div style="background:var(--bg-alt);border:1px solid var(--red);border-radius:12px;padding:24px 32px;max-width:420px;text-align:center;">
      <div style="font-size:32px;margin-bottom:12px;">${error.level==='error'?'&#x26D4;':'&#x26A0;&#xFE0F;'}</div>
      <div style="font-size:16px;font-weight:600;color:var(--red);margin-bottom:8px;">${escapeText(error.title)}</div>
      <div style="font-size:13px;color:var(--subtext);line-height:1.6;margin-bottom:16px;">${error.detail}</div>
      <button onclick="this.parentElement.parentElement.remove()" style="padding:6px 20px;border:1px solid var(--border);border-radius:6px;background:var(--bg);color:var(--text);cursor:pointer;">关闭</button>
    </div>`;
  document.getElementById('terminalContainer').appendChild(overlay);
}

function escapeText(text) {
  const div = document.createElement('div'); div.textContent = text; return div.innerHTML;
}

// =========================== 笔记管理 ===========================
function loadSnippets() {
  try {
    const saved = localStorage.getItem('windterm_snippets');
    state.snippets = saved ? JSON.parse(saved) : [];
  } catch (e) { state.snippets = []; }
}
function saveSnippets() {
  try {
    localStorage.setItem('windterm_snippets', JSON.stringify(state.snippets));
  } catch(e) {
    console.warn('存储空间不足', e);
  }
}

// =========================== 标签管理 ===========================
function createTab(host, port, username) {
  const tabId = 'tab_' + Date.now();
  const label = `${username || 'user'}@${host}`;

  const tabEl = document.createElement('div');
  tabEl.className = 'tab-item active';
  tabEl.innerHTML = `<span>${escapeText(label)}</span><span class="tab-close" data-tab="${tabId}">&times;</span>`;
  tabEl.dataset.tabId = tabId;

  document.querySelectorAll('.tab-item').forEach(t => t.classList.remove('active'));
  document.getElementById('tabList').appendChild(tabEl);
  document.getElementById('terminalHint').style.display = 'none';

  // xterm 实例
  const terminal = new Terminal({
    cursorBlink: true, cursorStyle: 'bar', fontSize: 14,
    fontFamily: "'JetBrains Mono', 'Fira Code', 'Cascadia Code', monospace",
    scrollback: 50000,
    theme: {
      background: '#1e1e2e', foreground: '#cdd6f4', cursor: '#89b4fa',
      cursorAccent: '#1e1e2e', selectionBackground: '#45475a',
      black: '#45475a', red: '#f38ba8', green: '#a6e3a1', yellow: '#f9e2af',
      blue: '#89b4fa', magenta: '#cba6f7', cyan: '#94e2d5', white: '#bac2de',
      brightBlack: '#585b70', brightRed: '#f38ba8', brightGreen: '#a6e3a1',
      brightYellow: '#f9e2af', brightBlue: '#89b4fa', brightMagenta: '#cba6f7',
      brightCyan: '#94e2d5', brightWhite: '#a6adc8'
    }
  });

  const fitAddon = new FitAddon.FitAddon();
  const webLinksAddon = new WebLinksAddon.WebLinksAddon();
  terminal.loadAddon(fitAddon);
  terminal.loadAddon(webLinksAddon);

  const container = document.getElementById('terminalContainer');
  terminal.open(container);

  setTimeout(() => { try { fitAddon.fit(); } catch(e) {} }, 100);

  const resizeObserver = new ResizeObserver(throttle(() => {
    try { fitAddon.fit(); } catch(e) {}
    const td = state.tabs.get(tabId);
    if (td && td.ws && td.ws.readyState === WebSocket.OPEN) {
      td.ws.send(JSON.stringify({ action: 'resize', sessionId: td.sessionId, cols: terminal.cols, rows: terminal.rows }));
    }
  }, 200));
  resizeObserver.observe(container);

  terminal.onData(data => {
    const td = state.tabs.get(tabId);
    if (td && td.ws && td.ws.readyState === WebSocket.OPEN) {
      td.ws.send(JSON.stringify({ action: 'input', sessionId: td.sessionId, input: data }));
      td.lastActivity = Date.now();
    }
  });

  terminal.element.addEventListener('contextmenu', (e) => {
    const selection = terminal.getSelection();
    const menu = document.getElementById('contextMenu');
    const copyItem = menu.querySelector('[data-action="copy"]');
    const snippetItem = menu.querySelector('[data-action="saveSnippet"]');
    if (copyItem) copyItem.style.display = selection ? '' : 'none';
    if (snippetItem) snippetItem.style.display = selection ? '' : 'none';
    menu.classList.remove('hidden');
    menu.style.left = e.clientX + 'px';
    menu.style.top = e.clientY + 'px';
    e.preventDefault();
  });

  const tabData = {
    sessionId: null, ws: null, terminal, fitAddon, resizeObserver,
    status: 'connecting', host, port, username,
    reconnectAttempts: 0, reconnectTimer: null,
    heartbeatTimer: null, heartbeatTimeout: null,
    lastActivity: Date.now()
  };

  state.tabs.set(tabId, tabData);
  state.currentTab = tabId;

  switchTab(tabId);
  connectTab(tabId);
  updateStatus();
  refreshSessionSidebar();

  tabEl.addEventListener('click', (e) => {
    if (e.target.classList.contains('tab-close')) return;
    switchTab(tabId);
  });
  tabEl.querySelector('.tab-close').addEventListener('click', (e) => {
    e.stopPropagation(); closeTab(tabId);
  });

  return tabId;
}

function switchTab(tabId) {
  document.querySelectorAll('.tab-item').forEach(t => t.classList.remove('active'));
  const tabEl = document.querySelector(`.tab-item[data-tab-id="${tabId}"]`);
  if (tabEl) tabEl.classList.add('active');

  state.tabs.forEach((tab, id) => {
    tab.terminal.element.style.display = id === tabId ? '' : 'none';
  });

  state.currentTab = tabId;
  const tabData = state.tabs.get(tabId);
  if (tabData) updateStatus(tabData.status);
}

function closeTab(tabId) {
  const tabData = state.tabs.get(tabId);
  if (!tabData) return;

  // 清理重连定时器
  if (tabData.reconnectTimer) { clearTimeout(tabData.reconnectTimer); tabData.reconnectTimer = null; }
  // 清理心跳
  if (tabData.heartbeatTimer) { clearInterval(tabData.heartbeatTimer); tabData.heartbeatTimer = null; }
  if (tabData.heartbeatTimeout) { clearTimeout(tabData.heartbeatTimeout); tabData.heartbeatTimeout = null; }

  if (tabData.ws) {
    if (tabData.sessionId) {
      tabData.ws.send(JSON.stringify({ action: 'destroy', sessionId: tabData.sessionId }));
    }
    tabData.ws.onclose = null;
    tabData.ws.close();
  }

  tabData.terminal.dispose();
  tabData.resizeObserver.disconnect();
  state.tabs.delete(tabId);

  const tabEl = document.querySelector(`.tab-item[data-tab-id="${tabId}"]`);
  if (tabEl) tabEl.remove();

  const errorOverlay = document.getElementById('errorOverlay');
  if (errorOverlay) errorOverlay.remove();

  if (state.currentTab === tabId) {
    const remaining = [...state.tabs.keys()];
    if (remaining.length > 0) {
      switchTab(remaining[0]);
    } else {
      state.currentTab = null;
      document.getElementById('terminalHint').style.display = '';
      updateStatus('disconnected');
    }
  }
  updateStatus();
  refreshSessionSidebar();
}

// =========================== P0-4: 断线重连 + P0-5: 心跳 ===========================
function connectTab(tabId) {
  const tabData = state.tabs.get(tabId);
  if (!tabData) return;

  updateTabStatus(tabId, 'connecting');

  const protocol = location.protocol === 'https:' ? 'wss:' : 'ws:';
  const host = document.getElementById('hostInput').value || 'localhost';
  const gwPort = document.getElementById('portInput').value || '8080';
  const gatewayUrl = `${protocol}//${host}:${gwPort}`;

  const ws = new WebSocket(gatewayUrl);
  tabData.ws = ws;

  ws.onopen = () => {
    tabData.reconnectAttempts = 0;
    ws.send(JSON.stringify({
      action: 'handshake',
      token: state.token || document.getElementById('tokenInput').value
    }));

    // P0-5: 心跳机制
    startHeartbeat(tabId);
  };

  ws.onmessage = (event) => {
    try {
      const msg = JSON.parse(event.data);
      handleGatewayMessage(tabId, msg);
    } catch (e) {
      console.error('消息解析失败:', e);
    }
  };

  ws.onerror = (event) => {
    const error = diagnoseError(null, tabData);
    showConnectionError(error);
  };

  ws.onclose = (event) => {
    // P0-5: 清理心跳
    stopHeartbeat(tabId);

    if (tabData.reconnectAttempts < RECONNECT_MAX_RETRIES && tabData.status !== 'disconnected_by_user') {
      const delay = Math.min(RECONNECT_BASE_DELAY * Math.pow(2, tabData.reconnectAttempts), RECONNECT_MAX_DELAY);
      const jitter = delay * 0.3 * Math.random();
      tabData.reconnectAttempts++;
      updateTabStatus(tabId, 'reconnecting');
      tabData.terminal.writeln(`\r\n\x1b[33m[重新连接中 (${tabData.reconnectAttempts}/${RECONNECT_MAX_RETRIES})...]\x1b[0m`);

      tabData.reconnectTimer = setTimeout(() => connectTab(tabId), delay + jitter);
    } else {
      updateTabStatus(tabId, 'disconnected');
      const error = diagnoseError(event, tabData);
      tabData.terminal.writeln(`\r\n\x1b[31m[${error.title} - 连接已断开]\x1b[0m`);
      if (tabData.reconnectAttempts >= RECONNECT_MAX_RETRIES) {
        tabData.terminal.writeln(`\x1b[33m[已达到最大重试次数]\x1b[0m`);
      }
    }
  };
}

function startHeartbeat(tabId) {
  stopHeartbeat(tabId);
  const tabData = state.tabs.get(tabId);
  if (!tabData) return;

  tabData.heartbeatTimer = setInterval(() => {
    if (tabData.ws && tabData.ws.readyState === WebSocket.OPEN) {
      tabData.ws.send(JSON.stringify({ action: 'ping' }));

      tabData.heartbeatTimeout = setTimeout(() => {
        tabData.terminal.writeln('\r\n\x1b[33m[心跳超时，连接可能已断开]\x1b[0m');
        tabData.ws.close();
      }, HEARTBEAT_TIMEOUT);
    }
  }, HEARTBEAT_INTERVAL);
}

function stopHeartbeat(tabId) {
  const tabData = state.tabs.get(tabId);
  if (!tabData) return;
  if (tabData.heartbeatTimer) { clearInterval(tabData.heartbeatTimer); tabData.heartbeatTimer = null; }
  if (tabData.heartbeatTimeout) { clearTimeout(tabData.heartbeatTimeout); tabData.heartbeatTimeout = null; }
}

// =========================== 消息处理 (含错误码区分) ===========================
function handleGatewayMessage(tabId, msg) {
  const tabData = state.tabs.get(tabId);
  if (!tabData) return;

  // P0-5: 心跳响应
  if (msg.type === 'pong') {
    if (tabData.heartbeatTimeout) { clearTimeout(tabData.heartbeatTimeout); tabData.heartbeatTimeout = null; }
    return;
  }

  switch (msg.type) {
    case 'handshake_ok':
      tabData.ws.send(JSON.stringify({
        action: 'create',
        host: tabData.host,
        port: parseInt(tabData.port) || 22,
        username: tabData.username || 'root'
      }));
      break;

    case 'session_created':
      tabData.sessionId = msg.sessionId;
      updateTabStatus(tabId, 'connected');
      break;

    case 'attached':
      tabData.sessionId = msg.sessionId;
      updateTabStatus(tabId, 'connected');
      break;

    case 'data':
      tabData.terminal.write(atob(msg.data));
      break;

    case 'session_closed':
      updateTabStatus(tabId, 'disconnected');
      tabData.terminal.writeln('\r\n\x1b[31m[会话已关闭]\x1b[0m');
      break;

    // P0-7: 错误码区分
    case 'error':
      handleGatewayError(tabId, msg);
      break;

    case 'input_sent':
    case 'resized':
      break;
  }
}

function handleGatewayError(tabId, msg) {
  const tabData = state.tabs.get(tabId);
  if (!tabData) return;

  let errorText, errorColor;
  switch (msg.code) {
    case 401:
      errorText = '[认证失败 (401)]: 令牌无效或已过期，请更新令牌后重试。';
      errorColor = '33'; // 黄色
      break;
    case 403:
      errorText = '[无权限 (403)]: 拒绝访问，请联系管理员。';
      errorColor = '33';
      break;
    case 404:
      errorText = '[未找到 (404)]: 请求的会话不存在，请创建新会话。';
      errorColor = '33';
      break;
    case 500:
      errorText = '[服务器错误 (500)]: 网关内部错误，请稍后重试。';
      errorColor = '31'; // 红色
      break;
    case 502:
      errorText = '[网关错误 (502)]: 后端服务不可用，请稍后重试。';
      errorColor = '31';
      break;
    default:
      errorText = `[错误 (${msg.code || '未知'})]: ${escapeText(msg.error || '未知错误')}`;
      errorColor = '31';
  }
  tabData.terminal.writeln(`\r\n\x1b[${errorColor}m${errorText}\x1b[0m`);
}

// =========================== 状态更新 ===========================
function updateTabStatus(tabId, status) {
  const tabData = state.tabs.get(tabId);
  if (!tabData) return;
  tabData.status = status;
  if (state.currentTab === tabId) updateStatus(status);
  renderActiveSessions();
}

function updateStatus(status) {
  const indicator = document.getElementById('statusIndicator');
  const text = document.getElementById('statusText');
  const tabCount = document.getElementById('tabCount');
  const snippetCount = document.getElementById('snippetCount');

  tabCount.textContent = `${state.tabs.size} 个标签`;
  snippetCount.textContent = `${state.snippets.length} 条笔记`;

  if (!status) {
    const tabData = state.tabs.get(state.currentTab);
    status = tabData ? tabData.status : 'disconnected';
  }

  indicator.classList.remove('connected', 'disconnected', 'reconnecting');
  switch (status) {
    case 'connected':
      indicator.classList.add('connected');
      text.textContent = '已连接';
      break;
    case 'connecting':
      indicator.classList.add('disconnected');
      text.textContent = '连接中...';
      break;
    case 'reconnecting':
      indicator.classList.add('reconnecting');
      text.textContent = '重连中...';
      break;
    case 'error':
      indicator.classList.add('disconnected');
      text.textContent = '连接错误';
      break;
    default:
      indicator.classList.add('disconnected');
      text.textContent = '未连接';
  }
}

// =========================== 会话管理面板 ===========================
function renderActiveSessions() {
  const list = document.getElementById('activeSessionList');
  if (state.tabs.size === 0) {
    list.innerHTML = '<div class="sidebar-empty">无活跃会话</div>';
    return;
  }
  list.innerHTML = [...state.tabs.entries()].map(([id, tab]) => `
    <div class="session-entry" data-session-id="${id}">
      <span class="session-icon"><i class="fa-solid fa-terminal"></i></span>
      <span class="session-info">
        <div class="session-host">${escapeText(tab.username || 'user')}@${escapeText(tab.host)}</div>
        <div class="session-meta">${tab.port}:${escapeText(tab.sessionId || '未分配').substring(0, 12)}</div>
      </span>
      <span class="session-status ${tab.status}"></span>
      <span class="session-actions">
        <button class="session-action-btn disconnect" data-action="disconnect" data-tab-id="${id}" title="断开"><i class="fa-solid fa-plug-circle-xmark"></i></button>
      </span>
    </div>
  `).join('');

  list.querySelectorAll('.session-entry').forEach(el => {
    el.addEventListener('click', () => switchTab(el.dataset.sessionId));
  });
  list.querySelectorAll('.session-action-btn.disconnect').forEach(btn => {
    btn.addEventListener('click', (e) => { e.stopPropagation(); closeTab(btn.dataset.tabId); });
  });
}

function renderHistorySessions() {
  const list = document.getElementById('historySessionList');
  const connections = loadConnections();
  if (connections.length === 0) {
    list.innerHTML = '<div class="sidebar-empty">无连接历史</div>';
    return;
  }
  list.innerHTML = connections.map((c, i) => `
    <div class="history-entry" data-idx="${i}">
      <span class="history-icon"><i class="fa-solid fa-clock-rotate-left"></i></span>
      <span class="history-info">
        <div class="history-host">${escapeText(c.username || 'user')}@${escapeText(c.host)}:${c.port}</div>
        <div class="history-time">${formatTime(c.time)}</div>
      </span>
      <button class="history-reconnect" data-idx="${i}" title="连接"><i class="fa-solid fa-plug"></i></button>
    </div>
  `).join('');

  list.querySelectorAll('.history-entry').forEach(el => {
    el.addEventListener('click', (e) => {
      if (e.target.closest('.history-reconnect')) return;
      const idx = parseInt(el.dataset.idx);
      const c = connections[idx];
      if (c) {
        document.getElementById('hostInput').value = c.host;
        document.getElementById('portInput').value = c.port;
        document.getElementById('userInput').value = c.username;
        state.token = c.token || '';
        storeToken(state.token);
        createTab(c.host, c.port, c.username);
      }
    });
  });
  list.querySelectorAll('.history-reconnect').forEach(btn => {
    btn.addEventListener('click', (e) => {
      e.stopPropagation();
      const idx = parseInt(btn.dataset.idx);
      const c = connections[idx];
      if (c) {
        state.token = c.token || '';
        storeToken(state.token);
        createTab(c.host, c.port, c.username);
      }
    });
  });
}

function refreshSessionSidebar() {
  renderActiveSessions();
  renderHistorySessions();
}

// =========================== P0-1: DOMPurify 笔记系统 ===========================
function createSnippet(title, content) {
  const snippet = {
    id: 'snp_' + Date.now(),
    title: title || '未命名笔记',
    content: sanitizeSnippetContent(content || ''),
    createdAt: new Date().toISOString(),
    updatedAt: new Date().toISOString()
  };
  state.snippets.push(snippet);
  saveSnippets();
  renderSnippetList();
  selectSnippet(snippet.id);
  updateStatus();
}

function selectSnippet(id) {
  state.currentSnippetId = id;
  const snippet = state.snippets.find(s => s.id === id);
  if (!snippet) return;

  document.getElementById('snippetTitle').value = snippet.title;
  // P0-1: 使用净化的 HTML
  document.getElementById('snippetContent').innerHTML = sanitizeSnippetContent(snippet.content);
  document.getElementById('deleteSnippetBtn').style.display = '';

  document.querySelectorAll('.snippet-list-item').forEach(el => {
    el.classList.toggle('active', el.dataset.snippetId === id);
  });
}

function saveCurrentSnippet() {
  if (!state.currentSnippetId) {
    const title = document.getElementById('snippetTitle').value || '未命名笔记';
    const content = document.getElementById('snippetContent').innerHTML;
    createSnippet(title, content);
    return;
  }

  const snippet = state.snippets.find(s => s.id === state.currentSnippetId);
  if (!snippet) return;

  snippet.title = document.getElementById('snippetTitle').value || '未命名笔记';
  // P0-1: 保存前净化
  snippet.content = sanitizeSnippetContent(document.getElementById('snippetContent').innerHTML);
  snippet.updatedAt = new Date().toISOString();
  saveSnippets();
  renderSnippetList();
}

function deleteCurrentSnippet() {
  if (!state.currentSnippetId) return;
  if (!confirm('确认删除这条笔记？')) return;

  state.snippets = state.snippets.filter(s => s.id !== state.currentSnippetId);
  saveSnippets();

  state.currentSnippetId = null;
  document.getElementById('snippetTitle').value = '';
  document.getElementById('snippetContent').innerHTML = '';
  document.getElementById('deleteSnippetBtn').style.display = 'none';
  renderSnippetList();
  updateStatus();
}

function renderSnippetList() {
  const list = document.getElementById('snippetList');
  list.innerHTML = state.snippets.length === 0
    ? '<div style="padding:14px;color:var(--overlay);font-size:12px;text-align:center">暂无笔记</div>'
    : state.snippets.map(s => `
      <div class="snippet-list-item" data-snippet-id="${s.id}">
        <span>${escapeText(s.title)}</span>
        <span class="snippet-time">${formatTime(s.updatedAt)}</span>
      </div>
    `).join('');

  list.querySelectorAll('.snippet-list-item').forEach(el => {
    el.addEventListener('click', () => selectSnippet(el.dataset.snippetId));
  });
}

function formatTime(iso) {
  const d = new Date(iso);
  const now = new Date();
  const diff = now - d;
  if (diff < 60000) return '刚刚';
  if (diff < 3600000) return Math.floor(diff / 60000) + ' 分钟前';
  if (diff < 86400000) return Math.floor(diff / 3600000) + ' 小时前';
  return d.toLocaleDateString();
}

function createSnippetFromSelection() {
  const tabData = state.tabs.get(state.currentTab);
  if (!tabData) return;
  const text = tabData.terminal.getSelection();
  if (!text) return;
  createSnippet('终端摘录', `<pre>${escapeText(text)}</pre>`);
  document.getElementById('snippetPanel').classList.remove('hidden');
}

// =========================== 工具栏 ===========================
function execToolbarCmd(cmd, arg) {
  document.getElementById('snippetContent').focus();
  document.execCommand(cmd, false, arg || null);
}

// =========================== 工具函数 ===========================
function throttle(fn, delay) {
  let last = 0;
  return function(...args) {
    const now = Date.now();
    if (now - last > delay) { last = now; fn.apply(this, args); }
  };
}

// =========================== P0-6: 未保存确认 ===========================
let hasUnsavedSnippet = false;

function markSnippetDirty() { hasUnsavedSnippet = true; }
function markSnippetClean() { hasUnsavedSnippet = false; }

window.addEventListener('beforeunload', (e) => {
  if (hasUnsavedSnippet) {
    e.preventDefault();
    e.returnValue = '有未保存的笔记，确定离开吗？';
    return e.returnValue;
  }
});

// =========================== 事件初始化 ===========================
document.addEventListener('DOMContentLoaded', () => {
  loadSnippets();
  renderSnippetList();
  updateStatus();
  refreshSessionSidebar();

  // 恢复最后使用的连接
  const savedConn = loadConnections();
  if (savedConn.length > 0) {
    document.getElementById('hostInput').value = savedConn[0].host || '';
    document.getElementById('portInput').value = savedConn[0].port || '22';
    document.getElementById('userInput').value = savedConn[0].username || '';
    state.token = savedConn[0].token || '';
  }

  // 连接按钮
  document.getElementById('connectBtn').addEventListener('click', () => {
    const host = document.getElementById('hostInput').value || 'localhost';
    const port = document.getElementById('portInput').value || '22';
    const username = document.getElementById('userInput').value || 'root';
    state.token = document.getElementById('tokenInput').value;
    storeToken(state.token);
    addConnection(host, port, username, state.token);
    createTab(host, port, username);
  });

  // 回车连接
  ['hostInput', 'portInput', 'userInput', 'tokenInput'].forEach(id => {
    document.getElementById(id).addEventListener('keydown', (e) => {
      if (e.key === 'Enter') document.getElementById('connectBtn').click();
    });
  });

  // 新建标签
  document.getElementById('newTabBtn').addEventListener('click', () => {
    createTab(
      document.getElementById('hostInput').value || 'localhost',
      document.getElementById('portInput').value || '22',
      document.getElementById('userInput').value || 'root'
    );
  });

  // 面板切换
  document.getElementById('toggleSessions').addEventListener('click', () => {
    document.getElementById('sessionSidebar').classList.toggle('hidden');
    refreshSessionSidebar();
  });
  document.getElementById('closeSessionSidebarBtn').addEventListener('click', () => {
    document.getElementById('sessionSidebar').classList.add('hidden');
  });
  document.getElementById('toggleSnippets').addEventListener('click', () => {
    document.getElementById('snippetPanel').classList.toggle('hidden');
  });
  document.getElementById('closeSnippetsBtn').addEventListener('click', () => {
    document.getElementById('snippetPanel').classList.add('hidden');
  });

  // 笔记操作
  document.getElementById('newSnippetBtn').addEventListener('click', () => {
    state.currentSnippetId = null;
    document.getElementById('snippetTitle').value = '';
    document.getElementById('snippetContent').innerHTML = '';
    document.getElementById('deleteSnippetBtn').style.display = 'none';
    markSnippetClean();
  });
  document.getElementById('saveSnippetBtn').addEventListener('click', () => {
    saveCurrentSnippet();
    markSnippetClean();
  });
  document.getElementById('deleteSnippetBtn').addEventListener('click', deleteCurrentSnippet);

  // P0-6: 笔记内容更改检测
  document.getElementById('snippetContent').addEventListener('input', markSnippetDirty);
  document.getElementById('snippetTitle').addEventListener('input', markSnippetDirty);

  // Ctrl+S 保存
  document.getElementById('snippetContent').addEventListener('keydown', (e) => {
    if ((e.ctrlKey || e.metaKey) && e.key === 's') {
      e.preventDefault();
      saveCurrentSnippet();
      markSnippetClean();
    }
  });

  // 自动保存
  document.getElementById('snippetContent').addEventListener('input', () => {
    if (state.autosaveTimer) clearTimeout(state.autosaveTimer);
    state.autosaveTimer = setTimeout(() => {
      if (hasUnsavedSnippet && state.currentSnippetId) {
        saveCurrentSnippet();
        markSnippetClean();
      }
    }, 5000);
  });

  // 工具栏
  document.querySelectorAll('.btn-toolbar').forEach(btn => {
    btn.addEventListener('click', () => execToolbarCmd(btn.dataset.cmd, btn.dataset.arg));
    btn.addEventListener('mousedown', (e) => e.preventDefault());
  });

  // 右键菜单
  document.addEventListener('click', () => {
    document.getElementById('contextMenu').classList.add('hidden');
  });

  document.querySelectorAll('.menu-item').forEach(item => {
    item.addEventListener('click', (e) => {
      e.stopPropagation();
      document.getElementById('contextMenu').classList.add('hidden');
      const action = item.dataset.action;
      const tabData = state.tabs.get(state.currentTab);

      switch (action) {
        case 'copy':
          if (tabData) {
            const text = tabData.terminal.getSelection();
            if (text) navigator.clipboard.writeText(text).catch(() => {});
          }
          break;
        case 'paste':
          navigator.clipboard.readText().then(text => {
            if (tabData && tabData.ws && tabData.ws.readyState === WebSocket.OPEN) {
              tabData.ws.send(JSON.stringify({ action: 'input', sessionId: tabData.sessionId, input: text }));
            }
          }).catch(() => {});
          break;
        case 'clear':
          if (tabData) tabData.terminal.clear();
          break;
        case 'saveSnippet':
          createSnippetFromSelection();
          break;
      }
    });
  });

  // 全局快捷键
  document.addEventListener('keydown', (e) => {
    if ((e.ctrlKey || e.metaKey) && e.shiftKey && e.key === 'C') {
      const tabData = state.tabs.get(state.currentTab);
      if (tabData) {
        const text = tabData.terminal.getSelection();
        if (text) navigator.clipboard.writeText(text).catch(() => {});
      }
    }
    if ((e.ctrlKey || e.metaKey) && e.shiftKey && e.key === 'V') {
      navigator.clipboard.readText().then(text => {
        const tabData = state.tabs.get(state.currentTab);
        if (tabData && tabData.ws && tabData.ws.readyState === WebSocket.OPEN) {
          tabData.ws.send(JSON.stringify({ action: 'input', sessionId: tabData.sessionId, input: text }));
        }
      }).catch(() => {});
    }
    if ((e.ctrlKey || e.metaKey) && e.key === 'b') {
      e.preventDefault();
      document.getElementById('snippetPanel').classList.toggle('hidden');
    }
    if ((e.ctrlKey || e.metaKey) && e.key === 't' && !e.shiftKey) {
      e.preventDefault();
      document.getElementById('newTabBtn').click();
    }
    if ((e.ctrlKey || e.metaKey) && e.key === 'w') {
      e.preventDefault();
      if (state.currentTab) closeTab(state.currentTab);
    }
    if ((e.ctrlKey || e.metaKey) && e.key >= '1' && e.key <= '9') {
      e.preventDefault();
      const idx = parseInt(e.key) - 1;
      const tabIds = [...state.tabs.keys()];
      if (idx < tabIds.length) switchTab(tabIds[idx]);
    }
  });

  // 窗口大小调整
  window.addEventListener('resize', throttle(() => {
    const tab = state.tabs.get(state.currentTab);
    if (tab) try { tab.fitAddon.fit(); } catch(e) {}
  }, 100));

  // 网络状态变化时自动尝试重连
  window.addEventListener('online', () => {
    state.tabs.forEach((tab, id) => {
      if (tab.status === 'disconnected') connectTab(id);
    });
  });
});

console.log('WindTerm AI Web Client (Production) 已就绪');
