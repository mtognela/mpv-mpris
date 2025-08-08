PKG_CONFIG = pkg-config
INSTALL := install
MKDIR := mkdir
RMDIR := rmdir
LN := ln
RM := rm

# Base flags, environment CFLAGS / LDFLAGS can be appended.
BASE_CFLAGS = -std=c99 -Wall -Wextra -O2 -pedantic $(shell $(PKG_CONFIG) --cflags gio-2.0 gio-unix-2.0 glib-2.0 mpv libavformat)
BASE_LDFLAGS = $(shell $(PKG_CONFIG) --libs gio-2.0 gio-unix-2.0 glib-2.0 mpv libavformat)

# Directory structure
SRC_DIR := src
INCLUDE_DIR := include
PREFIX := /usr/local
PLUGINDIR := $(PREFIX)/lib/mpv-mpris
SCRIPTS_DIR := $(HOME)/.config/mpv/scripts
SYS_SCRIPTS_DIR := /etc/mpv/scripts

# Target and source files
TARGET := mpris.so

# Source files (C files only)
SRCS := $(wildcard $(SRC_DIR)/*.c)

# Header files (for dependency tracking)
HEADERS := $(wildcard $(INCLUDE_DIR)/*.h)

# Include directory flag
INCLUDE_FLAGS := -I$(INCLUDE_DIR)

# User ID for install detection
UID ?= $(shell id -u)

.PHONY: \
 install install-user install-system \
 uninstall uninstall-user uninstall-system \
 test \
 clean \
 debug

# Main target - build the shared library directly from .c files
$(TARGET): $(SRCS) $(HEADERS)
	$(CC) $(SRCS) -o $(TARGET) $(BASE_CFLAGS) $(CFLAGS) $(CPPFLAGS) $(INCLUDE_FLAGS) $(BASE_LDFLAGS) $(LDFLAGS) -shared -fPIC

# Install logic based on user privileges
ifneq ($(UID),0)
install: install-user
uninstall: uninstall-user
else
install: install-system
uninstall: uninstall-system
endif

# User installation
install-user: $(TARGET)
	$(MKDIR) -p $(SCRIPTS_DIR)
	$(INSTALL) -t $(SCRIPTS_DIR) $(TARGET)

uninstall-user:
	$(RM) -f $(SCRIPTS_DIR)/$(TARGET)
	-$(RMDIR) -p $(SCRIPTS_DIR) 2>/dev/null || true

# System-wide installation
install-system: $(TARGET)
	$(MKDIR) -p $(DESTDIR)$(PLUGINDIR)
	$(INSTALL) -t $(DESTDIR)$(PLUGINDIR) $(TARGET)
	$(MKDIR) -p $(DESTDIR)$(SYS_SCRIPTS_DIR)
	$(LN) -sf $(PLUGINDIR)/$(TARGET) $(DESTDIR)$(SYS_SCRIPTS_DIR)

uninstall-system:
	$(RM) -f $(DESTDIR)$(SYS_SCRIPTS_DIR)/$(TARGET)
	-$(RMDIR) -p $(DESTDIR)$(SYS_SCRIPTS_DIR) 2>/dev/null || true
	$(RM) -f $(DESTDIR)$(PLUGINDIR)/$(TARGET)
	-$(RMDIR) -p $(DESTDIR)$(PLUGINDIR) 2>/dev/null || true

# Test target
test: $(TARGET)
	$(MAKE) -C test

# Debug build with debug symbols and no optimization
debug: BASE_CFLAGS := $(BASE_CFLAGS:-O2=-O0 -g -DDEBUG)
debug: $(TARGET)

# Clean target
clean:
	$(RM) -f $(TARGET)
	$(MAKE) -C test clean

# Print variables for debugging the Makefile
print-vars:
	@echo "SRCS: $(SRCS)"
	@echo "HEADERS: $(HEADERS)"
	@echo "BASE_CFLAGS: $(BASE_CFLAGS)"
	@echo "INCLUDE_FLAGS: $(INCLUDE_FLAGS)"

# Help target
help:
	@echo "Available targets:"
	@echo "  $(TARGET)          - Build the mpris.so plugin (default)"
	@echo "  install         - Install plugin (user or system based on privileges)"
	@echo "  install-user    - Install plugin to user directory"
	@echo "  install-system  - Install plugin system-wide"
	@echo "  uninstall       - Uninstall plugin"
	@echo "  uninstall-user  - Uninstall from user directory"
	@echo "  uninstall-system- Uninstall from system"
	@echo "  test           - Run tests"
	@echo "  debug          - Build with debug symbols"
	@echo "  clean          - Remove built files"
	@echo "  print-vars     - Print Makefile variables for debugging"
	@echo "  help           - Show this help message"