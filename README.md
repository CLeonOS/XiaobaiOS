# XiaoBaiOS

XiaoBaiOS is a standalone CLKS-based operating system project.

This repository now lives as its own distro workspace under `./xiaobaios`, separate from upstream `cleonos`. It keeps the CLKS kernel/boot chain, a terminal-based shell, a curses-style `menuconfig`, LLVM-first tooling, and the custom `xiaobaios` user-space apps that are built and packed into the ramdisk.

## Highlights

- Independent distro layout under `xiaobaios/`
- LLVM toolchain by default: `clang`, `ld.lld`, `llvm-objcopy`, `llvm-objdump`, `llvm-readelf`, `llvm-nm`, `llvm-addr2line`
- Terminal-only `menuconfig` with Chinese support
- PSF font pipeline built from `fonts/MapleMonoNormal-CN-Medium.ttf`
- Ramdisk packaging for built user apps from `xiaobaios/apps/`
- Built-in shell commands for `ansi`, `ansitest`, `elfrunner`, `memc`

## Repository Layout

- `clks/` - kernel, runtime, drivers, shell, and support code
- `cmake/` - build helper scripts
- `configs/` - Limine and menuconfig generated configuration
- `fonts/` - font assets used by the PSF pipeline
- `ramdisk/` - base ramdisk contents
- `scripts/` - helper scripts for font conversion and menuconfig
- `xiaobaios/apps/` - custom user-space app sources for this distro
- `xiaobaios/include/` - custom user-space headers for those apps
- `limine/` - Limine bootloader source tree

## Build

The project is driven by CMake and wrapped with a simple `Makefile`.

```bash
make menuconfig
make iso
make run
```

Useful targets:

```bash
make setup
make kernel
make ramdisk
make debug
make clean
make clean-all
```

## Menuconfig

`menuconfig` is terminal-based and does not use Qt.

- Interactive mode when run in a TTY
- Non-interactive mode when piped or scripted
- Writes generated config to `configs/menuconfig/.config.json` and `configs/menuconfig/config.cmake`

## User Apps

User-space sources under `xiaobaios/apps/` are compiled and packed into the ramdisk during the build.

- `ansi_main.c`
- `ansitest_main.c`
- `cmd_runtime.c` / `cmd_runtime.h`

The shell also exposes built-in commands for `ansi`, `ansitest`, `elfrunner`, and `memc` so they can be invoked directly even when you are just testing the distro image.

## Generated Outputs

Build artifacts are written under `build/`, for example:

- `build/XiaoBaiOS-x86_64.iso`
- `build/x86_64/clks_kernel.elf`
- `build/x86_64/xiaobaios_ramdisk.tar`
- `build/x86_64/generated/tty.psf`

## Notes

- The repo is intended to stay independent from upstream `cleonos` user-space app wiring.
- Kernel logs are kept out of the interactive shell view.
- If you regenerate the build tree, do not commit `build/`, `build-cmake*/`, `*.iso`, or `*.tar` outputs.
