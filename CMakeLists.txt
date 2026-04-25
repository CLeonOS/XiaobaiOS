cmake_minimum_required(VERSION 3.20)
project(XiaoBaiOS NONE)

set(NO_COLOR 0 CACHE STRING "Disable colored log output when set to 1")
include("${CMAKE_SOURCE_DIR}/cmake/log.cmake")
set(CL_LOG_EMIT_SCRIPT "${CMAKE_SOURCE_DIR}/cmake/log_emit.cmake")

set(CC "clang" CACHE STRING "Kernel C compiler")
set(KERNEL_CXX "clang++" CACHE STRING "Kernel C++ compiler")
set(LD "ld.lld" CACHE STRING "Kernel linker")
set(USER_CC "clang" CACHE STRING "User-space C compiler")
set(USER_LD "ld.lld" CACHE STRING "User-space linker")
set(RUSTC "rustc" CACHE STRING "Rust compiler")
set(PYTHON "python3" CACHE STRING "Python interpreter")
set(NM "nm" CACHE STRING "nm executable")
set(ADDR2LINE "addr2line" CACHE STRING "addr2line executable")
set(XORRISO "xorriso" CACHE STRING "xorriso executable")
set(TAR "tar" CACHE STRING "tar executable")
set(GIT_TOOL "git" CACHE STRING "git executable")
set(MAKE_TOOL "make" CACHE STRING "make executable")
set(SH_TOOL "sh" CACHE STRING "POSIX shell executable")

set(OBJCOPY_FOR_TARGET "llvm-objcopy" CACHE STRING "objcopy tool for limine configure")
set(OBJDUMP_FOR_TARGET "llvm-objdump" CACHE STRING "objdump tool for limine configure")
set(READELF_FOR_TARGET "llvm-readelf" CACHE STRING "readelf tool for limine configure")

set(QEMU_X86_64 "qemu-system-x86_64" CACHE STRING "QEMU executable")

set(LIMINE_DIR "${CMAKE_SOURCE_DIR}/limine" CACHE STRING "Limine source directory")
set(LIMINE_BIN_DIR "${LIMINE_DIR}/bin" CACHE STRING "Limine bin directory")
set(LIMINE_DIR_ABS "${LIMINE_DIR}")
set(LIMINE_BIN_DIR_ABS "${LIMINE_BIN_DIR}")
set(XIAOBAIOS_MENUCONFIG_CMAKE "${CMAKE_SOURCE_DIR}/configs/menuconfig/config.cmake" CACHE FILEPATH "menuconfig generated CMake config")

if(EXISTS "${XIAOBAIOS_MENUCONFIG_CMAKE}")
    include("${XIAOBAIOS_MENUCONFIG_CMAKE}")
    cl_log_info("menuconfig loaded from ${XIAOBAIOS_MENUCONFIG_CMAKE}")
endif()

function(resolve_tool_with_fallback VAR_NAME)
    set(_fallbacks ${ARGN})
    set(_requested "${${VAR_NAME}}")

    if("${_requested}" STREQUAL "")
        cl_log_error("empty tool variable: ${VAR_NAME}")
    endif()

    if(IS_ABSOLUTE "${_requested}")
        if(NOT EXISTS "${_requested}")
            cl_log_error("${VAR_NAME} not found: ${_requested}")
        endif()
        set(_resolved "${_requested}")
    else()
        find_program(_requested_path NAMES "${_requested}")
        if(_requested_path)
            set(_resolved "${_requested}")
        else()
            set(_resolved "")
            foreach(_cand IN LISTS _fallbacks)
                find_program(_cand_path NAMES "${_cand}")
                if(_cand_path)
                    cl_log_warn("${VAR_NAME} '${_requested}' not found; fallback to '${_cand}'")
                    set(_resolved "${_cand}")
                    break()
                endif()
            endforeach()
            if("${_resolved}" STREQUAL "")
                cl_log_error("${VAR_NAME} tool not found: '${_requested}', fallbacks='${_fallbacks}'")
            endif()
        endif()
    endif()

    set(${VAR_NAME} "${_resolved}" CACHE STRING "${VAR_NAME}" FORCE)
    set(${VAR_NAME} "${_resolved}" PARENT_SCOPE)
endfunction()

function(resolve_path INPUT_PATH OUTPUT_VAR)
    if(IS_ABSOLUTE "${INPUT_PATH}")
        set(${OUTPUT_VAR} "${INPUT_PATH}" PARENT_SCOPE)
    else()
        set(${OUTPUT_VAR} "${CMAKE_SOURCE_DIR}/${INPUT_PATH}" PARENT_SCOPE)
    endif()
endfunction()

resolve_path("${LIMINE_DIR}" LIMINE_DIR_ABS)
resolve_path("${LIMINE_BIN_DIR}" LIMINE_BIN_DIR_ABS)

if(EXISTS "${LIMINE_DIR_ABS}/limine-bios.sys")
    set(LIMINE_BIN_DIR_ABS "${LIMINE_DIR_ABS}")
elseif(EXISTS "${LIMINE_DIR_ABS}/bin/limine-bios.sys")
    set(LIMINE_BIN_DIR_ABS "${LIMINE_DIR_ABS}/bin")
endif()

set(BUILD_ROOT "${CMAKE_SOURCE_DIR}/build/x86_64")
set(OBJ_ROOT "${BUILD_ROOT}/obj")
set(ISO_ROOT "${BUILD_ROOT}/iso_root")
set(KERNEL_ELF "${BUILD_ROOT}/clks_kernel.elf")
set(RAMDISK_IMAGE "${BUILD_ROOT}/xiaobaios_ramdisk.tar")
set(DISK_IMAGE "${BUILD_ROOT}/xiaobaios_disk.img")
set(XIAOBAIOS_DISK_IMAGE_MB "64" CACHE STRING "Default runtime disk image size in MB")
set(XIAOBAIOS_TTY_FONT_SOURCE "${CMAKE_SOURCE_DIR}/fonts/system.ttf")
set(XIAOBAIOS_TTY_PSF_IMAGE "${BUILD_ROOT}/generated/tty.psf")
set(ISO_IMAGE "${CMAKE_SOURCE_DIR}/build/XiaoBaiOS-x86_64.iso")
set(KERNEL_RUST_LIB "${BUILD_ROOT}/libclks_kernel_rust.a")
set(KERNEL_SYMBOLS_FILE "${BUILD_ROOT}/kernel.sym")
set(LIMINE_READY_STAMP "${BUILD_ROOT}/.limine.ready")

set(CLKS_ARCH "x86_64" CACHE STRING "Target CLKS arch")
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/clks/arch/${CLKS_ARCH}/linker.ld")

resolve_tool_with_fallback(CC clang gcc cc)
resolve_tool_with_fallback(KERNEL_CXX clang++ g++ c++)
resolve_tool_with_fallback(LD ld.lld ld)
resolve_tool_with_fallback(USER_CC clang cc gcc)
resolve_tool_with_fallback(USER_LD ld.lld ld)
resolve_tool_with_fallback(PYTHON python3 python)
resolve_tool_with_fallback(NM llvm-nm x86_64-elf-nm nm)
resolve_tool_with_fallback(ADDR2LINE llvm-addr2line x86_64-elf-addr2line addr2line)
resolve_tool_with_fallback(OBJCOPY_FOR_TARGET llvm-objcopy x86_64-linux-gnu-objcopy objcopy)
resolve_tool_with_fallback(OBJDUMP_FOR_TARGET llvm-objdump x86_64-linux-gnu-objdump objdump)
resolve_tool_with_fallback(READELF_FOR_TARGET llvm-readelf x86_64-linux-gnu-readelf readelf)

