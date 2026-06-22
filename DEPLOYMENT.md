# WindTerm-AI 部署指南

## 系统架构

```
WindTerm-AI
├── 桌面原生应用 (C++/Qt5)
│   ├── windterm-terminal (Linux/macOS)
│   └── WindTerm.exe (Windows)
├── Web 客户端 (TypeScript + Vite)
│   ├── 静态部署 / PWA
│   └── Tauri 2 桌面壳 (Rust)
└── 移动端 (Tauri + Android)
    └── WindTerm-AI-v1.1.0.apk
```

---

## 一、桌面原生应用 (C++/Qt5)

### 1.1 系统要求

| 依赖 | 最低版本 |
|------|---------|
| C++ 编译器 | GCC 7+, Clang 5+, MSVC 2017+ |
| Qt5 | 5.12+ (Core, Gui, Widgets, OpenGL, Sql, Network) |
| CMake | 3.16+ |
| libssh | 0.9+ (SSH2 协议支持) |
| OpenGL | 3.3+ |

### 1.2 快速构建

```bash
git clone https://github.com/iamkarlkm/WindTerm-AI.git
cd WindTerm-AI
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
./windterm-terminal
```

### 1.3 构建选项

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_RENDERER=ON \
  -DBUILD_BUFFER=ON \
  -DBUILD_THEME=ON \
  -DBUILD_WIDGET=ON \
  -DBUILD_APP=ON \
  -DBUILD_PLUGINS=ON \
  -DBUILD_TESTS=OFF \
  -DWITH_OPENGL=ON \
  -DWITH_METAL=OFF \
  -DWITH_DIRECTX=OFF \
  -DWITH_VULKAN=OFF
