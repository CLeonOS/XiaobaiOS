# XiaoBaiOS

XiaoBaiOS 是一个独立的、基于 CLKS 内核的操作系统项目。

当前仓库已经作为 `./xiaobaios` 下的独立发行版工作区存在，不再依赖上游 `cleonos` 的用户态应用接入方式。它保留了 CLKS 内核与启动链、终端式 shell、终端版 `menuconfig`、默认 LLVM 工具链，以及会被构建并打包进 ramdisk 的 `xiaobaios` 用户态程序。

## 主要特性

- 独立的发行版目录，位于 `xiaobaios/`
- 默认 LLVM 工具链：`clang`、`ld.lld`、`llvm-objcopy`、`llvm-objdump`、`llvm-readelf`、`llvm-nm`、`llvm-addr2line`
- 纯终端版 `menuconfig`，支持中文
- 从 `fonts/MapleMonoNormal-CN-Medium.ttf` 构建 PSF 字体
- `xiaobaios/apps/` 下的用户程序会被编译并打包进 ramdisk
- shell 内建命令支持 `ansi`、`ansitest`、`elfrunner`、`memc`

## 仓库结构

- `clks/` - 内核、运行时、驱动、shell 和支撑代码
- `cmake/` - 构建辅助脚本
- `configs/` - Limine 与 menuconfig 生成配置
- `fonts/` - 字体资源
- `ramdisk/` - 基础 ramdisk 内容
- `scripts/` - 字体转换与 menuconfig 辅助脚本
- `xiaobaios/apps/` - 本发行版的自定义用户态程序源码
- `xiaobaios/include/` - 这些用户态程序使用的定制头文件
- `limine/` - Limine 启动器源码树

## 构建

项目使用 CMake 驱动，并由顶层 `Makefile` 包装。

```bash
make menuconfig
make iso
make run
```

常用目标：

```bash
make setup
make kernel
make ramdisk
make debug
make clean
make clean-all
```

## menuconfig

`menuconfig` 是纯终端交互界面，不使用 Qt。

- 在 TTY 中运行时进入交互模式
- 在管道或脚本环境中自动使用非交互模式
- 生成的配置写入 `configs/menuconfig/.config.json` 和 `configs/menuconfig/config.cmake`

## 用户程序

`xiaobaios/apps/` 下的用户态源码会在构建过程中编译，并打包进 ramdisk。

- `ansi_main.c`
- `ansitest_main.c`
- `cmd_runtime.c` / `cmd_runtime.h`

shell 里也已经内建了 `ansi`、`ansitest`、`elfrunner`、`memc`，方便直接测试发行版镜像。

## 构建产物

构建输出位于 `build/` 目录，例如：

- `build/XiaoBaiOS-x86_64.iso`
- `build/x86_64/clks_kernel.elf`
- `build/x86_64/xiaobaios_ramdisk.tar`
- `build/x86_64/generated/tty.psf`

## 说明

- 本仓库保持独立，不再走上游 `cleonos` 的用户态程序接入链路。
- 内核日志不会再显示到交互式 shell 里。
- 如果重新生成构建目录，请不要提交 `build/`、`build-cmake*/`、`*.iso` 或 `*.tar` 这类产物。
