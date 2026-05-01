.RECIPEPREFIX := >
MAKEFLAGS += --no-print-directory

BDT_BUILD_DIR ?= $(CURDIR)/build/bdt
BDT ?= $(BDT_BUILD_DIR)/bdt
BDT_PROJECT ?= project.bdt
BDT_CFLAGS ?= -std=c11 -O2 -Wall -Wextra -D_XOPEN_SOURCE=700
JOBS ?= 4
PYTHON ?= python3

.PHONY: all bdt setup setup-tools kernel kernel-symbols userapps ramdisk-root ramdisk disk-image iso run debug menuconfig clean clean-all clean-drive-image list scan graph doctor help

all: iso

bdt:
> @$(MAKE) -C bdt BUILD_DIR="$(BDT_BUILD_DIR)" CFLAGS="$(BDT_CFLAGS)"

setup: setup-tools

setup-tools kernel kernel-symbols userapps ramdisk-root ramdisk disk-image iso run debug clean clean-all clean-drive-image list scan graph doctor: bdt
> @case "$@" in \
>   list) "$(BDT)" --project "$(BDT_PROJECT)" --list ;; \
>   scan) "$(BDT)" --project "$(BDT_PROJECT)" --scan ;; \
>   graph) "$(BDT)" --project "$(BDT_PROJECT)" --graph ;; \
>   doctor) "$(BDT)" --project "$(BDT_PROJECT)" doctor ;; \
>   *) "$(BDT)" --project "$(BDT_PROJECT)" "$@" -j "$(JOBS)" ;; \
> esac

menuconfig: bdt
> @if [ -t 0 ]; then \
>     $(PYTHON) scripts/menuconfig.py --tui; \
> else \
>     $(PYTHON) scripts/menuconfig.py --non-interactive; \
> fi

help:
> @echo "XiaoBaiOS (bdt-backed wrapper)"
> @echo "  make bdt"
> @echo "  make menuconfig"
> @echo "  make setup-tools"
> @echo "  make kernel"
> @echo "  make userapps"
> @echo "  make ramdisk"
> @echo "  make disk-image"
> @echo "  make iso"
> @echo "  make run"
> @echo "  make debug"
> @echo "  make clean"
> @echo "  make clean-all"