```

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `BUILD_RENDERER` | ON | GPU 渲染引擎 |
| `BUILD_BUFFER` | ON | 环形文本缓冲区 |
| `BUILD_THEME` | ON | 主题系统 |
| `BUILD_WIDGET` | ON | 终端控件 + 主窗口 |
| `BUILD_APP` | ON | 可执行文件 |
| `BUILD_PLUGINS` | ON | 插件系统 |
| `BUILD_TESTS` | ON | 单元测试 |
| `WITH_OPENGL` | ON | OpenGL 后端 |
| `WITH_METAL` | ON | Metal 后端 (仅 macOS) |
| `WITH_DIRECTX` | ON | DirectX12 后端 (仅 Windows) |
| `WITH_VULKAN` | OFF | Vulkan 后端 (预留) |

### 1.4 运行单元测试

```bash
cd build
QT_QPA_PLATFORM=offscreen ctest --output-on-failure
```

测试覆盖：CircularTextBuffer, AnsiParser, TerminalState, UrlDetector, UnicodeUtil, PerformanceBenchmark。

### 1.5 数据存储路径

| 模块 | 数据库 | 路径 |
|------|--------|------|
| SSH 连接 | `ssh_profiles.db` | `~/.local/share/WindTerm-AI/` |
| 命令历史 | `command_history.db` | `~/.local/share/WindTerm-AI/` |
| 书签 | `bookmarks.db` | `~/.local/share/WindTerm-AI/` |
| 设置 | QSettings | 系统默认位置 |

### 1.6 发布打包

**Linux**：

```bash
mkdir -p dist/WindTerm-AI/bin dist/WindTerm-AI/plugins
cp build/bin/Release/windterm-terminal dist/WindTerm-AI/bin/
cp build/bin/Release/*.so dist/WindTerm-AI/plugins/
cp build/bin/Release/plugins/* dist/WindTerm-AI/plugins/
tar -czf WindTerm-AI-linux-x64.tar.gz dist/WindTerm-AI/
```

**macOS**：

```bash
mkdir -p dist/WindTerm-AI.app/Contents/MacOS
cp build/bin/Release/windterm-terminal dist/WindTerm-AI.app/Contents/MacOS/
macdeployqt dist/WindTerm-AI.app -dmg
```

**Windows**（详见 `BUILD_WINDOWS.md`）：

```powershell
cd build\bin\Release
windeployqt WindTerm.exe --release --no-translations
```

### 1.7 依赖安装命令

**Ubuntu/Debian**:

```bash
apt-get install -y build-essential cmake qtbase5-dev libqt5opengl5-dev libssh-dev
```

**Fedora**:

```bash
dnf install -y gcc-c++ cmake qt5-qtbase-devel libssh-devel
```

**macOS**:

```bash
brew install cmake qt@5 libssh
```

---

## 二、Tauri 桌面应用 (Rust + Web 前端)

基于 `webclient/` 子项目，将 Web 终端客户端打包为原生桌面窗口。

### 2.1 前置条件

```bash
# Rust 工具链
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source $HOME/.cargo/env

# Tauri CLI
cargo install tauri-cli --version "^2.0"

# Node.js (>=18)
curl -fsSL https://deb.nodesource.com/setup_20.x | bash -
apt-get install -y nodejs
```

**Linux 桌面依赖**：

```bash
apt-get install -y libwebkit2gtk-4.1-dev libappindicator3-dev \
  librsvg2-dev patchelf libgtk-3-dev libsoup-3.0-dev libjavascriptcoregtk-4.1-dev
```

### 2.2 构建

```bash
cd webclient

# 安装 npm 依赖
npm install

# 开发模式 (热更新)
npx tauri dev

# 生产构建
npx tauri build
```

### 2.3 产物

```
webclient/src-tauri/target/release/
├── bundle/
│   ├── deb/          (.deb 安装包, Linux)
│   ├── rpm/          (.rpm 安装包, Linux)
│   ├── appimage/     (AppImage, Linux)
│   ├── dmg/          (.dmg 镜像, macOS)
│   └── msi/          (.msi 安装程序, Windows)
└── windterm-webclient  (裸可执行文件)
```

### 2.4 兼容性说明

| 平台 | 状态 | 备注 |
|------|------|------|
| Linux | 完整支持 | Ubuntu 22.04+, Debian 12+, Fedora 38+ |
| macOS | 完整支持 | 12.0+ (Monterey+), Intel/Apple Silicon |
| Windows | 完整支持 | Windows 10+ (64 位) |

---

## 三、Web 客户端 (静态 / PWA)

### 3.1 构建

```bash
cd webclient
npm install
npm run build
```

产物在 `webclient/dist/` 目录：

```
dist/
├── index.html          (13.6 KB, gzip 3.6 KB)
├── assets/
│   ├── main-*.js       (53 KB, gzip 16 KB)
│   └── main-*.css      (19 KB, gzip 4 KB)
└── sw.js               (Service Worker, PWA)
```

### 3.2 部署方式

**方式一：任意 HTTP 服务器**

```bash
# Python
python3 -m http.server 3000 --directory dist/

# Node.js
npx serve dist -l 3000

# Nginx
```

Nginx 配置：

```nginx
server {
    listen 80;
    server_name terminal.example.com;

    root /var/www/windterm-webclient;
    index index.html;

    location / {
        try_files $uri $uri/ /index.html;
    }

    location /ws/ {
        proxy_pass http://127.0.0.1:9000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_set_header Host $host;
        proxy_read_timeout 86400s;
    }

    # 静态资源强缓存
    location /assets/ {
        expires 1y;
        add_header Cache-Control "public, immutable";
    }
}
```

**方式二：Cloudflare Pages / Vercel / Netlify**

- 构建命令：`cd webclient && npm run build`
- 输出目录：`webclient/dist`
- 框架预设：无（静态 HTML）

**方式三：Docker**

```dockerfile
FROM nginx:alpine
COPY webclient/dist /usr/share/nginx/html
COPY webclient/nginx.conf /etc/nginx/conf.d/default.conf
EXPOSE 80
```

```bash
docker build -t windterm-webclient .
docker run -d -p 8080:80 windterm-webclient
```

### 3.3 PWA 功能

- 支持添加到桌面（iOS/Android/Desktop）
- 离线缓存（Service Worker）
- iOS Safari 全屏模式（`apple-mobile-web-app-capable`）
- 安全区域适配（`viewport-fit=cover`）

---

## 四、Android 移动端

### 4.1 环境准备

```bash
# Java JDK 17
apt-get install -y openjdk-17-jdk-headless

# Android SDK (cmdline-tools)
mkdir -p /root/Android/Sdk
cd /root/Android/Sdk
wget https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip
unzip commandlinetools-*.zip

# 安装 SDK 组件
export ANDROID_HOME=/root/Android/Sdk
yes | $ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager \
  "platforms;android-34" \
  "build-tools;34.0.0" \
  "ndk;27.0.12077973"

# Rust 移动目标
rustup target add aarch64-linux-android armv7-linux-androideabi \
  i686-linux-android x86_64-linux-android
```

### 4.2 环境变量

构建前必须设置：

```bash
export ANDROID_HOME=/root/Android/Sdk
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
export NDK_HOME=$ANDROID_HOME/ndk/27.0.12077973
export PATH=$PATH:$HOME/.cargo/bin
```

### 4.3 构建

```bash
cd webclient

# 首次初始化 Android 项目目录
npx tauri android init

# 构建 APK (Release)
npx tauri android build
```

### 4.4 产物

```
webclient/src-tauri/gen/android/app/build/outputs/apk/universal/release/
└── app-universal-release-unsigned.apk  (约 48 MB)
```

APK 包含 4 个 ABI 架构：

| ABI | 对应的设备 |
|-----|-----------|
| arm64-v8a | 现代 64 位 ARM (主流) |
| armeabi-v7a | 旧款 32 位 ARM |
| x86 | 模拟器 / Intel Atom |
| x86_64 | 64 位模拟器 |

### 4.5 签名发布

```bash
# 生成签名密钥 (仅首次)
keytool -genkey -v -keystore windterm-release.keystore \
  -alias windterm -keyalg RSA -keysize 2048 -validity 10000 \
  -storepass STORE_PASS -keypass KEY_PASS \
  -dname "CN=WindTerm, O=WindTerm, C=CN"

# 签名 APK
jarsigner -verbose -sigalg SHA256withRSA -digestalg SHA-256 \
  -keystore windterm-release.keystore \
  -storepass STORE_PASS -keypass KEY_PASS \
  app-universal-release-unsigned.apk windterm

# Zipalign 对齐 (优化内存使用)
$ANDROID_HOME/build-tools/34.0.0/zipalign -v 4 \
  app-universal-release-unsigned.apk \
  WindTerm-AI-v1.1.0-release.apk
```

### 4.6 安装测试

```bash
# ADB 安装
adb install WindTerm-AI-v1.1.0-release.apk

# 通过 HTTP 下载安装
python3 -m http.server 8000
# Android 浏览器访问 http://<IP>:8000/WindTerm-AI-v1.1.0-release.apk
```

### 4.7 兼容性

| 要求 | 值 |
|------|-----|
| 最低 Android 版本 | 8.0 (API 26) |
| 目标 SDK | 34 |
| 最低 NDK | r27 |
| WebView | Android System WebView (自动) |

---

## 五、iOS 移动端

### 5.1 系统要求（硬性限制）

- **macOS** + Xcode 15+
- Apple Developer Account（发布需要）

### 5.2 环境准备

```bash
# Rust iOS 目标（可在 Linux 上安装，但无法编译）
rustup target add aarch64-apple-ios x86_64-apple-ios aarch64-apple-ios-sim
```

### 5.3 项目结构（已就绪）

```
webclient/src-tauri/gen/ios/xcode/Sources/
```

### 5.4 构建流程（需 macOS）

```bash
cd webclient

# 初始化 Xcode 项目
cargo tauri ios init

# 开发模式
cargo tauri ios dev

# 构建 IPA
cargo tauri ios build
```

---

## 六、CI/CD (GitHub Actions)

### 6.1 桌面原生应用

GitHub Actions 已在 `~/.github/workflows/` 中配置。

Windows 构建工作流：https://github.com/iamkarlkm/WindTerm-AI/actions/workflows/windows-build.yml

### 6.2 Tauri 桌面构建

```yaml
# .github/workflows/tauri-build.yml
name: Tauri Build
on:
  push:
    branches: [master]
    paths:
      - 'webclient/**'

jobs:
  linux:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-node@v4
        with: { node-version: 20 }
      - run: sudo apt-get install -y libwebkit2gtk-4.1-dev libappindicator3-dev librsvg2-dev patchelf
      - uses: dtolnay/rust-toolchain@stable
      - run: cargo install tauri-cli
      - run: npm ci
        working-directory: webclient
      - run: cargo tauri build
        working-directory: webclient
      - uses: actions/upload-artifact@v4
        with:
          name: tauri-linux
          path: webclient/src-tauri/target/release/bundle/
```

### 6.3 Android 构建

```yaml
# .github/workflows/android-build.yml
name: Android Build
on:
  push:
    branches: [master]

jobs:
  android:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-java@v4
        with:
          distribution: temurin
          java-version: 17
      - uses: actions/setup-node@v4
        with: { node-version: 20 }
      - uses: dtolnay/rust-toolchain@stable

      - name: Install Rust targets
        run: |
          rustup target add aarch64-linux-android armv7-linux-androideabi \
            i686-linux-android x86_64-linux-android

      - name: Setup Android SDK
        run: |
          mkdir -p ${{ runner.temp }}/android-sdk
          cd ${{ runner.temp }}/android-sdk
          wget -q https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip
          unzip -q commandlinetools-*.zip
          yes | cmdline-tools/latest/bin/sdkmanager \
            "platforms;android-34" "build-tools;34.0.0" "ndk;27.0.12077973"

      - name: Build APK
        env:
          ANDROID_HOME: ${{ runner.temp }}/android-sdk
          JAVA_HOME: ${{ env.JAVA_HOME }}
          NDK_HOME: ${{ runner.temp }}/android-sdk/ndk/27.0.12077973
        run: |
          cd webclient
          npm ci
          npx tauri android build

      - uses: actions/upload-artifact@v4
        with:
          name: android-apk
          path: webclient/src-tauri/gen/android/app/build/outputs/apk/**/*.apk