function(cl_set_bool_cache VAR_NAME DEFAULT_VALUE DOC_TEXT)
    set(_raw_value "")
    set(_has_value OFF)

    if(DEFINED ${VAR_NAME})
        set(_raw_value "${${VAR_NAME}}")
        set(_has_value ON)
    elseif(DEFINED ${VAR_NAME}_IS_ENABLED)
        set(_raw_value "${${VAR_NAME}_IS_ENABLED}")
        set(_has_value ON)
    endif()

    if(NOT _has_value)
        set(${VAR_NAME} ${DEFAULT_VALUE} CACHE BOOL "${DOC_TEXT}")
    else()
        string(TOUPPER "${_raw_value}" _raw_upper)
        if(_raw_upper STREQUAL "Y" OR _raw_upper STREQUAL "M" OR _raw_upper STREQUAL "ON" OR _raw_upper STREQUAL "TRUE" OR _raw_upper STREQUAL "1")
            set(_bool_value ON)
        else()
            set(_bool_value OFF)
        endif()
        set(${VAR_NAME} ${_bool_value} CACHE BOOL "${DOC_TEXT}" FORCE)
    endif()
endfunction()

function(cl_bool_to_int BOOL_VAR OUT_VAR)
    if(${BOOL_VAR})
        set(${OUT_VAR} 1 PARENT_SCOPE)
    else()
        set(${OUT_VAR} 0 PARENT_SCOPE)
    endif()
endfunction()

cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_AUDIO ON "Initialize kernel audio subsystem")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_MOUSE ON "Initialize kernel mouse subsystem")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_DESKTOP ON "Initialize desktop compositor subsystem")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_NET ON "Initialize kernel network subsystem")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_DRIVER_MANAGER ON "Initialize kernel driver manager")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_KELF ON "Enable KELF app dispatcher")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_USERLAND_AUTO_EXEC ON "Auto exec /shell/shell.elf on boot")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_HEAP_SELFTEST ON "Run heap selftest at boot")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_EXTERNAL_PSF OFF "Load /system/tty.psf external font")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_KEYBOARD ON "Initialize keyboard input subsystem")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_ELFRUNNER_PROBE ON "Run ELFRUNNER kernel ELF probe")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_KLOGD_TASK ON "Enable scheduler klogd task")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_KWORKER_TASK ON "Enable scheduler kworker task")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_USRD_TASK ON "Enable user/runtime dispatch task (shell tick, tty tick, exec tick)")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_BOOT_VIDEO_LOG ON "Print boot framebuffer geometry logs")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_PMM_STATS_LOG ON "Print PMM stats logs")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_HEAP_STATS_LOG ON "Print heap stats logs")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_FS_ROOT_LOG ON "Print root children FS log")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_SYSTEM_DIR_CHECK ON "Require /system directory sanity check")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_ELFRUNNER_INIT ON "Initialize ELFRUNNER framework")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_SYSCALL_TICK_QUERY ON "Query syscall timer ticks at boot")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_TTY_READY_LOG ON "Print TTY count/active/cursor ready logs")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_IDLE_DEBUG_LOG ON "Print idle loop debug log")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_PROCFS ON "Enable virtual /proc procfs syscall layer")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_EXEC_SERIAL_LOG ON "Enable EXEC info logs on serial output")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_SYSCALL_SERIAL_LOG ON "Enable syscall CALL/RET serial logs")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_SYSCALL_USERID_SERIAL_LOG ON "Enable syscall USER_TRACE/PID/USER_ID serial logs")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_LOG_LEVEL_DEBUG ON "Allow debug log level")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_LOG_LEVEL_INFO ON "Allow info log level")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_LOG_LEVEL_WARN ON "Allow warn log level")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_LOG_LEVEL_ERROR ON "Allow error log level")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_LOG_OUTPUT_SERIAL ON "Send kernel logs to serial")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_LOG_OUTPUT_TTY ON "Send kernel logs to tty")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_LOG_OUTPUT_JOURNAL ON "Store kernel logs in journal")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_USC ON "Enable dangerous syscall confirmations")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_KBD_TTY_SWITCH_HOTKEY ON "Enable ALT+F1..F4 TTY switch hotkey")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_KBD_CTRL_SHORTCUTS ON "Enable Ctrl+A/C/V keyboard shortcuts")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_KBD_FORCE_STOP_HOTKEY ON "Enable Ctrl+Alt+C force-stop hotkey")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_USER_INIT_SCRIPT_PROBE ON "Probe /shell/init.cmd during userland init")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_USER_SYSTEM_APP_PROBE ON "Probe /system apps during userland init")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_SCHED_TASK_COUNT_LOG ON "Print scheduler task count during boot")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_INTERRUPT_READY_LOG ON "Print interrupt ready log after IDT/PIC init")
cl_set_bool_cache(XIAOBAIOS_CLKS_ENABLE_SHELL_MODE_LOG ON "Print default shell mode log during boot")

cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_AUDIO CLKS_CFG_AUDIO_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_MOUSE CLKS_CFG_MOUSE_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_DESKTOP CLKS_CFG_DESKTOP_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_NET CLKS_CFG_NET_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_DRIVER_MANAGER CLKS_CFG_DRIVER_MANAGER_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_KELF CLKS_CFG_KELF_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_USERLAND_AUTO_EXEC CLKS_CFG_USERLAND_AUTO_EXEC_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_HEAP_SELFTEST CLKS_CFG_HEAP_SELFTEST_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_EXTERNAL_PSF CLKS_CFG_EXTERNAL_PSF_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_KEYBOARD CLKS_CFG_KEYBOARD_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_ELFRUNNER_PROBE CLKS_CFG_ELFRUNNER_PROBE_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_KLOGD_TASK CLKS_CFG_KLOGD_TASK_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_KWORKER_TASK CLKS_CFG_KWORKER_TASK_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_USRD_TASK CLKS_CFG_USRD_TASK_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_BOOT_VIDEO_LOG CLKS_CFG_BOOT_VIDEO_LOG_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_PMM_STATS_LOG CLKS_CFG_PMM_STATS_LOG_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_HEAP_STATS_LOG CLKS_CFG_HEAP_STATS_LOG_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_FS_ROOT_LOG CLKS_CFG_FS_ROOT_LOG_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_SYSTEM_DIR_CHECK CLKS_CFG_SYSTEM_DIR_CHECK_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_ELFRUNNER_INIT CLKS_CFG_ELFRUNNER_INIT_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_SYSCALL_TICK_QUERY CLKS_CFG_SYSCALL_TICK_QUERY_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_TTY_READY_LOG CLKS_CFG_TTY_READY_LOG_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_IDLE_DEBUG_LOG CLKS_CFG_IDLE_DEBUG_LOG_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_PROCFS CLKS_CFG_PROCFS_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_EXEC_SERIAL_LOG CLKS_CFG_EXEC_SERIAL_LOG_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_SYSCALL_SERIAL_LOG CLKS_CFG_SYSCALL_SERIAL_LOG_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_SYSCALL_USERID_SERIAL_LOG CLKS_CFG_SYSCALL_USERID_SERIAL_LOG_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_LOG_LEVEL_DEBUG CLKS_CFG_LOG_LEVEL_DEBUG_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_LOG_LEVEL_INFO CLKS_CFG_LOG_LEVEL_INFO_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_LOG_LEVEL_WARN CLKS_CFG_LOG_LEVEL_WARN_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_LOG_LEVEL_ERROR CLKS_CFG_LOG_LEVEL_ERROR_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_LOG_OUTPUT_SERIAL CLKS_CFG_LOG_OUTPUT_SERIAL_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_LOG_OUTPUT_TTY CLKS_CFG_LOG_OUTPUT_TTY_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_LOG_OUTPUT_JOURNAL CLKS_CFG_LOG_OUTPUT_JOURNAL_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_USC CLKS_CFG_USC_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_KBD_TTY_SWITCH_HOTKEY CLKS_CFG_KBD_TTY_SWITCH_HOTKEY_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_KBD_CTRL_SHORTCUTS CLKS_CFG_KBD_CTRL_SHORTCUTS_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_KBD_FORCE_STOP_HOTKEY CLKS_CFG_KBD_FORCE_STOP_HOTKEY_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_USER_INIT_SCRIPT_PROBE CLKS_CFG_USER_INIT_SCRIPT_PROBE_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_USER_SYSTEM_APP_PROBE CLKS_CFG_USER_SYSTEM_APP_PROBE_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_SCHED_TASK_COUNT_LOG CLKS_CFG_SCHED_TASK_COUNT_LOG_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_INTERRUPT_READY_LOG CLKS_CFG_INTERRUPT_READY_LOG_INT)
cl_bool_to_int(XIAOBAIOS_CLKS_ENABLE_SHELL_MODE_LOG CLKS_CFG_SHELL_MODE_LOG_INT)

