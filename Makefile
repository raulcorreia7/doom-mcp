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

ENGINES := crispy

.PHONY: help
help:
	@echo "DMCP SDK - Development Makefile"
	@echo ""
	@echo "Build targets:"
	@echo "  make              - Build DMCP (default)"
	@echo "  make dmcp         - Build DMCP library + examples"
	@echo "  make all          - Build DMCP + all engines"
	@echo "  make debug        - Build with sanitizers"
	@echo "  make release      - Build optimized"
	@echo "  make clean        - Remove build directory"
	@echo "  make distclean    - Remove build + cache"
	@echo ""
	@echo "Engine targets (ENGINE=crispy):"
	@echo "  make crispy-doom  - Build Crispy Doom adapter build"
	@echo "  make engine       - Build engine with DMCP"
	@echo "  make engine-test  - Run headless tests on engine"
	@echo "  make engine-clean - Clean engine build"
	@echo "  make engine-all   - Build all engines"
	@echo ""
	@echo "  Engine: $(ENGINES)"
	@echo "  Example: make engine ENGINE=crispy"
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
	@echo "  make submodules   - Init/update git submodules"
	@echo "  make info         - Show configuration"
	@echo "  make compdb       - Generate compile_commands.json"

# ==============================================================================
# Build
# ==============================================================================

.PHONY: all dmcp build configure submodules

# Default target builds just DMCP
default: dmcp

# Build everything including all engines
all: dmcp engine-all

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
	ctest --test-dir $(BUILD_DIR) -j1 --output-on-failure

test-verbose: configure-tests
	cmake --build $(BUILD_DIR) -j$(JOBS)
	ctest --test-dir $(BUILD_DIR) -j1 --verbose

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
# Crispy Doom
# ==============================================================================

CRISPY_BUILD_DIR ?= crispy-doom/build
CRISPY_SOURCE_DIR ?= crispy-doom
CRISPY_KEEP_PATCH ?= 0
.PHONY: crispy-doom crispy-doom-clean
crispy-doom:
	@if [ ! -d "$(CRISPY_SOURCE_DIR)" ]; then \
		echo "error: Crispy Doom source dir not found: $(CRISPY_SOURCE_DIR)"; \
		echo "hint: run 'make submodules' first"; \
		exit 1; \
	fi
	@JOBS=$(JOBS) DMCP_BUILD_DIR=$$(pwd)/$(BUILD_DIR) CRISPY_BUILD_DIR=$$(pwd)/$(CRISPY_BUILD_DIR) \
		DMCP_KEEP_CRISPY_PATCH=$(CRISPY_KEEP_PATCH) ./tests/integration/build_crispy_doom.sh

crispy-doom-clean:
	rm -rf $(CRISPY_BUILD_DIR)

# ==============================================================================
# Unified Engine Interface
# ==============================================================================

ENGINE ?= crispy

.PHONY: engine engine-test engine-clean engine-all engine-update
engine:
ifndef ENGINE
	$(error ENGINE is required. Use: make engine ENGINE=crispy)
endif
	@case "$(ENGINE)" in \
		crispy) \
			$(MAKE) crispy-doom ;; \
		*) \
			echo "error: Unknown engine '$(ENGINE)'. Supported: $(ENGINES)"; \
			exit 1 ;; \
	esac

engine-test:
ifndef ENGINE
	$(error ENGINE is required. Use: make engine-test ENGINE=crispy)
endif
	@DOOM_ENGINE=$(ENGINE) $(MAKE) headless

engine-clean:
ifndef ENGINE
	$(error ENGINE is required. Use: make engine-clean ENGINE=crispy)
endif
	@case "$(ENGINE)" in \
		crispy) \
			$(MAKE) crispy-doom-clean ;; \
		*) \
			echo "error: Unknown engine '$(ENGINE)'. Supported: $(ENGINES)"; \
			exit 1 ;; \
	esac

engine-all:
	@for e in $(ENGINES); do \
		echo "==> Building $$e..."; \
		$(MAKE) engine ENGINE=$$e || exit 1; \
	done

engine-update:
ifndef ENGINE
	$(error ENGINE is required. Use: make engine-update ENGINE=crispy VERSION=<tag-or-commit>)
endif
	@echo "Updating $(ENGINE) submodule..."
	@cd $(ENGINE)-doom && git fetch --tags origin && \
		git switch --detach $${VERSION:-origin/main}
	@echo "Done. Submodule is now detached at $${VERSION:-origin/main}."

# ==============================================================================
# Shortcuts
# ==============================================================================

.PHONY: b c r d t
b: dmcp
c: clean
r: run
d: debug
t: test