```

### 6.4 Web 静态部署

```yaml
# .github/workflows/web-deploy.yml
name: Web Deploy
on:
  push:
    branches: [master]
    paths:
      - 'webclient/src/**'
      - 'webclient/index.html'
      - 'webclient/styles.css'

jobs:
  deploy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-node@v4
        with: { node-version: 20 }
      - run: |
          cd webclient
          npm ci
          npm run build
      - uses: cloudflare/pages-action@v1
        with:
          apiToken: ${{ secrets.CLOUDFLARE_API_TOKEN }}
          accountId: ${{ secrets.CLOUDFLARE_ACCOUNT_ID }}
          projectName: windterm-webclient
          directory: webclient/dist
```

---

## 七、Web 终端网关

Web 客户端依赖 Web 终端网关作为后端，负责 WebSocket 到 SSH/TCP 的桥接。

### 7.1 启动网关

```bash
cd build
./windterm-terminal --gateway --port 9000
```

环境变量：

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `GATEWAY_PORT` | 9000 | 网关监听端口 |
| `GATEWAY_HOST` | 0.0.0.0 | 绑定地址 |
| `GATEWAY_AUTH_TOKEN` | (空) | 认证令牌（启用时必需） |

### 7.2 验证网关

```bash
# 健康检查
curl http://localhost:9000/health

