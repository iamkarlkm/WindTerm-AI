/* WindTerm AI - Web Terminal Client (TypeScript) */
/* P0: 安全加固 + P1: 类型安全 + 会话管理 */

// =========================== 类型定义 ===========================

interface ConnectionConfig {
  host: string;
  port: string;
  username: string;
  token: string;
  time: number;
  name?: string;
}

interface TabData {
  sessionId: string | null;
  ws: WebSocket | null;
  terminal: any;
  fitAddon: any;
  searchAddon: any;
  resizeObserver: ResizeObserver;
  status: ConnectionStatus;
  host: string;
  port: string;
  username: string;
  label: string;
  fontSize: number;
  reconnectAttempts: number;
  reconnectTimer: number | null;
  heartbeatTimer: number | null;
  heartbeatTimeout: number | null;
  lastActivity: number;
}

type ConnectionStatus = 'connected' | 'connecting' | 'disconnected' | 'reconnecting' | 'error' | 'disconnected_by_user';

interface Snippet {
  id: string;
  title: string;
  content: string;
  createdAt: string;
  updatedAt: string;
}

interface GatewayMessage {
  type: string;
  sessionId?: string;
  data?: string;
  code?: number;
  error?: string;
}

interface ErrorDiagnosis {
  level: 'error' | 'warn';
  title: string;
  detail: string;
}

interface AppState {
  currentTab: string | null;
  tabs: Map<string, TabData>;
  snippets: Snippet[];
  currentSnippetId: string | null;
  token: string;
  fontSize: number;
  autosaveTimer: number | null;
}

// =========================== 全局类型声明 ===========================

/* eslint-disable @typescript-eslint/no-explicit-any */
declare const Terminal: any;
declare const FitAddon: any;
declare const WebLinksAddon: any;
declare const SearchAddon: any;
declare const Unicode11Addon: any;
declare const LigaturesAddon: any;
declare const DOMPurify: any;

// Terminal/FitAddon types for tab data
type TerminalInstance = InstanceType<typeof Terminal>;
type FitAddonInstance = InstanceType<typeof FitAddon.FitAddon>;

// =========================== 常量 ===========================

const ALLOWED_TAGS = ['b', 'i', 'u', 's', 'pre', 'code', 'a', 'ul', 'ol', 'li',
  'span', 'br', 'strong', 'em', 'h1', 'h2', 'h3', 'p', 'img', 'blockquote', 'table', 'tr', 'td', 'th'];
const ALLOWED_ATTR = ['href', 'src', 'alt', 'style', 'class', 'title', 'target'];

const RECONNECT_MAX_RETRIES = 10;
const RECONNECT_BASE_DELAY = 1000;
const RECONNECT_MAX_DELAY = 30000;
const HEARTBEAT_INTERVAL = 30000;
const HEARTBEAT_TIMEOUT = 10000;

// =========================== P2-2: 国际化 ===========================

const messages: Record<string, Record<string, string>> = {
  zh: {
    disconnected: '未连接',
    connecting: '连接中...',
    connected: '已连接',
    reconnecting: '重连中...',
    connectionError: '连接错误',
    tabs: '个标签',
    notes: '条笔记',
    activeSessions: '活跃会话',
    noActiveSessions: '无活跃会话',
    connectionHistory: '连接历史',
    noConnectionHistory: '无连接历史',
    noNotes: '暂无笔记',
    unsavedConfirm: '有未保存的笔记，确定离开吗？',
    heartbeatTimeout: '[心跳超时，连接可能已断开]',
    reconnectingMsg: '[重新连接中 ({0}/{1})...]',
    maxRetries: '[已达到最大重试次数]',
    disconnectedMsg: '[{0} - 连接已断开]',
    sessionClosed: '[会话已关闭]',
    authFailed: '[认证失败 (401)]: 令牌无效或已过期，请更新令牌后重试。',
    forbidden: '[无权限 (403)]: 拒绝访问，请联系管理员。',
    notFound: '[未找到 (404)]: 请求的会话不存在，请创建新会话。',
    serverError: '[服务器错误 (500)]: 网关内部错误，请稍后重试。',
    gatewayError: '[网关错误 (502)]: 后端服务不可用，请稍后重试。',
    unknownError: '[错误 ({0})]: {1}',
    justNow: '刚刚',
    minutesAgo: '{0} 分钟前',
    hoursAgo: '{0} 小时前',
    untitledNote: '未命名笔记',
    terminalExcerpt: '终端摘录',
    unsaved: '未保存',
    sessionPanel: '会话管理',
    snippetPanel: '碎片笔记',
    switchTheme: '切换主题',
    fullscreen: '全屏',
    exitFullscreen: '退出全屏',
    switchLang: 'Switch to English',
    newTab: '新建标签',
    closePanel: '关闭面板',
  },
  en: {
    disconnected: 'Disconnected',
    connecting: 'Connecting...',
    connected: 'Connected',
    reconnecting: 'Reconnecting...',
    connectionError: 'Connection Error',
    tabs: 'tabs',
    notes: 'notes',
    activeSessions: 'Active Sessions',
    noActiveSessions: 'No active sessions',
    connectionHistory: 'Connection History',
    noConnectionHistory: 'No connection history',
    noNotes: 'No notes',
    unsavedConfirm: 'You have unsaved notes. Leave anyway?',
    heartbeatTimeout: '[Heartbeat timeout, connection may be lost]',
    reconnectingMsg: '[Reconnecting ({0}/{1})...]',
    maxRetries: '[Max retry attempts reached]',
    disconnectedMsg: '[{0} - Disconnected]',
    sessionClosed: '[Session closed]',
    authFailed: '[Auth Failed (401)]: Invalid or expired token. Please update and retry.',
    forbidden: '[Forbidden (403)]: Access denied. Contact admin.',
    notFound: '[Not Found (404)]: Session does not exist. Create a new session.',
    serverError: '[Server Error (500)]: Internal gateway error. Please retry.',
    gatewayError: '[Gateway Error (502)]: Backend unavailable. Please retry.',
    unknownError: '[Error ({0})]: {1}',
    justNow: 'just now',
    minutesAgo: '{0} min ago',
    hoursAgo: '{0} hr ago',
    untitledNote: 'Untitled Note',
    terminalExcerpt: 'Terminal Excerpt',
    unsaved: 'Unsaved',
    sessionPanel: 'Session Manager',
    snippetPanel: 'Snippets',
    switchTheme: 'Switch Theme',
    fullscreen: 'Fullscreen',
    exitFullscreen: 'Exit Fullscreen',
    switchLang: '切换到中文',
    newTab: 'New Tab',
    closePanel: 'Close Panel',
  },
};

let currentLang: string = (() => {
  const saved = localStorage.getItem('windterm_lang');
  if (saved === 'en' || saved === 'zh') return saved;
  return navigator.language.startsWith('zh') ? 'zh' : 'en';
})();

function t(key: string, ...args: (string | number)[]): string {
  const msg = (messages[currentLang]?.[key] || messages.zh[key] || key);
  return msg.replace(/\{(\d+)\}/g, (_, i) => String(args[parseInt(i)] ?? ''));
}

function toggleLanguage(): void {
  currentLang = currentLang === 'zh' ? 'en' : 'zh';
  localStorage.setItem('windterm_lang', currentLang);
}

// =========================== 主题管理 ===========================

function toggleTheme(): void {
  document.documentElement.classList.toggle('light-theme');
  const isLight = document.documentElement.classList.contains('light-theme');
  localStorage.setItem('windterm_theme', isLight ? 'light' : 'dark');
  const icon = $('toggleTheme')?.querySelector('i');
  if (icon) {
    icon.className = isLight ? 'fa-solid fa-sun' : 'fa-solid fa-moon';
  }
}

function initTheme(): void {
  const saved = localStorage.getItem('windterm_theme');
  if (saved === 'light') {
    document.documentElement.classList.add('light-theme');
    const icon = $('toggleTheme')?.querySelector('i');
    if (icon) icon.className = 'fa-solid fa-sun';
  }
}

// =========================== 全屏管理 ===========================

function toggleFullscreen(): void {
  if (document.fullscreenElement) {
    document.exitFullscreen();
    const icon = $('toggleFullscreen')?.querySelector('i');
    if (icon) icon.className = 'fa-solid fa-expand';
  } else {
    document.documentElement.requestFullscreen();
    const icon = $('toggleFullscreen')?.querySelector('i');
    if (icon) icon.className = 'fa-solid fa-compress';
  }
}

document.addEventListener('fullscreenchange', () => {
  const icon = $('toggleFullscreen')?.querySelector('i');
  if (icon) {
    icon.className = document.fullscreenElement ? 'fa-solid fa-compress' : 'fa-solid fa-expand';
  }
});

// =========================== P3-4: 设置持久化 (必须在 state 之前) ===========================

interface UserSettings {
  fontSize: number;
  language: string;
  theme: string;
  sidebarVisible: boolean;
}

const defaultSettings: UserSettings = {
  fontSize: 14,
  language: 'auto',
  theme: 'dark',
  sidebarVisible: false,
};

