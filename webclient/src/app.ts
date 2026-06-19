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
    savedTemplates: '配置模板',
    chatTitle: '会话聊天',
    connectHint: '在顶部输入主机地址、端口和认证令牌，然后点击连接',
    saveTemplate: '保存为配置模板',
    newSnippet: '新建',
    toolbarBold: '粗体',
    toolbarItalic: '斜体',
    toolbarUnderline: '下划线',
    toolbarStrikethrough: '删除线',
    hintEnterHost: '输入目标主机',
    hintEnterToken: '输入认证令牌',
    hintClickConnect: '点击连接建立会话',
    saveSnippet: '保存',
    shortcutsDialogTitle: '快捷键',
    contextCopy: '复制',
    contextPaste: '粘贴',
    contextClear: '清除终端',
    contextSaveSnippet: '保存为笔记',
    contextExport: '导出终端内容',
    tabClose: '关闭标签',
    tabCloseOthers: '关闭其他',
    tabCloseAll: '关闭全部',
    tabDuplicate: '复制标签',
    tabRename: '重命名',
    connect: '连接',
    hostPlaceholder: '主机',
    portPlaceholder: '端口',
    userPlaceholder: '用户',
    tokenPlaceholder: '令牌',
    hostTitle: '主机地址',
    portTitle: '端口',
    userTitle: '用户名',
    tokenTitle: '认证令牌',
    searchPlaceholder: '搜索...',
    palettePlaceholder: '输入命令...',
    chatPlaceholder: '输入消息...',
    noteTitlePlaceholder: '笔记标题...',
    noteContentPlaceholder: '在此输入笔记内容...',
    shortsTabGroup: '标签页',
    shortsNewTab: '新建标签',
    shortsCloseTab: '关闭标签',
    shortsSwitchTab: '切换到第N个标签',
    shortsTermGroup: '终端',
    shortsCopy: '复制选中文本',
    shortsPaste: '粘贴到终端',
    shortsSearch: '终端搜索',
    shortsUiGroup: '界面',
    shortsToggleNotes: '切换笔记面板',
    shortsSaveNote: '保存当前笔记',
    shortsShowShortcuts: '显示快捷键',
    shortsFullscreen: '全屏切换',
    paletteConnect: '连接终端',
    paletteNewTab: '新建标签',
    paletteToggleSessions: '切换会话管理面板',
    paletteToggleSnippets: '切换笔记面板',
    paletteToggleTheme: '切换深色/亮色主题',
    paletteToggleFullscreen: '切换全屏',
    paletteToggleLang: '切换语言',
    paletteSearch: '终端搜索',
    paletteZoomIn: '放大字体',
    paletteZoomOut: '缩小字体',
    paletteResetZoom: '重置字体大小',
    paletteExport: '导出终端内容',
    paletteExportBackup: '导出数据备份',
    paletteImportBackup: '导入数据备份',
    paletteSaveTemplate: '保存为配置模板',
    paletteShortcuts: '显示快捷键帮助',
    paletteToggleChat: '切换聊天面板',
    paletteCloseTab: '关闭当前标签',
    paletteNoMatch: '无匹配命令',
    diagOffline: '网络断开',
    diagOfflineDetail: '设备未连接到网络，请检查网络设置。',
    diagAbnormalClose: '连接异常关闭',
    diagAbnormalCloseDetail: '无法连接到 {0}:{1}。请检查:</br>1. 服务器是否运行</br>2. 防火墙是否放行端口 {1}</br>3. 主机地址是否正确',
    diagServerClose: '服务端关闭连接',
    diagServerCloseDetail: '服务器主动关闭了连接，可能是会话超时。',
    diagProtocolMismatch: '协议不匹配',
    diagProtocolMismatchDetail: '当前页面使用 HTTPS，但 WebSocket 目标为 {0}。浏览器会阻止不安全连接。请使用 wss:// 协议。',
    diagConnectFail: '连接失败',
    diagConnectFailDetail: '无法建立 WebSocket 连接到 {0}:{1}。<br/>请检查主机地址和端口是否正确。',
    close: '关闭',
    promptTemplateName: '配置名称:',
    restoreSessionsConfirm: '检测到上次有 {0} 个活跃会话，是否恢复连接？',
    unknown: '未知',
    unassigned: '未分配',
    disconnect: '断开',
    delete: '删除',
    me: '我',
    viewers: '{0} 位观众',
    anonymousUser: '用户',
    joinedSession: '{0} 加入会话',
    leftSession: '{0} 离开会话',
    roleChanged: '{0} 角色变更为 {1}',
    sessionShared: '会话已共享，邀请码: {0}',
    shareStopped: '共享已停止',
    importFailed: '导入失败：文件格式无效',
    deleteSnippetConfirm: '确认删除这条笔记？',
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
    savedTemplates: 'Templates',
    chatTitle: 'Chat',
    connectHint: 'Enter host address, port, and token above, then click Connect',
    saveTemplate: 'Save Template',
    newSnippet: 'New',
    toolbarBold: 'Bold',
    toolbarItalic: 'Italic',
    toolbarUnderline: 'Underline',
    toolbarStrikethrough: 'Strikethrough',
    hintEnterHost: 'Enter target host',
    hintEnterToken: 'Enter auth token',
    hintClickConnect: 'Click Connect to start session',
    saveSnippet: 'Save',
    shortcutsDialogTitle: 'Shortcuts',
    contextCopy: 'Copy',
    contextPaste: 'Paste',
    contextClear: 'Clear Terminal',
    contextSaveSnippet: 'Save as Note',
    contextExport: 'Export Content',
    tabClose: 'Close Tab',
    tabCloseOthers: 'Close Others',
    tabCloseAll: 'Close All',
    tabDuplicate: 'Duplicate Tab',
    tabRename: 'Rename',
    connect: 'Connect',
    hostPlaceholder: 'Host',
    portPlaceholder: 'Port',
    userPlaceholder: 'User',
    tokenPlaceholder: 'Token',
    hostTitle: 'Host Address',
    portTitle: 'Port',
    userTitle: 'Username',
    tokenTitle: 'Auth Token',
    searchPlaceholder: 'Search...',
    palettePlaceholder: 'Type a command...',
    chatPlaceholder: 'Type a message...',
    noteTitlePlaceholder: 'Note title...',
    noteContentPlaceholder: 'Type your note...',
    shortsTabGroup: 'Tabs',
    shortsNewTab: 'New Tab',
    shortsCloseTab: 'Close Tab',
    shortsSwitchTab: 'Switch to Tab N',
    shortsTermGroup: 'Terminal',
    shortsCopy: 'Copy Selection',
    shortsPaste: 'Paste to Terminal',
    shortsSearch: 'Search in Terminal',
    shortsUiGroup: 'Interface',
    shortsToggleNotes: 'Toggle Notes Panel',
    shortsSaveNote: 'Save Current Note',
    shortsShowShortcuts: 'Show Shortcuts',
    shortsFullscreen: 'Toggle Fullscreen',
    paletteConnect: 'Connect to Terminal',
    paletteNewTab: 'New Tab',
    paletteToggleSessions: 'Toggle Session Panel',
    paletteToggleSnippets: 'Toggle Notes Panel',
    paletteToggleTheme: 'Toggle Dark/Light Theme',
    paletteToggleFullscreen: 'Toggle Fullscreen',
    paletteToggleLang: 'Toggle Language',
    paletteSearch: 'Search in Terminal',
    paletteZoomIn: 'Zoom In',
    paletteZoomOut: 'Zoom Out',
    paletteResetZoom: 'Reset Font Size',
    paletteExport: 'Export Terminal Content',
    paletteExportBackup: 'Export Data Backup',
    paletteImportBackup: 'Import Data Backup',
    paletteSaveTemplate: 'Save as Template',
    paletteShortcuts: 'Show Shortcuts Help',
    paletteToggleChat: 'Toggle Chat Panel',
    paletteCloseTab: 'Close Current Tab',
    paletteNoMatch: 'No matching commands',
    diagOffline: 'Network Offline',
    diagOfflineDetail: 'Device is not connected to the network. Please check your network settings.',
    diagAbnormalClose: 'Connection Abnormally Closed',
    diagAbnormalCloseDetail: 'Cannot connect to {0}:{1}. Please check:</br>1. Whether the server is running</br>2. Whether firewall allows port {1}</br>3. Whether the host address is correct',
    diagServerClose: 'Server Closed Connection',
    diagServerCloseDetail: 'The server closed the connection, possibly due to session timeout.',
    diagProtocolMismatch: 'Protocol Mismatch',
    diagProtocolMismatchDetail: 'The current page uses HTTPS, but the WebSocket target is {0}. Browsers block insecure connections. Please use wss:// protocol.',
    diagConnectFail: 'Connection Failed',
    diagConnectFailDetail: 'Cannot establish WebSocket connection to {0}:{1}.<br/>Please check the host address and port are correct.',
    close: 'Close',
    promptTemplateName: 'Template name:',
    restoreSessionsConfirm: 'Found {0} active sessions from last time. Reconnect?',
    unknown: 'Unknown',
    unassigned: 'Unassigned',
    disconnect: 'Disconnect',
    delete: 'Delete',
    me: 'Me',
    viewers: '{0} viewers',
    anonymousUser: 'User',
    joinedSession: '{0} joined the session',
    leftSession: '{0} left the session',
    roleChanged: '{0} role changed to {1}',
    sessionShared: 'Session shared, invite code: {0}',
    shareStopped: 'Sharing stopped',
    importFailed: 'Import failed: Invalid file format',
    deleteSnippetConfirm: 'Delete this note?',
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
  refreshAllUI();
}

