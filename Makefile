# DMCP - Makefile for Development
# Run `make help` to see all available targets
#
# Quick start:
#   make check    # Build + test
#   make run      # Build + run server

# ==============================================================================
# Configuration
# ==============================================================================

BUILD_DIR ?= build
BUILD_TYPE ?= Release
PREFIX ?= /usr/local
JOBS ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# CMake arguments
CMAKE_ARGS = \
	-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
	-DDMCP_BUILD_EXAMPLES=ON

# Sanitizers for debug builds
ifeq ($(BUILD_TYPE),Debug)
	CMAKE_ARGS += -DDMCP_ENABLE_SANITIZERS=ON
endif

# ==============================================================================
# Help
# ==============================================================================

.PHONY: help
help:
	@echo "DMCP SDK - Development Makefile"
	@echo ""
	@echo "Build targets:"
	@echo "  make              - Build (release)"
	@echo "  make debug        - Build with sanitizers"
	@echo "  make release      - Build optimized"
	@echo "  make clean        - Remove build directory"
	@echo "  make distclean    - Remove build + cache"
	@echo ""
	@echo "Test targets:"
	@echo "  make test         - Run unit tests"
	@echo "  make test-verbose - Run tests with details"
	@echo "  make download-wad - Download DOOM shareware"
	@echo "  make headless     - Run e2e headless tests"
	@echo ""
	@echo "Run targets:"
	@echo "  make run          - Run dummy server"
	@echo "  make run-bg       - Run server in background"
	@echo ""
	@echo "Quality targets:"
	@echo "  make check        - Build + test (full verification)"
	@echo "  make format       - Format source code"
	@echo "  make lint         - Run clang-tidy"
	@echo ""
	@echo "Other:"
	@echo "  make install      - Install to $(PREFIX)"
	@echo "  make info         - Show configuration"
	@echo "  make compdb       - Generate compile_commands.json"

# ==============================================================================
# Build
# ==============================================================================

.PHONY: all build configure
all: build

configure:
	@mkdir -p $(BUILD_DIR)
	cmake -B $(BUILD_DIR) $(CMAKE_ARGS)

build: configure
	cmake --build $(BUILD_DIR) -j$(JOBS)

debug:
	@$(MAKE) BUILD_TYPE=Debug

release:
	@$(MAKE) BUILD_TYPE=Release

# ==============================================================================
# Clean
# ==============================================================================

.PHONY: clean distclean
clean:
	rm -rf $(BUILD_DIR)

distclean: clean
	rm -rf .cache
	rm -f compile_commands.json

# ==============================================================================
# Testing
# ==============================================================================

.PHONY: configure-tests
configure-tests:
	cmake -B $(BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DDMCP_BUILD_TESTS=ON \
		-DDMCP_BUILD_EXAMPLES=ON

.PHONY: test test-verbose test-valgrind
test: configure-tests
	cmake --build $(BUILD_DIR) -j$(JOBS)
	ctest --test-dir $(BUILD_DIR) -j$(JOBS) --output-on-failure

test-verbose: configure-tests
	cmake --build $(BUILD_DIR) -j$(JOBS)
	ctest --test-dir $(BUILD_DIR) --verbose

test-valgrind: configure-tests
	cmake --build $(BUILD_DIR) -j$(JOBS)
	valgrind --leak-check=full --error-exitcode=1 \
		$(BUILD_DIR)/tests/dmcp_tests

# ==============================================================================
# Integration Tests
# ==============================================================================

.PHONY: configure-integration download-wad headless
configure-integration:
	cmake -B $(BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DDMCP_BUILD_TESTS=ON \
		-DDMCP_BUILD_INTEGRATION_TESTS=ON \
		-DDMCP_BUILD_EXAMPLES=ON

download-wad:
	@./tests/integration/download_wad.sh

headless: configure-integration download-wad
	cmake --build $(BUILD_DIR) -j$(JOBS)
	@./tests/integration/run_headless.sh

# ==============================================================================
# Run Server
# ==============================================================================

.PHONY: run run-bg
run: build
	./$(BUILD_DIR)/dummy_server

run-bg: build
	@./$(BUILD_DIR)/dummy_server &
	@echo "Server running on http://localhost:6060 (PID: $$!)"

# ==============================================================================
# Code Quality
# ==============================================================================

.PHONY: format lint check
format:
	@if command -v clang-format >/dev/null 2>&1; then \
		find src include adapters -type f \( -name "*.cpp" -o -name "*.c" -o -name "*.h" -o -name "*.hpp" \) \
			-exec clang-format -i {} \; ; \
		echo "Code formatted"; \
	else \
		echo "clang-format not found"; \
	fi

lint: configure
	@if command -v clang-tidy >/dev/null 2>&1; then \
		clang-tidy -p $(BUILD_DIR) \
			$$(find src include -name "*.cpp" -o -name "*.hpp") 2>/dev/null; \
	else \
		echo "clang-tidy not found"; \
	fi

check: test
	@echo ""
	@echo "✓ Build: OK"
	@echo "✓ Tests: OK"

# ==============================================================================
# Installation
# ==============================================================================

.PHONY: install uninstall
install: build
	cmake --install $(BUILD_DIR) --prefix $(PREFIX)

uninstall:
	@xargs rm -vf < $(BUILD_DIR)/install_manifest.txt 2>/dev/null || \
		echo "No install manifest found"

# ==============================================================================
# Info
# ==============================================================================

.PHONY: info size
info:
	@echo "DMCP SDK v0.6.0"
	@echo ""
	@echo "Configuration:"
	@echo "  Build type:    $(BUILD_TYPE)"
	@echo "  Build dir:     $(BUILD_DIR)"
	@echo "  Install prefix: $(PREFIX)"
	@echo "  Parallel jobs: $(JOBS)"
	@echo ""
	@echo "CMake: $(shell cmake --version | head -1)"
	@echo "CXX:   $(shell c++ --version 2>&1 | head -1)"

size:
	@echo "Library sizes:"
	@ls -lh $(BUILD_DIR)/*.a 2>/dev/null || echo "  No static libraries"
	@echo ""
	@echo "Binary sizes:"
	@ls -lh $(BUILD_DIR)/dummy_server 2>/dev/null || true
	@ls -lh $(BUILD_DIR)/tests/dmcp_tests 2>/dev/null || true

# ==============================================================================
# IDE Support
# ==============================================================================

.PHONY: compdb
compdb: configure
	@ln -sf $(BUILD_DIR)/compile_commands.json compile_commands.json
	@echo "compile_commands.json linked"

# ==============================================================================
# Shortcuts
# ==============================================================================

.PHONY: b c r d t
b: build
c: clean
r: run
d: debug
t: test