function loadSettings(): UserSettings {
  try {
    const saved = localStorage.getItem('windterm_settings');
    return saved ? { ...defaultSettings, ...JSON.parse(saved) } : { ...defaultSettings };
  } catch { return { ...defaultSettings }; }
}

function saveSettings(settings: UserSettings): void {
  localStorage.setItem('windterm_settings', JSON.stringify(settings));
}

// =========================== 状态管理 ===========================

const state: AppState = {
  currentTab: null,
  tabs: new Map(),
  snippets: [],
  currentSnippetId: null,
  token: '',
  fontSize: loadSettings().fontSize,
  autosaveTimer: null,
};

// =========================== 安全工具 ===========================

function sanitizeHtml(dirty: string): string {
  if (typeof DOMPurify === 'undefined') {
    const div = document.createElement('div'); div.textContent = dirty; return div.innerHTML;
  }
  return DOMPurify.sanitize(dirty, { ALLOWED_TAGS, ALLOWED_ATTR });
}

function sanitizeSnippetContent(dirty: string): string {
  if (typeof DOMPurify === 'undefined') {
    const div = document.createElement('div'); div.textContent = dirty; return div.innerHTML;
  }
  return DOMPurify.sanitize(dirty, {
    ALLOWED_TAGS: [...ALLOWED_TAGS, 'font'],
    ALLOWED_ATTR: [...ALLOWED_ATTR, 'color'],
    ALLOW_DATA_ATTR: false,
  });
}

// =========================== Token 管理 ===========================

function storeToken(token: string): void {
  sessionStorage.setItem('windterm_token', btoa(String(token)));
}

function getToken(): string {
  try { return atob(sessionStorage.getItem('windterm_token') || ''); } catch { return ''; }
}

function clearToken(): void {
  sessionStorage.removeItem('windterm_token');
}

// =========================== 连接配置持久化 ===========================

function loadConnections(): ConnectionConfig[] {
  try {
    const raw = localStorage.getItem('windterm_connections');
    return raw ? JSON.parse(raw) : [];
  } catch { return []; }
}

function saveConnections(list: ConnectionConfig[]): void {
  localStorage.setItem('windterm_connections', JSON.stringify(list.slice(0, 50)));
}

function addConnection(host: string, port: string, username: string, token: string): void {
  const list = loadConnections();
  const idx = list.findIndex((c) => c.host === host && c.port === port && c.username === username);
  if (idx >= 0) { list.splice(idx, 1); }
  list.unshift({ host, port, username, token, time: Date.now() });
  saveConnections(list);
}

// =========================== P5-1: 连接配置模板 ===========================

function loadTemplates(): ConnectionConfig[] {
  try {
    const raw = localStorage.getItem('windterm_templates');
    return raw ? JSON.parse(raw) : [];
  } catch { return []; }
}

function saveTemplates(list: ConnectionConfig[]): void {
  localStorage.setItem('windterm_templates', JSON.stringify(list));
}

function saveAsTemplate(): void {
  const host = ($('hostInput') as HTMLInputElement)?.value || 'localhost';
  const port = ($('portInput') as HTMLInputElement)?.value || '22';
  const username = ($('userInput') as HTMLInputElement)?.value || 'root';
  const name = prompt('配置名称:', `${username}@${host}`);
  if (!name) return;
  const templates = loadTemplates();
  templates.push({ host, port, username, token: '', time: Date.now(), name });
  saveTemplates(templates);
  refreshSessionSidebar();
}

function deleteTemplate(idx: number): void {
  const templates = loadTemplates();
  templates.splice(idx, 1);
  saveTemplates(templates);
  refreshSessionSidebar();
}

// =========================== P5-2: 会话自动恢复 ===========================

function saveSessionState(): void {
  const sessions: { host: string; port: string; username: string; token: string; label: string }[] = [];
  state.tabs.forEach((tab) => {
    if (tab.status === 'connected' || tab.status === 'connecting' || tab.status === 'reconnecting') {
      sessions.push({
        host: tab.host, port: tab.port, username: tab.username,
        token: state.token, label: tab.label,
      });
    }
  });
  if (sessions.length > 0) {
    localStorage.setItem('windterm_last_sessions', JSON.stringify(sessions));
  } else {
    localStorage.removeItem('windterm_last_sessions');
  }
}

function restoreLastSessions(): void {
  try {
    const raw = localStorage.getItem('windterm_last_sessions');
    if (!raw) return;
    const sessions: { host: string; port: string; username: string; token: string; label: string }[] = JSON.parse(raw);
    if (sessions.length === 0) return;
    const confirmRestore = confirm(`检测到上次有 ${sessions.length} 个活跃会话，是否恢复连接？`);
    if (!confirmRestore) { localStorage.removeItem('windterm_last_sessions'); return; }
    sessions.forEach((s) => {
      ($('hostInput') as HTMLInputElement).value = s.host;
      ($('portInput') as HTMLInputElement).value = s.port;
      ($('userInput') as HTMLInputElement).value = s.username;
      state.token = s.token || '';
      storeToken(state.token);
      createTab(s.host, s.port, s.username);
    });
  } catch { /* ignore */ }
}

// 页面关闭前保存活跃会话
window.addEventListener('beforeunload', () => {
  saveSessionState();
});

// =========================== 连接错误诊断 ===========================

function diagnoseError(event: CloseEvent | null, tabData: TabData): ErrorDiagnosis {
  const host = tabData.host;
  const port = tabData.port;

  if (!navigator.onLine) {
    return { level: 'error', title: '网络断开', detail: '设备未连接到网络，请检查网络设置。' };
  }

  if (event && event.code === 1006) {
    return {
      level: 'error', title: '连接异常关闭',
      detail: `无法连接到 ${host}:${port}。请检查:</br>1. 服务器是否运行</br>2. 防火墙是否放行端口 ${port}</br>3. 主机地址是否正确`,
    };
  }

  if (event && event.code === 1001) {
    return { level: 'warn', title: '服务端关闭连接', detail: '服务器主动关闭了连接，可能是会话超时。' };
  }

  const url = tabData.ws?.url || '';
  if (url.startsWith('ws://') && location.protocol === 'https:') {
    return {
      level: 'error', title: '协议不匹配',
      detail: `当前页面使用 HTTPS，但 WebSocket 目标为 ${url}。浏览器会阻止不安全连接。请使用 wss:// 协议。`,
    };
  }

  return { level: 'error', title: '连接失败', detail: `无法建立 WebSocket 连接到 ${host}:${port}。<br/>请检查主机地址和端口是否正确。` };
}

function showConnectionError(error: ErrorDiagnosis): void {
  const container = document.getElementById('terminalContainer');
  if (!container) return;
  const existing = document.getElementById('errorOverlay');
  if (existing) existing.remove();

  const overlay = document.createElement('div');
  overlay.id = 'errorOverlay';
  overlay.style.cssText =
    'position:absolute;inset:0;display:flex;align-items:center;justify-content:center;background:rgba(0,0,0,0.7);z-index:100;';
  overlay.innerHTML = `
    <div style="background:var(--bg-alt);border:1px solid var(--red);border-radius:12px;padding:24px 32px;max-width:420px;text-align:center;">
      <div style="font-size:32px;margin-bottom:12px;">${error.level === 'error' ? '&#x26D4;' : '&#x26A0;&#xFE0F;'}</div>
      <div style="font-size:16px;font-weight:600;color:var(--red);margin-bottom:8px;">${escapeText(error.title)}</div>
      <div style="font-size:13px;color:var(--subtext);line-height:1.6;margin-bottom:16px;">${error.detail}</div>
      <button onclick="this.parentElement.parentElement.remove()" style="padding:6px 20px;border:1px solid var(--border);border-radius:6px;background:var(--bg);color:var(--text);cursor:pointer;">关闭</button>
    </div>`;
  container.appendChild(overlay);
}

function escapeText(text: string): string {
  const div = document.createElement('div');
  div.textContent = text;
  return div.innerHTML;
}

// =========================== 笔记管理 ===========================

function loadSnippets(): void {
  try {
    const saved = localStorage.getItem('windterm_snippets');
    state.snippets = saved ? JSON.parse(saved) : [];
  } catch { state.snippets = []; }
}

function saveSnippets(): void {
  try {
    localStorage.setItem('windterm_snippets', JSON.stringify(state.snippets));
  } catch (e) {
    console.warn('存储空间不足', e);
  }
}

// =========================== 工具函数 ===========================

function throttle<T extends (...args: unknown[]) => void>(fn: T, delay: number): (...args: Parameters<T>) => void {
  let last = 0;
  return function (...args: Parameters<T>) {
    const now = Date.now();
    if (now - last > delay) { last = now; fn(...args); }
  };
}

function formatTime(iso: string): string {
  const d = new Date(iso);
  const now = Date.now();
  const diff = now - d.getTime();
  if (diff < 60000) return '刚刚';
  if (diff < 3600000) return `${Math.floor(diff / 60000)} 分钟前`;
  if (diff < 86400000) return `${Math.floor(diff / 3600000)} 小时前`;
  return d.toLocaleDateString();
}