function refreshAllUI(): void {
  updateStatus();
  refreshSessionSidebar();
  // 更新侧边栏标题
  const headers = document.querySelectorAll('#sessionSidebar .sidebar-section-title');
  if (headers[0]) headers[0].textContent = t('activeSessions');
  if (headers[1]) headers[1].textContent = t('connectionHistory');
  if (headers[2]) headers[2].textContent = t('savedTemplates');
  // 更新会话管理面板标题
  const sessionTitle = document.querySelector('#sessionSidebarHeader h3');
  if (sessionTitle) sessionTitle.innerHTML = `<i class="fa-solid fa-server"></i> ${t('sessionPanel')}`;
  // 更新笔记面板标题
  const snippetTitle = document.querySelector('#snippetHeader h3');
  if (snippetTitle) snippetTitle.innerHTML = `<i class="fa-solid fa-note-sticky"></i> ${t('snippetPanel')}`;
  // 更新聊天面板标题
  const chatTitle = document.querySelector('#chatHeader h3');
  if (chatTitle) chatTitle.innerHTML = `<i class="fa-solid fa-comments"></i> ${t('chatTitle')}`;
  // 更新提示文字
  const hintH2 = document.querySelector('#terminalHint h2');
  if (hintH2) hintH2.textContent = t('disconnected');
  const hintP = document.querySelector('#terminalHint p');
  if (hintP) hintP.textContent = t('connectHint');
  // 更新引导步骤文字
  const hintLabels: Record<string, string> = { host: 'hintEnterHost', token: 'hintEnterToken', connect: 'hintClickConnect' };
  document.querySelectorAll('#terminalHint .hint-item').forEach((el) => {
    const key = (el as HTMLElement).dataset.hint;
    if (key && hintLabels[key]) {
      const i = el.querySelector('i')!;
      const span = el.querySelector('span');
      if (span) span.textContent = t(hintLabels[key]);
    }
  });
  // 更新按钮 tooltip
  const tooltips: Record<string, string> = {
    'toggleSessions': t('sessionPanel'), 'toggleSnippets': t('snippetPanel'),
    'toggleTheme': t('switchTheme'), 'toggleFullscreen': t('fullscreen'),
    'toggleLang': t('switchLang'), 'newTabBtn': t('newTab'),
    'closeSessionSidebarBtn': t('closePanel'), 'closeSnippetsBtn': t('closePanel'),
    'closeChatBtn': t('closePanel'), 'saveTemplateBtn': t('saveTemplate'),
  };
  Object.entries(tooltips).forEach(([id, tip]) => {
    const el = document.getElementById(id);
    if (el) el.title = tip;
  });
  // 更新连接按钮文字
  const connectBtnText = document.getElementById('connectBtnText');
  if (connectBtnText) connectBtnText.textContent = t('connect');
  // 更新新建笔记按钮文字
  const newSnippetBtnText = document.getElementById('newSnippetBtnText');
  if (newSnippetBtnText) newSnippetBtnText.textContent = t('newSnippet');
  // 更新保存笔记按钮文字
  const saveSnippetBtnText = document.getElementById('saveSnippetBtnText');
  if (saveSnippetBtnText) saveSnippetBtnText.textContent = t('saveSnippet');
  // 更新快捷键对话框标题
  const shortcutsTitle = document.getElementById('shortcutsDialogTitle');
  if (shortcutsTitle) shortcutsTitle.textContent = t('shortcutsDialogTitle');
  // 更新全屏按钮 tooltip (根据当前状态)
  const toggleFullscreenBtn = document.getElementById('toggleFullscreen');
  if (toggleFullscreenBtn) toggleFullscreenBtn.title = document.fullscreenElement ? t('exitFullscreen') : t('fullscreen');
  // 更新输入框 placeholder 和 title
  const inputConfigs: Record<string, [string, string]> = {
    hostInput: ['hostPlaceholder', 'hostTitle'],
    portInput: ['portPlaceholder', 'portTitle'],
    userInput: ['userPlaceholder', 'userTitle'],
    tokenInput: ['tokenPlaceholder', 'tokenTitle'],
  };
  Object.entries(inputConfigs).forEach(([id, [ph, tit]]) => {
    const input = document.getElementById(id) as HTMLInputElement | null;
    if (input) {
      input.placeholder = t(ph);
      input.title = t(tit);
    }
  });
  // 更新搜索/命令面板/聊天/笔记 placeholder
  const placeholderConfigs: Record<string, string> = {
    searchInput: 'searchPlaceholder',
    paletteInput: 'palettePlaceholder',
    chatInput: 'chatPlaceholder',
    snippetTitle: 'noteTitlePlaceholder',
  };
  Object.entries(placeholderConfigs).forEach(([id, key]) => {
    const el = document.getElementById(id);
    if (el) (el as HTMLInputElement).placeholder = t(key);
  });
  // 更新笔记内容区 placeholder
  const snippetContent = document.getElementById('snippetContent');
  if (snippetContent) {
    snippetContent.setAttribute('placeholder', t('noteContentPlaceholder'));
    snippetContent.setAttribute('aria-label', t('noteContentPlaceholder'));
  }
  // 更新编辑器工具栏 tooltip
  const toolbarBtns: Record<string, string> = {
    bold: 'toolbarBold', italic: 'toolbarItalic',
    underline: 'toolbarUnderline', strikeThrough: 'toolbarStrikethrough',
  };
  document.querySelectorAll('.btn-toolbar').forEach((btn) => {
    const cmd = (btn as HTMLElement).dataset.cmd;
    if (cmd && toolbarBtns[cmd]) (btn as HTMLElement).title = t(toolbarBtns[cmd]);
  });
  // 更新笔记标题 (仅当仍是默认值时)
  const oldDefaults = ['未命名笔记', 'Untitled Note'];
  const snippetTitleEl = document.getElementById('snippetTitle') as HTMLInputElement | null;
  if (snippetTitleEl && oldDefaults.includes(snippetTitleEl.value)) {
    snippetTitleEl.value = t('untitledNote');
  }
  // 更新快捷键对话框
  renderShortcutsDialog();
  // 更新右键菜单
  updateContextMenuText();
  // 更新标签右键菜单
  updateTabContextMenuText();
  // 更新标签关闭按钮 aria-label
  document.querySelectorAll('.tab-close').forEach((el) => { el.setAttribute('aria-label', t('tabClose')); });
  // 更新笔记列表 (重新渲染以刷新标题和 time)
  renderSnippetList();
  if (state.currentSnippetId) {
    const activeEl = document.querySelector(`.snippet-list-item[data-snippet-id="${state.currentSnippetId}"]`);
    if (activeEl) activeEl.classList.add('active');
  }
}

