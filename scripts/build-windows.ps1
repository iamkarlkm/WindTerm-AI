# WindTerm-Extensions Windows 构建脚本
# PowerShell 脚本

param(
    [string]$BuildType = "Release",
    [string]$QtPath = "C:/Qt/5.15.2/msvc2019_64",
    [switch]$Clean,
    [switch]$Test,
    [switch]$Package
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $ProjectRoot "build"

Write-Host "=== WindTerm-Extensions Windows Build ===" -ForegroundColor Cyan
Write-Host "Build Type: $BuildType"
Write-Host "Qt Path: $QtPath"
Write-Host "Project Root: $ProjectRoot"

# 清理构建目录
if ($Clean -and (Test-Path $BuildDir)) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BuildDir
}

# 创建构建目录
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

# 设置环境变量
$env:PATH = "$QtPath\bin;$env:PATH"

# 配置 CMake
Write-Host "`nConfiguring CMake..." -ForegroundColor Green
Set-Location $BuildDir

cmake -G "Visual Studio 16 2019" -A x64 `
  -DCMAKE_BUILD_TYPE=$BuildType `
  -DCMAKE_PREFIX_PATH=$QtPath `
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
  $ProjectRoot

if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed!"
    exit 1
}

# 构建
Write-Host "`nBuilding project..." -ForegroundColor Green
$parallel = [Environment]::ProcessorCount
cmake --build . --config $BuildType --parallel $parallel

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed!"
    exit 1
}

# 运行测试
if ($Test) {
    Write-Host "`nRunning tests..." -ForegroundColor Green
    ctest --output-on-failure --build-config $BuildType
}

# 打包
if ($Package -and $BuildType -eq "Release") {
    Write-Host "`nPackaging..." -ForegroundColor Green
    $ArtifactDir = Join-Path $BuildDir "artifacts"
    if (-not (Test-Path $ArtifactDir)) {
        New-Item -ItemType Directory -Path $ArtifactDir | Out-Null
    }
    
    $BinDir = Join-Path $BuildDir "bin\$BuildType"
    Copy-Item "$BinDir\*.exe" -Destination $ArtifactDir -ErrorAction SilentlyContinue
    Copy-Item "$BinDir\*.dll" -Destination $ArtifactDir -ErrorAction SilentlyContinue
    
    # 部署 Qt 依赖
    Write-Host "Deploying Qt dependencies..." -ForegroundColor Yellow
    & "$QtPath\bin\windeployqt.exe" "$ArtifactDir\WindTerm.exe" --release --no-translations
}

Write-Host "`n=== Build completed successfully! ===" -ForegroundColor Green
Write-Host "Output: $BuildDir\bin\$BuildType\"