function $(id: string): HTMLElement | null {
  return document.getElementById(id);
}

// =========================== 标签管理 ===========================

function createTab(host: string, port: string, username: string): string {
  const tabId = 'tab_' + Date.now();
  const label = `${username || 'user'}@${host}`;

  const tabEl = document.createElement('div');
  tabEl.className = 'tab-item active';
  tabEl.innerHTML = `<span>${escapeText(label)}</span><span class="tab-close" data-tab="${tabId}">&times;</span>`;
  tabEl.dataset.tabId = tabId;

  document.querySelectorAll('.tab-item').forEach((t) => t.classList.remove('active'));
  const tabList = $('tabList');
  if (tabList) tabList.appendChild(tabEl);
  const hint = $('terminalHint');
  if (hint) hint.style.display = 'none';

  const terminal = new Terminal({
    cursorBlink: true,
    cursorStyle: 'bar',
    fontSize: state.fontSize,
    fontFamily: "'JetBrains Mono', 'Fira Code', 'Cascadia Code', monospace",
    scrollback: 50000,
    theme: {
      background: '#1e1e2e', foreground: '#cdd6f4', cursor: '#89b4fa',
      cursorAccent: '#1e1e2e', selectionBackground: '#45475a',
      black: '#45475a', red: '#f38ba8', green: '#a6e3a1', yellow: '#f9e2af',
      blue: '#89b4fa', magenta: '#cba6f7', cyan: '#94e2d5', white: '#bac2de',
      brightBlack: '#585b70', brightRed: '#f38ba8', brightGreen: '#a6e3a1',
      brightYellow: '#f9e2af', brightBlue: '#89b4fa', brightMagenta: '#cba6f7',
      brightCyan: '#94e2d5', brightWhite: '#a6adc8',
    },
  });

  const fitAddon = new FitAddon.FitAddon();
  const webLinksAddon = new WebLinksAddon.WebLinksAddon();
  const searchAddon = new SearchAddon.SearchAddon();
  const unicode11Addon = new Unicode11Addon.Unicode11Addon();
  terminal.loadAddon(fitAddon);
  terminal.loadAddon(webLinksAddon);
  terminal.loadAddon(searchAddon);
  try { terminal.loadAddon(unicode11Addon); terminal.unicode.activeVersion = '11'; } catch { /* ignore */ }
  try { terminal.loadAddon(new LigaturesAddon.LigaturesAddon()); } catch { /* ignore */ }

  const container = $('terminalContainer');
  if (container) terminal.open(container);

  setTimeout(() => { try { fitAddon.fit(); } catch { /* ignore */ } }, 100);

  const resizeObserver = new ResizeObserver(throttle(() => {
    try { fitAddon.fit(); } catch { /* ignore */ }
    const td = state.tabs.get(tabId);
    if (td && td.ws && td.ws.readyState === WebSocket.OPEN) {
      td.ws.send(JSON.stringify({ action: 'resize', sessionId: td.sessionId, cols: terminal.cols, rows: terminal.rows }));
    }
  }, 200));
  if (container) resizeObserver.observe(container);

  terminal.onData((data: string) => {
    const td = state.tabs.get(tabId);
    if (td && td.ws && td.ws.readyState === WebSocket.OPEN) {
      td.ws.send(JSON.stringify({ action: 'input', sessionId: td.sessionId, input: data }));
      td.lastActivity = Date.now();
    }
  });

  terminal.element!.addEventListener('contextmenu', (e: Event) => {
    const me = e as MouseEvent;
    const selection = terminal.getSelection();
    const menu = $('contextMenu');
    if (!menu) return;
    const copyItem = menu.querySelector('[data-action="copy"]') as HTMLElement | null;
    const snippetItem = menu.querySelector('[data-action="saveSnippet"]') as HTMLElement | null;
    if (copyItem) copyItem.style.display = selection ? '' : 'none';
    if (snippetItem) snippetItem.style.display = selection ? '' : 'none';
    menu.classList.remove('hidden');
    menu.style.left = me.clientX + 'px';
    menu.style.top = me.clientY + 'px';
    e.preventDefault();
  });

  const tabData: TabData = {
    sessionId: null, ws: null, terminal, fitAddon, searchAddon, resizeObserver,
    status: 'connecting', host, port, username, label, fontSize: state.fontSize,
    reconnectAttempts: 0, reconnectTimer: null,
    heartbeatTimer: null, heartbeatTimeout: null,
    lastActivity: Date.now(),
  };

  state.tabs.set(tabId, tabData);
  state.currentTab = tabId;

  switchTab(tabId);
  connectTab(tabId);
  updateStatus();
  refreshSessionSidebar();

  tabEl.addEventListener('click', (e) => {
    if ((e.target as HTMLElement).classList.contains('tab-close')) return;
    if ((e.target as HTMLElement).classList.contains('tab-rename-input')) return;
    switchTab(tabId);
  });

  // P4-2: 标签右键菜单
  tabEl.addEventListener('contextmenu', (e) => {
    e.preventDefault();
    showTabContextMenu(tabId, (e as MouseEvent).clientX, (e as MouseEvent).clientY);
  });

  // P3-2: 双击重命名标签
  tabEl.addEventListener('dblclick', (e) => {
    if ((e.target as HTMLElement).closest('.tab-close')) return;
    const span = tabEl.querySelector('span');
    if (!span || tabEl.querySelector('.tab-rename-input')) return;
    const input = document.createElement('input');
    input.type = 'text';
    input.className = 'tab-rename-input';
    input.value = tabData.label;
    span.replaceWith(input);
    input.focus();
    input.select();
    const finishRename = () => {
      const newLabel = input.value.trim() || tabData.label;
      tabData.label = newLabel;
      const newSpan = document.createElement('span');
      newSpan.textContent = newLabel;
      input.replaceWith(newSpan);
    };
    input.addEventListener('blur', finishRename);
    input.addEventListener('keydown', (ke) => {
      if ((ke as KeyboardEvent).key === 'Enter') finishRename();
      if ((ke as KeyboardEvent).key === 'Escape') { input.value = tabData.label; finishRename(); }
    });
  });
  tabEl.querySelector('.tab-close')!.addEventListener('click', (e) => {
    e.stopPropagation();
    closeTab(tabId);
  });

  // P2-1: 标签页拖拽排序
  tabEl.draggable = true;
  tabEl.addEventListener('dragstart', () => { tabEl.classList.add('dragging'); });
  tabEl.addEventListener('dragend', () => { document.querySelectorAll('.tab-item').forEach(t => t.classList.remove('dragging', 'drag-over')); });
  tabEl.addEventListener('dragover', (e) => { e.preventDefault(); tabEl.classList.add('drag-over'); });
  tabEl.addEventListener('dragleave', () => { tabEl.classList.remove('drag-over'); });
  tabEl.addEventListener('drop', (e) => {
    e.preventDefault();
    tabEl.classList.remove('drag-over');
    const dragging = document.querySelector('.tab-item.dragging');
    if (dragging && dragging !== tabEl) {
      const tabList = $('tabList');
      const items = [...tabList!.querySelectorAll('.tab-item')];
      const fromIdx = items.indexOf(dragging);
      const toIdx = items.indexOf(tabEl);
      if (fromIdx < toIdx) {
        tabList!.insertBefore(dragging, tabEl.nextSibling);
      } else {
        tabList!.insertBefore(dragging, tabEl);
      }
    }
  });

  return tabId;
}

function switchTab(tabId: string): void {
  document.querySelectorAll('.tab-item').forEach((t) => t.classList.remove('active'));
  const tabEl = document.querySelector(`.tab-item[data-tab-id="${tabId}"]`);
  if (tabEl) tabEl.classList.add('active');

  state.tabs.forEach((tab, id) => {
    tab.terminal.element!.style.display = id === tabId ? '' : 'none';
  });

  state.currentTab = tabId;
  const tabData = state.tabs.get(tabId);
  if (tabData) updateStatus(tabData.status);
}

function closeTab(tabId: string): void {
  const tabData = state.tabs.get(tabId);
  if (!tabData) return;

  if (tabData.reconnectTimer) { clearTimeout(tabData.reconnectTimer); tabData.reconnectTimer = null; }
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

  const errorOverlay = $('errorOverlay');
  if (errorOverlay) errorOverlay.remove();

  if (state.currentTab === tabId) {
    const remaining = [...state.tabs.keys()];
    if (remaining.length > 0) {
      switchTab(remaining[0]);
    } else {
      state.currentTab = null;
      const hint = $('terminalHint');
      if (hint) hint.style.display = '';
      updateStatus('disconnected');
    }
  }
  updateStatus();
  refreshSessionSidebar();
}

// =========================== P0-4: 断线重连 + P0-5: 心跳 ===========================

