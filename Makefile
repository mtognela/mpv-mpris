# Include the original Makefile
PKG_CONFIG = pkg-config
INSTALL := install
MKDIR := mkdir -p
RMDIR := rmdir
LN := ln
RM := rm
CC = zig cc

# Base flags, environment CFLAGS / LDFLAGS can be appended.
BASE_CFLAGS = -std=c99 -Wall -Wextra -O2 -pedantic $(shell $(PKG_CONFIG) --cflags gio-2.0 gio-unix-2.0 glib-2.0 mpv libavformat)
BASE_LDFLAGS = $(shell $(PKG_CONFIG) --libs gio-2.0 gio-unix-2.0 glib-2.0 mpv libavformat)

# Directory structure
SRC_DIR := src
C_SRC_DIR := $(SRC_DIR)/c
INCLUDE_DIR := include
PREFIX := /usr/local
PLUGINDIR := $(PREFIX)/lib/mpv-mpris
SCRIPTS_DIR := $(HOME)/.config/mpv/scripts
SYS_SCRIPTS_DIR := /etc/mpv/scripts

# Target and source files
TARGET := build/c-out/lib/libmpris.so

# Source files (C files only)
SRCS := $(wildcard $(C_SRC_DIR)/*.c)

# Header files (for dependency tracking)
HEADERS := $(wildcard $(INCLUDE_DIR)/*.h)

# Include directory flag
INCLUDE_FLAGS := -I$(INCLUDE_DIR)

# User ID for install detection
UID ?= $(shell id -u)

# Zig-specific variables
ZIG := zig
ZIG_TARGET := build/zig-out/lib/libmpris.so
ZIG_TEST_TARGET := build/zig-out/bin/mpv-mpris-test

.PHONY: \
 install install-user install-system \
 uninstall uninstall-user uninstall-system \
 test test-zig test-c \
 clean clean-zig clean-all \
 debug debug-zig \
 build-zig build-c \
 setup help

all: build-zig

setup:
	$(MKDIR) build/zig-out/bin
	$(MKDIR) build/zig-out/lib
	$(MKDIR) build/c-out/lib


# C build target - build the shared library from .c files
build-c $(TARGET): $(SRCS) $(HEADERS)
	$(CC) $(BASE_CFLAGS) $(CFLAGS) $(INCLUDE_FLAGS) -fPIC -shared -o $(TARGET) $(SRCS) $(BASE_LDFLAGS) $(LDFLAGS)

# Zig build targets
build-zig:
	$(ZIG) build

$(ZIG_TARGET): build-zig

debug-zig:
	$(ZIG) build debug

test-zig: build-zig
	$(ZIG) build test

test-c: $(TARGET)
	$(MAKE) -C test

# Combined test target - run both C and Zig tests
test: test-c test-zig

# Install logic based on user privileges
ifneq ($(UID),0)
install: install-user
uninstall: uninstall-user
else
install: install-system
uninstall: uninstall-system
endif

# User installation - can install either regular or Zig-built version
install-user: $(TARGET)
	$(MKDIR) -p $(SCRIPTS_DIR)
	$(INSTALL) -t $(SCRIPTS_DIR) $(TARGET)

# Alternative: install Zig-built version
install-user-zig: $(ZIG_TARGET)
	$(MKDIR) -p $(SCRIPTS_DIR)
	$(INSTALL) -t $(SCRIPTS_DIR) $(ZIG_TARGET)
	cd $(SCRIPTS_DIR) && ln -sf libmpris.so mpris.so

uninstall-user:
	$(RM) -f $(SCRIPTS_DIR)/$(TARGET) $(SCRIPTS_DIR)/libmpris.so
	-$(RMDIR) -p $(SCRIPTS_DIR) 2>/dev/null || true

# System-wide installation
install-system: $(TARGET)
	$(MKDIR) -p $(DESTDIR)$(PLUGINDIR)
	$(INSTALL) -t $(DESTDIR)$(PLUGINDIR) $(TARGET)
	$(MKDIR) -p $(DESTDIR)$(SYS_SCRIPTS_DIR)
	$(LN) -sf $(PLUGINDIR)/$(TARGET) $(DESTDIR)$(SYS_SCRIPTS_DIR)

install-system-zig: $(ZIG_TARGET)
	$(MKDIR) -p $(DESTDIR)$(PLUGINDIR)
	$(INSTALL) -t $(DESTDIR)$(PLUGINDIR) $(ZIG_TARGET)
	cd $(DESTDIR)$(PLUGINDIR) && ln -sf libmpris.so mpris.so
	$(MKDIR) -p $(DESTDIR)$(SYS_SCRIPTS_DIR)
	$(LN) -sf $(PLUGINDIR)/mpris.so $(DESTDIR)$(SYS_SCRIPTS_DIR)

uninstall-system:
	$(RM) -f $(DESTDIR)$(SYS_SCRIPTS_DIR)/$(TARGET) $(DESTDIR)$(SYS_SCRIPTS_DIR)/mpris.so
	-$(RMDIR) -p $(DESTDIR)$(SYS_SCRIPTS_DIR) 2>/dev/null || true
	$(RM) -f $(DESTDIR)$(PLUGINDIR)/$(TARGET) $(DESTDIR)$(PLUGINDIR)/libmpris.so $(DESTDIR)$(PLUGINDIR)/mpris.so
	-$(RMDIR) -p $(DESTDIR)$(PLUGINDIR) 2>/dev/null || true

# Debug build with debug symbols and no optimization
debug: BASE_CFLAGS := $(BASE_CFLAGS:-O2=-O0 -g -DDEBUG)
debug: $(TARGET)

# Clean targets
clean:
	$(RM) -f $(TARGET)
	$(MAKE) -C test clean

clean-zig:
	$(ZIG) build clean
	$(RM) -rf build/zig-out .zig-cache/

clean-all: clean clean-zig

# Print variables for debugging the Makefile
print-vars:
	@echo "SRCS: $(SRCS)"
	@echo "HEADERS: $(HEADERS)"
	@echo "BASE_CFLAGS: $(BASE_CFLAGS)"
	@echo "INCLUDE_FLAGS: $(INCLUDE_FLAGS)"
	@echo "ZIG: $(ZIG)"
	@echo "ZIG_TARGET: $(ZIG_TARGET)"
	@echo "C_SRC_DIR: $(C_SRC_DIR)"

# Help target
help:
	@echo "Available targets:"
	@echo ""
	@echo "Building:"
	@echo "  all             - Build with Zig compiler (default)"
	@echo "  build-zig       - Build with Zig compiler"
	@echo "  build-c         - Build mpris.so with GCC/CC"
	@echo "  $(TARGET)          - Build mpris.so with GCC/CC (alias)"
	@echo "  debug           - Build with GCC debug symbols"
	@echo "  debug-zig       - Build with Zig debug symbols"
	@echo ""
	@echo "Testing:"
	@echo "  test            - Run both C and Zig tests"
	@echo "  test-c          - Run original C tests"
	@echo "  test-zig        - Run Zig-based tests"
	@echo ""
	@echo "Installation:"
	@echo "  install         - Install plugin (user or system based on privileges)"
	@echo "  install-user    - Install GCC-built plugin to user directory"
	@echo "  install-user-zig- Install Zig-built plugin to user directory"
	@echo "  install-system  - Install GCC-built plugin system-wide"
	@echo "  install-system-zig- Install Zig-built plugin system-wide"
	@echo "  uninstall       - Uninstall plugin"
	@echo ""
	@echo "Maintenance:"
	@echo "  clean           - Remove GCC build files"
	@echo "  clean-zig       - Remove Zig build files"
	@echo "  clean-all       - Remove all build files"
	@echo "  print-vars      - Print Makefile variables for debugging"
	@echo "  help            - Show this help message"