set(CFLAGS_COMMON
    -std=c11
    -ffreestanding
    -fno-stack-protector
    -fno-builtin
    -mgeneral-regs-only
    -O2
    -ffunction-sections
    -fdata-sections
    -fno-unwind-tables
    -fno-asynchronous-unwind-tables
    -fno-omit-frame-pointer
    -g
    -Wall
    -Wextra
    -Werror
    "-I${CMAKE_SOURCE_DIR}/clks/include"
)

set(CXXFLAGS_COMMON
    -std=c++17
    -ffreestanding
    -fno-stack-protector
    -fno-builtin
    -fno-exceptions
    -fno-rtti
    -fno-threadsafe-statics
    -fno-use-cxa-atexit
    -O2
    -ffunction-sections
    -fdata-sections
    -fno-unwind-tables
    -fno-asynchronous-unwind-tables
    -fno-omit-frame-pointer
    -g
    -Wall
    -Wextra
    -Werror
    "-I${CMAKE_SOURCE_DIR}/clks/include"
)

set(ARCH_CFLAGS
    -DCLKS_ARCH_X86_64=1
    -m64
    -mno-red-zone
    -mcmodel=kernel
    -fno-pic
    -fno-pie
)

macro(cl_append_bool_define DEF_NAME VAR_NAME)
    if(${VAR_NAME})
        list(APPEND ARCH_CFLAGS "-D${DEF_NAME}=1")
    else()
        list(APPEND ARCH_CFLAGS "-D${DEF_NAME}=0")
    endif()
endmacro()

cl_append_bool_define(CLKS_CFG_AUDIO XIAOBAIOS_CLKS_ENABLE_AUDIO)
cl_append_bool_define(CLKS_CFG_MOUSE XIAOBAIOS_CLKS_ENABLE_MOUSE)
cl_append_bool_define(CLKS_CFG_DESKTOP XIAOBAIOS_CLKS_ENABLE_DESKTOP)
cl_append_bool_define(CLKS_CFG_NET XIAOBAIOS_CLKS_ENABLE_NET)
cl_append_bool_define(CLKS_CFG_DRIVER_MANAGER XIAOBAIOS_CLKS_ENABLE_DRIVER_MANAGER)
cl_append_bool_define(CLKS_CFG_KELF XIAOBAIOS_CLKS_ENABLE_KELF)
cl_append_bool_define(CLKS_CFG_USERLAND_AUTO_EXEC XIAOBAIOS_CLKS_ENABLE_USERLAND_AUTO_EXEC)
cl_append_bool_define(CLKS_CFG_HEAP_SELFTEST XIAOBAIOS_CLKS_ENABLE_HEAP_SELFTEST)
cl_append_bool_define(CLKS_CFG_EXTERNAL_PSF XIAOBAIOS_CLKS_ENABLE_EXTERNAL_PSF)
cl_append_bool_define(CLKS_CFG_KEYBOARD XIAOBAIOS_CLKS_ENABLE_KEYBOARD)
cl_append_bool_define(CLKS_CFG_ELFRUNNER_PROBE XIAOBAIOS_CLKS_ENABLE_ELFRUNNER_PROBE)
cl_append_bool_define(CLKS_CFG_KLOGD_TASK XIAOBAIOS_CLKS_ENABLE_KLOGD_TASK)
cl_append_bool_define(CLKS_CFG_KWORKER_TASK XIAOBAIOS_CLKS_ENABLE_KWORKER_TASK)
cl_append_bool_define(CLKS_CFG_USRD_TASK XIAOBAIOS_CLKS_ENABLE_USRD_TASK)
cl_append_bool_define(CLKS_CFG_BOOT_VIDEO_LOG XIAOBAIOS_CLKS_ENABLE_BOOT_VIDEO_LOG)
cl_append_bool_define(CLKS_CFG_PMM_STATS_LOG XIAOBAIOS_CLKS_ENABLE_PMM_STATS_LOG)
cl_append_bool_define(CLKS_CFG_HEAP_STATS_LOG XIAOBAIOS_CLKS_ENABLE_HEAP_STATS_LOG)
cl_append_bool_define(CLKS_CFG_FS_ROOT_LOG XIAOBAIOS_CLKS_ENABLE_FS_ROOT_LOG)
cl_append_bool_define(CLKS_CFG_SYSTEM_DIR_CHECK XIAOBAIOS_CLKS_ENABLE_SYSTEM_DIR_CHECK)
cl_append_bool_define(CLKS_CFG_ELFRUNNER_INIT XIAOBAIOS_CLKS_ENABLE_ELFRUNNER_INIT)
cl_append_bool_define(CLKS_CFG_SYSCALL_TICK_QUERY XIAOBAIOS_CLKS_ENABLE_SYSCALL_TICK_QUERY)
cl_append_bool_define(CLKS_CFG_TTY_READY_LOG XIAOBAIOS_CLKS_ENABLE_TTY_READY_LOG)
cl_append_bool_define(CLKS_CFG_IDLE_DEBUG_LOG XIAOBAIOS_CLKS_ENABLE_IDLE_DEBUG_LOG)
cl_append_bool_define(CLKS_CFG_PROCFS XIAOBAIOS_CLKS_ENABLE_PROCFS)
cl_append_bool_define(CLKS_CFG_EXEC_SERIAL_LOG XIAOBAIOS_CLKS_ENABLE_EXEC_SERIAL_LOG)
cl_append_bool_define(CLKS_CFG_SYSCALL_SERIAL_LOG XIAOBAIOS_CLKS_ENABLE_SYSCALL_SERIAL_LOG)
cl_append_bool_define(CLKS_CFG_SYSCALL_USERID_SERIAL_LOG XIAOBAIOS_CLKS_ENABLE_SYSCALL_USERID_SERIAL_LOG)
cl_append_bool_define(CLKS_CFG_LOG_LEVEL_DEBUG XIAOBAIOS_CLKS_ENABLE_LOG_LEVEL_DEBUG)
cl_append_bool_define(CLKS_CFG_LOG_LEVEL_INFO XIAOBAIOS_CLKS_ENABLE_LOG_LEVEL_INFO)
cl_append_bool_define(CLKS_CFG_LOG_LEVEL_WARN XIAOBAIOS_CLKS_ENABLE_LOG_LEVEL_WARN)
cl_append_bool_define(CLKS_CFG_LOG_LEVEL_ERROR XIAOBAIOS_CLKS_ENABLE_LOG_LEVEL_ERROR)
cl_append_bool_define(CLKS_CFG_LOG_OUTPUT_SERIAL XIAOBAIOS_CLKS_ENABLE_LOG_OUTPUT_SERIAL)
cl_append_bool_define(CLKS_CFG_LOG_OUTPUT_TTY XIAOBAIOS_CLKS_ENABLE_LOG_OUTPUT_TTY)
cl_append_bool_define(CLKS_CFG_LOG_OUTPUT_JOURNAL XIAOBAIOS_CLKS_ENABLE_LOG_OUTPUT_JOURNAL)
cl_append_bool_define(CLKS_CFG_USC XIAOBAIOS_CLKS_ENABLE_USC)
cl_append_bool_define(CLKS_CFG_KBD_TTY_SWITCH_HOTKEY XIAOBAIOS_CLKS_ENABLE_KBD_TTY_SWITCH_HOTKEY)
cl_append_bool_define(CLKS_CFG_KBD_CTRL_SHORTCUTS XIAOBAIOS_CLKS_ENABLE_KBD_CTRL_SHORTCUTS)
cl_append_bool_define(CLKS_CFG_KBD_FORCE_STOP_HOTKEY XIAOBAIOS_CLKS_ENABLE_KBD_FORCE_STOP_HOTKEY)
cl_append_bool_define(CLKS_CFG_USER_INIT_SCRIPT_PROBE XIAOBAIOS_CLKS_ENABLE_USER_INIT_SCRIPT_PROBE)
cl_append_bool_define(CLKS_CFG_USER_SYSTEM_APP_PROBE XIAOBAIOS_CLKS_ENABLE_USER_SYSTEM_APP_PROBE)
cl_append_bool_define(CLKS_CFG_SCHED_TASK_COUNT_LOG XIAOBAIOS_CLKS_ENABLE_SCHED_TASK_COUNT_LOG)
cl_append_bool_define(CLKS_CFG_INTERRUPT_READY_LOG XIAOBAIOS_CLKS_ENABLE_INTERRUPT_READY_LOG)
cl_append_bool_define(CLKS_CFG_SHELL_MODE_LOG XIAOBAIOS_CLKS_ENABLE_SHELL_MODE_LOG)