function connectTab(tabId: string): void {
  const tabData = state.tabs.get(tabId);
  if (!tabData) return;

  updateTabStatus(tabId, 'connecting');

  const protocol: string = location.protocol === 'https:' ? 'wss:' : 'ws:';
  const host = ($('hostInput') as HTMLInputElement)?.value || 'localhost';
  const gwPort = ($('portInput') as HTMLInputElement)?.value || '8080';
  const gatewayUrl = `${protocol}//${host}:${gwPort}`;

  const ws = new WebSocket(gatewayUrl);
  tabData.ws = ws;

  ws.onopen = () => {
    tabData.reconnectAttempts = 0;
    ws.send(JSON.stringify({
      action: 'handshake',
      token: state.token || ($('tokenInput') as HTMLInputElement)?.value,
    }));
    startHeartbeat(tabId);
  };

  ws.onmessage = (event: MessageEvent) => {
    try {
      const msg: GatewayMessage = JSON.parse(event.data);
      handleGatewayMessage(tabId, msg);
    } catch (e) {
      console.error('消息解析失败:', e);
    }
  };

  ws.onerror = () => {
    const error = diagnoseError(null, tabData);
    showConnectionError(error);
  };

  ws.onclose = (event: CloseEvent) => {
    stopHeartbeat(tabId);

    if (tabData.reconnectAttempts < RECONNECT_MAX_RETRIES && tabData.status !== 'disconnected_by_user') {
      const delay = Math.min(RECONNECT_BASE_DELAY * Math.pow(2, tabData.reconnectAttempts), RECONNECT_MAX_DELAY);
      const jitter = delay * 0.3 * Math.random();
      tabData.reconnectAttempts++;
      updateTabStatus(tabId, 'reconnecting');
      tabData.terminal.writeln(`\r\n\x1b[33m${t('reconnectingMsg', String(tabData.reconnectAttempts), String(RECONNECT_MAX_RETRIES))}\x1b[0m`);

      tabData.reconnectTimer = window.setTimeout(() => connectTab(tabId), delay + jitter);
    } else {
      updateTabStatus(tabId, 'disconnected');
      const error = diagnoseError(event, tabData);
      tabData.terminal.writeln(`\r\n\x1b[31m${t('disconnectedMsg', error.title)}\x1b[0m`);
      if (tabData.reconnectAttempts >= RECONNECT_MAX_RETRIES) {
        tabData.terminal.writeln(`\x1b[33m${t('maxRetries')}\x1b[0m`);
      }
    }
  };
}

function startHeartbeat(tabId: string): void {
  stopHeartbeat(tabId);
  const tabData = state.tabs.get(tabId);
  if (!tabData) return;

  tabData.heartbeatTimer = window.setInterval(() => {
    if (tabData.ws && tabData.ws.readyState === WebSocket.OPEN) {
      tabData.ws.send(JSON.stringify({ action: 'ping' }));

      tabData.heartbeatTimeout = window.setTimeout(() => {
        tabData.terminal.writeln(`\r\n\x1b[33m${t('heartbeatTimeout')}\x1b[0m`);
        tabData.ws!.close();
      }, HEARTBEAT_TIMEOUT);
    }
  }, HEARTBEAT_INTERVAL);
}

function stopHeartbeat(tabId: string): void {
  const tabData = state.tabs.get(tabId);
  if (!tabData) return;
  if (tabData.heartbeatTimer) { clearInterval(tabData.heartbeatTimer); tabData.heartbeatTimer = null; }
  if (tabData.heartbeatTimeout) { clearTimeout(tabData.heartbeatTimeout); tabData.heartbeatTimeout = null; }
}

// =========================== 消息处理 (含错误码区分) ===========================

function handleGatewayMessage(tabId: string, msg: GatewayMessage): void {
  const tabData = state.tabs.get(tabId);
  if (!tabData) return;

  if (msg.type === 'pong') {
    if (tabData.heartbeatTimeout) { clearTimeout(tabData.heartbeatTimeout); tabData.heartbeatTimeout = null; }
    return;
  }

  switch (msg.type) {
    case 'handshake_ok':
      tabData.ws!.send(JSON.stringify({
        action: 'create',
        host: tabData.host,
        port: parseInt(tabData.port) || 22,
        username: tabData.username || 'root',
      }));
      break;
    case 'session_created':
      tabData.sessionId = msg.sessionId || null;
      updateTabStatus(tabId, 'connected');
      break;
    case 'attached':
      tabData.sessionId = msg.sessionId || null;
      updateTabStatus(tabId, 'connected');
      break;
    case 'data':
      if (msg.data) tabData.terminal.write(atob(msg.data));
      break;
    case 'session_closed':
      updateTabStatus(tabId, 'disconnected');
      tabData.terminal.writeln(`\r\n\x1b[31m${t('sessionClosed')}\x1b[0m`);
      break;
    case 'error':
      handleGatewayError(tabId, msg);
      break;
    case 'input_sent':
    case 'resized':
      break;
    // P7-1: 共享/聊天消息
    default:
      handleSharingMessage(msg);
      break;
  }
}

function handleGatewayError(tabId: string, msg: GatewayMessage): void {
  const tabData = state.tabs.get(tabId);
  if (!tabData) return;

  let errorText: string;
  let errorColor: string;
  switch (msg.code) {
    case 401:
      errorText = t('authFailed');
      errorColor = '33';
      break;
    case 403:
      errorText = t('forbidden');
      errorColor = '33';
      break;
    case 404:
      errorText = t('notFound');
      errorColor = '33';
      break;
    case 500:
      errorText = t('serverError');
      errorColor = '31';
      break;
    case 502:
      errorText = t('gatewayError');
      errorColor = '31';
      break;
    default:
      errorText = t('unknownError', String(msg.code || '未知'), escapeText(msg.error || '未知错误'));
      errorColor = '31';
  }
  tabData.terminal.writeln(`\r\n\x1b[${errorColor}m${errorText}\x1b[0m`);
}

// =========================== P3-1: 终端搜索 ===========================

function toggleSearchBar(): void {
  const bar = $('searchBar');
  if (!bar) return;
  bar.classList.toggle('hidden');
  const input = $('searchInput') as HTMLInputElement;
  if (!bar.classList.contains('hidden') && input) {
    input.focus();
    input.select();
  } else {
    clearSearch();
  }
}

function performSearch(): void {
  const input = $('searchInput') as HTMLInputElement;
  const count = $('searchCount');
  const tabData = state.tabs.get(state.currentTab || '');
  if (!tabData || !tabData.searchAddon || !input) return;

  const query = input.value;
  if (!query) {
    tabData.searchAddon.clearDecorations();
    if (count) count.textContent = '';
    return;
  }

  try {
    tabData.searchAddon.findNext(query, { caseSensitive: false, regex: false, wholeWord: false });
  } catch { /* ignore regex errors */ }

  try {
    const resultCount = tabData.searchAddon.findPrevious(query, { caseSensitive: false, regex: false, wholeWord: false });
    if (count && typeof resultCount === 'number') {
      count.textContent = resultCount > 0 ? `${resultCount}` : '0';
    }
  } catch { if (count) count.textContent = '?'; }
}

function searchNext(): void {
  const input = ($('searchInput') as HTMLInputElement)?.value;
  const tabData = state.tabs.get(state.currentTab || '');
  if (!tabData || !tabData.searchAddon || !input) return;
  try {
    tabData.searchAddon.findNext(input, { caseSensitive: false, regex: false, wholeWord: false });
  } catch { /* ignore */ }
}

function searchPrev(): void {
  const input = ($('searchInput') as HTMLInputElement)?.value;
  const tabData = state.tabs.get(state.currentTab || '');
  if (!tabData || !tabData.searchAddon || !input) return;
  try {
    tabData.searchAddon.findPrevious(input, { caseSensitive: false, regex: false, wholeWord: false });
  } catch { /* ignore */ }
}

function clearSearch(): void {
  const tabData = state.tabs.get(state.currentTab || '');
  if (tabData && tabData.searchAddon) {
    try { tabData.searchAddon.clearDecorations(); } catch { /* ignore */ }
  }
  const count = $('searchCount');
  if (count) count.textContent = '';
}

// =========================== P3-3: 快捷键对话框 ===========================

function toggleShortcutsDialog(): void {
  $('shortcutsDialog')?.classList.toggle('hidden');
}

// =========================== P4-1: 字体缩放 ===========================

function zoomIn(): void {
  state.fontSize = Math.min(state.fontSize + 1, 32);
  applyFontSizeToAll();
  saveSettings({
    fontSize: state.fontSize,
    language: currentLang,
    theme: document.documentElement.classList.contains('light-theme') ? 'light' : 'dark',
    sidebarVisible: !$('sessionSidebar')?.classList.contains('hidden'),
  });
}

function zoomOut(): void {
  state.fontSize = Math.max(state.fontSize - 1, 8);
  applyFontSizeToAll();
  saveSettings({
    fontSize: state.fontSize,
    language: currentLang,
    theme: document.documentElement.classList.contains('light-theme') ? 'light' : 'dark',
    sidebarVisible: !$('sessionSidebar')?.classList.contains('hidden'),
  });
}