function updateContextMenuText(): void {
  const items = document.querySelectorAll('#contextMenu .menu-item');
  const labels: Record<string, string> = {
    copy: t('contextCopy'), paste: t('contextPaste'), clear: t('contextClear'),
    saveSnippet: t('contextSaveSnippet'), export: t('contextExport'),
  };
  items.forEach((item) => {
    const action = (item as HTMLElement).dataset.action;
    if (action && labels[action]) {
      const i = item.querySelector('i');
      const icon = i ? i.outerHTML : '';
      item.innerHTML = `${icon} ${labels[action]}`;
    }
  });
}

function updateTabContextMenuText(): void {
  const items = document.querySelectorAll('#tabContextMenu .menu-item');
  const labels: Record<string, string> = {
    close: t('tabClose'), closeOthers: t('tabCloseOthers'), closeAll: t('tabCloseAll'),
    duplicate: t('tabDuplicate'), rename: t('tabRename'),
  };
  items.forEach((item) => {
    const action = (item as HTMLElement).dataset.action;
    if (action && labels[action]) {
      const i = item.querySelector('i');
      const icon = i ? i.outerHTML : '';
      item.innerHTML = `${icon} ${labels[action]}`;
    }
  });
}

function renderShortcutsDialog(): void {
  const body = $('shortcutsBody');
  if (!body) return;

  const groups = [
    {
      title: t('shortsTabGroup'),
      items: [
        { keys: ['Ctrl', 'T'], label: t('shortsNewTab') },
        { keys: ['Ctrl', 'W'], label: t('shortsCloseTab') },
        { keys: ['Ctrl', '1-9'], label: t('shortsSwitchTab') },
      ],
    },
    {
      title: t('shortsTermGroup'),
      items: [
        { keys: ['Ctrl', 'Shift', 'C'], label: t('shortsCopy') },
        { keys: ['Ctrl', 'Shift', 'V'], label: t('shortsPaste') },
        { keys: ['Ctrl', 'Shift', 'F'], label: t('shortsSearch') },
      ],
    },
    {
      title: t('shortsUiGroup'),
      items: [
        { keys: ['Ctrl', 'B'], label: t('shortsToggleNotes') },
        { keys: ['Ctrl', 'S'], label: t('shortsSaveNote') },
        { keys: ['Ctrl', '/'], label: t('shortsShowShortcuts') },
        { keys: ['F11'], label: t('shortsFullscreen') },
      ],
    },
  ];

  body.innerHTML = groups.map(g => `
    <div class="shortcut-group">
      <div class="shortcut-group-title">${g.title}</div>
      ${g.items.map(i => `
        <div class="shortcut-row">
          ${i.keys.map(k => `<kbd>${k}</kbd>`).join('+')}
          <span>${i.label}</span>
        </div>
      `).join('')}
    </div>
  `).join('');
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
  } else {
    document.documentElement.requestFullscreen();
  }
  updateFullscreenButton();
}