set(ASFLAGS_COMMON
    -ffreestanding
    "-I${CMAKE_SOURCE_DIR}/clks/include"
)

set(LDFLAGS_COMMON
    -nostdlib
    --gc-sections
    -z
    max-page-size=0x1000
)

if(NOT CLKS_ARCH STREQUAL "x86_64")
    cl_log_error("CLKS_ARCH=${CLKS_ARCH} is not supported by this XiaoBaiOS build yet; use x86_64")
endif()

set(KERNEL_SOURCE_DIRS
    "${CMAKE_SOURCE_DIR}/clks/kernel"
    "${CMAKE_SOURCE_DIR}/clks/arch/${CLKS_ARCH}"
    "${CMAKE_SOURCE_DIR}/clks/third_party"
)

set(C_SOURCES)
set(CPP_SOURCES)
set(ASM_SOURCES)
foreach(_src_dir IN LISTS KERNEL_SOURCE_DIRS)
    if(EXISTS "${_src_dir}")
        file(GLOB_RECURSE _c_abs CONFIGURE_DEPENDS "${_src_dir}/*.c")
        file(GLOB_RECURSE _cpp_abs CONFIGURE_DEPENDS "${_src_dir}/*.cpp")
        file(GLOB_RECURSE _s_abs CONFIGURE_DEPENDS "${_src_dir}/*.S")
        foreach(_abs IN LISTS _c_abs)
            file(RELATIVE_PATH _rel "${CMAKE_SOURCE_DIR}" "${_abs}")
            list(APPEND C_SOURCES "${_rel}")
        endforeach()
        foreach(_abs IN LISTS _cpp_abs)
            file(RELATIVE_PATH _rel "${CMAKE_SOURCE_DIR}" "${_abs}")
            list(APPEND CPP_SOURCES "${_rel}")
        endforeach()
        foreach(_abs IN LISTS _s_abs)
            file(RELATIVE_PATH _rel "${CMAKE_SOURCE_DIR}" "${_abs}")
            list(APPEND ASM_SOURCES "${_rel}")
        endforeach()
    endif()
endforeach()
list(REMOVE_DUPLICATES C_SOURCES)
list(REMOVE_DUPLICATES CPP_SOURCES)
list(REMOVE_DUPLICATES ASM_SOURCES)
list(SORT C_SOURCES)
list(SORT CPP_SOURCES)
list(SORT ASM_SOURCES)

set(CLKS_BOOT_LIMINE_SOURCE "clks/kernel/boot/limine/limine_requests.c")
if(EXISTS "${CMAKE_SOURCE_DIR}/${CLKS_BOOT_LIMINE_SOURCE}")
    list(APPEND C_SOURCES "${CLKS_BOOT_LIMINE_SOURCE}")
    list(REMOVE_DUPLICATES C_SOURCES)
    list(SORT C_SOURCES)
else()
    cl_log_error("missing required boot source: ${CLKS_BOOT_LIMINE_SOURCE}")
endif()

file(GLOB_RECURSE KERNEL_INC_SOURCES_ABS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/clks/**/*.inc"
)
list(SORT KERNEL_INC_SOURCES_ABS)

if(NOT C_SOURCES)
    cl_log_error("no kernel C sources found in clks/")
endif()
if(NOT EXISTS "${LINKER_SCRIPT}")
    cl_log_error("missing linker script: ${LINKER_SCRIPT}")
endif()

if(XIAOBAIOS_CLKS_ENABLE_EXTERNAL_PSF AND NOT EXISTS "${XIAOBAIOS_TTY_FONT_SOURCE}")
    cl_log_error("missing TTF font source: ${XIAOBAIOS_TTY_FONT_SOURCE}")
endif()

function(add_kernel_c_object SRC OUTPUT_LIST_VAR)
    string(REGEX REPLACE "\\.c$" ".o" OBJ_REL "${SRC}")
    set(OBJ_PATH "${OBJ_ROOT}/${OBJ_REL}")
    get_filename_component(OBJ_DIR "${OBJ_PATH}" DIRECTORY)
    set(_deps "${CMAKE_SOURCE_DIR}/${SRC}" ${KERNEL_INC_SOURCES_ABS})
    set(_cflags ${CFLAGS_COMMON})

    if("${SRC}" MATCHES "^clks/third_party/")
        list(APPEND _cflags
            -Wno-error
            -Wno-unused-function
            -Wno-unused-parameter
            -Wno-sign-compare
            -Wno-type-limits
            -Wno-missing-field-initializers
        )
    endif()

    add_custom_command(
        OUTPUT "${OBJ_PATH}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${OBJ_DIR}"
        COMMAND ${CC} ${_cflags} ${ARCH_CFLAGS} -c "${CMAKE_SOURCE_DIR}/${SRC}" -o "${OBJ_PATH}"
        DEPENDS ${_deps}
        VERBATIM
    )

    set(${OUTPUT_LIST_VAR} ${${OUTPUT_LIST_VAR}} "${OBJ_PATH}" PARENT_SCOPE)
endfunction()

function(add_kernel_cpp_object SRC OUTPUT_LIST_VAR)
    string(REGEX REPLACE "\\.cpp$" ".o" OBJ_REL "${SRC}")
    set(OBJ_PATH "${OBJ_ROOT}/${OBJ_REL}")
    get_filename_component(OBJ_DIR "${OBJ_PATH}" DIRECTORY)
    set(_deps "${CMAKE_SOURCE_DIR}/${SRC}" ${KERNEL_INC_SOURCES_ABS})

    add_custom_command(
        OUTPUT "${OBJ_PATH}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${OBJ_DIR}"
        COMMAND ${KERNEL_CXX} ${CXXFLAGS_COMMON} ${ARCH_CFLAGS} -c "${CMAKE_SOURCE_DIR}/${SRC}" -o "${OBJ_PATH}"
        DEPENDS ${_deps}
        VERBATIM
    )

    set(${OUTPUT_LIST_VAR} ${${OUTPUT_LIST_VAR}} "${OBJ_PATH}" PARENT_SCOPE)
endfunction()

function(add_kernel_asm_object SRC OUTPUT_LIST_VAR)
    string(REGEX REPLACE "\\.S$" ".o" OBJ_REL "${SRC}")
    set(OBJ_PATH "${OBJ_ROOT}/${OBJ_REL}")
    get_filename_component(OBJ_DIR "${OBJ_PATH}" DIRECTORY)
    set(_deps "${CMAKE_SOURCE_DIR}/${SRC}" ${KERNEL_INC_SOURCES_ABS})

    add_custom_command(
        OUTPUT "${OBJ_PATH}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${OBJ_DIR}"
        COMMAND ${CC} ${ASFLAGS_COMMON} ${ARCH_CFLAGS} -c "${CMAKE_SOURCE_DIR}/${SRC}" -o "${OBJ_PATH}"
        DEPENDS ${_deps}
        VERBATIM
    )

    set(${OUTPUT_LIST_VAR} ${${OUTPUT_LIST_VAR}} "${OBJ_PATH}" PARENT_SCOPE)