function resetZoom(): void {
  state.fontSize = 14;
  applyFontSizeToAll();
  saveSettings({
    fontSize: state.fontSize,
    language: currentLang,
    theme: document.documentElement.classList.contains('light-theme') ? 'light' : 'dark',
    sidebarVisible: !$('sessionSidebar')?.classList.contains('hidden'),
  });
}

function applyFontSizeToAll(): void {
  state.tabs.forEach((tab) => {
    tab.fontSize = state.fontSize;
    try { tab.terminal.options.fontSize = state.fontSize; } catch { /* ignore */ }
    try { tab.fitAddon.fit(); } catch { /* ignore */ }
  });
}

// =========================== P4-4: 终端内容导出 ===========================

function exportTerminalContent(): void {
  const tabData = state.tabs.get(state.currentTab || '');
  if (!tabData) return;
  try {
    const lines = tabData.terminal.buffer.active.base;
    let text = '';
    for (let i = 0; i < lines; i++) {
      const line = tabData.terminal.buffer.active.getLine(i);
      if (line) text += line.translateToString() + '\n';
    }
    downloadText(text, `windterm-export-${new Date().toISOString().slice(0, 19).replace(/:/g, '-')}.txt`);
  } catch {
    tabData.terminal.selectAll();
    const text = tabData.terminal.getSelection();
    if (text) downloadText(text);
  }
}

