# DMCP - Makefile for Development
# Run `make help` to see all available targets
#
# Quick start:
#   make check    # Build + test
#   make run      # Build + run server

# ==============================================================================
# Configuration
# ==============================================================================

BUILD_DIR             ?= build/default
INTEGRATION_BUILD_DIR ?= build/integration
BUILD_TYPE            ?= Release

# CMake arguments
CMAKE_ARGS = \
	-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
	-DDMCP_BUILD_TESTS=OFF \
	-DDMCP_BUILD_EXAMPLES=OFF \
	-DDMCP_BUILD_INTEGRATION_TESTS=OFF \
	-DDMCP_BUILD_ADAPTER_FAKE=OFF

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
	@echo "  make dmcp         - Build DMCP core library"
	@echo "  make all          - Build DMCP SDK"
	@echo "  make debug        - Build with sanitizers"
	@echo "  make release      - Build optimized"
	@echo "  make clean        - Remove build + integration build directories"
	@echo "  make distclean    - Remove all build dirs + cache"
	@echo ""
	@echo "Test targets:"
	@echo "  make test         - Run unit tests (default test target)"
	@echo "  make test-unit    - Run unit tests only"
	@echo "  make test-smoke   - Run fast fake-adapter transport smoke"
	@echo "  make test-integration - Run SDK integration suite"
	@echo "  make test-verbose - Run tests with details"
	@echo ""
	@echo "Run targets:"
	@echo "  make run          - Run dummy server"
	@echo "  make run-bg       - Run server in background"
	@echo ""
	@echo "Quality targets:"
	@echo "  make check        - Build + test (full verification)"
	@echo "  make validate     - Run validation matrix (core defaults + opt-in paths)"
	@echo "  make format       - Format source code"
	@echo "  make lint         - Run clang-tidy"
	@echo ""
	@echo "Other:"
	@echo "  make info         - Show configuration"

# ==============================================================================
# Build
# ==============================================================================

.PHONY: all dmcp build configure
.NOTPARALLEL: all

# Default target builds just DMCP
default: dmcp

all: dmcp

# Build DMCP library and examples
dmcp: configure
	cmake --build $(BUILD_DIR) --parallel

# Convenience alias
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
	rm -rf $(BUILD_DIR) $(INTEGRATION_BUILD_DIR) build/validate

distclean: clean
	rm -rf build
	rm -rf .cache
	rm -f compile_commands.json

# ==============================================================================
# Testing
# ==============================================================================

.PHONY: configure-tests
configure-tests:
	cmake -B $(BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DDMCP_BUILD_TESTS=ON \
		-DDMCP_BUILD_EXAMPLES=OFF

.PHONY: test test-unit test-smoke test-integration test-verbose test-valgrind
test: configure-tests
	cmake --build $(BUILD_DIR) --parallel
	ctest --test-dir $(BUILD_DIR) -L unit -j1 --output-on-failure

test-unit: test

test-smoke: configure-integration
	cmake --build $(INTEGRATION_BUILD_DIR) --parallel
	ctest --test-dir $(INTEGRATION_BUILD_DIR) -R dmcp_fake_transport_integration -j1 --output-on-failure

test-integration: configure-integration
	cmake --build $(INTEGRATION_BUILD_DIR) --parallel
	ctest --test-dir $(INTEGRATION_BUILD_DIR) -L integration -LE requires_game -j1 --output-on-failure

test-verbose: configure-tests
	cmake --build $(BUILD_DIR) --parallel
	ctest --test-dir $(BUILD_DIR) -j1 --verbose

test-valgrind: configure-tests
	cmake --build $(BUILD_DIR) --parallel
	valgrind --leak-check=full --error-exitcode=1 \
		$(BUILD_DIR)/tests/dmcp_tests

# ==============================================================================
# Integration Tests
# ==============================================================================

.PHONY: configure-integration
configure-integration:
	cmake -B $(INTEGRATION_BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DDMCP_BUILD_TESTS=ON \
		-DDMCP_BUILD_INTEGRATION_TESTS=ON \
		-DDMCP_BUILD_ADAPTER_FAKE=ON \
		-DDMCP_BUILD_EXAMPLES=OFF

# ==============================================================================
# Run Server
# ==============================================================================

.PHONY: configure-run run run-bg
configure-run:
	cmake -B $(BUILD_DIR) $(CMAKE_ARGS) -DDMCP_BUILD_EXAMPLES=ON
	@$(MAKE) --no-print-directory compdb-refresh

run: configure-run
	cmake --build $(BUILD_DIR) --parallel
	./$(BUILD_DIR)/dummy_server

run-bg: configure-run
	cmake --build $(BUILD_DIR) --parallel
	@./$(BUILD_DIR)/dummy_server &
	@echo "Server running on http://localhost:6060 (PID: $$!)"

# ==============================================================================
# Code Quality
# ==============================================================================

.PHONY: format lint check validate
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

validate:
	@./scripts/validate.sh

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
	@echo "  Integration dir: $(INTEGRATION_BUILD_DIR)"
	@echo "  Parallel jobs: CMake default (--parallel)"
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
# Shortcuts
# ==============================================================================

.PHONY: b c r d t
b: dmcp
c: clean
r: run
d: debug
t: test
