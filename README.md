# XiaoBaiOS

XiaoBaiOS is a standalone CLKS-based operating system project.

This repository now lives as its own distro workspace under `./xiaobaios`, separate from upstream `cleonos`. It keeps the CLKS kernel/boot chain, a terminal-based shell, a curses-style `menuconfig`, LLVM-first tooling, and the custom `xiaobaios` user-space apps that are built and packed into the ramdisk.

## Highlights

- Independent distro layout under `xiaobaios/`
- LLVM toolchain by default: `clang`, `ld.lld`, `llvm-objcopy`, `llvm-objdump`, `llvm-readelf`, `llvm-nm`, `llvm-addr2line`
- Terminal-only `menuconfig` with Chinese support
- PSF font pipeline built from `fonts/MapleMonoNormal-CN-Medium.ttf`
- Ramdisk packaging for built user apps from `xiaobaios/apps/`
- Linux-style account basics: `/system/etc/passwd`, `/system/etc/shadow`, `/system/etc/group`
- Account management user ELFs: `whoami`, `id`, `su`, `useradd`, `passwd`
- New user-space shell `xsh` (external-ELF only; PATH-based command lookup)
- Built-in shell commands for `ansi`, `ansitest`, `elfrunner`, `memc` (and external ELF fallback)

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
- `whoami_main.c`, `id_main.c`, `su_main.c`, `useradd_main.c`, `passwd_main.c`
- `cmd_runtime.c` / `cmd_runtime.h`

The kernel shell still has a small built-in set, but now prefers launching standalone ELFs from `/shell/*.elf` (including account commands), with fallback to built-ins when an external ELF is missing.

When the session is root, kernel shell automatically hands off to `/shell/xsh.elf`.

`xsh` itself has no built-in admin/file commands: commands like `cd`, `pwd`, `ls`, `cat`, `mkdir`, `touch`, `write`, `append`, `cp`, `mv`, `rm`, `help`, `exit`, `whoami`, `id`, `su`, `useradd`, and `passwd` are all independent ELF programs resolved via `PATH`.

## Accounts

- Account db files live in `/system/etc/passwd`, `/system/etc/shadow`, `/system/etc/group`
- Default account: `root` (uid/gid `0`, home `/home/root`)
- Password hash format is a lightweight project format (`x1$<hex-hash>`) suitable for this stage of the OS
- `useradd <name>` creates user/group entries and `/home/<name>`
- `passwd [user]` updates password (`root` can update any user)
- `su [user]` switches effective shell identity and updates prompt/permissions

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
