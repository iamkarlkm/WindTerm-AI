/* WindTerm AI - Web Terminal Client */
'use strict';

// =========================== 状态管理 ===========================
const state = {
  gatewayUrl: 'ws://localhost:8080',
  currentTab: null,
  tabs: new Map(),       // tabId -> { sessionId, ws, terminal, fitAddon, status }
  snippets: [],
  currentSnippetId: null,
  token: ''
};

function loadSnippets() {
  try {
    const saved = localStorage.getItem('windterm_snippets');
    state.snippets = saved ? JSON.parse(saved) : [];
  } catch (e) {
    state.snippets = [];
  }
}
function saveSnippets() {
  localStorage.setItem('windterm_snippets', JSON.stringify(state.snippets));
}

// =========================== 标签管理 ===========================
function createTab(host, port, username) {
  const tabId = 'tab_' + Date.now();
  const label = `${username || 'user'}@${host}`;

  const tabEl = document.createElement('div');
  tabEl.className = 'tab-item active';
  tabEl.innerHTML = `<span>${label}</span><span class="tab-close" data-tab="${tabId}">&times;</span>`;
  tabEl.dataset.tabId = tabId;

  document.querySelectorAll('.tab-item').forEach(t => t.classList.remove('active'));
  document.getElementById('tabList').appendChild(tabEl);
  document.getElementById('terminalHint').style.display = 'none';

  // 创建 xterm 实例
  const terminal = new Terminal({
    cursorBlink: true,
    cursorStyle: 'bar',
    fontSize: 14,
    fontFamily: "'JetBrains Mono', 'Fira Code', 'Cascadia Code', monospace",
    theme: {
      background: '#1e1e2e',
      foreground: '#cdd6f4',
      cursor: '#89b4fa',
      cursorAccent: '#1e1e2e',
      selectionBackground: '#45475a',
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

  // 自适应大小
  setTimeout(() => {
    try { fitAddon.fit(); } catch(e) {}
  }, 100);

  const resizeObserver = new ResizeObserver(() => {
    try { fitAddon.fit(); } catch(e) {}
    if (tabData.ws && tabData.ws.readyState === WebSocket.OPEN) {
      tabData.ws.send(JSON.stringify({
        action: 'resize', sessionId: tabData.sessionId,
        cols: terminal.cols, rows: terminal.rows
      }));
    }
  });
  resizeObserver.observe(container);

  // 用户输入
  terminal.onData(data => {
    const tabData = state.tabs.get(tabId);
    if (tabData && tabData.ws && tabData.ws.readyState === WebSocket.OPEN) {
      tabData.ws.send(JSON.stringify({
        action: 'input', sessionId: tabData.sessionId, input: data
      }));
    }
  });

  // 选中内容时显示右键复制菜单
  terminal.element.addEventListener('contextmenu', (e) => {
    const selection = terminal.getSelection();
    const menu = document.getElementById('contextMenu');
    menu.querySelector('[data-action="copy"]').style.display = selection ? '' : 'none';
    menu.querySelector('[data-action="saveSnippet"]').style.display = selection ? '' : 'none';
    menu.classList.remove('hidden');
    menu.style.left = e.clientX + 'px';
    menu.style.top = e.clientY + 'px';
    e.preventDefault();
  });

  const tabData = {
    sessionId: null,
    ws: null,
    terminal,
    fitAddon,
    resizeObserver,
    status: 'connecting',
    host, port, username
  };

  state.tabs.set(tabId, tabData);
  state.currentTab = tabId;

  switchTab(tabId);
  connectTab(tabId);

  updateStatus();

  // 标签事件
  tabEl.addEventListener('click', (e) => {
    if (e.target.classList.contains('tab-close')) return;
    switchTab(tabId);
  });

  tabEl.querySelector('.tab-close').addEventListener('click', (e) => {
    e.stopPropagation();
    closeTab(tabId);
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

  // 关闭 WebSocket
  if (tabData.ws) {
    if (tabData.sessionId) {
      tabData.ws.send(JSON.stringify({ action: 'destroy', sessionId: tabData.sessionId }));
    }
    tabData.ws.close();
  }

  tabData.terminal.dispose();
  tabData.resizeObserver.disconnect();
  state.tabs.delete(tabId);

  const tabEl = document.querySelector(`.tab-item[data-tab-id="${tabId}"]`);
  if (tabEl) tabEl.remove();

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
}

// =========================== WebSocket 连接 ===========================
function connectTab(tabId) {
  const tabData = state.tabs.get(tabId);
  if (!tabData) return;

  updateTabStatus(tabId, 'connecting');
  const gatewayUrl = updateGatewayUrl();

  const ws = new WebSocket(gatewayUrl);
  tabData.ws = ws;

  ws.onopen = () => {
    // 握手
    ws.send(JSON.stringify({
      action: 'handshake',
      token: state.token || document.getElementById('tokenInput').value
    }));
  };

  ws.onmessage = (event) => {
    try {
      const msg = JSON.parse(event.data);
      handleGatewayMessage(tabId, msg);
    } catch (e) {
      console.error('解析消息失败:', e);
    }
  };

  ws.onerror = () => updateTabStatus(tabId, 'error');
  ws.onclose = () => updateTabStatus(tabId, 'disconnected');
}

function handleGatewayMessage(tabId, msg) {
  const tabData = state.tabs.get(tabId);
  if (!tabData) return;

  switch (msg.type) {
    case 'handshake_ok':
      // 握手成功，创建会话
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

    case 'error':
      tabData.terminal.writeln(`\r\n\x1b[31m[错误: ${msg.error}]\x1b[0m`);
      break;

    case 'input_sent':
    case 'resized':
      break; // 静默处理
  }
}

function updateGatewayUrl() {
  const host = document.getElementById('hostInput').value || 'localhost';
  const port = document.getElementById('portInput').value || '8080';
  // WebSocket 连接到网关的 WebSocket 端口
  return `ws://${host}:${port}`;
}

function updateTabStatus(tabId, status) {
  const tabData = state.tabs.get(tabId);
  if (!tabData) return;
  tabData.status = status;
  if (state.currentTab === tabId) updateStatus(status);
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

  indicator.classList.remove('connected', 'disconnected');
  switch (status) {
    case 'connected':
      indicator.classList.add('connected');
      text.textContent = '已连接';
      break;
    case 'connecting':
      indicator.classList.add('disconnected');
      text.textContent = '连接中...';
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

// =========================== 碎片笔记 ===========================
function createSnippet(title, content) {
  const snippet = {
    id: 'snp_' + Date.now(),
    title: title || '未命名笔记',
    content: content || '',
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
  document.getElementById('snippetContent').innerHTML = snippet.content;
  document.getElementById('deleteSnippetBtn').style.display = '';

  // 高亮
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
  snippet.content = document.getElementById('snippetContent').innerHTML;
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
        <span>${escapeHtml(s.title)}</span>
        <span class="snippet-time">${formatTime(s.updatedAt)}</span>
      </div>
    `).join('');

  list.querySelectorAll('.snippet-list-item').forEach(el => {
    el.addEventListener('click', () => selectSnippet(el.dataset.snippetId));
  });
}

function escapeHtml(str) {
  const div = document.createElement('div');
  div.textContent = str;
  return div.innerHTML;
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

// 从终端选中文本创建笔记
function createSnippetFromSelection() {
  const tabData = state.tabs.get(state.currentTab);
  if (!tabData) return;
  const text = tabData.terminal.getSelection();
  if (!text) return;
  createSnippet('终端摘录', `<pre>${escapeHtml(text)}</pre>`);
  document.getElementById('snippetPanel').classList.remove('hidden');
}

// =========================== 工具栏命令 ===========================
function execToolbarCmd(cmd, arg) {
  document.getElementById('snippetContent').focus();
  document.execCommand(cmd, false, arg || null);
}

// =========================== 事件监听 ===========================
document.addEventListener('DOMContentLoaded', () => {
  loadSnippets();
  renderSnippetList();
  updateStatus();

  // 连接按钮
  document.getElementById('connectBtn').addEventListener('click', () => {
    const host = document.getElementById('hostInput').value || 'localhost';
    const port = document.getElementById('portInput').value || '22';
    const username = document.getElementById('userInput').value || 'root';
    state.token = document.getElementById('tokenInput').value;
    createTab(host, port, username);
  });

  // 回车连接
  ['hostInput', 'portInput', 'userInput', 'tokenInput'].forEach(id => {
    document.getElementById(id).addEventListener('keydown', (e) => {
      if (e.key === 'Enter') document.getElementById('connectBtn').click();
    });
  });

  // 新建空白标签
  document.getElementById('newTabBtn').addEventListener('click', () => {
    createTab(
      document.getElementById('hostInput').value || 'localhost',
      document.getElementById('portInput').value || '22',
      document.getElementById('userInput').value || 'root'
    );
  });

  // 碎片笔记面板
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
  });
  document.getElementById('saveSnippetBtn').addEventListener('click', saveCurrentSnippet);
  document.getElementById('deleteSnippetBtn').addEventListener('click', deleteCurrentSnippet);

  // 快捷键保存
  document.getElementById('snippetContent').addEventListener('keydown', (e) => {
    if ((e.ctrlKey || e.metaKey) && e.key === 's') {
      e.preventDefault();
      saveCurrentSnippet();
    }
  });

  // 工具栏
  document.querySelectorAll('.btn-toolbar').forEach(btn => {
    btn.addEventListener('click', () => {
      execToolbarCmd(btn.dataset.cmd, btn.dataset.arg);
    });
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
            if (text) navigator.clipboard.writeText(text);
          }
          break;
        case 'paste':
          navigator.clipboard.readText().then(text => {
            if (tabData && tabData.ws && tabData.ws.readyState === WebSocket.OPEN) {
              tabData.ws.send(JSON.stringify({
                action: 'input', sessionId: tabData.sessionId, input: text
              }));
            }
          });
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
    // Ctrl+Shift+C 复制
    if ((e.ctrlKey || e.metaKey) && e.shiftKey && e.key === 'C') {
      const tabData = state.tabs.get(state.currentTab);
      if (tabData) {
        const text = tabData.terminal.getSelection();
        if (text) navigator.clipboard.writeText(text);
      }
    }
    // Ctrl+Shift+V 粘贴
    if ((e.ctrlKey || e.metaKey) && e.shiftKey && e.key === 'V') {
      navigator.clipboard.readText().then(text => {
        const tabData = state.tabs.get(state.currentTab);
        if (tabData && tabData.ws && tabData.ws.readyState === WebSocket.OPEN) {
          tabData.ws.send(JSON.stringify({
            action: 'input', sessionId: tabData.sessionId, input: text
          }));
        }
      });
    }
    // Ctrl+B 切换笔记面板
    if ((e.ctrlKey || e.metaKey) && e.key === 'b') {
      e.preventDefault();
      document.getElementById('snippetPanel').classList.toggle('hidden');
    }
    // Ctrl+T 新建标签
    if ((e.ctrlKey || e.metaKey) && e.key === 't' && !e.shiftKey) {
      e.preventDefault();
      document.getElementById('newTabBtn').click();
    }
    // Ctrl+W 关闭标签
    if ((e.ctrlKey || e.metaKey) && e.key === 'w') {
      e.preventDefault();
      if (state.currentTab) closeTab(state.currentTab);
    }
    // Ctrl+1-9 切换标签
    if ((e.ctrlKey || e.metaKey) && e.key >= '1' && e.key <= '9') {
      e.preventDefault();
      const idx = parseInt(e.key) - 1;
      const tabIds = [...state.tabs.keys()];
      if (idx < tabIds.length) switchTab(tabIds[idx]);
    }
  });

  // 窗口大小变化时调整终端
  window.addEventListener('resize', () => {
    state.tabs.forEach(tab => {
      try { tab.fitAddon.fit(); } catch(e) {}
    });
  });
});

console.log('WindTerm AI Web Client 已就绪');
