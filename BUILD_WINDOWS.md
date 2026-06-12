# Windows 构建指南

## 系统要求

- **操作系统**: Windows 10/11 (64 位)
- **编译器**: Visual Studio 2019 或 2022 (带 C++ 桌面开发)
- **CMake**: 3.16 或更高版本
- **Qt**: 5.15.2 或更高版本 (MSVC 2019 64 位)

## 安装依赖

### 1. 安装 Visual Studio

1. 下载 [Visual Studio Community 2022](https://visualstudio.microsoft.com/)
2. 安装时选择 "使用 C++ 的桌面开发" 工作负载
3. 确保勾选以下组件:
   - MSVC v142 - VS 2019 C++ x64/x86 生成工具
   - Windows 10/11 SDK
   - CMake 工具

### 2. 安装 Qt

```powershell
# 使用离线安装器下载 Qt 5.15.2
# 或在线安装: https://download.qt.io/archive/qt/5.15/

# 选择组件:
# - MSVC 2019 64-bit
# - Qt WebSockets
# - Qt Network Authentication
```

### 3. 安装 CMake

```powershell
# 从 https://cmake.org/download/ 下载
# 或使用 winget:
winget install Kitware.CMake
```

## 构建步骤

### 方法一: 命令行构建

```powershell
# 1. 克隆仓库
git clone https://github.com/iamkarlkm/WindTerm-AI.git
cd WindTerm-AI

# 2. 初始化子模块
git submodule update --init --recursive --depth 1

# 3. 创建构建目录
mkdir build
cd build

# 4. 配置 CMake (Release 版本)
cmake -G "Visual Studio 16 2019" -A x64 `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH="C:/Qt/5.15.2/msvc2019_64" `
  -DBUILD_TESTS=ON `
  -DBUILD_RENDERER=ON `
  -DBUILD_BUFFER=ON `
  -DBUILD_WIDGET=ON `
  -DBUILD_APPLICATION=ON `
  -DBUILD_PLUGINS=ON `
  -DENABLE_METAL=OFF `
  -DENABLE_OPENGL=ON `
  -DENABLE_DIRECTX12=ON `
  -DENABLE_VULKAN=OFF `
  ..

# 5. 构建
cmake --build . --config Release --parallel

# 6. 运行测试
ctest --output-on-failure --build-config Release
```

### 方法二: Visual Studio IDE

1. 打开 Visual Studio 2019/2022
2. 选择 "打开本地文件夹"
3. 选择 WindTerm-Extensions 目录
4. CMake 会自动配置项目
5. 在顶部工具栏选择配置 (Debug/Release)
6. 选择 "全部生成" (Ctrl+Shift+B)

## 构建选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `BUILD_RENDERER` | ON | 构建 GPU 渲染器 |
| `BUILD_BUFFER` | ON | 构建文本缓冲区 |
| `BUILD_WIDGET` | ON | 构建终端控件 |
| `BUILD_APPLICATION` | ON | 构建应用程序 |
| `BUILD_PLUGINS` | ON | 构建插件系统 |
| `BUILD_TESTS` | ON | 构建单元测试 |
| `ENABLE_METAL` | OFF | Metal 后端 (仅 macOS) |
| `ENABLE_OPENGL` | ON | OpenGL 后端 |
| `ENABLE_DIRECTX12` | ON | DirectX 12 后端 (仅 Windows) |
| `ENABLE_VULKAN` | OFF | Vulkan 后端 |

## 常见问题

### Q: CMake 找不到 Qt

**解决方案**: 指定 `CMAKE_PREFIX_PATH`

```powershell
cmake -DCMAKE_PREFIX_PATH="C:/Qt/5.15.2/msvc2019_64" ..
```

### Q: 链接错误 LNK1158: 无法运行 cvtres.exe

**解决方案**: 
1. 确保 Windows SDK 已正确安装
2. 复制 cvtres.exe:
   ```powershell
   Copy-Item "C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\cvtres.exe" `
             "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.29.30133\bin\Hostx64\x64\"
   ```

### Q: 缺少 DLL 文件

**解决方案**: 将 Qt bin 目录添加到 PATH

```powershell
$env:PATH += ";C:\Qt\5.15.2\msvc2019_64\bin"
```

### Q: 运行时错误 0xc000007b

**解决方案**: 确保使用 64 位 Qt 和 64 位编译器，不要混用 32 位和 64 位库。

## 输出文件

构建完成后，输出文件位于:

```
build/bin/Release/
├── WindTerm.exe              # 主应用程序
├── WindTermRenderer.dll      # 渲染器库
├── WindTermBuffer.dll        # 缓冲区库
├── WindTermWidget.dll        # 控件库
├── WindTermSsh.dll           # SSH 模块
├── WindTermAIAssist.dll      # AI 助手模块
├── WindTermCloud.dll         # 云集成模块
└── plugins/                  # 插件目录
```

## 性能优化建议

### 启用优化编译

```powershell
# Release with optimizations
cmake -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_CXX_FLAGS_RELEASE="/O2 /GL /DNDEBUG" `
  ..
```

### 使用链接时优化 (LTO)

```powershell
cmake -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON ..
```

### 并行编译

```powershell
# 使用所有 CPU 核心
cmake --build . --parallel $env:NUMBER_OF_PROCESSORS
```

## 打包发布

```powershell
# 使用 windeployqt 部署 Qt 依赖
cd build/bin/Release
windeployqt WindTerm.exe --release --no-translations

# 创建安装包 (使用 Inno Setup 或 NSIS)
```

## CI/CD

Windows 构建通过 GitHub Actions 自动执行。查看工作流状态:
https://github.com/iamkarlkm/WindTerm-AI/actions/workflows/windows-build.yml

## 相关文档

- [Linux 构建指南](BUILD_LINUX.md)
- [macOS 构建指南](BUILD_MACOS.md)
- [开发指南](DEVELOPMENT.md)