function updateFullscreenButton(): void {
  const btn = $('toggleFullscreen');
  if (!btn) return;
  const icon = btn.querySelector('i');
  if (icon) icon.className = document.fullscreenElement ? 'fa-solid fa-compress' : 'fa-solid fa-expand';
  btn.title = document.fullscreenElement ? t('exitFullscreen') : t('fullscreen');
}

document.addEventListener('fullscreenchange', updateFullscreenButton);

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
  const name = prompt(t('promptTemplateName'), `${username}@${host}`);
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
    const confirmRestore = confirm(t('restoreSessionsConfirm', sessions.length));
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
    return { level: 'error', title: t('diagOffline'), detail: t('diagOfflineDetail') };
  }

  if (event && event.code === 1006) {
    return {
      level: 'error', title: t('diagAbnormalClose'),
      detail: t('diagAbnormalCloseDetail', host, port),
    };
  }

  if (event && event.code === 1001) {
    return { level: 'warn', title: t('diagServerClose'), detail: t('diagServerCloseDetail') };
  }

  const url = tabData.ws?.url || '';
  if (url.startsWith('ws://') && location.protocol === 'https:') {
    return {
      level: 'error', title: t('diagProtocolMismatch'),
      detail: t('diagProtocolMismatchDetail', url),
    };
  }

  return { level: 'error', title: t('diagConnectFail'), detail: t('diagConnectFailDetail', host, port) };
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
      <button class="error-close-btn" style="padding:6px 20px;border:1px solid var(--border);border-radius:6px;background:var(--bg);color:var(--text);cursor:pointer;">${t('close')}</button>
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
  if (diff < 60000) return t('justNow');
  if (diff < 3600000) return t('minutesAgo', Math.floor(diff / 60000));
  if (diff < 86400000) return t('hoursAgo', Math.floor(diff / 3600000));
  return d.toLocaleDateString();
}