endfunction()

set(KERNEL_OBJECTS)
foreach(SRC IN LISTS C_SOURCES)
    add_kernel_c_object("${SRC}" KERNEL_OBJECTS)
endforeach()
foreach(SRC IN LISTS CPP_SOURCES)
    add_kernel_cpp_object("${SRC}" KERNEL_OBJECTS)
endforeach()
foreach(SRC IN LISTS ASM_SOURCES)
    add_kernel_asm_object("${SRC}" KERNEL_OBJECTS)
endforeach()

add_custom_command(
    OUTPUT "${KERNEL_RUST_LIB}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_ROOT}"
    COMMAND ${RUSTC} --crate-type staticlib -C panic=abort -O "${CMAKE_SOURCE_DIR}/clks/rust/src/lib.rs" -o "${KERNEL_RUST_LIB}"
    DEPENDS "${CMAKE_SOURCE_DIR}/clks/rust/src/lib.rs"
    VERBATIM
)

add_custom_command(
    OUTPUT "${KERNEL_ELF}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_ROOT}"
    COMMAND ${LD} ${LDFLAGS_COMMON} -T "${LINKER_SCRIPT}" -o "${KERNEL_ELF}" ${KERNEL_OBJECTS} "${KERNEL_RUST_LIB}"
    DEPENDS ${KERNEL_OBJECTS} "${LINKER_SCRIPT}" "${KERNEL_RUST_LIB}" "${CMAKE_SOURCE_DIR}/clks/rust/src/lib.rs"
    VERBATIM
)

add_custom_target(kernel DEPENDS "${KERNEL_ELF}")

add_custom_command(
    OUTPUT "${KERNEL_SYMBOLS_FILE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_ROOT}"
    COMMAND ${CMAKE_COMMAND}
        "-DNM_TOOL=${NM}"
        "-DADDR2LINE_TOOL=${ADDR2LINE}"
        "-DKERNEL_ELF=${KERNEL_ELF}"
        "-DOUT_SYMBOL_FILE=${KERNEL_SYMBOLS_FILE}"
        -P "${CMAKE_SOURCE_DIR}/cmake/gen_kernel_symbols.cmake"
    DEPENDS "${KERNEL_ELF}" "${CMAKE_SOURCE_DIR}/cmake/gen_kernel_symbols.cmake"
    VERBATIM
)

add_custom_target(kernel-symbols DEPENDS "${KERNEL_SYMBOLS_FILE}")
add_dependencies(kernel-symbols kernel)