# WebSocket 测试
wscat -c ws://localhost:9000/ws
```

---

## 八、版本对应关系

| 组件 | 版本 | 部署模式 |
|------|------|---------|
| 桌面原生 (C++) | 0.2.0 | 独立可执行文件 |
| Tauri 桌面 (Rust) | 1.1.0 | 原生安装包 |
| Web 前端 | 1.1.0 | 静态文件 + PWA |
| Android | 1.1.0 | APK (48MB universal) |
| iOS | 1.1.0 | IPA (需 macOS 构建) |

---

## 九、常见问题

### Q: Tauri 构建报 "no library targets found"

**原因**：`Cargo.toml` 缺少 `[lib]` 段，Android 构建需要 `lib.rs` 入口。

**解决**：已修复，`src-tauri/src/lib.rs` 包含 `#[mobile_entry_point]`，`Cargo.toml` 含 `crate-type = ["lib", "cdylib", "staticlib"]`。

### Q: Android build 提示 "frontendDist includes ..."

**原因**：`tauri.conf.json` 中 `frontendDist` 指向了父目录而非 `dist/` 目录。

**解决**：已修正为 `"frontendDist": "../dist"`。

### Q: Tauri 桌面构建缺少 libsoup

**解决**：

```bash
apt-get install -y libsoup-3.0-dev libjavascriptcoregtk-4.1-dev
```

### Q: 移动端无法连接到 WebSocket

**检查项**：
1. 确认网关已启动且可从移动设备访问（非 localhost）
2. CSP 策略已允许 `ws:` / `wss:` 连接
3. Android 检查网络权限（`INTERNET` permission 已内置）

### Q: iOS 能否在 Linux 上构建

**不能**。iOS 需要 Apple 的 Xcode 工具链和代码签名，必须在 macOS 上完成。

---

## 相关文档

- [项目 README](README.md) — 功能特性和快捷键总览
- [Web 客户端 README](webclient/README.md) — Web 客户端开发
- [Windows 构建指南](BUILD_WINDOWS.md) — Windows 桌面构建
- [系统架构](.monkeycode/docs/ARCHITECTURE.md) — 架构设计文档
- [AI 集成文档](README_AI.md) — AI 功能集成说明
