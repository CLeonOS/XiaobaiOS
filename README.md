# XiaoBaiOS

XiaoBaiOS is an independent operating system project based on the CLKS kernel.

This repository now exists as an independent CLKS distribution workspace and no longer depends on the upstream CLeonOS user-space application integration path. It retains the CLKS kernel and boot chain, terminal-style shell, terminal-based `menuconfig`, the default LLVM toolchain, and XiaoBaiOS user-space programs that are built and packaged into the ramdisk.

## Key Features

- Standalone distribution directory located at `xiaobaios/`
- Default LLVM toolchain: `clang`, `ld.lld`, `llvm-objcopy`, `llvm-objdump`, `llvm-readelf`, `llvm-nm`, `llvm-addr2line`
- Pure terminal `menuconfig` with Chinese support
- Builds a PSF font from `fonts/system.ttf`
- User programs under `xiaobaios/apps/` are compiled and packaged into the ramdisk
- Provides a runtime disk image plus disk tools: `diskinfo`, `mkfsfat32`, `mount`, `partctl`
- Provides Linux-like account foundations: `/system/etc/passwd`, `/system/etc/shadow`, `/system/etc/group`
- Provides user-space account management ELF tools: `whoami`, `id`, `su`, `useradd`, `passwd`
- Provides user-space shell: `xsh` (external ELF commands only, resolved via `PATH`)

## Repository Structure

- `clks/` - Kernel, runtime, drivers, shell, and supporting code  
- `cmake/` - Build helper scripts  
- `configs/` - Limine and `menuconfig` generated configuration  
- `fonts/` - Font assets  
- `ramdisk/` - Base ramdisk contents  
- `scripts/` - Font conversion and `menuconfig` helper scripts  
- `xiaobaios/apps/` - Source code for this distribution’s custom user-space programs  
- `xiaobaios/include/` - Custom headers used by these user-space programs  
- `limine/` - Limine bootloader source tree  

## Build

The project is driven by CMake and wrapped by the top-level Makefile.

```bash
make menuconfig
make iso
make run
```

Common targets:

```bash
make setup
make kernel
make disk-image
make ramdisk
make debug
make clean
make clean-all
make clean-drive-image
```

## menuconfig

`menuconfig` is a TUI interactive interface and does not use Qt.

- Runs in interactive mode when executed in a TTY
- Automatically uses non-interactive mode in pipelines or script environments
- Generated configuration is written to `configs/menuconfig/.config.json` and `configs/menuconfig/config.cmake`

## User Programs

User-space source code under `xiaobaios/apps/` is compiled during the build process and packaged into the ramdisk.

The kernel shell keeps a small number of built-in commands, but now prioritizes standalone programs in `/shell/*.elf` (including account management commands). If an external ELF does not exist, it falls back to the built-in command.

When the current session identity is `root`, the kernel shell automatically switches to `/shell/xsh.elf`.

`xsh` itself no longer has built-in file/management commands: `cd`, `pwd`, `ls`, `cat`, `mkdir`, `touch`, `write`, `append`, `cp`, `mv`, `rm`, `help`, `exit`, `whoami`, `id`, `su`, `useradd`, and `passwd` are all standalone ELF programs executed through `PATH` resolution.

The runtime disk image is `build/x86_64/xiaobaios_disk.img`, and `make run` / `make debug` attach it automatically.

## Account System

- Account databases are located at `/system/etc/passwd`, `/system/etc/shadow`, `/system/etc/group`
- The default account is `root` (`uid/gid` are both `0`, home is `/home/root`)
- Password hashes currently use the project’s lightweight format `x1$<hex-hash>` (suitable for the current stage)
- `useradd <name>` creates user/group entries and creates `/home/<name>`
- `passwd [user]` changes passwords (`root` can change any user’s password)
- `su [user]` switches the current shell identity and updates prompt and permissions

## Runtime Disk

- The runtime disk image is `build/x86_64/xiaobaios_disk.img`
- `make run` and `make debug` attach the disk automatically
- `make disk-image` creates or grows the image to the configured size

## Build Artifacts

Build outputs are in the `build/` directory, for example:

```text
build/XiaoBaiOS-x86_64.iso
build/x86_64/clks_kernel.elf
build/x86_64/xiaobaios_ramdisk.tar
build/x86_64/generated/tty.psf
```

## Notes

- This repository remains independent and no longer uses the upstream `cleonos` user-space program integration path.
- Kernel logs are no longer displayed in the interactive shell.
- If you regenerate the build directory, do not commit artifacts such as `build/`, `build-cmake*/`, `*.iso`, or `*.tar`.
