.RECIPEPREFIX := >
MAKEFLAGS += --no-print-directory

CMAKE ?= cmake
CMAKE_BUILD_DIR ?= build-cmake
CMAKE_BUILD_TYPE ?= Release
CMAKE_GENERATOR ?=
CMAKE_EXTRA_ARGS ?=
NO_COLOR ?= 0
PYTHON ?= python3

ifeq ($(strip $(CMAKE_GENERATOR)),)
GEN_ARG :=
else
GEN_ARG := -G "$(CMAKE_GENERATOR)"
endif

CMAKE_PASSTHROUGH_ARGS :=


.PHONY: all configure reconfigure setup setup-tools kernel ramdisk-root ramdisk iso run debug menuconfig clean clean-all help

all: iso

configure:
> @$(CMAKE) -S . -B $(CMAKE_BUILD_DIR) $(GEN_ARG) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -DNO_COLOR=$(NO_COLOR) $(CMAKE_EXTRA_ARGS) $(CMAKE_PASSTHROUGH_ARGS)

setup: configure
> @$(CMAKE) --build $(CMAKE_BUILD_DIR) --target setup

setup-tools: configure
> @$(CMAKE) --build $(CMAKE_BUILD_DIR) --target setup-tools

kernel: configure
> @$(CMAKE) --build $(CMAKE_BUILD_DIR) --target kernel

ramdisk-root: configure
> @$(CMAKE) --build $(CMAKE_BUILD_DIR) --target ramdisk-root

ramdisk: configure
> @$(CMAKE) --build $(CMAKE_BUILD_DIR) --target ramdisk

iso: configure
> @$(CMAKE) --build $(CMAKE_BUILD_DIR) --target iso

run: configure
> @$(CMAKE) --build $(CMAKE_BUILD_DIR) --target run

debug: configure
> @$(CMAKE) --build $(CMAKE_BUILD_DIR) --target debug

menuconfig:
> @if [ -t 0 ]; then \
>     $(PYTHON) scripts/menuconfig.py --tui; \
> else \
>     $(PYTHON) scripts/menuconfig.py --non-interactive; \
> fi

clean:
> @if [ -d "$(CMAKE_BUILD_DIR)" ]; then \
>     $(CMAKE) --build $(CMAKE_BUILD_DIR) --target clean-x86; \
> else \
>     rm -rf build/x86_64; \
> fi

clean-all:
> @if [ -d "$(CMAKE_BUILD_DIR)" ]; then \
>     $(CMAKE) --build $(CMAKE_BUILD_DIR) --target clean-all; \
> else \
>     rm -rf build build-cmake; \
> fi

help:
> @echo "XiaoBaiOS (CMake-backed wrapper)"
> @echo "  make configure"
> @echo "  make setup"
> @echo "  make kernel"
> @echo "  make iso"
> @echo "  make run"
> @echo "  make debug"
> @echo "  make menuconfig"
> @echo "  make clean"
> @echo "  make clean-all"
> @echo ""
> @echo "Default toolchain: clang + ld.lld + LLVM binutils"