function $(id: string): HTMLElement | null {
  return document.getElementById(id);
}

function safeWsSend(ws: WebSocket | null | undefined, data: unknown): boolean {
  if (!ws || ws.readyState !== WebSocket.OPEN) return false;
  try {
    ws.send(JSON.stringify(data));
    return true;
  } catch {
    return false;
  }
}

// =========================== 标签管理 ===========================

function createTab(host: string, port: string, username: string): string {
  const tabId = 'tab_' + Date.now();
  const label = `${username || 'user'}@${host}`;

  const tabEl = document.createElement('div');
  tabEl.className = 'tab-item active';
  tabEl.setAttribute('role', 'tab');
  tabEl.setAttribute('aria-selected', 'true');
  tabEl.innerHTML = `<span>${escapeText(label)}</span><span class="tab-close" data-tab="${tabId}" aria-label="${t('tabClose')}">&times;</span>`;
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
    if (td && td.ws) {
      safeWsSend(td.ws, { action: 'resize', sessionId: td.sessionId, cols: terminal.cols, rows: terminal.rows });
    }
  }, 200));
  if (container) resizeObserver.observe(container);

  terminal.onData((data: string) => {
    const td = state.tabs.get(tabId);
    if (td && td.ws) {
      if (safeWsSend(td.ws, { action: 'input', sessionId: td.sessionId, input: data })) {
        td.lastActivity = Date.now();
      }
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
  if (state.currentTab) {
    const prevTabEl = document.querySelector(`.tab-item[data-tab-id="${state.currentTab}"]`);
    if (prevTabEl) { prevTabEl.classList.remove('active'); prevTabEl.setAttribute('aria-selected', 'false'); }
    const prevTerm = state.tabs.get(state.currentTab);
    if (prevTerm) prevTerm.terminal.element!.style.display = 'none';
  }
  const tabEl = document.querySelector(`.tab-item[data-tab-id="${tabId}"]`);
  if (tabEl) { tabEl.classList.add('active'); tabEl.setAttribute('aria-selected', 'true'); }

  const tabData = state.tabs.get(tabId);
  if (tabData) {
    tabData.terminal.element!.style.display = '';
    updateStatus(tabData.status);
  }

  state.currentTab = tabId;
}

function closeTab(tabId: string): void {
  const tabData = state.tabs.get(tabId);
  if (!tabData) return;

  tabData.status = 'disconnected_by_user';
  if (tabData.reconnectTimer) { clearTimeout(tabData.reconnectTimer); tabData.reconnectTimer = null; }
  if (tabData.heartbeatTimer) { clearInterval(tabData.heartbeatTimer); tabData.heartbeatTimer = null; }
  if (tabData.heartbeatTimeout) { clearTimeout(tabData.heartbeatTimeout); tabData.heartbeatTimeout = null; }

  if (tabData.ws) {
    if (tabData.sessionId) {
      safeWsSend(tabData.ws, { action: 'destroy', sessionId: tabData.sessionId });
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

  let ws: WebSocket;
  try {
    ws = new WebSocket(gatewayUrl);
  } catch {
    updateTabStatus(tabId, 'error');
    tabData.terminal.writeln(`\r\n\x1b[31m${t('connectionError')}: ${t('unknownError', 0, gatewayUrl)}\x1b[0m`);
    return;
  }
  tabData.ws = ws;

  ws.onopen = () => {
    tabData.reconnectAttempts = 0;
    safeWsSend(ws, {
      action: 'handshake',
      token: state.token || ($('tokenInput') as HTMLInputElement)?.value,
    });
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
    try {
      const error = diagnoseError(null, tabData);
      showConnectionError(error);
    } catch { /* ignore render errors */ }
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
    if (tabData.ws) {
      safeWsSend(tabData.ws, { action: 'ping' });

      tabData.heartbeatTimeout = window.setTimeout(() => {
        tabData.terminal.writeln(`\r\n\x1b[33m${t('heartbeatTimeout')}\x1b[0m`);
        tabData.ws?.close();
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
      errorText = t('unknownError', String(msg.code || t('unknown')), escapeText(msg.error || t('unknownError', '')));
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
  const dialog = $('shortcutsDialog');
  if (!dialog) return;
  dialog.classList.toggle('hidden');
  if (!dialog.classList.contains('hidden')) {
    renderShortcutsDialog();
  }
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
        <div class="session-meta">${tab.port}:${escapeText((tab.sessionId || t('unassigned')).substring(0, 12))}</div>
      </span>
      <span class="session-status ${tab.status}"></span>
      <span class="session-actions">
        <button class="session-action-btn disconnect" data-action="disconnect" data-tab-id="${id}" title="${t('disconnect')}"><i class="fa-solid fa-plug-circle-xmark"></i></button>
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
      <button class="history-reconnect" data-idx="${i}" title="${t('connect')}"><i class="fa-solid fa-plug"></i></button>
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
      <button class="history-reconnect" data-idx="${i}" title="${t('connect')}"><i class="fa-solid fa-plug"></i></button>
      <button class="template-delete" data-idx="${i}" title="${t('delete')}"><i class="fa-solid fa-xmark"></i></button>
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
    user: t('me'), text: input.value.trim(), time: Date.now(),
  };
  chatMessages.push(msg);
  input.value = '';
  renderChatMessages();

  const tabData = state.tabs.get(state.currentTab || '');
  if (tabData && tabData.ws) {
    safeWsSend(tabData.ws, {
      action: 'chat', sessionId: tabData.sessionId, text: msg.text,
    });
  }
}

function addSystemChatMessage(text: string): void {
  chatMessages.push({ user: '', text, time: Date.now(), system: true });
  renderChatMessages();
}

let chatRenderedCount = 0;

function renderChatMessages(): void {
  const container = $('chatMessages');
  if (!container) return;
  const newMessages = chatMessages.slice(chatRenderedCount);
  if (newMessages.length === 0) return;
  const frag = document.createDocumentFragment();
  newMessages.forEach((m) => {
    const div = document.createElement('div');
    if (m.system) {
      div.className = 'chat-msg system';
      div.textContent = m.text;
    } else {
      div.className = 'chat-msg';
      div.innerHTML = `<span class="chat-user">${escapeText(m.user)}</span><span class="chat-time">${formatTime(new Date(m.time).toISOString())}</span><br><span class="chat-text">${escapeText(m.text)}</span>`;
    }
    frag.appendChild(div);
  });
  container.appendChild(frag);
  chatRenderedCount = chatMessages.length;
  container.scrollTop = container.scrollHeight;
}

function updateViewerCount(count: number): void {
  const el = $('viewerCount');
  if (el) el.textContent = count > 0 ? t('viewers', count) : '';
}

function handleSharingMessage(msg: any): void {
  if (msg.type === 'chat_message') {
    chatMessages.push({ user: msg.user || 'viewer', text: msg.text || '', time: Date.now() });
    renderChatMessages();
  } else if (msg.type === 'viewer_joined') {
    addSystemChatMessage(t('joinedSession', msg.user || t('anonymousUser')));
    updateViewerCount(msg.viewerCount || 0);
  } else if (msg.type === 'viewer_left') {
    addSystemChatMessage(t('leftSession', msg.user || t('anonymousUser')));
    updateViewerCount(msg.viewerCount || 0);
  } else if (msg.type === 'role_changed') {
    addSystemChatMessage(t('roleChanged', msg.user || t('anonymousUser'), msg.role || 'viewer'));
  } else if (msg.type === 'share_started') {
    addSystemChatMessage(t('sessionShared', msg.shareCode || 'N/A'));
    const chatBtn = $('toggleChat');
    if (chatBtn) chatBtn.style.display = '';
  } else if (msg.type === 'share_stopped') {
    addSystemChatMessage(t('shareStopped'));
    const chatBtn = $('toggleChat');
    if (chatBtn) chatBtn.style.display = 'none';
  }
}

// =========================== 笔记系统 ===========================

function createSnippet(title: string, content: string): void {
  const snippet: Snippet = {
    id: 'snp_' + Date.now(),
    title: title || t('untitledNote'),
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
    const title = ($('snippetTitle') as HTMLInputElement)?.value || t('untitledNote');
    const content = $('snippetContent')?.innerHTML || '';
    createSnippet(title, content);
    return;
  }

  const snippet = state.snippets.find((s) => s.id === state.currentSnippetId);
  if (!snippet) return;

  snippet.title = ($('snippetTitle') as HTMLInputElement)?.value || t('untitledNote');
  snippet.content = sanitizeSnippetContent($('snippetContent')?.innerHTML || '');
  snippet.updatedAt = new Date().toISOString();
  saveSnippets();
  renderSnippetList();
}

function deleteCurrentSnippet(): void {
  if (!state.currentSnippetId) return;
  if (!confirm(t('deleteSnippetConfirm'))) return;

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
  const defaults = ['未命名笔记', 'Untitled Note'];
  list.innerHTML = state.snippets.length === 0
    ? `<div style="padding:14px;color:var(--overlay);font-size:12px;text-align:center">${t('noNotes')}</div>`
    : state.snippets.map((s) => `
      <div class="snippet-list-item" data-snippet-id="${s.id}">
        <span>${escapeText(defaults.includes(s.title) ? t('untitledNote') : s.title)}</span>
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
  createSnippet(t('terminalExcerpt'), `<pre>${escapeText(text)}</pre>`);
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

function importData(): void {
  const input = document.createElement('input');
  input.type = 'file';
  input.accept = '.json';
  input.addEventListener('change', () => {
    const file = input.files?.[0];
    if (!file) return;
    const reader = new FileReader();
    reader.onload = () => {
      try {
        const data = JSON.parse(reader.result as string);
        if (data.settings) localStorage.setItem('windterm_settings', JSON.stringify(data.settings));
        if (data.connections) localStorage.setItem('windterm_connections', JSON.stringify(data.connections));
        if (data.snippets) {
          const existing = state.snippets.map((s) => s.id);
          data.snippets.forEach((s: Snippet) => {
            if (!existing.includes(s.id)) state.snippets.push(s);
          });
          saveSnippets();
          renderSnippetList();
        }
        location.reload();
      } catch {
        alert(t('importFailed'));
      }
    };
    reader.readAsText(file);
  });
  input.click();
}

// =========================== P6-1: 命令面板 ===========================

interface PaletteCommand {
  id: string;
  label: string;
  shortcut?: string;
  action: () => void;
}

const paletteCommands: PaletteCommand[] = [
  { id: 'connect', label: 'paletteConnect', shortcut: '', action: () => ($('connectBtn') as HTMLButtonElement)?.click() },
  { id: 'newTab', label: 'paletteNewTab', shortcut: 'Ctrl+T', action: () => createTab(
    ($('hostInput') as HTMLInputElement)?.value || 'localhost',
    ($('portInput') as HTMLInputElement)?.value || '22',
    ($('userInput') as HTMLInputElement)?.value || 'root',
  ) },
  { id: 'toggleSessions', label: 'paletteToggleSessions', action: () => $('sessionSidebar')?.classList.toggle('hidden') },
  { id: 'toggleSnippets', label: 'paletteToggleSnippets', shortcut: 'Ctrl+B', action: () => $('snippetPanel')?.classList.toggle('hidden') },
  { id: 'toggleTheme', label: 'paletteToggleTheme', action: toggleTheme },
  { id: 'toggleFullscreen', label: 'paletteToggleFullscreen', shortcut: 'F11', action: toggleFullscreen },
  { id: 'toggleLang', label: 'paletteToggleLang', action: toggleLanguage },
  { id: 'search', label: 'paletteSearch', shortcut: 'Ctrl+Shift+F', action: toggleSearchBar },
  { id: 'zoomIn', label: 'paletteZoomIn', shortcut: 'Ctrl+=', action: zoomIn },
  { id: 'zoomOut', label: 'paletteZoomOut', shortcut: 'Ctrl+-', action: zoomOut },
  { id: 'resetZoom', label: 'paletteResetZoom', shortcut: 'Ctrl+0', action: resetZoom },
  { id: 'export', label: 'paletteExport', action: exportTerminalContent },
  { id: 'exportBackup', label: 'paletteExportBackup', action: exportData },
  { id: 'importBackup', label: 'paletteImportBackup', action: importData },
  { id: 'saveTemplate', label: 'paletteSaveTemplate', action: saveAsTemplate },
  { id: 'shortcuts', label: 'paletteShortcuts', shortcut: 'Ctrl+/', action: toggleShortcutsDialog },
  { id: 'toggleChat', label: 'paletteToggleChat', shortcut: 'Ctrl+Shift+C', action: toggleChatPanel },
  { id: 'closeTab', label: 'paletteCloseTab', shortcut: 'Ctrl+W', action: () => { if (state.currentTab) closeTab(state.currentTab); } },
];

let paletteActiveIndex = 0;
let paletteFilterTimer: number | null = null;

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
  } else {
    if (paletteFilterTimer) { clearTimeout(paletteFilterTimer); paletteFilterTimer = null; }
  }
}

function filterCommands(query: string): void {
  const results = $('paletteResults');
  if (!results) return;

  const q = query.toLowerCase().trim();
  const filtered = q ? paletteCommands.filter((c) => t(c.label).toLowerCase().includes(q) || c.id.includes(q)) : paletteCommands;

  if (filtered.length === 0) {
    results.innerHTML = `<div class="palette-item disabled">${t('paletteNoMatch')}</div>`;
    return;
  }

  results.innerHTML = filtered.map((cmd, i) => `
    <div class="palette-item${i === paletteActiveIndex ? ' active' : ''}" data-index="${i}">
      <span>${t(cmd.label)}</span>
      ${cmd.shortcut ? `<span class="palette-shortcut">${cmd.shortcut}</span>` : ''}
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
    const value = (e.target as HTMLInputElement).value;
    if (paletteFilterTimer) clearTimeout(paletteFilterTimer);
    paletteFilterTimer = window.setTimeout(() => filterCommands(value), 50);
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
    const closeBtn = (e.target as HTMLElement).closest('.error-close-btn');
    if (closeBtn) { (closeBtn.parentElement as HTMLElement)?.parentElement?.remove(); }
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
            if (tabData && tabData.ws) {
              safeWsSend(tabData.ws, { action: 'input', sessionId: tabData.sessionId, input: text });
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
        if (tabData && tabData.ws) {
          safeWsSend(tabData.ws, { action: 'input', sessionId: tabData.sessionId, input: text });
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

