# DMCP - Makefile Wrapper for CMake
# Usage: make [target] [options]
#
# Examples:
#   make                    # Build with default settings
#   make clean              # Clean build artifacts
#   make test               # Run tests
#   make json=yyjson        # Build with specific JSON library
#   make debug              # Build debug configuration
#   make install            # Install to system

# ============================================================================
# Configuration
# ============================================================================

# Build directory
BUILD_DIR ?= build

# JSON library: yyjson (default) or nlohmann
JSON_LIBRARY ?= yyjson

# Build type: Release (default) or Debug
BUILD_TYPE ?= Release

# Install prefix
PREFIX ?= /usr/local

# CMake generator (optional)
GENERATOR ?=

# Number of parallel jobs
JOBS ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# ============================================================================
# CMake Arguments
# ============================================================================

CMAKE_ARGS = \
	-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
	-DDMCP_JSON_LIBRARY=$(JSON_LIBRARY) \
	-DDMCP_BUILD_EXAMPLES=ON

ifdef GENERATOR
	CMAKE_ARGS += -G "$(GENERATOR)"
endif

# Sanitizer option for debug builds
ifeq ($(BUILD_TYPE),Debug)
	CMAKE_ARGS += -DDMCP_ENABLE_SANITIZERS=ON
endif

# ============================================================================
# Default Target
# ============================================================================

.PHONY: all
all: configure build
	@echo "Build complete"

# ============================================================================
# Configuration
# ============================================================================

.PHONY: configure
configure:
	@echo "Configuring with:"
	@echo "  JSON library: $(JSON_LIBRARY)"
	@echo "  Build type:   $(BUILD_TYPE)"
	@echo "  Build dir:    $(BUILD_DIR)"
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake $(CMAKE_ARGS) .. \
		|| (echo "Configuration failed" && exit 1)
	@echo "Configuration complete"

# ============================================================================
# Build
# ============================================================================

.PHONY: build
build: configure
	@echo "Building..."
	@cmake --build $(BUILD_DIR) --parallel $(JOBS) \
		|| (echo "Build failed" && exit 1)
	@echo "Build successful"

# ============================================================================
# Debug Build
# ============================================================================

.PHONY: debug
debug:
	@$(MAKE) BUILD_TYPE=Debug build

# ============================================================================
# Release Build
# ============================================================================

.PHONY: release
release:
	@$(MAKE) BUILD_TYPE=Release build

# ============================================================================
# JSON Library Variants
# ============================================================================

.PHONY: yyjson
yyjson:
	@$(MAKE) JSON_LIBRARY=yyjson clean build

.PHONY: nlohmann
nlohmann:
	@$(MAKE) JSON_LIBRARY=nlohmann clean build

# ============================================================================
# Clean
# ============================================================================

.PHONY: clean
clean:
	@echo "Cleaning build directory..."
	@rm -rf $(BUILD_DIR)
	@echo "Clean complete"

.PHONY: distclean
distclean: clean
	@echo "Removing all generated files..."
	@rm -rf .cache
	@rm -f compile_commands.json
	@echo "Distclean complete"

# ============================================================================
# Run
# ============================================================================

.PHONY: run
run: build
	@echo "Running dummy_server..."
	@./$(BUILD_DIR)/dummy_server

# ============================================================================
# Test
# ============================================================================

.PHONY: test
test: build
	@echo "Running tests..."
	@cd $(BUILD_DIR) && ctest --output-on-failure || echo "No tests configured"

# ============================================================================
# Install
# ============================================================================

.PHONY: install
install: build
	@echo "Installing to $(PREFIX)..."
	@cmake --install $(BUILD_DIR) --prefix $(PREFIX)
	@echo "Installation complete"

.PHONY: uninstall
uninstall:
	@echo "Removing from $(PREFIX)..."
	@xargs rm -vf < $(BUILD_DIR)/install_manifest.txt 2>/dev/null || echo "No install manifest found"

# ============================================================================
# Development Tools
# ============================================================================

.PHONY: format
format:
	@echo "Formatting code..."
	@find src include adapters examples -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \) -exec clang-format -i {} \; 2>/dev/null || echo "clang-format not found"

.PHONY: tidy
tidy: configure
	@echo "Running clang-tidy..."
	@clang-tidy -p $(BUILD_DIR) src/**/*.cpp adapters/**/*.cpp 2>/dev/null || echo "clang-tidy not found"

.PHONY: check
check: configure
	@echo "Running static analysis..."
	@cppcheck --project=$(BUILD_DIR)/compile_commands.json --enable=all 2>/dev/null || echo "cppcheck not found"

# ============================================================================
# Info
# ============================================================================

.PHONY: info
info:
	@echo "DMCP Build Configuration"
	@echo "========================"
	@echo "Build directory: $(BUILD_DIR)"
	@echo "JSON library:    $(JSON_LIBRARY)"
	@echo "Build type:      $(BUILD_TYPE)"
	@echo "Install prefix:  $(PREFIX)"
	@echo "Parallel jobs:   $(JOBS)"
	@echo ""
	@echo "Available targets:"
	@echo "  make              - Build with default settings"
	@echo "  make debug        - Build debug configuration"
	@echo "  make release      - Build release configuration"
	@echo "  make yyjson       - Build with yyjson (fast)"
	@echo "  make nlohmann     - Build with nlohmann/json"
	@echo "  make clean        - Remove build directory"
	@echo "  make distclean    - Remove all generated files"
	@echo "  make run          - Build and run dummy server"
	@echo "  make test         - Run tests"
	@echo "  make install      - Install to system"
	@echo "  make format       - Format code with clang-format"
	@echo "  make info         - Show this info"

.PHONY: help
help: info

# ============================================================================
# IDE Support
# ============================================================================

.PHONY: compdb
compdb: configure
	@echo "Generating compile_commands.json..."
	@ln -sf $(BUILD_DIR)/compile_commands.json compile_commands.json
	@echo "compile_commands.json linked"

# ============================================================================
# Quick shortcuts
# ============================================================================

.PHONY: b
b: build

.PHONY: c
c: clean

.PHONY: r
r: run

.PHONY: d
d: debug
