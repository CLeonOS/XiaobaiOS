# XiaoBaiOS

XiaoBaiOS 是基于 CLKS 内核的独立发行版目录。

这个目录只保留内核与最小启动链路，用于后续从 CLKS 派生自己的发行版。

默认工具链使用 LLVM：`clang`、`ld.lld`、`llvm-objcopy`、`llvm-objdump`、`llvm-readelf`、`llvm-nm`、`llvm-addr2line`。

## 入口

- `clks/`：CLKS 内核源码
- `cmake/`：构建辅助脚本
- `configs/`：启动与构建配置
- `limine/`：Limine 启动器源码与产物

## 构建

```bash
cd xiaobaios
make menuconfig
make iso
make run
```

`menuconfig` 是纯命令行界面，不依赖 Qt；它会把配置写到 `configs/menuconfig/.config.json` 和 `configs/menuconfig/config.cmake`。

如果启用了外部 PSF 字体，会把 `fonts/MapleMonoNormal-CN-Medium.ttf` 在构建时转换成 `build/x86_64/generated/tty.psf`，然后自动打包到 ramdisk 的 `/system/tty.psf`。