set(XIAOBAIOS_USER_APP_SOURCE_DIR "${CMAKE_SOURCE_DIR}/xiaobaios/apps")
set(XIAOBAIOS_USER_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/xiaobaios/include")
set(XIAOBAIOS_USER_COMMON_SOURCE_DIR "${CMAKE_SOURCE_DIR}/xiaobaios/src")
set(XIAOBAIOS_USER_LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/xiaobaios/user.ld")
set(XIAOBAIOS_USER_KELF_LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/xiaobaios/kelf.ld")
set(XIAOBAIOS_USER_BUILD_ROOT "${BUILD_ROOT}/xiaobaios_user")
set(XIAOBAIOS_USER_OBJ_ROOT "${XIAOBAIOS_USER_BUILD_ROOT}/obj")
set(XIAOBAIOS_USER_APP_DIR "${XIAOBAIOS_USER_BUILD_ROOT}/apps")

file(GLOB_RECURSE XIAOBAIOS_USER_INC_SOURCES_ABS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/xiaobaios/**/*.h"
    "${CMAKE_SOURCE_DIR}/xiaobaios/**/*.inc"
)
list(SORT XIAOBAIOS_USER_INC_SOURCES_ABS)

file(GLOB_RECURSE XIAOBAIOS_USER_COMMON_SOURCES_ABS CONFIGURE_DEPENDS "${XIAOBAIOS_USER_COMMON_SOURCE_DIR}/*.c")
list(SORT XIAOBAIOS_USER_COMMON_SOURCES_ABS)

file(GLOB_RECURSE XIAOBAIOS_USER_APP_SOURCES_ABS CONFIGURE_DEPENDS "${XIAOBAIOS_USER_APP_SOURCE_DIR}/*.c")
file(GLOB_RECURSE XIAOBAIOS_USER_PREBUILT_ELF_ABS CONFIGURE_DEPENDS "${XIAOBAIOS_USER_APP_SOURCE_DIR}/*.elf")

set(XIAOBAIOS_USER_COMMON_SOURCES ${XIAOBAIOS_USER_COMMON_SOURCES_ABS})
list(SORT XIAOBAIOS_USER_COMMON_SOURCES)

set(XIAOBAIOS_USER_APP_SOURCES ${XIAOBAIOS_USER_APP_SOURCES_ABS})
list(SORT XIAOBAIOS_USER_APP_SOURCES)

set(XIAOBAIOS_USER_APP_MAIN_SOURCES)
set(XIAOBAIOS_USER_APP_KMAIN_SOURCES)
set(XIAOBAIOS_USER_APP_SUPPORT_SOURCES)
foreach(SRC IN LISTS XIAOBAIOS_USER_APP_SOURCES)
    get_filename_component(_stem "${SRC}" NAME_WE)

    if(_stem MATCHES ".*_main$")
        list(APPEND XIAOBAIOS_USER_APP_MAIN_SOURCES "${SRC}")
    elseif(_stem MATCHES ".*_kmain$")
        list(APPEND XIAOBAIOS_USER_APP_KMAIN_SOURCES "${SRC}")
    else()
        list(APPEND XIAOBAIOS_USER_APP_SUPPORT_SOURCES "${SRC}")
    endif()
endforeach()

set(XIAOBAIOS_USER_CFLAGS
    -std=c11
    -ffreestanding
    -fno-stack-protector
    -fno-builtin
    -mgeneral-regs-only
    -O2
    -ffunction-sections
    -fdata-sections
    -fno-unwind-tables
    -fno-asynchronous-unwind-tables
    -fno-omit-frame-pointer
    -Wall
    -Wextra
    -Werror
    "-I${XIAOBAIOS_USER_INCLUDE_DIR}"
)

set(XIAOBAIOS_USER_KMAIN_CFLAGS
    -std=c11
    -ffreestanding
    -fno-stack-protector
    -fno-builtin
    -mgeneral-regs-only
    -O2
    -ffunction-sections
    -fdata-sections
    -fno-unwind-tables
    -fno-asynchronous-unwind-tables
    -fno-omit-frame-pointer
    -Wall
    -Wextra
    -Werror
    "-I${XIAOBAIOS_USER_INCLUDE_DIR}"
)

set(XIAOBAIOS_USER_LDFLAGS
    -nostdlib
    --gc-sections
    -z
    max-page-size=0x1000
    -T
    "${XIAOBAIOS_USER_LINKER_SCRIPT}"
)

set(XIAOBAIOS_USER_KELF_LDFLAGS
    -nostdlib
    --gc-sections
    -z
    max-page-size=0x1000
    -T
    "${XIAOBAIOS_USER_KELF_LINKER_SCRIPT}"
)

function(add_xiaobaios_user_c_object SRC OUTPUT_VAR)
    string(REGEX REPLACE "\\.c$" ".o" OBJ_REL "${SRC}")
    set(OBJ_PATH "${XIAOBAIOS_USER_OBJ_ROOT}/${OBJ_REL}")
    get_filename_component(OBJ_DIR "${OBJ_PATH}" DIRECTORY)
    if(IS_ABSOLUTE "${SRC}")
        set(_src_path "${SRC}")
    else()
        set(_src_path "${CMAKE_SOURCE_DIR}/${SRC}")
    endif()
    set(_deps "${_src_path}" ${XIAOBAIOS_USER_INC_SOURCES_ABS})

    add_custom_command(
        OUTPUT "${OBJ_PATH}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${OBJ_DIR}"
        COMMAND ${USER_CC} ${XIAOBAIOS_USER_CFLAGS} -c "${_src_path}" -o "${OBJ_PATH}"
        DEPENDS ${_deps}
        VERBATIM
    )

    set(${OUTPUT_VAR} "${OBJ_PATH}" PARENT_SCOPE)
endfunction()

function(add_xiaobaios_user_kelf_object SRC OUTPUT_VAR)
    string(REGEX REPLACE "\\.c$" ".o" OBJ_REL "${SRC}")
    set(OBJ_PATH "${XIAOBAIOS_USER_OBJ_ROOT}/${OBJ_REL}")
    get_filename_component(OBJ_DIR "${OBJ_PATH}" DIRECTORY)
    if(IS_ABSOLUTE "${SRC}")
        set(_src_path "${SRC}")
    else()
        set(_src_path "${CMAKE_SOURCE_DIR}/${SRC}")
    endif()
    set(_deps "${_src_path}" ${XIAOBAIOS_USER_INC_SOURCES_ABS})

    add_custom_command(
        OUTPUT "${OBJ_PATH}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${OBJ_DIR}"
        COMMAND ${USER_CC} ${XIAOBAIOS_USER_KMAIN_CFLAGS} -c "${_src_path}" -o "${OBJ_PATH}"
        DEPENDS ${_deps}
        VERBATIM
    )

    set(${OUTPUT_VAR} "${OBJ_PATH}" PARENT_SCOPE)
endfunction()

set(XIAOBAIOS_USER_COMMON_OBJECTS)
foreach(SRC IN LISTS XIAOBAIOS_USER_COMMON_SOURCES)
    add_xiaobaios_user_c_object("${SRC}" OBJ_OUT)
    list(APPEND XIAOBAIOS_USER_COMMON_OBJECTS "${OBJ_OUT}")
endforeach()

set(XIAOBAIOS_USER_SUPPORT_OBJECTS)
foreach(SRC IN LISTS XIAOBAIOS_USER_APP_SUPPORT_SOURCES)
    add_xiaobaios_user_c_object("${SRC}" OBJ_OUT)
    list(APPEND XIAOBAIOS_USER_SUPPORT_OBJECTS "${OBJ_OUT}")
endforeach()

set(XIAOBAIOS_USER_APP_NAMES)
set(XIAOBAIOS_USER_APP_OUTPUTS)
set(XIAOBAIOS_USER_RAMDISK_COPY_COMMANDS)

foreach(SRC IN LISTS XIAOBAIOS_USER_APP_MAIN_SOURCES)
    get_filename_component(_stem "${SRC}" NAME_WE)
    string(REGEX REPLACE "_main$" "" _app_name "${_stem}")
    string(TOUPPER "${_app_name}" _app_name_upper)
    string(REGEX REPLACE "[^A-Z0-9]" "_" _app_name_key "${_app_name_upper}")
    set(_app_enabled_var "XIAOBAIOS_USER_APP_${_app_name_key}")

    if(NOT DEFINED ${_app_enabled_var})
        set(${_app_enabled_var} ON CACHE BOOL "Build xiaobaios user app ${_app_name}")
    else()
        set(${_app_enabled_var} ${${_app_enabled_var}} CACHE BOOL "Build xiaobaios user app ${_app_name}" FORCE)
    endif()

    if(NOT ${_app_enabled_var})
        cl_log_info("menuconfig: skip disabled xiaobaios user app ${_app_name}")
        continue()
    endif()

    list(FIND XIAOBAIOS_USER_APP_NAMES "${_app_name}" _dup_name_idx)
    if(NOT _dup_name_idx EQUAL -1)
        cl_log_error("duplicate xiaobaios user app name: ${_app_name}")
    endif()
    list(APPEND XIAOBAIOS_USER_APP_NAMES "${_app_name}")

    add_xiaobaios_user_c_object("${SRC}" _user_obj)

    set(_app_specific_objs ${XIAOBAIOS_USER_SUPPORT_OBJECTS})
    list(APPEND _app_specific_objs "${_user_obj}")

    if(EXISTS "${XIAOBAIOS_USER_APP_SOURCE_DIR}/${_app_name}_cmd.c")
        add_xiaobaios_user_c_object("${XIAOBAIOS_USER_APP_SOURCE_DIR}/${_app_name}_cmd.c" _cmd_obj)
        list(APPEND _app_specific_objs "${_cmd_obj}")
    endif()

    set(_app_out "${XIAOBAIOS_USER_APP_DIR}/${_app_name}.elf")
    add_custom_command(
        OUTPUT "${_app_out}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${XIAOBAIOS_USER_APP_DIR}"
        COMMAND ${USER_LD} ${XIAOBAIOS_USER_LDFLAGS} -o "${_app_out}" ${XIAOBAIOS_USER_COMMON_OBJECTS} ${_app_specific_objs}
        DEPENDS ${XIAOBAIOS_USER_COMMON_OBJECTS} ${_app_specific_objs} "${XIAOBAIOS_USER_LINKER_SCRIPT}"
        VERBATIM
    )

    list(APPEND XIAOBAIOS_USER_APP_OUTPUTS "${_app_out}")

    if(_app_name MATCHES ".*drv$")
        list(APPEND XIAOBAIOS_USER_RAMDISK_COPY_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy "${_app_out}" "${BUILD_ROOT}/ramdisk_root/driver/${_app_name}.elf")
    elseif(_app_name STREQUAL "hello")
        list(APPEND XIAOBAIOS_USER_RAMDISK_COPY_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy "${_app_out}" "${BUILD_ROOT}/ramdisk_root/${_app_name}.elf")
    else()
        list(APPEND XIAOBAIOS_USER_RAMDISK_COPY_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy "${_app_out}" "${BUILD_ROOT}/ramdisk_root/shell/${_app_name}.elf")
    endif()
endforeach()

foreach(SRC IN LISTS XIAOBAIOS_USER_APP_KMAIN_SOURCES)
    get_filename_component(_stem "${SRC}" NAME_WE)
    string(REGEX REPLACE "_kmain$" "" _app_name "${_stem}")
    string(TOUPPER "${_app_name}" _app_name_upper)
    string(REGEX REPLACE "[^A-Z0-9]" "_" _app_name_key "${_app_name_upper}")
    set(_app_enabled_var "XIAOBAIOS_USER_APP_${_app_name_key}")

    if(NOT DEFINED ${_app_enabled_var})
        set(${_app_enabled_var} ON CACHE BOOL "Build xiaobaios user app ${_app_name}")
    else()
        set(${_app_enabled_var} ${${_app_enabled_var}} CACHE BOOL "Build xiaobaios user app ${_app_name}" FORCE)
    endif()

    if(NOT ${_app_enabled_var})
        cl_log_info("menuconfig: skip disabled xiaobaios user app ${_app_name}")
        continue()
    endif()

    list(FIND XIAOBAIOS_USER_APP_NAMES "${_app_name}" _dup_name_idx)
    if(NOT _dup_name_idx EQUAL -1)
        cl_log_error("duplicate xiaobaios user app name: ${_app_name}")
    endif()
    list(APPEND XIAOBAIOS_USER_APP_NAMES "${_app_name}")

    add_xiaobaios_user_kelf_object("${SRC}" _kelf_obj)
    set(_app_out "${XIAOBAIOS_USER_APP_DIR}/${_app_name}.elf")
    add_custom_command(
        OUTPUT "${_app_out}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${XIAOBAIOS_USER_APP_DIR}"
        COMMAND ${USER_LD} ${XIAOBAIOS_USER_KELF_LDFLAGS} -o "${_app_out}" "${_kelf_obj}"
        DEPENDS "${_kelf_obj}" "${XIAOBAIOS_USER_KELF_LINKER_SCRIPT}"
        VERBATIM
    )

    list(APPEND XIAOBAIOS_USER_APP_OUTPUTS "${_app_out}")
    list(APPEND XIAOBAIOS_USER_RAMDISK_COPY_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy "${_app_out}" "${BUILD_ROOT}/ramdisk_root/system/${_app_name}.elf")
endforeach()

if(XIAOBAIOS_USER_PREBUILT_ELF_ABS)
    foreach(_abs IN LISTS XIAOBAIOS_USER_PREBUILT_ELF_ABS)
        file(RELATIVE_PATH _rel "${CMAKE_SOURCE_DIR}" "${_abs}")
        get_filename_component(_name "${_abs}" NAME_WE)

        list(FIND XIAOBAIOS_USER_APP_NAMES "${_name}" _dup_name_idx)
        if(NOT _dup_name_idx EQUAL -1)
            cl_log_error("duplicate xiaobaios user app name from prebuilt ELF: ${_name}")
        endif()
        list(APPEND XIAOBAIOS_USER_APP_NAMES "${_name}")

        if(_name MATCHES ".*drv$")
            list(APPEND XIAOBAIOS_USER_RAMDISK_COPY_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy "${_abs}" "${BUILD_ROOT}/ramdisk_root/driver/${_name}.elf")
        elseif(_name STREQUAL "hello")
            list(APPEND XIAOBAIOS_USER_RAMDISK_COPY_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy "${_abs}" "${BUILD_ROOT}/ramdisk_root/${_name}.elf")
        else()
            list(APPEND XIAOBAIOS_USER_RAMDISK_COPY_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy "${_abs}" "${BUILD_ROOT}/ramdisk_root/shell/${_name}.elf")
        endif()
    endforeach()
endif()

if(NOT XIAOBAIOS_USER_APP_OUTPUTS AND NOT XIAOBAIOS_USER_PREBUILT_ELF_ABS)
    cl_log_warn("no xiaobaios user ELFs found under ${XIAOBAIOS_USER_APP_SOURCE_DIR}")
endif()

add_custom_target(xiaobaios-userapps DEPENDS ${XIAOBAIOS_USER_APP_OUTPUTS})

add_custom_command(
    TARGET xiaobaios-userapps
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        "-DNO_COLOR=${NO_COLOR}"
        "-DLOG_LEVEL=INFO"
        "-DLOG_TEXT=xiaobaios user elfs ready"
        -P "${CL_LOG_EMIT_SCRIPT}"
)

file(GLOB_RECURSE RAMDISK_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/ramdisk/*")
set(RAMDISK_ROOT_STAMP "${BUILD_ROOT}/.ramdisk_root.stamp")
set(XIAOBAIOS_RAMDISK_ELF_COMMANDS ${XIAOBAIOS_USER_RAMDISK_COPY_COMMANDS})

if(XIAOBAIOS_CLKS_ENABLE_EXTERNAL_PSF)
    add_custom_command(
        OUTPUT "${XIAOBAIOS_TTY_PSF_IMAGE}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_ROOT}/generated"
        COMMAND ${PYTHON} "${CMAKE_SOURCE_DIR}/scripts/make_psf_font.py"
            --font "${XIAOBAIOS_TTY_FONT_SOURCE}"
            --output "${XIAOBAIOS_TTY_PSF_IMAGE}"
            --width 10
            --height 20
            --font-size 16
            --threshold 172
        DEPENDS "${CMAKE_SOURCE_DIR}/scripts/make_psf_font.py" "${XIAOBAIOS_TTY_FONT_SOURCE}"
        VERBATIM
    )

    add_custom_target(tty-font DEPENDS "${XIAOBAIOS_TTY_PSF_IMAGE}")

    set(XIAOBAIOS_RAMDISK_FONT_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_ROOT}/ramdisk_root/system"
        COMMAND ${CMAKE_COMMAND} -E copy "${XIAOBAIOS_TTY_PSF_IMAGE}" "${BUILD_ROOT}/ramdisk_root/system/tty.psf"
    )
    set(XIAOBAIOS_RAMDISK_ROOT_DEPS ${RAMDISK_FILES} "${XIAOBAIOS_TTY_PSF_IMAGE}")
else()
    set(XIAOBAIOS_RAMDISK_FONT_COMMANDS)
    set(XIAOBAIOS_RAMDISK_ROOT_DEPS ${RAMDISK_FILES})
endif()

add_custom_command(
    OUTPUT "${RAMDISK_ROOT_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${BUILD_ROOT}/ramdisk_root"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_ROOT}/ramdisk_root"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/ramdisk" "${BUILD_ROOT}/ramdisk_root"
    ${XIAOBAIOS_RAMDISK_FONT_COMMANDS}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_ROOT}/ramdisk_root/system" "${BUILD_ROOT}/ramdisk_root/system/etc" "${BUILD_ROOT}/ramdisk_root/shell" "${BUILD_ROOT}/ramdisk_root/driver" "${BUILD_ROOT}/ramdisk_root/dev" "${BUILD_ROOT}/ramdisk_root/home" "${BUILD_ROOT}/ramdisk_root/temp"
    ${XIAOBAIOS_RAMDISK_ELF_COMMANDS}
    COMMAND ${CMAKE_COMMAND} -E touch "${RAMDISK_ROOT_STAMP}"
    DEPENDS ${XIAOBAIOS_RAMDISK_ROOT_DEPS} ${XIAOBAIOS_USER_APP_OUTPUTS} ${XIAOBAIOS_USER_PREBUILT_ELF_ABS}
    VERBATIM
)

add_custom_target(ramdisk-root DEPENDS "${RAMDISK_ROOT_STAMP}")
add_dependencies(ramdisk-root xiaobaios-userapps)

add_custom_command(
    OUTPUT "${RAMDISK_IMAGE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_ROOT}"
    COMMAND ${TAR} -C "${BUILD_ROOT}/ramdisk_root" -cf "${RAMDISK_IMAGE}" .
    DEPENDS "${RAMDISK_ROOT_STAMP}"
    VERBATIM
)

add_custom_target(ramdisk DEPENDS "${RAMDISK_IMAGE}")

add_custom_target(disk-image
    COMMAND ${CMAKE_COMMAND}
        "-DDISK_IMAGE=${DISK_IMAGE}"
        "-DDISK_MB=${XIAOBAIOS_DISK_IMAGE_MB}"
        -P "${CMAKE_SOURCE_DIR}/cmake/ensure_disk_image.cmake"
    BYPRODUCTS "${DISK_IMAGE}"
    VERBATIM
)

add_custom_target(setup-tools
    COMMAND ${CMAKE_COMMAND}
        "-DGIT_TOOL=${GIT_TOOL}"
        "-DTAR_TOOL=${TAR}"
        "-DXORRISO_TOOL=${XORRISO}"
        "-DNO_COLOR=${NO_COLOR}"
        "-DCC_TOOL=${CC}"
        "-DKERNEL_CXX_TOOL=${KERNEL_CXX}"
        "-DLD_TOOL=${LD}"
        "-DUSER_CC_TOOL=${USER_CC}"
        "-DUSER_LD_TOOL=${USER_LD}"
        "-DOBJCOPY_TOOL=${OBJCOPY_FOR_TARGET}"
        "-DOBJDUMP_TOOL=${OBJDUMP_FOR_TARGET}"
        "-DREADELF_TOOL=${READELF_FOR_TARGET}"
        "-DNM_TOOL=${NM}"
        "-DADDR2LINE_TOOL=${ADDR2LINE}"
        "-DRUSTC_TOOL=${RUSTC}"
        "-DMAKE_TOOL=${MAKE_TOOL}"
        "-DSH_TOOL=${SH_TOOL}"
        -P "${CMAKE_SOURCE_DIR}/cmake/check_tools.cmake"
)

add_custom_target(setup DEPENDS setup-tools)
add_custom_command(
    TARGET setup
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        "-DNO_COLOR=${NO_COLOR}"
        "-DLOG_LEVEL=INFO"
        "-DLOG_TEXT=environment ready"
        -P "${CL_LOG_EMIT_SCRIPT}"
)

add_custom_command(
    OUTPUT "${LIMINE_READY_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_ROOT}"
    COMMAND ${SH_TOOL} -c "if [ -f \"${LIMINE_DIR_ABS}/limine-bios.sys\" ] && [ -f \"${LIMINE_DIR_ABS}/limine\" ] && [ -f \"${LIMINE_DIR_ABS}/limine-bios-cd.bin\" ] && [ -f \"${LIMINE_DIR_ABS}/limine-uefi-cd.bin\" ] && [ -f \"${LIMINE_DIR_ABS}/BOOTX64.EFI\" ]; then exit 0; fi; if [ -f \"${LIMINE_BIN_DIR_ABS}/limine-bios.sys\" ] && [ -f \"${LIMINE_BIN_DIR_ABS}/limine\" ] && [ -f \"${LIMINE_BIN_DIR_ABS}/limine-bios-cd.bin\" ] && [ -f \"${LIMINE_BIN_DIR_ABS}/limine-uefi-cd.bin\" ] && [ -f \"${LIMINE_BIN_DIR_ABS}/BOOTX64.EFI\" ]; then exit 0; fi; if [ ! -f \"${LIMINE_DIR_ABS}/Makefile\" ]; then \"${SH_TOOL}\" \"${LIMINE_DIR_ABS}/bootstrap\" && \"${SH_TOOL}\" -c 'cd \"${LIMINE_DIR_ABS}\" && ./configure --enable-bios --enable-bios-cd --enable-uefi-x86-64 --enable-uefi-cd'; fi; \"${MAKE_TOOL}\" -C \"${LIMINE_DIR_ABS}\" -j4"
    COMMAND ${SH_TOOL} -c "if [ -f \"${LIMINE_DIR_ABS}/limine-bios.sys\" ] && [ -f \"${LIMINE_DIR_ABS}/limine\" ] && [ -f \"${LIMINE_DIR_ABS}/limine-bios-cd.bin\" ] && [ -f \"${LIMINE_DIR_ABS}/limine-uefi-cd.bin\" ] && [ -f \"${LIMINE_DIR_ABS}/BOOTX64.EFI\" ]; then exit 0; fi; if [ -f \"${LIMINE_BIN_DIR_ABS}/limine-bios.sys\" ] && [ -f \"${LIMINE_BIN_DIR_ABS}/limine\" ] && [ -f \"${LIMINE_BIN_DIR_ABS}/limine-bios-cd.bin\" ] && [ -f \"${LIMINE_BIN_DIR_ABS}/limine-uefi-cd.bin\" ] && [ -f \"${LIMINE_BIN_DIR_ABS}/BOOTX64.EFI\" ]; then exit 0; fi; echo 'limine build missing expected artifacts' >&2; exit 1"
    COMMAND ${CMAKE_COMMAND} -E touch "${LIMINE_READY_STAMP}"
    DEPENDS "${LIMINE_DIR_ABS}/.git"
    VERBATIM
)

add_custom_target(limine-ready DEPENDS "${LIMINE_READY_STAMP}")

add_custom_target(menuconfig
    COMMAND ${PYTHON} "${CMAKE_SOURCE_DIR}/scripts/menuconfig.py" --non-interactive
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    USES_TERMINAL
)

add_custom_command(
    OUTPUT "${ISO_IMAGE}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${ISO_ROOT}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ISO_ROOT}/boot/limine" "${ISO_ROOT}/EFI/BOOT"
    COMMAND ${CMAKE_COMMAND} -E copy "${KERNEL_ELF}" "${ISO_ROOT}/boot/clks_kernel.elf"
    COMMAND ${CMAKE_COMMAND} -E copy "${RAMDISK_IMAGE}" "${ISO_ROOT}/boot/xiaobaios_ramdisk.tar"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/configs/limine.conf" "${ISO_ROOT}/boot/limine/limine.conf"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/configs/limine.conf" "${ISO_ROOT}/EFI/BOOT/limine.conf"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/configs/limine.conf" "${ISO_ROOT}/limine.conf"
    COMMAND ${CMAKE_COMMAND} -E copy "${LIMINE_BIN_DIR_ABS}/limine-bios.sys" "${ISO_ROOT}/boot/limine/limine-bios.sys"
    COMMAND ${CMAKE_COMMAND} -E copy "${LIMINE_BIN_DIR_ABS}/limine-bios-cd.bin" "${ISO_ROOT}/boot/limine/limine-bios-cd.bin"
    COMMAND ${CMAKE_COMMAND} -E copy "${LIMINE_BIN_DIR_ABS}/limine-uefi-cd.bin" "${ISO_ROOT}/boot/limine/limine-uefi-cd.bin"
    COMMAND ${CMAKE_COMMAND} -E copy "${LIMINE_BIN_DIR_ABS}/BOOTX64.EFI" "${ISO_ROOT}/EFI/BOOT/BOOTX64.EFI"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/build"
    COMMAND ${CMAKE_COMMAND}
        "-DNO_COLOR=${NO_COLOR}"
        "-DLOG_LEVEL=STEP"
        "-DLOG_TEXT=building iso -> ${ISO_IMAGE}"
        -P "${CL_LOG_EMIT_SCRIPT}"
    COMMAND ${XORRISO} -as mkisofs
        -b boot/limine/limine-bios-cd.bin
        -no-emul-boot
        -boot-load-size 4
        -boot-info-table
        --efi-boot boot/limine/limine-uefi-cd.bin
        -efi-boot-part
        --efi-boot-image
        --protective-msdos-label
        "${ISO_ROOT}"
        -o "${ISO_IMAGE}"
    COMMAND ${CMAKE_COMMAND}
        "-DNO_COLOR=${NO_COLOR}"
        "-DLOG_LEVEL=STEP"
        "-DLOG_TEXT=installing limine boot sectors"
        -P "${CL_LOG_EMIT_SCRIPT}"
    COMMAND "${LIMINE_BIN_DIR_ABS}/limine" bios-install "${ISO_IMAGE}"
    COMMAND ${CMAKE_COMMAND}
        "-DNO_COLOR=${NO_COLOR}"
        "-DLOG_LEVEL=INFO"
        "-DLOG_TEXT=iso ready: ${ISO_IMAGE}"
        -P "${CL_LOG_EMIT_SCRIPT}"
    DEPENDS "${KERNEL_ELF}" "${RAMDISK_IMAGE}" "${CMAKE_SOURCE_DIR}/configs/limine.conf"
    VERBATIM
)

add_custom_target(iso ALL DEPENDS "${ISO_IMAGE}")
add_dependencies(iso setup-tools kernel ramdisk limine-ready)

add_custom_target(run
    COMMAND ${CMAKE_COMMAND}
        "-DNO_COLOR=${NO_COLOR}"
        "-DLOG_LEVEL=STEP"
        "-DLOG_TEXT=launching qemu run"
        -P "${CL_LOG_EMIT_SCRIPT}"
    COMMAND ${QEMU_X86_64} -M pc -m 1024M -cdrom "${ISO_IMAGE}" -drive "file=${DISK_IMAGE},format=raw,if=none,id=xiaobaidisk,media=disk" -device "ide-hd,drive=xiaobaidisk,bus=ide.0" -serial stdio
    DEPENDS iso disk-image
    USES_TERMINAL
)

add_custom_target(debug
    COMMAND ${CMAKE_COMMAND}
        "-DNO_COLOR=${NO_COLOR}"
        "-DLOG_LEVEL=STEP"
        "-DLOG_TEXT=launching qemu debug (-s -S)"
        -P "${CL_LOG_EMIT_SCRIPT}"
    COMMAND ${QEMU_X86_64} -M pc -m 1024M -cdrom "${ISO_IMAGE}" -drive "file=${DISK_IMAGE},format=raw,if=none,id=xiaobaidisk,media=disk" -device "ide-hd,drive=xiaobaidisk,bus=ide.0" -serial stdio -s -S
    DEPENDS iso disk-image
    USES_TERMINAL
)

add_custom_target(clean-x86
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${BUILD_ROOT}"
)

add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_SOURCE_DIR}/build"
)
