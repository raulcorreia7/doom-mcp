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
	@echo "  make              - Build DMCP (default)"
	@echo "  make dmcp         - Build DMCP library + examples"
	@echo "  make all          - Build DMCP + Chocolate + Crispy"
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
	@echo "  make headless-crispy - Run e2e headless tests on Crispy"
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
	@echo "  make submodules   - Init/update git submodules"
	@echo "  make chocolate-doom - Build Chocolate Doom with DMCP"
	@echo "  make crispy-doom  - Build Crispy Doom with DMCP"
	@echo "  (set CRISPY_KEEP_PATCH=1 to keep Crispy patch applied)"
	@echo "  make info         - Show configuration"
	@echo "  make compdb       - Generate compile_commands.json"

# ==============================================================================
# Build
# ==============================================================================

.PHONY: all dmcp build configure submodules

# Default target builds just DMCP (not chocolate-doom)
default: dmcp

# Build everything including Chocolate and Crispy Doom
all: dmcp chocolate-doom crispy-doom

submodules:
	@git submodule update --init --recursive

# Build DMCP library and examples
dmcp: configure
	cmake --build $(BUILD_DIR) -j$(JOBS)

# Alias for backward compatibility
build: dmcp

configure:
	@mkdir -p $(BUILD_DIR)
	cmake -B $(BUILD_DIR) $(CMAKE_ARGS)

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

.PHONY: configure-integration download-wad headless headless-crispy
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

headless-crispy: configure-integration download-wad
	cmake --build $(BUILD_DIR) -j$(JOBS)
	@DOOM_ENGINE=crispy ./tests/integration/run_headless.sh

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
# Chocolate Doom
# ==============================================================================

CHOCOLATE_BUILD_DIR ?= chocolate-doom/build
CHOCOLATE_SOURCE_DIR ?= chocolate-doom

.PHONY: chocolate-doom chocolate-doom-clean
chocolate-doom: dmcp submodules
	@if [ ! -d "$(CHOCOLATE_SOURCE_DIR)" ]; then \
		echo "error: Chocolate Doom source dir not found: $(CHOCOLATE_SOURCE_DIR)"; \
		exit 1; \
	fi
	@echo "Building Chocolate Doom with DMCP..."
	@mkdir -p $(CHOCOLATE_BUILD_DIR)
	@cmake -S $(CHOCOLATE_SOURCE_DIR) -B $(CHOCOLATE_BUILD_DIR) \
		-DDMCP_ENABLE=ON \
		-DDMCP_INCLUDE_DIR=$$(pwd)/include \
		-DDMCP_LIB_DIR=$$(pwd)/$(BUILD_DIR)
	@cmake --build $(CHOCOLATE_BUILD_DIR) -j$(JOBS)
	@echo ""
	@echo "Chocolate Doom built: $(CHOCOLATE_BUILD_DIR)/src/chocolate-doom"

chocolate-doom-clean:
	rm -rf $(CHOCOLATE_BUILD_DIR)

# ==============================================================================
# Crispy Doom
# ==============================================================================

CRISPY_BUILD_DIR ?= crispy-doom/build
CRISPY_SOURCE_DIR ?= crispy-doom
CRISPY_KEEP_PATCH ?= 0

.PHONY: crispy-doom crispy-doom-clean
crispy-doom: dmcp submodules
	@if [ ! -d "$(CRISPY_SOURCE_DIR)" ]; then \
		echo "error: Crispy Doom source dir not found: $(CRISPY_SOURCE_DIR)"; \
		exit 1; \
	fi
	@JOBS=$(JOBS) DMCP_BUILD_DIR=$$(pwd)/$(BUILD_DIR) CRISPY_BUILD_DIR=$$(pwd)/$(CRISPY_BUILD_DIR) \
		DMCP_KEEP_CRISPY_PATCH=$(CRISPY_KEEP_PATCH) ./tests/integration/build_crispy_doom.sh

crispy-doom-clean:
	rm -rf $(CRISPY_BUILD_DIR)

# ==============================================================================
# Shortcuts
# ==============================================================================

.PHONY: b c r d t
b: dmcp
c: clean
r: run
d: debug
t: test
