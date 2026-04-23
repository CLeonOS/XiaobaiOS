# XiaoBaiOS

XiaoBaiOS 是一个独立的、基于 CLKS 内核的操作系统项目。

当前仓库已经作为 `./xiaobaios` 下的独立发行版工作区存在，不再依赖上游 `cleonos` 的用户态应用接入方式。它保留了 CLKS 内核与启动链、终端式 shell、终端版 `menuconfig`、默认 LLVM 工具链，以及会被构建并打包进 ramdisk 的 `xiaobaios` 用户态程序。

## 主要特性

- 独立的发行版目录，位于 `xiaobaios/`
- 默认 LLVM 工具链：`clang`、`ld.lld`、`llvm-objcopy`、`llvm-objdump`、`llvm-readelf`、`llvm-nm`、`llvm-addr2line`
- 纯终端版 `menuconfig`，支持中文
- 从 `fonts/MapleMonoNormal-CN-Medium.ttf` 构建 PSF 字体
- `xiaobaios/apps/` 下的用户程序会被编译并打包进 ramdisk
- 提供类 Linux 账户基础：`/system/etc/passwd`、`/system/etc/shadow`、`/system/etc/group`
- 提供账户管理用户态 ELF：`whoami`、`id`、`su`、`useradd`、`passwd`
- 新增用户态 shell：`xsh`（仅外部 ELF 命令，基于 PATH 查找）
- shell 内建命令支持 `ansi`、`ansitest`、`elfrunner`、`memc`（并支持外部 ELF 优先）

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
- `whoami_main.c`、`id_main.c`、`su_main.c`、`useradd_main.c`、`passwd_main.c`
- `cmd_runtime.c` / `cmd_runtime.h`

内核 shell 保留少量内建命令，但现在会优先执行 `/shell/*.elf` 的独立程序（包括账户管理命令）；若外部 ELF 不存在再回退到内建。

当当前会话身份为 root 时，内核 shell 会自动切换到 `/shell/xsh.elf`。

`xsh` 本身不再内建文件/管理命令：`cd`、`pwd`、`ls`、`cat`、`mkdir`、`touch`、`write`、`append`、`cp`、`mv`、`rm`、`help`、`exit`、`whoami`、`id`、`su`、`useradd`、`passwd` 都是独立 ELF，通过 `PATH` 解析执行。

## 账户系统

- 账户数据库位于 `/system/etc/passwd`、`/system/etc/shadow`、`/system/etc/group`
- 默认账户为 `root`（uid/gid 均为 `0`，home 为 `/home/root`）
- 密码哈希当前使用项目内轻量格式 `x1$<hex-hash>`（适合当前阶段）
- `useradd <name>` 会创建用户/组条目并创建 `/home/<name>`
- `passwd [user]` 修改密码（`root` 可修改任意用户）
- `su [user]` 可切换 shell 当前身份，并更新提示符与权限

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
