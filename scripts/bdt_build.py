#!/usr/bin/env python3
"""XiaoBaiOS build actions used by project.bdt.

The bdt project file owns the target graph. This helper keeps the dynamic parts
that are awkward to express in a static target file: source discovery,
menuconfig feature mapping, Limine preparation, and ISO layout.
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import tarfile
from pathlib import Path
from typing import Iterable, Sequence


ROOT = Path(__file__).resolve().parent.parent
BUILD_ROOT = ROOT / "build" / "x86_64"
OBJ_ROOT = BUILD_ROOT / "obj"
ISO_ROOT = BUILD_ROOT / "iso_root"
KERNEL_ELF = BUILD_ROOT / "clks_kernel.elf"
KERNEL_RUST_LIB = BUILD_ROOT / "libclks_kernel_rust.a"
KERNEL_SYMBOLS_FILE = BUILD_ROOT / "kernel.sym"
RAMDISK_ROOT = BUILD_ROOT / "ramdisk_root"
RAMDISK_IMAGE = BUILD_ROOT / "xiaobaios_ramdisk.tar"
DISK_IMAGE = BUILD_ROOT / "xiaobaios_disk.img"
ISO_IMAGE = ROOT / "build" / "XiaoBaiOS-x86_64.iso"
LIMINE_READY_STAMP = BUILD_ROOT / ".limine.ready"

MENUCONFIG_JSON = ROOT / "configs" / "menuconfig" / ".config.json"
FEATURES_JSON = ROOT / "configs" / "menuconfig" / "clks_features.json"

CLKS_ARCH = "x86_64"
LINKER_SCRIPT = ROOT / "clks" / "arch" / CLKS_ARCH / "linker.ld"
TTY_FONT_SOURCE = ROOT / "fonts" / "system.ttf"
XDE_FONT_SOURCE = ROOT / "fonts" / "xde.ttf"
TTY_PSF_IMAGE = BUILD_ROOT / "generated" / "tty.psf"
XDE_START_ICON_SOURCE = ROOT / "icons" / "XDE_48x48.png"
XDE_AVATAR_ICON_SOURCE = ROOT / "icons" / "XDE_128x128.png"
XDE_START_ICON_RAW = BUILD_ROOT / "generated" / "xde_start.rgba"
XDE_AVATAR_ICON_RAW = BUILD_ROOT / "generated" / "xde_avatar.rgba"

USER_APP_SOURCE_DIR = ROOT / "xiaobaios" / "apps"
USER_INCLUDE_DIR = ROOT / "xiaobaios" / "include"
USER_COMMON_SOURCE_DIR = ROOT / "xiaobaios" / "src"
USER_LINKER_SCRIPT = ROOT / "xiaobaios" / "user.ld"
USER_KELF_LINKER_SCRIPT = ROOT / "xiaobaios" / "kelf.ld"
USER_BUILD_ROOT = BUILD_ROOT / "xiaobaios_user"
USER_OBJ_ROOT = USER_BUILD_ROOT / "obj"
USER_APP_DIR = USER_BUILD_ROOT / "apps"
LEXBOR_SOURCE_DIR = ROOT / "third_party" / "lexbor" / "source"
BEARSSL_SOURCE_DIR = ROOT / "third_party" / "bearssl"

SOURCE_NAMESPACE = "CLEONOS_"
TARGET_NAMESPACE = "XIAOBAIOS_"

FEATURE_DEFAULTS = {
    "XIAOBAIOS_CLKS_ENABLE_AUDIO": True,
    "XIAOBAIOS_CLKS_ENABLE_MOUSE": True,
    "XIAOBAIOS_CLKS_ENABLE_DESKTOP": True,
    "XIAOBAIOS_CLKS_ENABLE_NET": True,
    "XIAOBAIOS_CLKS_ENABLE_DRIVER_MANAGER": True,
    "XIAOBAIOS_CLKS_ENABLE_KELF": True,
    "XIAOBAIOS_CLKS_ENABLE_USERLAND_AUTO_EXEC": True,
    "XIAOBAIOS_CLKS_ENABLE_HEAP_SELFTEST": True,
    "XIAOBAIOS_CLKS_ENABLE_EXTERNAL_PSF": False,
    "XIAOBAIOS_CLKS_ENABLE_KEYBOARD": True,
    "XIAOBAIOS_CLKS_ENABLE_ELFRUNNER_PROBE": True,
    "XIAOBAIOS_CLKS_ENABLE_KLOGD_TASK": True,
    "XIAOBAIOS_CLKS_ENABLE_KWORKER_TASK": True,
    "XIAOBAIOS_CLKS_ENABLE_USRD_TASK": True,
    "XIAOBAIOS_CLKS_ENABLE_BOOT_VIDEO_LOG": True,
    "XIAOBAIOS_CLKS_ENABLE_PMM_STATS_LOG": True,
    "XIAOBAIOS_CLKS_ENABLE_HEAP_STATS_LOG": True,
    "XIAOBAIOS_CLKS_ENABLE_FS_ROOT_LOG": True,
    "XIAOBAIOS_CLKS_ENABLE_SYSTEM_DIR_CHECK": True,
    "XIAOBAIOS_CLKS_ENABLE_ELFRUNNER_INIT": True,
    "XIAOBAIOS_CLKS_ENABLE_SYSCALL_TICK_QUERY": True,
    "XIAOBAIOS_CLKS_ENABLE_TTY_READY_LOG": True,
    "XIAOBAIOS_CLKS_ENABLE_IDLE_DEBUG_LOG": True,
    "XIAOBAIOS_CLKS_ENABLE_PROCFS": True,
    "XIAOBAIOS_CLKS_ENABLE_EXEC_SERIAL_LOG": True,
    "XIAOBAIOS_CLKS_ENABLE_SYSCALL_SERIAL_LOG": True,
    "XIAOBAIOS_CLKS_ENABLE_SYSCALL_USERID_SERIAL_LOG": True,
    "XIAOBAIOS_CLKS_ENABLE_LOG_LEVEL_DEBUG": True,
    "XIAOBAIOS_CLKS_ENABLE_LOG_LEVEL_INFO": True,
    "XIAOBAIOS_CLKS_ENABLE_LOG_LEVEL_WARN": True,
    "XIAOBAIOS_CLKS_ENABLE_LOG_LEVEL_ERROR": True,
    "XIAOBAIOS_CLKS_ENABLE_LOG_OUTPUT_SERIAL": True,
    "XIAOBAIOS_CLKS_ENABLE_LOG_OUTPUT_TTY": True,
    "XIAOBAIOS_CLKS_ENABLE_LOG_OUTPUT_JOURNAL": True,
    "XIAOBAIOS_CLKS_ENABLE_USC": False,
    "XIAOBAIOS_CLKS_ENABLE_KBD_TTY_SWITCH_HOTKEY": True,
    "XIAOBAIOS_CLKS_ENABLE_KBD_CTRL_SHORTCUTS": True,
    "XIAOBAIOS_CLKS_ENABLE_KBD_FORCE_STOP_HOTKEY": True,
    "XIAOBAIOS_CLKS_ENABLE_USER_INIT_SCRIPT_PROBE": True,
    "XIAOBAIOS_CLKS_ENABLE_USER_SYSTEM_APP_PROBE": False,
    "XIAOBAIOS_CLKS_ENABLE_SCHED_TASK_COUNT_LOG": True,
    "XIAOBAIOS_CLKS_ENABLE_INTERRUPT_READY_LOG": True,
    "XIAOBAIOS_CLKS_ENABLE_SHELL_MODE_LOG": True,
}

XSH_AUTO_BOOT_REQUIRES = (
    "XIAOBAIOS_CLKS_ENABLE_DRIVER_MANAGER",
    "XIAOBAIOS_CLKS_ENABLE_KELF",
    "XIAOBAIOS_CLKS_ENABLE_USERLAND_AUTO_EXEC",
    "XIAOBAIOS_CLKS_ENABLE_KEYBOARD",
    "XIAOBAIOS_CLKS_ENABLE_ELFRUNNER_INIT",
    "XIAOBAIOS_CLKS_ENABLE_ELFRUNNER_PROBE",
    "XIAOBAIOS_CLKS_ENABLE_USRD_TASK",
)


class BuildError(RuntimeError):
    pass


def log(level: str, text: str) -> None:
    print(f"[{level}]{text}")


def step(text: str) -> None:
    log("STEP", f" {text}")


def info(text: str) -> None:
    log("INFO", f" {text}")


def warn(text: str) -> None:
    log("WARN", f" {text}")


def fail(text: str) -> None:
    raise BuildError(text)


def env_tool(name: str, default: str, fallbacks: Sequence[str] = ()) -> str:
    requested = os.environ.get(name, default)
    if os.path.isabs(requested):
        if Path(requested).exists():
            return requested
        fail(f"{name} not found: {requested}")
    if shutil.which(requested):
        return requested
    for candidate in fallbacks:
        if shutil.which(candidate):
            warn(f"{name} '{requested}' not found; fallback to '{candidate}'")
            return candidate
    fail(f"{name} tool not found: '{requested}'")
    return requested


def tools() -> dict[str, str]:
    return {
        "CC": env_tool("CC", "clang", ("gcc", "cc")),
        "KERNEL_CXX": env_tool("KERNEL_CXX", "clang++", ("g++", "c++")),
        "LD": env_tool("LD", "ld.lld", ("ld",)),
        "USER_CC": env_tool("USER_CC", "clang", ("cc", "gcc")),
        "USER_LD": env_tool("USER_LD", "ld.lld", ("ld",)),
        "RUSTC": env_tool("RUSTC", "rustc"),
        "PYTHON": env_tool("PYTHON", "python3", ("python",)),
        "NM": env_tool("NM", "nm", ("llvm-nm", "x86_64-elf-nm")),
        "ADDR2LINE": env_tool("ADDR2LINE", "addr2line", ("llvm-addr2line", "x86_64-elf-addr2line")),
        "XORRISO": env_tool("XORRISO", "xorriso"),
        "TAR": env_tool("TAR", "tar"),
        "GIT_TOOL": env_tool("GIT_TOOL", "git"),
        "MAKE_TOOL": env_tool("MAKE_TOOL", "make"),
        "SH_TOOL": env_tool("SH_TOOL", "sh"),
        "MAGICK": env_tool("MAGICK", "magick", ("convert",)),
        "QEMU_X86_64": env_tool("QEMU_X86_64", "qemu-system-x86_64"),
        "OBJCOPY_FOR_TARGET": env_tool("OBJCOPY_FOR_TARGET", "llvm-objcopy", ("x86_64-linux-gnu-objcopy", "objcopy")),
        "OBJDUMP_FOR_TARGET": env_tool("OBJDUMP_FOR_TARGET", "llvm-objdump", ("x86_64-linux-gnu-objdump", "objdump")),
        "READELF_FOR_TARGET": env_tool("READELF_FOR_TARGET", "llvm-readelf", ("x86_64-linux-gnu-readelf", "readelf")),
    }


def run(cmd: Sequence[str], cwd: Path | None = None, env: dict[str, str] | None = None) -> None:
    shown = " ".join(str(part) for part in cmd)
    if cwd:
        shown = f"(cd {cwd} && {shown})"
    if os.environ.get("VERBOSE"):
        info(shown)
    try:
        subprocess.run([str(part) for part in cmd], cwd=cwd, env=env, check=True)
    except subprocess.CalledProcessError as exc:
        fail(f"command failed ({exc.returncode}): {shown}")


def capture(cmd: Sequence[str]) -> str:
    try:
        return subprocess.check_output([str(part) for part in cmd], text=True)
    except subprocess.CalledProcessError as exc:
        fail(f"command failed ({exc.returncode}): {' '.join(map(str, cmd))}")
    return ""


def newer(output: Path, inputs: Iterable[Path]) -> bool:
    if not output.exists():
        return False
    out_mtime = output.stat().st_mtime
    return all((not item.exists()) or item.stat().st_mtime <= out_mtime for item in inputs)


def ensure_parent(path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)


def list_files(root: Path, suffixes: tuple[str, ...]) -> list[Path]:
    if not root.exists():
        return []
    return sorted(path for path in root.rglob("*") if path.is_file() and path.suffix in suffixes)


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def obj_path(source: Path, obj_root: Path) -> Path:
    return obj_root / Path(rel(source)).with_suffix(".o")


def remap_key(key: str) -> str:
    if key.startswith(SOURCE_NAMESPACE):
        return TARGET_NAMESPACE + key[len(SOURCE_NAMESPACE):]
    return key


def normalize_bool(value: object) -> bool:
    if isinstance(value, bool):
        return value
    if isinstance(value, (int, float)):
        return int(value) != 0
    if isinstance(value, str):
        return value.strip().lower() in {"1", "y", "yes", "true", "on", "m", "module"}
    return False


def feature_defaults_from_json() -> dict[str, bool]:
    defaults = dict(FEATURE_DEFAULTS)
    if not FEATURES_JSON.exists():
        return defaults
    try:
        raw = json.loads(FEATURES_JSON.read_text(encoding="utf-8"))
    except Exception:
        return defaults
    for item in raw.get("features", []):
        if not isinstance(item, dict) or "key" not in item:
            continue
        defaults[remap_key(str(item["key"]))] = normalize_bool(item.get("default", False))
    return defaults


def config_values() -> dict[str, bool]:
    values = feature_defaults_from_json()
    if MENUCONFIG_JSON.exists():
        try:
            raw = json.loads(MENUCONFIG_JSON.read_text(encoding="utf-8"))
        except Exception as exc:
            warn(f"ignoring invalid menuconfig JSON: {exc}")
            raw = {}
        if isinstance(raw, dict):
            for key, value in raw.items():
                values[remap_key(str(key))] = normalize_bool(value)

    if normalize_bool(os.environ.get("XIAOBAIOS_ENABLE_XSH_AUTO_BOOT", "1")):
        for key in XSH_AUTO_BOOT_REQUIRES:
            if not values.get(key, False):
                warn(f"xsh auto boot requires {key}; forcing ON")
            values[key] = True
    return values


def arch_define_flags() -> list[str]:
    values = config_values()
    flags = ["-DCLKS_ARCH_X86_64=1"]
    for key in sorted(values):
        if not key.startswith("XIAOBAIOS_CLKS_ENABLE_"):
            continue
        define = "CLKS_CFG_" + key.removeprefix("XIAOBAIOS_CLKS_ENABLE_")
        flags.append(f"-D{define}={1 if values[key] else 0}")
    return flags


def common_kernel_cflags() -> list[str]:
    return [
        "-std=c11",
        "-ffreestanding",
        "-fno-stack-protector",
        "-fno-builtin",
        "-mgeneral-regs-only",
        "-O2",
        "-ffunction-sections",
        "-fdata-sections",
        "-fno-unwind-tables",
        "-fno-asynchronous-unwind-tables",
        "-fno-omit-frame-pointer",
        "-g",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-Wno-error",
        f"-I{ROOT / 'clks' / 'include'}",
    ]


def common_kernel_cxxflags() -> list[str]:
    return [
        "-std=c++17",
        "-ffreestanding",
        "-fno-stack-protector",
        "-fno-builtin",
        "-fno-exceptions",
        "-fno-rtti",
        "-fno-threadsafe-statics",
        "-fno-use-cxa-atexit",
        "-O2",
        "-ffunction-sections",
        "-fdata-sections",
        "-fno-unwind-tables",
        "-fno-asynchronous-unwind-tables",
        "-fno-omit-frame-pointer",
        "-g",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-Wno-error",
        f"-I{ROOT / 'clks' / 'include'}",
    ]


def arch_cflags() -> list[str]:
    return [
        *arch_define_flags(),
        "-m64",
        "-mno-red-zone",
        "-mcmodel=kernel",
        "-fno-pic",
        "-fno-pie",
    ]


def asflags() -> list[str]:
    return ["-ffreestanding", f"-I{ROOT / 'clks' / 'include'}", *arch_cflags()]


def kernel_sources() -> tuple[list[Path], list[Path], list[Path]]:
    roots = [ROOT / "clks" / "kernel", ROOT / "clks" / "arch" / CLKS_ARCH, ROOT / "clks" / "third_party"]
    c_sources: list[Path] = []
    cpp_sources: list[Path] = []
    asm_sources: list[Path] = []
    for source_root in roots:
        c_sources.extend(list_files(source_root, (".c",)))
        cpp_sources.extend(list_files(source_root, (".cpp",)))
        asm_sources.extend(list_files(source_root, (".S",)))
    return sorted(set(c_sources)), sorted(set(cpp_sources)), sorted(set(asm_sources))


def kernel_header_inputs() -> list[Path]:
    return list_files(ROOT / "clks", (".h", ".inc"))


def compile_one(tool: str, source: Path, output: Path, flags: Sequence[str], extra_inputs: Sequence[Path] = ()) -> None:
    inputs = [source, *extra_inputs]
    if newer(output, inputs):
        return
    ensure_parent(output)
    run([tool, *flags, "-c", source, "-o", output])


def action_setup_tools() -> None:
    step("checking host tools")
    tools()
    info("required tools are available")


def action_limine_ready() -> None:
    t = tools()
    step("preparing limine")
    limine_dir = ROOT / "limine"
    if not limine_dir.exists():
        fail("limine submodule is missing; run git submodule update --init limine")

    limine_bin_dir = limine_dir
    if not (limine_dir / "limine-bios.sys").exists() and (limine_dir / "bin" / "limine-bios.sys").exists():
        limine_bin_dir = limine_dir / "bin"

    required = ["limine", "limine-bios.sys", "limine-bios-cd.bin", "limine-uefi-cd.bin", "BOOTX64.EFI"]
    if all((limine_bin_dir / item).exists() for item in required):
        ensure_parent(LIMINE_READY_STAMP)
        LIMINE_READY_STAMP.write_text("ready\n", encoding="utf-8")
        info("limine artifacts ready")
        return

    if not (limine_dir / "Makefile").exists():
        if (limine_dir / "bootstrap").exists():
            run([t["SH_TOOL"], "bootstrap"], cwd=limine_dir)
        env = os.environ.copy()
        env.update(
            {
                "OBJCOPY_FOR_TARGET": t["OBJCOPY_FOR_TARGET"],
                "OBJDUMP_FOR_TARGET": t["OBJDUMP_FOR_TARGET"],
                "READELF_FOR_TARGET": t["READELF_FOR_TARGET"],
            }
        )
        if not (limine_dir / "configure").exists():
            fail("limine configure script missing")
        run([t["SH_TOOL"], "configure", "--enable-bios", "--enable-bios-cd", "--enable-uefi-x86-64", "--enable-uefi-cd"], cwd=limine_dir, env=env)

    run([t["MAKE_TOOL"], "-C", limine_dir, "-j4"])
    if all((limine_dir / item).exists() for item in required):
        limine_bin_dir = limine_dir
    elif all((limine_dir / "bin" / item).exists() for item in required):
        limine_bin_dir = limine_dir / "bin"
    else:
        missing = [item for item in required if not (limine_bin_dir / item).exists()]
        fail(f"limine artifacts missing: {', '.join(missing)}")
    ensure_parent(LIMINE_READY_STAMP)
    LIMINE_READY_STAMP.write_text("ready\n", encoding="utf-8")
    info("limine artifacts ready")


def action_kernel() -> None:
    t = tools()
    if not LINKER_SCRIPT.exists():
        fail(f"missing linker script: {LINKER_SCRIPT}")
    c_sources, cpp_sources, asm_sources = kernel_sources()
    if not c_sources:
        fail("no kernel C sources found in clks/")
    headers = kernel_header_inputs()

    common_c = [*common_kernel_cflags(), *arch_cflags()]
    third_party_extra = [
        "-Wno-error",
        "-Wno-unused-function",
        "-Wno-unused-parameter",
        "-Wno-sign-compare",
        "-Wno-type-limits",
        "-Wno-missing-field-initializers",
    ]
    objects: list[Path] = []
    for source in c_sources:
        flags = list(common_c)
        if rel(source).startswith("clks/third_party/"):
            flags.extend(third_party_extra)
        out = obj_path(source, OBJ_ROOT)
        compile_one(t["CC"], source, out, flags, headers)
        objects.append(out)

    cxxflags = [*common_kernel_cxxflags(), *arch_cflags()]
    for source in cpp_sources:
        out = obj_path(source, OBJ_ROOT)
        compile_one(t["KERNEL_CXX"], source, out, cxxflags, headers)
        objects.append(out)

    for source in asm_sources:
        out = obj_path(source, OBJ_ROOT)
        compile_one(t["CC"], source, out, asflags(), headers)
        objects.append(out)

    rust_src = ROOT / "clks" / "rust" / "src" / "lib.rs"
    if not newer(KERNEL_RUST_LIB, [rust_src]):
        ensure_parent(KERNEL_RUST_LIB)
        run([t["RUSTC"], "--crate-type", "staticlib", "-C", "panic=abort", "-O", rust_src, "-o", KERNEL_RUST_LIB])

    link_inputs = [*objects, KERNEL_RUST_LIB, LINKER_SCRIPT]
    if not newer(KERNEL_ELF, link_inputs):
        ensure_parent(KERNEL_ELF)
        run([t["LD"], "-nostdlib", "--gc-sections", "-z", "max-page-size=0x1000", "-T", LINKER_SCRIPT, "-o", KERNEL_ELF, *objects, KERNEL_RUST_LIB])
    info(f"kernel ready: {KERNEL_ELF}")


def action_kernel_symbols() -> None:
    t = tools()
    if not KERNEL_ELF.exists():
        action_kernel()
    if newer(KERNEL_SYMBOLS_FILE, [KERNEL_ELF]):
        info(f"kernel symbols ready: {KERNEL_SYMBOLS_FILE}")
        return
    nm_output = capture([t["NM"], "-n", KERNEL_ELF])
    symbols: list[tuple[str, str]] = []
    for line in nm_output.splitlines():
        parts = line.strip().split(None, 2)
        if len(parts) != 3:
            continue
        addr, kind, name = parts
        if kind not in {"t", "T", "w", "W"} or name.startswith("."):
            continue
        symbols.append((addr.upper(), name))

    src_lines: list[str] = []
    if symbols:
        query = [f"0x{addr}" for addr, _name in symbols]
        a2l = capture([t["ADDR2LINE"], "-f", "-C", "-e", KERNEL_ELF, *query]).splitlines()
        for idx in range(len(symbols)):
            src_lines.append(a2l[(idx * 2) + 1].strip() if (idx * 2) + 1 < len(a2l) and a2l[(idx * 2) + 1].strip() else "??:0")

    ensure_parent(KERNEL_SYMBOLS_FILE)
    with KERNEL_SYMBOLS_FILE.open("w", encoding="utf-8") as out:
        out.write("CLEONOS_KERNEL_SYMBOLS_V2\n")
        for idx, (addr, name) in enumerate(symbols):
            src = src_lines[idx] if idx < len(src_lines) else "??:0"
            out.write(f"0X{addr}\t{name}\t{src.replace(chr(9), ' ')}\n")
    info(f"kernel symbols ready: {KERNEL_SYMBOLS_FILE}")


def user_cflags() -> list[str]:
    return [
        "-std=c11",
        "-ffreestanding",
        "-fno-stack-protector",
        "-fno-builtin",
        "-mgeneral-regs-only",
        "-O2",
        "-ffunction-sections",
        "-fdata-sections",
        "-fno-unwind-tables",
        "-fno-asynchronous-unwind-tables",
        "-fno-omit-frame-pointer",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-Wno-error",
        f"-I{USER_INCLUDE_DIR}",
    ]


def user_headers() -> list[Path]:
    headers = list_files(ROOT / "xiaobaios", (".h", ".inc"))
    if LEXBOR_SOURCE_DIR.exists():
        headers.extend(list_files(LEXBOR_SOURCE_DIR / "lexbor", (".h", ".inc")))
    if BEARSSL_SOURCE_DIR.exists():
        headers.extend(list_files(BEARSSL_SOURCE_DIR / "inc", (".h", ".inc")))
    return headers


def lexbor_html_sources() -> list[Path]:
    roots = [
        LEXBOR_SOURCE_DIR / "lexbor" / "core",
        LEXBOR_SOURCE_DIR / "lexbor" / "dom",
        LEXBOR_SOURCE_DIR / "lexbor" / "html",
        LEXBOR_SOURCE_DIR / "lexbor" / "ns",
        LEXBOR_SOURCE_DIR / "lexbor" / "tag",
    ]

    sources: list[Path] = []
    if not LEXBOR_SOURCE_DIR.exists():
        return sources

    for root in roots:
        sources.extend(list_files(root, (".c",)))

    sources.append(LEXBOR_SOURCE_DIR / "lexbor" / "ports" / "posix" / "lexbor" / "core" / "memory.c")
    return sorted(set(sources))


def bearssl_sources() -> list[Path]:
    if not BEARSSL_SOURCE_DIR.exists():
        return []

    return sorted(
        source
        for source in list_files(BEARSSL_SOURCE_DIR / "src", (".c",))
    )


def app_name_from_main(source: Path, suffix: str) -> str:
    name = source.name
    return name[: -len(suffix)]


def user_app_enabled(name: str) -> bool:
    key = "XIAOBAIOS_USER_APP_" + "".join(ch if ch.isalnum() else "_" for ch in name.upper())
    raw = os.environ.get(key)
    if raw is not None:
        return normalize_bool(raw)
    if name == "xde_terminal":
        return False
    return True


def compile_user_object(source: Path, headers: Sequence[Path], extra_flags: Sequence[str] = ()) -> Path:
    out = obj_path(source, USER_OBJ_ROOT)
    flags = [*user_cflags(), *extra_flags]
    compile_one(tools()["USER_CC"], source, out, flags, headers)
    return out


def compile_lexbor_objects(headers: Sequence[Path]) -> list[Path]:
    flags = [
        f"-I{LEXBOR_SOURCE_DIR}",
        "-Wno-error=unused-function",
        "-Wno-error=unused-parameter",
        "-Wno-error=sign-compare",
        "-Wno-error=type-limits",
        "-Wno-error=unused-variable",
        "-Wno-error=implicit-fallthrough",
    ]
    return [compile_user_object(source, headers, flags) for source in lexbor_html_sources()]


def compile_bearssl_objects(headers: Sequence[Path]) -> list[Path]:
    flags = [
        f"-I{BEARSSL_SOURCE_DIR / 'inc'}",
        f"-I{BEARSSL_SOURCE_DIR / 'src'}",
        "-DBR_RDRAND=0",
        "-DBR_USE_URANDOM=0",
        "-DBR_USE_GETENTROPY=0",
        "-DBR_USE_UNIX_TIME=0",
        "-DBR_USE_WIN32_RAND=0",
        "-DBR_USE_WIN32_TIME=0",
        "-Wno-error=unused-function",
        "-Wno-error=unused-parameter",
        "-Wno-error=sign-compare",
        "-Wno-error=type-limits",
        "-Wno-error=unused-variable",
        "-Wno-error=implicit-fallthrough",
    ]
    return [compile_user_object(source, headers, flags) for source in bearssl_sources()]


def action_userapps() -> None:
    t = tools()
    headers = user_headers()
    common_sources = list_files(USER_COMMON_SOURCE_DIR, (".c",))
    app_sources = list_files(USER_APP_SOURCE_DIR, (".c",))
    main_sources = sorted(source for source in app_sources if source.name.endswith("_main.c"))
    kmain_sources = sorted(source for source in app_sources if source.name.endswith("_kmain.c"))
    support_sources = sorted(source for source in app_sources if not source.name.endswith("_main.c") and not source.name.endswith("_kmain.c"))

    common_objects = [compile_user_object(source, headers) for source in common_sources]
    support_objects = [compile_user_object(source, headers) for source in support_sources]
    lexbor_objects: list[Path] | None = None
    bearssl_objects: list[Path] | None = None
    USER_APP_DIR.mkdir(parents=True, exist_ok=True)

    outputs: list[Path] = []
    for source in main_sources:
        app = app_name_from_main(source, "_main.c")
        if not user_app_enabled(app):
            info(f"menuconfig: skip disabled xiaobaios user app {app}")
            continue
        extra = ["-Wno-error=unused-function"] if app in {"xde", "xde_terminal"} else []
        if app == "web":
            if not LEXBOR_SOURCE_DIR.exists():
                fail("web app requires third_party/lexbor")
            if not BEARSSL_SOURCE_DIR.exists():
                fail("web app requires third_party/bearssl")
            if lexbor_objects is None:
                lexbor_objects = compile_lexbor_objects(headers)
            if bearssl_objects is None:
                bearssl_objects = compile_bearssl_objects(headers)
            extra = [*extra, f"-I{LEXBOR_SOURCE_DIR}", f"-I{BEARSSL_SOURCE_DIR / 'inc'}"]
        app_objects = [*common_objects, *support_objects, compile_user_object(source, headers, extra)]
        if app == "web":
            app_objects.extend(lexbor_objects or [])
            app_objects.extend(bearssl_objects or [])
        cmd_source = USER_APP_SOURCE_DIR / f"{app}_cmd.c"
        if cmd_source.exists():
            app_objects.append(compile_user_object(cmd_source, headers))
        out = USER_APP_DIR / f"{app}.elf"
        inputs = [*app_objects, USER_LINKER_SCRIPT]
        if not newer(out, inputs):
            run([t["USER_LD"], "-nostdlib", "--gc-sections", "-z", "max-page-size=0x1000", "-T", USER_LINKER_SCRIPT, "-o", out, *app_objects])
        outputs.append(out)

    for source in kmain_sources:
        app = app_name_from_main(source, "_kmain.c")
        if not user_app_enabled(app):
            info(f"menuconfig: skip disabled xiaobaios user app {app}")
            continue
        obj = compile_user_object(source, headers)
        out = USER_APP_DIR / f"{app}.elf"
        if not newer(out, [obj, USER_KELF_LINKER_SCRIPT]):
            run([t["USER_LD"], "-nostdlib", "--gc-sections", "-z", "max-page-size=0x1000", "-T", USER_KELF_LINKER_SCRIPT, "-o", out, obj])
        outputs.append(out)

    for prebuilt in sorted(USER_APP_SOURCE_DIR.glob("*.elf")):
        out = USER_APP_DIR / prebuilt.name
        if not newer(out, [prebuilt]):
            ensure_parent(out)
            shutil.copy2(prebuilt, out)
        outputs.append(out)

    if not outputs:
        warn(f"no xiaobaios user ELFs found under {USER_APP_SOURCE_DIR}")
    info("xiaobaios user elfs ready")


def generate_assets() -> None:
    t = tools()
    values = config_values()
    if not XDE_FONT_SOURCE.exists():
        fail(f"missing XDE TTF font source: {XDE_FONT_SOURCE}")
    if values.get("XIAOBAIOS_CLKS_ENABLE_EXTERNAL_PSF", False):
        if not TTY_FONT_SOURCE.exists():
            fail(f"missing TTF font source: {TTY_FONT_SOURCE}")
        if not newer(TTY_PSF_IMAGE, [TTY_FONT_SOURCE, ROOT / "scripts" / "make_psf_font.py"]):
            ensure_parent(TTY_PSF_IMAGE)
            run(
                [
                    t["PYTHON"],
                    ROOT / "scripts" / "make_psf_font.py",
                    "--font",
                    TTY_FONT_SOURCE,
                    "--output",
                    TTY_PSF_IMAGE,
                    "--width",
                    "10",
                    "--height",
                    "20",
                    "--font-size",
                    "16",
                    "--threshold",
                    "172",
                ]
            )

    for source, output in ((XDE_START_ICON_SOURCE, XDE_START_ICON_RAW), (XDE_AVATAR_ICON_SOURCE, XDE_AVATAR_ICON_RAW)):
        if not source.exists():
            fail(f"missing icon source: {source}")
        if not newer(output, [source]):
            ensure_parent(output)
            run([t["MAGICK"], source, "-depth", "8", "-define", "stream:format=rgba", output])


def copy_ramdisk_file(source: Path, dest: Path) -> None:
    if source.is_dir():
        shutil.copytree(source, dest, dirs_exist_ok=True)
        return
    ensure_parent(dest)
    shutil.copy2(source, dest)


def action_ramdisk_root() -> None:
    if not USER_APP_DIR.exists():
        action_userapps()
    generate_assets()
    if RAMDISK_ROOT.exists():
        shutil.rmtree(RAMDISK_ROOT)
    RAMDISK_ROOT.mkdir(parents=True)
    base = ROOT / "ramdisk"
    if base.exists():
        shutil.copytree(base, RAMDISK_ROOT, dirs_exist_ok=True)
    for dirname in ("system", "system/etc", "shell", "driver", "dev", "home", "temp"):
        (RAMDISK_ROOT / dirname).mkdir(parents=True, exist_ok=True)

    shutil.copy2(XDE_FONT_SOURCE, RAMDISK_ROOT / "system" / "xde.ttf")
    if config_values().get("XIAOBAIOS_CLKS_ENABLE_EXTERNAL_PSF", False):
        shutil.copy2(TTY_PSF_IMAGE, RAMDISK_ROOT / "system" / "tty.psf")
    shutil.copy2(XDE_START_ICON_RAW, RAMDISK_ROOT / "system" / "xde_start.rgba")
    shutil.copy2(XDE_AVATAR_ICON_RAW, RAMDISK_ROOT / "system" / "xde_avatar.rgba")

    for app in sorted(USER_APP_DIR.glob("*.elf")):
        name = app.stem
        if name.endswith("drv"):
            dest = RAMDISK_ROOT / "driver" / app.name
        elif name == "hello":
            dest = RAMDISK_ROOT / app.name
        elif (USER_APP_SOURCE_DIR / f"{name}_kmain.c").exists():
            dest = RAMDISK_ROOT / "system" / app.name
        else:
            dest = RAMDISK_ROOT / "shell" / app.name
        shutil.copy2(app, dest)
        if name == "xsh":
            shutil.copy2(app, RAMDISK_ROOT / "shell" / "shell.elf")

    info(f"ramdisk root ready: {RAMDISK_ROOT}")


def action_ramdisk() -> None:
    if not RAMDISK_ROOT.exists():
        action_ramdisk_root()
    ensure_parent(RAMDISK_IMAGE)
    with tarfile.open(RAMDISK_IMAGE, "w") as tar:
        for item in sorted(RAMDISK_ROOT.rglob("*")):
            tar.add(item, arcname=item.relative_to(RAMDISK_ROOT))
    info(f"ramdisk ready: {RAMDISK_IMAGE}")


def action_disk_image() -> None:
    disk_mb = int(os.environ.get("DISK_IMAGE_MB", "64") or "64")
    disk_mb = max(4, disk_mb)
    size = disk_mb * 1024 * 1024
    ensure_parent(DISK_IMAGE)
    current = DISK_IMAGE.stat().st_size if DISK_IMAGE.exists() else 0
    if current < size:
        with DISK_IMAGE.open("ab") as out:
            out.truncate(size)
    info(f"ensure_disk_image: ready '{DISK_IMAGE}' ({DISK_IMAGE.stat().st_size} bytes)")


def limine_bin_dir() -> Path:
    limine_dir = ROOT / "limine"
    if (limine_dir / "limine-bios.sys").exists():
        return limine_dir
    if (limine_dir / "bin" / "limine-bios.sys").exists():
        return limine_dir / "bin"
    fail("limine artifacts are missing")
    return limine_dir


def action_iso() -> None:
    t = tools()
    if not KERNEL_ELF.exists():
        action_kernel()
    if not RAMDISK_IMAGE.exists():
        action_ramdisk()
    if not LIMINE_READY_STAMP.exists():
        action_limine_ready()

    bin_dir = limine_bin_dir()
    if ISO_ROOT.exists():
        shutil.rmtree(ISO_ROOT)
    (ISO_ROOT / "boot" / "limine").mkdir(parents=True, exist_ok=True)
    (ISO_ROOT / "EFI" / "BOOT").mkdir(parents=True, exist_ok=True)
    shutil.copy2(KERNEL_ELF, ISO_ROOT / "boot" / "clks_kernel.elf")
    shutil.copy2(RAMDISK_IMAGE, ISO_ROOT / "boot" / "xiaobaios_ramdisk.tar")
    for dest in (ISO_ROOT / "boot" / "limine" / "limine.conf", ISO_ROOT / "EFI" / "BOOT" / "limine.conf", ISO_ROOT / "limine.conf"):
        shutil.copy2(ROOT / "configs" / "limine.conf", dest)
    for name in ("limine-bios.sys", "limine-bios-cd.bin", "limine-uefi-cd.bin"):
        shutil.copy2(bin_dir / name, ISO_ROOT / "boot" / "limine" / name)
    shutil.copy2(bin_dir / "BOOTX64.EFI", ISO_ROOT / "EFI" / "BOOT" / "BOOTX64.EFI")

    ensure_parent(ISO_IMAGE)
    step(f"building iso -> {ISO_IMAGE}")
    run(
        [
            t["XORRISO"],
            "-as",
            "mkisofs",
            "-b",
            "boot/limine/limine-bios-cd.bin",
            "-no-emul-boot",
            "-boot-load-size",
            "4",
            "-boot-info-table",
            "--efi-boot",
            "boot/limine/limine-uefi-cd.bin",
            "-efi-boot-part",
            "--efi-boot-image",
            "--protective-msdos-label",
            ISO_ROOT,
            "-o",
            ISO_IMAGE,
        ]
    )
    step("installing limine boot sectors")
    run([bin_dir / "limine", "bios-install", ISO_IMAGE])
    info(f"iso ready: {ISO_IMAGE}")


def qemu_base_args() -> list[object]:
    t = tools()
    return [
        t["QEMU_X86_64"],
        "-M",
        "pc",
        "-m",
        "1024M",
        "-cdrom",
        ISO_IMAGE,
        "-drive",
        f"file={DISK_IMAGE},format=raw,if=none,id=xiaobaidisk,media=disk",
        "-device",
        "ide-hd,drive=xiaobaidisk,bus=ide.0",
        "-serial",
        "stdio",
    ]


def action_run(debug: bool = False) -> None:
    if not ISO_IMAGE.exists():
        action_iso()
    if not DISK_IMAGE.exists():
        action_disk_image()
    step("launching qemu debug (-s -S)" if debug else "launching qemu run")
    args = qemu_base_args()
    if debug:
        args.extend(["-s", "-S"])
    run(args)


def action_clean() -> None:
    if BUILD_ROOT.exists():
        shutil.rmtree(BUILD_ROOT)
    info("cleaned build/x86_64")


def action_clean_all() -> None:
    build = ROOT / "build"
    if build.exists():
        shutil.rmtree(build)
    info("cleaned build")


def action_clean_drive_image() -> None:
    if DISK_IMAGE.exists():
        DISK_IMAGE.unlink()
    info(f"removed {DISK_IMAGE}")


def main(argv: Sequence[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("action")
    args = parser.parse_args(argv)
    actions = {
        "setup-tools": action_setup_tools,
        "limine-ready": action_limine_ready,
        "kernel": action_kernel,
        "kernel-symbols": action_kernel_symbols,
        "userapps": action_userapps,
        "ramdisk-root": action_ramdisk_root,
        "ramdisk": action_ramdisk,
        "disk-image": action_disk_image,
        "iso": action_iso,
        "run": lambda: action_run(False),
        "debug": lambda: action_run(True),
        "clean": action_clean,
        "clean-all": action_clean_all,
        "clean-drive-image": action_clean_drive_image,
    }
    if args.action not in actions:
        fail(f"unknown action: {args.action}")
    actions[args.action]()
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main(sys.argv[1:]))
    except BuildError as exc:
        log("ERROR", f" {exc}")
        raise SystemExit(1)
