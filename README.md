# Novoline Launcher

一个纯 Win32 / GDI+ 编写的 Minecraft 启动器，界面按参考图重绘。无 Electron、无 .NET、无 Qt、无 Python、无 VC++ 运行库，单个 `exe` 即可运行。

![界面预览](assets/preview.png)

## 直接运行

双击：

```
dist\NovolineLauncher.exe
```

要求：Windows 10 / 11 x64。

- **界面本身**只使用 Windows 自带的 `kernel32 / user32 / gdi32 / gdiplus / shell32 / advapi32 / dwmapi` 等系统组件。
- 不依赖 VC++ Redistributable，不依赖 .NET，不依赖任何第三方 DLL，也没有外部皮肤/图片/字体文件。
- 程序内嵌了图标、Manifest 和版本信息。
- 启动 Minecraft 时仍然需要本机安装 Java（游戏本身的要求），启动器会自动探测 Java 8 / 17 / 21 等常见安装位置。

## 功能

- 与参考图一致的无边框深色窗口：Logo、标题栏、最小化 / 最大化 / 关闭按钮、镜像文字、左侧版本列表、底部按钮和绿色 Launch 按钮。
- 窗口支持拖动、双击标题栏最大化、边缘缩放、最小化 / 最大化 / 还原 / 关闭。
- 高 DPI 感知（PerMonitorV2），根据屏幕工作区自动缩放，避免界面超出屏幕。
- 左侧四个版本条目可切换，右侧内容随之更新。
- `Settings`：显示游戏目录、Java、内存和离线档案；可打开游戏目录、自动探测 Java、调整内存。
- `Log out`：切换为离线档案 `Player`。
- `Launch`：
  - 自动查找 `%APPDATA%\.minecraft`；
  - 优先使用已安装的 `Novoline` 版本，否则使用最近修改的版本；
  - 自动解析版本 JSON、库文件、启动参数和资源索引；
  - 使用系统自带 `tar.exe` 解压 native 库；
  - 使用离线 UUID / accessToken=0 启动，不读取、不保存任何微软账号密码；
  - 启动日志：`%APPDATA%\NovolineLauncher\launcher.log` 和 `game.log`。

> 这是启动器，不包含 Minecraft 游戏本体。请先在本机安装 Minecraft（以及 Java），再点击 Launch。

## 文件结构

```
NovolineLauncher.exe          编译后的主程序
dist\NovolineLauncher.exe     可直接分发的单文件副本
src\                           C 源码
  main.c                       Win32 窗口、绘制、交互
  launcher.c / launcher.h      Minecraft / Java 探测与启动
  json.c / json.h              轻量 JSON 解析
  gdiplus_flat.h               GDI+ 扁平 API 声明
  win_extra.h                  TCC 缺失的系统 API 声明
assets\
  novoline.ico / novoline_1024.png
  app.manifest
  preview.png                 实际运行截图
  reference.jpg               参考图
tools\
  tcc\                         便携式 TinyCC 编译器
  rcedit.exe                   写入图标 / 版本信息 / Manifest
  make_icon.py                 重新生成图标
build.ps1                      一键重新编译
```

## 重新编译

在 PowerShell 中运行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
```

脚本使用 `tools\tcc` 编译，使用 `tools\rcedit.exe` 写入图标、Manifest 和版本信息，并同步到 `dist\`。

## 说明

- 界面尺寸按参考图设计为 1237 × 827（96 DPI 基准），在 100% 缩放下与参考图基本逐像素对齐。
- 字体使用 Windows 自带的 `Segoe UI`，不携带字体文件。
- 所有图标和 Logo 均为矢量绘制，不加载外部图片。
- 当前版本采用离线档案启动；没有实现 Microsoft 账号登录。首次启动默认用户名为 `Player`。
- 对现代 Forge / NeoForge 等需要额外 JVM 参数的整合包，启动参数适配可能不完整；原版 / Novoline 风格版本可直接使用。

## 许可证

本项目使用 GNU General Public License v3.0，完整文本见 `LICENSE`。

构建工具 TinyCC 与 rcedit 的第三方许可证见 `THIRD_PARTY_NOTICES.md` 和 `LICENSES/`。