function downloadText(content: string, filename?: string): void {
  const blob = new Blob([content], { type: 'text/plain' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = filename || 'export.txt';
  a.click();
  URL.revokeObjectURL(url);
}

// =========================== P4-2: 标签右键菜单 ===========================

let tabContextMenuTarget: string | null = null;

function showTabContextMenu(tabId: string, x: number, y: number): void {
  tabContextMenuTarget = tabId;
  const menu = $('tabContextMenu');
  if (!menu) return;
  menu.classList.remove('hidden');
  menu.style.left = x + 'px';
  menu.style.top = y + 'px';
}

function hideTabContextMenu(): void {
  const menu = $('tabContextMenu');
  if (menu) menu.classList.add('hidden');
  tabContextMenuTarget = null;
}

function closeAllTabs(): void {
  const ids = [...state.tabs.keys()];
  ids.forEach((id) => closeTab(id));
}

function closeOtherTabs(tabId: string): void {
  const ids = [...state.tabs.keys()].filter((id) => id !== tabId);
  ids.forEach((id) => closeTab(id));
}

function duplicateTab(tabId: string): void {
  const tabData = state.tabs.get(tabId);
  if (!tabData) return;
  createTab(tabData.host, tabData.port, tabData.username);
}

function triggerTabRename(tabId: string): void {
  const tabEl = document.querySelector(`.tab-item[data-tab-id="${tabId}"]`);
  if (!tabEl) return;
  const span = tabEl.querySelector('span');
  if (!span || tabEl.querySelector('.tab-rename-input')) return;
  const input = document.createElement('input');
  input.type = 'text';
  input.className = 'tab-rename-input';
  const tabData = state.tabs.get(tabId);
  input.value = tabData?.label || '';
  span.replaceWith(input);
  input.focus();
  input.select();
  const finishRename = () => {
    const newLabel = input.value.trim() || (tabData?.label || '');
    if (tabData) tabData.label = newLabel;
    const newSpan = document.createElement('span');
    newSpan.textContent = newLabel;
    input.replaceWith(newSpan);
  };
  input.addEventListener('blur', finishRename);
  input.addEventListener('keydown', (ke) => {
    if ((ke as KeyboardEvent).key === 'Enter') finishRename();
    if ((ke as KeyboardEvent).key === 'Escape') { if (tabData) input.value = tabData.label; finishRename(); }
  });
}

// =========================== 状态更新 ===========================

function updateTabStatus(tabId: string, status: ConnectionStatus): void {
  const tabData = state.tabs.get(tabId);
  if (!tabData) return;
  tabData.status = status;
  if (state.currentTab === tabId) updateStatus(status);
  renderActiveSessions();
}

function updateStatus(status?: ConnectionStatus): void {
  const indicator = $('statusIndicator');
  const text = $('statusText');
  const tabCount = $('tabCount');
  const snippetCount = $('snippetCount');

  if (tabCount) tabCount.textContent = `${state.tabs.size} ${t('tabs')}`;
  if (snippetCount) snippetCount.textContent = `${state.snippets.length} ${t('notes')}`;

  if (!status) {
    const tabData = state.tabs.get(state.currentTab || '');
    status = tabData ? tabData.status : 'disconnected';
  }

  if (indicator) {
    indicator.classList.remove('connected', 'disconnected', 'reconnecting');
    switch (status) {
      case 'connected':
        indicator.classList.add('connected');
        if (text) text.textContent = t('connected');
        break;
      case 'connecting':
        indicator.classList.add('disconnected');
        if (text) text.textContent = t('connecting');
        break;
      case 'reconnecting':
        indicator.classList.add('reconnecting');
        if (text) text.textContent = t('reconnecting');
        break;
      case 'error':
        indicator.classList.add('disconnected');
        if (text) text.textContent = t('connectionError');
        break;
      default:
        indicator.classList.add('disconnected');
        if (text) text.textContent = t('disconnected');
    }
  }
}

// =========================== 会话管理面板 ===========================

function renderActiveSessions(): void {
  const list = $('activeSessionList');
  if (!list) return;
  if (state.tabs.size === 0) {
    list.innerHTML = `<div class="sidebar-empty">${t('noActiveSessions')}</div>`;
    return;
  }
  list.innerHTML = [...state.tabs.entries()].map(([id, tab]) => `
    <div class="session-entry" data-session-id="${id}">
      <span class="session-icon"><i class="fa-solid fa-terminal"></i></span>
      <span class="session-info">
        <div class="session-host">${escapeText(tab.username || 'user')}@${escapeText(tab.host)}</div>
        <div class="session-meta">${tab.port}:${escapeText((tab.sessionId || '未分配').substring(0, 12))}</div>
      </span>
      <span class="session-status ${tab.status}"></span>
      <span class="session-actions">
        <button class="session-action-btn disconnect" data-action="disconnect" data-tab-id="${id}" title="断开"><i class="fa-solid fa-plug-circle-xmark"></i></button>
      </span>
    </div>
  `).join('');

  list.querySelectorAll('.session-entry').forEach((el) => {
    el.addEventListener('click', () => switchTab((el as HTMLElement).dataset.sessionId!));
  });
  list.querySelectorAll('.session-action-btn.disconnect').forEach((btn) => {
    btn.addEventListener('click', (e) => { e.stopPropagation(); closeTab((btn as HTMLElement).dataset.tabId!); });
  });
}

function renderHistorySessions(): void {
  const list = $('historySessionList');
  if (!list) return;
  const connections = loadConnections();
  if (connections.length === 0) {
    list.innerHTML = `<div class="sidebar-empty">${t('noConnectionHistory')}</div>`;
    return;
  }
  list.innerHTML = connections.map((c, i) => `
    <div class="history-entry" data-idx="${i}">
      <span class="history-icon"><i class="fa-solid fa-clock-rotate-left"></i></span>
      <span class="history-info">
        <div class="history-host">${escapeText(c.username || 'user')}@${escapeText(c.host)}:${c.port}</div>
        <div class="history-time">${formatTime(String(c.time))}</div>
      </span>
      <button class="history-reconnect" data-idx="${i}" title="连接"><i class="fa-solid fa-plug"></i></button>
    </div>
  `).join('');

  list.querySelectorAll('.history-entry').forEach((el) => {
    el.addEventListener('click', (e) => {
      if ((e.target as HTMLElement).closest('.history-reconnect')) return;
      const idx = parseInt((el as HTMLElement).dataset.idx!);
      const c = connections[idx];
      if (c) {
        (($('hostInput') as HTMLInputElement)).value = c.host;
        (($('portInput') as HTMLInputElement)).value = c.port;
        (($('userInput') as HTMLInputElement)).value = c.username;
        state.token = c.token || '';
        storeToken(state.token);
        createTab(c.host, c.port, c.username);
      }
    });
  });
  list.querySelectorAll('.history-reconnect').forEach((btn) => {
    btn.addEventListener('click', (e) => {
      e.stopPropagation();
      const idx = parseInt((btn as HTMLElement).dataset.idx!);
      const c = connections[idx];
      if (c) {
        state.token = c.token || '';
        storeToken(state.token);
        createTab(c.host, c.port, c.username);
      }
    });
  });
}

function refreshSessionSidebar(): void {
  renderActiveSessions();
  renderHistorySessions();
  renderTemplateSessions();
}

// =========================== 模板渲染 ===========================

function renderTemplateSessions(): void {
  const list = $('templateSessionList');
  if (!list) return;
  const templates = loadTemplates();
  if (templates.length === 0) {
    list.innerHTML = `<div class="sidebar-empty">${t('noConnectionHistory')}</div>`;
    return;
  }
  list.innerHTML = templates.map((c, i) => `
    <div class="template-entry" data-idx="${i}">
      <span class="history-icon"><i class="fa-solid fa-bookmark"></i></span>
      <span class="history-info">
        <div class="history-host">${escapeText(c.name || `${c.username}@${c.host}`)}</div>
        <div class="history-time">${escapeText(c.username || 'user')}@${escapeText(c.host)}:${c.port}</div>
      </span>
      <button class="history-reconnect" data-idx="${i}" title="连接"><i class="fa-solid fa-plug"></i></button>
      <button class="template-delete" data-idx="${i}" title="删除"><i class="fa-solid fa-xmark"></i></button>
    </div>
  `).join('');

  list.querySelectorAll('.template-entry').forEach((el) => {
    el.addEventListener('click', (e) => {
      if ((e.target as HTMLElement).closest('.history-reconnect, .template-delete')) return;
      const idx = parseInt((el as HTMLElement).dataset.idx!);
      const c = templates[idx];
      if (c) {
        (($('hostInput') as HTMLInputElement)).value = c.host;
        (($('portInput') as HTMLInputElement)).value = c.port;
        (($('userInput') as HTMLInputElement)).value = c.username;
        state.token = c.token || '';
        storeToken(state.token);
        createTab(c.host, c.port, c.username);
      }
    });
  });
  list.querySelectorAll('.history-reconnect').forEach((btn) => {
    btn.addEventListener('click', (e) => {
      e.stopPropagation();
      const idx = parseInt((btn as HTMLElement).dataset.idx!);
      const c = templates[idx];
      if (c) {
        state.token = c.token || '';
        storeToken(state.token);
        createTab(c.host, c.port, c.username);
      }
    });
  });
  list.querySelectorAll('.template-delete').forEach((btn) => {
    btn.addEventListener('click', (e) => {
      e.stopPropagation();
      const idx = parseInt((btn as HTMLElement).dataset.idx!);
      deleteTemplate(idx);
    });
  });
}

// =========================== P7-1: 共享会话聊天 ===========================

interface ChatMessage {
  user: string;
  text: string;
  time: number;
  system?: boolean;
}

const chatMessages: ChatMessage[] = [];

function toggleChatPanel(): void {
  const panel = $('chatPanel');
  if (panel) panel.classList.toggle('hidden');
}

function sendChatMessage(): void {
  const input = $('chatInput') as HTMLInputElement;
  if (!input || !input.value.trim()) return;

  const msg: ChatMessage = {
    user: '我', text: input.value.trim(), time: Date.now(),
  };
  chatMessages.push(msg);
  input.value = '';
  renderChatMessages();

  const tabData = state.tabs.get(state.currentTab || '');
  if (tabData && tabData.ws && tabData.ws.readyState === WebSocket.OPEN) {
    tabData.ws.send(JSON.stringify({
      action: 'chat', sessionId: tabData.sessionId, text: msg.text,
    }));
  }
}

function addSystemChatMessage(text: string): void {
  chatMessages.push({ user: '', text, time: Date.now(), system: true });
  renderChatMessages();
}

function renderChatMessages(): void {
  const container = $('chatMessages');
  if (!container) return;
  container.innerHTML = chatMessages.map((m) => {
    if (m.system) return `<div class="chat-msg system">${escapeText(m.text)}</div>`;
    return `<div class="chat-msg"><span class="chat-user">${escapeText(m.user)}</span><span class="chat-time">${formatTime(new Date(m.time).toISOString())}</span><br><span class="chat-text">${escapeText(m.text)}</span></div>`;
  }).join('');
  container.scrollTop = container.scrollHeight;
}

function updateViewerCount(count: number): void {
  const el = $('viewerCount');
  if (el) el.textContent = count > 0 ? `${count} 位观众` : '';
}

function handleSharingMessage(msg: any): void {
  if (msg.type === 'chat_message') {
    chatMessages.push({ user: msg.user || 'viewer', text: msg.text || '', time: Date.now() });
    renderChatMessages();
  } else if (msg.type === 'viewer_joined') {
    addSystemChatMessage(`${msg.user || '用户'} 加入会话`);
    updateViewerCount(msg.viewerCount || 0);
  } else if (msg.type === 'viewer_left') {
    addSystemChatMessage(`${msg.user || '用户'} 离开会话`);
    updateViewerCount(msg.viewerCount || 0);
  } else if (msg.type === 'role_changed') {
    addSystemChatMessage(`${msg.user || '用户'} 角色变更为 ${msg.role || 'viewer'}`);
  } else if (msg.type === 'share_started') {
    addSystemChatMessage(`会话已共享，邀请码: ${msg.shareCode || 'N/A'}`);
    const chatBtn = $('toggleChat');
    if (chatBtn) chatBtn.style.display = '';
  } else if (msg.type === 'share_stopped') {
    addSystemChatMessage('共享已停止');
    const chatBtn = $('toggleChat');
    if (chatBtn) chatBtn.style.display = 'none';
  }
}

// =========================== 笔记系统 ===========================

function createSnippet(title: string, content: string): void {
  const snippet: Snippet = {
    id: 'snp_' + Date.now(),
    title: title || '未命名笔记',
    content: sanitizeSnippetContent(content || ''),
    createdAt: new Date().toISOString(),
    updatedAt: new Date().toISOString(),
  };
  state.snippets.push(snippet);
  saveSnippets();
  renderSnippetList();
  selectSnippet(snippet.id);
  updateStatus();
}

function selectSnippet(id: string): void {
  state.currentSnippetId = id;
  const snippet = state.snippets.find((s) => s.id === id);
  if (!snippet) return;

  (($('snippetTitle') as HTMLInputElement)).value = snippet.title;
  const content = $('snippetContent');
  if (content) content.innerHTML = sanitizeSnippetContent(snippet.content);
  const deleteBtn = $('deleteSnippetBtn');
  if (deleteBtn) deleteBtn.style.display = '';

  document.querySelectorAll('.snippet-list-item').forEach((el) => {
    el.classList.toggle('active', (el as HTMLElement).dataset.snippetId === id);
  });
}

function saveCurrentSnippet(): void {
  if (!state.currentSnippetId) {
    const title = ($('snippetTitle') as HTMLInputElement)?.value || '未命名笔记';
    const content = $('snippetContent')?.innerHTML || '';
    createSnippet(title, content);
    return;
  }

  const snippet = state.snippets.find((s) => s.id === state.currentSnippetId);
  if (!snippet) return;

  snippet.title = ($('snippetTitle') as HTMLInputElement)?.value || '未命名笔记';
  snippet.content = sanitizeSnippetContent($('snippetContent')?.innerHTML || '');
  snippet.updatedAt = new Date().toISOString();
  saveSnippets();
  renderSnippetList();
}

function deleteCurrentSnippet(): void {
  if (!state.currentSnippetId) return;
  if (!confirm('确认删除这条笔记？')) return;

  state.snippets = state.snippets.filter((s) => s.id !== state.currentSnippetId);
  saveSnippets();

  state.currentSnippetId = null;
  (($('snippetTitle') as HTMLInputElement)).value = '';
  const content = $('snippetContent');
  if (content) content.innerHTML = '';
  const deleteBtn = $('deleteSnippetBtn');
  if (deleteBtn) deleteBtn.style.display = 'none';
  renderSnippetList();
  updateStatus();
}

function renderSnippetList(): void {
  const list = $('snippetList');
  if (!list) return;
  list.innerHTML = state.snippets.length === 0
    ? '<div style="padding:14px;color:var(--overlay);font-size:12px;text-align:center">暂无笔记</div>'
    : state.snippets.map((s) => `
      <div class="snippet-list-item" data-snippet-id="${s.id}">
        <span>${escapeText(s.title)}</span>
        <span class="snippet-time">${formatTime(s.updatedAt)}</span>
      </div>
    `).join('');

  list.querySelectorAll('.snippet-list-item').forEach((el) => {
    el.addEventListener('click', () => selectSnippet((el as HTMLElement).dataset.snippetId!));
  });
}

function createSnippetFromSelection(): void {
  const tabData = state.tabs.get(state.currentTab || '');
  if (!tabData) return;
  const text = tabData.terminal.getSelection();
  if (!text) return;
  createSnippet('终端摘录', `<pre>${escapeText(text)}</pre>`);
  const panel = $('snippetPanel');
  if (panel) panel.classList.remove('hidden');
}

// =========================== 工具栏 ===========================

function execToolbarCmd(cmd: string, arg?: string): void {
  const content = $('snippetContent');
  if (content) content.focus();
  document.execCommand(cmd, false, arg ?? undefined);
}

// =========================== P0-6: 未保存确认 ===========================

let hasUnsavedSnippet = false;

function markSnippetDirty(): void { hasUnsavedSnippet = true; }
function markSnippetClean(): void { hasUnsavedSnippet = false; }

window.addEventListener('beforeunload', (e: BeforeUnloadEvent) => {
  if (hasUnsavedSnippet) {
    e.preventDefault();
    e.returnValue = t('unsavedConfirm');
    return e.returnValue;
  }
});

// =========================== 设置持久化 (已移到 state 之前) ===========================

function exportData(): void {
  const data = {
    settings: loadSettings(),
    connections: loadConnections(),
    snippets: state.snippets,
    exportedAt: new Date().toISOString(),
  };
  const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = `windterm-backup-${new Date().toISOString().slice(0, 10)}.json`;
  a.click();
  URL.revokeObjectURL(url);
}

// =========================== P6-1: 命令面板 ===========================

interface PaletteCommand {
  id: string;
  label: string;
  shortcut?: string;
  action: () => void;
}

const paletteCommands: PaletteCommand[] = [
  { id: 'connect', label: '连接终端', shortcut: '', action: () => ($('connectBtn') as HTMLButtonElement)?.click() },
  { id: 'newTab', label: '新建标签', shortcut: 'Ctrl+T', action: () => createTab(
    ($('hostInput') as HTMLInputElement)?.value || 'localhost',
    ($('portInput') as HTMLInputElement)?.value || '22',
    ($('userInput') as HTMLInputElement)?.value || 'root',
  ) },
  { id: 'toggleSessions', label: '切换会话管理面板', action: () => $('sessionSidebar')?.classList.toggle('hidden') },
  { id: 'toggleSnippets', label: '切换笔记面板', shortcut: 'Ctrl+B', action: () => $('snippetPanel')?.classList.toggle('hidden') },
  { id: 'toggleTheme', label: '切换深色/亮色主题', action: toggleTheme },
  { id: 'toggleFullscreen', label: '切换全屏', shortcut: 'F11', action: toggleFullscreen },
  { id: 'toggleLang', label: '切换语言', action: () => { toggleLanguage(); updateStatus(); refreshSessionSidebar(); } },
  { id: 'search', label: '终端搜索', shortcut: 'Ctrl+Shift+F', action: toggleSearchBar },
  { id: 'zoomIn', label: '放大字体', shortcut: 'Ctrl+=', action: zoomIn },
  { id: 'zoomOut', label: '缩小字体', shortcut: 'Ctrl+-', action: zoomOut },
  { id: 'resetZoom', label: '重置字体大小', shortcut: 'Ctrl+0', action: resetZoom },
  { id: 'export', label: '导出终端内容', action: exportTerminalContent },
  { id: 'saveTemplate', label: '保存为配置模板', action: saveAsTemplate },
  { id: 'shortcuts', label: '显示快捷键帮助', shortcut: 'Ctrl+/', action: toggleShortcutsDialog },
  { id: 'toggleChat', label: '切换聊天面板', shortcut: 'Ctrl+Shift+C', action: toggleChatPanel },
  { id: 'closeTab', label: '关闭当前标签', shortcut: 'Ctrl+W', action: () => { if (state.currentTab) closeTab(state.currentTab); } },
];

let paletteActiveIndex = 0;

function toggleCommandPalette(): void {
  const palette = $('commandPalette');
  if (!palette) return;
  palette.classList.toggle('hidden');
  const input = $('paletteInput') as HTMLInputElement;
  if (!palette.classList.contains('hidden') && input) {
    input.value = '';
    input.focus();
    paletteActiveIndex = 0;
    filterCommands('');
  }
}

function filterCommands(query: string): void {
  const results = $('paletteResults');
  if (!results) return;

  const q = query.toLowerCase().trim();
  const filtered = q ? paletteCommands.filter((c) => c.label.toLowerCase().includes(q) || c.id.includes(q)) : paletteCommands;

  if (filtered.length === 0) {
    results.innerHTML = '<div class="palette-empty">无匹配命令</div>';
    return;
  }

  paletteActiveIndex = Math.min(paletteActiveIndex, filtered.length - 1);
  results.innerHTML = filtered.map((cmd, i) => `
    <div class="palette-item${i === paletteActiveIndex ? ' active' : ''}" data-cmd-id="${cmd.id}">
      <span class="palette-label">${escapeText(cmd.label)}</span>
      ${cmd.shortcut ? `<span class="palette-shortcut">${cmd.shortcut.replace(/\+/g, '+').split('+').map((k) => `<kbd>${k}</kbd>`).join('')}</span>` : ''}
    </div>
  `).join('');

  results.querySelectorAll('.palette-item').forEach((el) => {
    el.addEventListener('click', () => {
      const cmd = paletteCommands.find((c) => c.id === (el as HTMLElement).dataset.cmdId);
      if (cmd) { cmd.action(); toggleCommandPalette(); }
    });
  });
}

function executeActivePaletteCommand(): void {
  const results = $('paletteResults');
  if (!results) return;
  const active = results.querySelector('.palette-item.active') as HTMLElement;
  if (!active) return;
  const cmd = paletteCommands.find((c) => c.id === active.dataset.cmdId);
  if (cmd) { cmd.action(); toggleCommandPalette(); }
}

// =========================== P6-2: PWA ===========================
if ('serviceWorker' in navigator) {
  window.addEventListener('load', () => {
    navigator.serviceWorker.register('/sw.js').catch(() => {});
  });
}
window.addEventListener('unload', () => {
  state.tabs.forEach((tab) => {
    if (tab.reconnectTimer) clearTimeout(tab.reconnectTimer);
    if (tab.heartbeatTimer) clearInterval(tab.heartbeatTimer);
    if (tab.heartbeatTimeout) clearTimeout(tab.heartbeatTimeout);
  });
});

// =========================== 事件初始化 ===========================

function initApp(): void {
  state.token = getToken();
  loadSnippets();
  renderSnippetList();
  updateStatus();
  refreshSessionSidebar();
  initTheme();

  // P5-2: 恢复上次会话
  setTimeout(() => restoreLastSessions(), 500);

  const savedConn = loadConnections();
  if (savedConn.length > 0) {
    (($('hostInput') as HTMLInputElement)).value = savedConn[0].host || '';
    (($('portInput') as HTMLInputElement)).value = savedConn[0].port || '22';
    (($('userInput') as HTMLInputElement)).value = savedConn[0].username || '';
    state.token = savedConn[0].token || '';
  }

  // 连接按钮
  $('connectBtn')?.addEventListener('click', () => {
    const host = ($('hostInput') as HTMLInputElement)?.value || 'localhost';
    const port = ($('portInput') as HTMLInputElement)?.value || '22';
    const username = ($('userInput') as HTMLInputElement)?.value || 'root';
    state.token = ($('tokenInput') as HTMLInputElement)?.value || '';
    storeToken(state.token);
    addConnection(host, port, username, state.token);
    createTab(host, port, username);
  });

  // P5-1: 保存为配置模板
  $('saveTemplateBtn')?.addEventListener('click', saveAsTemplate);

  // P6-1: 命令面板
  $('commandPalette')?.querySelector('.dialog-overlay')?.addEventListener('click', toggleCommandPalette);
  $('paletteInput')?.addEventListener('input', (e) => {
    filterCommands((e.target as HTMLInputElement).value);
  });
  $('paletteInput')?.addEventListener('keydown', (e) => {
    const kb = e as KeyboardEvent;
    const results = $('paletteResults');
    if (!results) return;
    const items = results.querySelectorAll('.palette-item');
    if (kb.key === 'ArrowDown') {
      kb.preventDefault();
      paletteActiveIndex = Math.min(paletteActiveIndex + 1, items.length - 1);
      filterCommands(($('paletteInput') as HTMLInputElement)?.value || '');
    } else if (kb.key === 'ArrowUp') {
      kb.preventDefault();
      paletteActiveIndex = Math.max(paletteActiveIndex - 1, 0);
      filterCommands(($('paletteInput') as HTMLInputElement)?.value || '');
    } else if (kb.key === 'Enter') {
      kb.preventDefault();
      executeActivePaletteCommand();
    } else if (kb.key === 'Escape') {
      toggleCommandPalette();
    }
  });

  // 回车连接
  ['hostInput', 'portInput', 'userInput', 'tokenInput'].forEach((id) => {
    $(id)?.addEventListener('keydown', (e) => {
      if ((e as KeyboardEvent).key === 'Enter') $('connectBtn')?.click();
    });
  });

  // 新建标签
  $('newTabBtn')?.addEventListener('click', () => {
    createTab(
      ($('hostInput') as HTMLInputElement)?.value || 'localhost',
      ($('portInput') as HTMLInputElement)?.value || '22',
      ($('userInput') as HTMLInputElement)?.value || 'root',
    );
  });

  // 面板切换
  $('toggleSessions')?.addEventListener('click', () => {
    $('sessionSidebar')?.classList.toggle('hidden');
    refreshSessionSidebar();
  });
  $('closeSessionSidebarBtn')?.addEventListener('click', () => {
    $('sessionSidebar')?.classList.add('hidden');
  });
  $('toggleSnippets')?.addEventListener('click', () => {
    $('snippetPanel')?.classList.toggle('hidden');
  });
  $('closeSnippetsBtn')?.addEventListener('click', () => {
    $('snippetPanel')?.classList.add('hidden');
  });

  // P7-1: 聊天面板
  $('toggleChat')?.addEventListener('click', toggleChatPanel);
  $('closeChatBtn')?.addEventListener('click', toggleChatPanel);
  $('sendChatBtn')?.addEventListener('click', sendChatMessage);
  $('chatInput')?.addEventListener('keydown', (e) => {
    if ((e as KeyboardEvent).key === 'Enter') { e.preventDefault(); sendChatMessage(); }
  });

  // P2-2: 主题/全屏/语言切换
  $('toggleTheme')?.addEventListener('click', toggleTheme);
  $('toggleFullscreen')?.addEventListener('click', toggleFullscreen);
  $('toggleLang')?.addEventListener('click', () => {
    toggleLanguage();
    updateStatus();
    refreshSessionSidebar();
  });

  // P3-1: 搜索栏
  $('searchInput')?.addEventListener('input', performSearch);
  $('searchInput')?.addEventListener('keydown', (e) => {
    const kb = e as KeyboardEvent;
    if (kb.key === 'Enter') { kb.preventDefault(); kb.shiftKey ? searchPrev() : searchNext(); }
    if (kb.key === 'Escape') { toggleSearchBar(); }
  });
  $('searchNextBtn')?.addEventListener('click', searchNext);
  $('searchPrevBtn')?.addEventListener('click', searchPrev);
  $('searchCloseBtn')?.addEventListener('click', toggleSearchBar);

  // P3-3: 快捷键对话框
  $('closeShortcutsBtn')?.addEventListener('click', toggleShortcutsDialog);
  $('shortcutsDialog')?.querySelector('.dialog-overlay')?.addEventListener('click', toggleShortcutsDialog);

  // P4-2: 标签右键菜单处理
  document.addEventListener('click', (e) => {
    if (!(e.target as HTMLElement).closest('#tabContextMenu')) hideTabContextMenu();
  });
  document.querySelectorAll('#tabContextMenu .menu-item').forEach((item) => {
    item.addEventListener('click', (e) => {
      e.stopPropagation();
      const action = (item as HTMLElement).dataset.action;
      if (!action || !tabContextMenuTarget) { hideTabContextMenu(); return; }
      switch (action) {
        case 'close': closeTab(tabContextMenuTarget); break;
        case 'closeOthers': closeOtherTabs(tabContextMenuTarget); break;
        case 'closeAll': closeAllTabs(); break;
        case 'duplicate': duplicateTab(tabContextMenuTarget); break;
        case 'rename': triggerTabRename(tabContextMenuTarget); break;
      }
      hideTabContextMenu();
    });
  });

  // 笔记操作
  $('newSnippetBtn')?.addEventListener('click', () => {
    state.currentSnippetId = null;
    (($('snippetTitle') as HTMLInputElement)).value = '';
    const content = $('snippetContent');
    if (content) content.innerHTML = '';
    const deleteBtn = $('deleteSnippetBtn');
    if (deleteBtn) deleteBtn.style.display = 'none';
    markSnippetClean();
  });
  $('saveSnippetBtn')?.addEventListener('click', () => {
    saveCurrentSnippet();
    markSnippetClean();
  });
  $('deleteSnippetBtn')?.addEventListener('click', deleteCurrentSnippet);

  // 笔记内容更改检测
  $('snippetContent')?.addEventListener('input', markSnippetDirty);
  $('snippetTitle')?.addEventListener('input', markSnippetDirty);

  // Ctrl+S 保存
  $('snippetContent')?.addEventListener('keydown', (e) => {
    if (((e as KeyboardEvent).ctrlKey || (e as KeyboardEvent).metaKey) && (e as KeyboardEvent).key === 's') {
      e.preventDefault();
      saveCurrentSnippet();
      markSnippetClean();
    }
  });

  // 自动保存
  $('snippetContent')?.addEventListener('input', () => {
    if (state.autosaveTimer) clearTimeout(state.autosaveTimer);
    state.autosaveTimer = window.setTimeout(() => {
      if (hasUnsavedSnippet && state.currentSnippetId) {
        saveCurrentSnippet();
        markSnippetClean();
      }
    }, 5000);
  });

  // 工具栏
  document.querySelectorAll('.btn-toolbar').forEach((btn) => {
    btn.addEventListener('click', () => execToolbarCmd(
      (btn as HTMLElement).dataset.cmd!,
      (btn as HTMLElement).dataset.arg,
    ));
    btn.addEventListener('mousedown', (e) => e.preventDefault());
  });

  // 右键菜单
  document.addEventListener('click', () => {
    $('contextMenu')?.classList.add('hidden');
  });

  document.querySelectorAll('.menu-item').forEach((item) => {
    item.addEventListener('click', (e) => {
      e.stopPropagation();
      $('contextMenu')?.classList.add('hidden');
      const action = (item as HTMLElement).dataset.action;
      const tabData = state.tabs.get(state.currentTab || '');

      switch (action) {
        case 'copy':
          if (tabData) {
            const text = tabData.terminal.getSelection();
            if (text) navigator.clipboard.writeText(text).catch(() => {});
          }
          break;
        case 'paste':
          navigator.clipboard.readText().then((text) => {
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
        case 'export':
          exportTerminalContent();
          break;
      }
    });
  });

  // 全局快捷键
  document.addEventListener('keydown', (e) => {
    const kb = e as KeyboardEvent;
    if ((kb.ctrlKey || kb.metaKey) && kb.shiftKey && kb.key === 'C') {
      const tabData = state.tabs.get(state.currentTab || '');
      if (tabData) {
        const text = tabData.terminal.getSelection();
        if (text) navigator.clipboard.writeText(text).catch(() => {});
      }
    }
    if ((kb.ctrlKey || kb.metaKey) && kb.shiftKey && kb.key === 'V') {
      navigator.clipboard.readText().then((text) => {
        const tabData = state.tabs.get(state.currentTab || '');
        if (tabData && tabData.ws && tabData.ws.readyState === WebSocket.OPEN) {
          tabData.ws.send(JSON.stringify({ action: 'input', sessionId: tabData.sessionId, input: text }));
        }
      }).catch(() => {});
    }
    if ((kb.ctrlKey || kb.metaKey) && kb.key === 'b') {
      kb.preventDefault();
      $('snippetPanel')?.classList.toggle('hidden');
    }
    if ((kb.ctrlKey || kb.metaKey) && kb.key === 't' && !kb.shiftKey) {
      kb.preventDefault();
      $('newTabBtn')?.click();
    }
    if ((kb.ctrlKey || kb.metaKey) && kb.key === 'w') {
      kb.preventDefault();
      if (state.currentTab) closeTab(state.currentTab);
    }
    if ((kb.ctrlKey || kb.metaKey) && kb.key >= '1' && kb.key <= '9') {
      kb.preventDefault();
      const idx = parseInt(kb.key) - 1;
      const tabIds = [...state.tabs.keys()];
      if (idx < tabIds.length) switchTab(tabIds[idx]);
    }
    // P3-1: Ctrl+Shift+F 搜索
    if ((kb.ctrlKey || kb.metaKey) && kb.shiftKey && kb.key === 'F') {
      kb.preventDefault();
      toggleSearchBar();
    }
    // P3-3: Ctrl+/ 快捷键帮助
    if ((kb.ctrlKey || kb.metaKey) && kb.key === '/') {
      kb.preventDefault();
      toggleShortcutsDialog();
    }
    // F11 全屏
    if (kb.key === 'F11') {
      kb.preventDefault();
      toggleFullscreen();
    }
    // P4-1: 字体缩放
    if ((kb.ctrlKey || kb.metaKey) && (kb.key === '=' || kb.key === '+')) {
      kb.preventDefault();
      zoomIn();
    }
    if ((kb.ctrlKey || kb.metaKey) && kb.key === '-') {
      kb.preventDefault();
      zoomOut();
    }
    if ((kb.ctrlKey || kb.metaKey) && kb.key === '0') {
      kb.preventDefault();
      resetZoom();
    }
    // P6-1: 命令面板
    if ((kb.ctrlKey || kb.metaKey) && kb.shiftKey && kb.key === 'P') {
      kb.preventDefault();
      toggleCommandPalette();
    }
    // P7-1: 聊天面板
    if ((kb.ctrlKey || kb.metaKey) && kb.shiftKey && kb.key === 'C') {
      kb.preventDefault();
      toggleChatPanel();
    }
  });

  // 窗口大小调整
  window.addEventListener('resize', throttle(() => {
    const tab = state.tabs.get(state.currentTab || '');
    if (tab) try { tab.fitAddon.fit(); } catch { /* ignore */ }
  }, 100));

  // 网络状态变化时自动尝试重连
  window.addEventListener('online', () => {
    state.tabs.forEach((tab, id) => {
      if (tab.status === 'disconnected') connectTab(id);
    });
  });
}

// 模块脚本延迟执行,DOMContentLoaded 已经触发,直接初始化
if (document.readyState !== 'loading') {
  initApp();
} else {
  document.addEventListener('DOMContentLoaded', initApp);
}

console.log('WindTerm AI Web Client (TypeScript Production) 已就绪');

