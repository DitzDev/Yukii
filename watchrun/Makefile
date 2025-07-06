CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200809L
LDFLAGS = 

ifeq ($(OS),Windows_NT)
    CFLAGS += -D_WIN32_WINNT=0x0600
    LDFLAGS += -lkernel32
    TARGET = watchrun.exe
else
    TARGET = watchrun
endif

SOURCES = src/main.c src/args.c src/watcher.c src/colors.c src/utils.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = watchrun.h

# Deteksi environment Termux
TERMUX_DETECTED := $(shell if [ -n "$$TERMUX_VERSION" ] || [ -d "/data/data/com.termux" ]; then echo "yes"; else echo "no"; fi)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build completed: $(TARGET)"

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Cleaned build files"

install: $(TARGET)
ifeq ($(OS),Windows_NT)
	@echo "Install target not supported on Windows"
	@echo "   Copy $(TARGET) to a directory in your PATH manually"
else ifeq ($(TERMUX_DETECTED),yes)
	@echo "Installing for Termux/Android..."
	install -m 755 $(TARGET) $(PREFIX)/bin/
	@echo "Installed $(TARGET) to $(PREFIX)/bin/"
	@echo "You can now run 'watchrun' from anywhere in Termux"
else
	install -m 755 $(TARGET) /usr/local/bin/
	@echo "Installed $(TARGET) to /usr/local/bin/"
endif

uninstall:
ifeq ($(OS),Windows_NT)
	@echo "Uninstall target not supported on Windows"
else ifeq ($(TERMUX_DETECTED),yes)
	@echo "Uninstalling from Termux/Android..."
	rm -f $(PREFIX)/bin/$(TARGET)
	@echo "Uninstalled $(TARGET) from $(PREFIX)/bin/"
else
	rm -f /usr/local/bin/$(TARGET)
	@echo "Uninstalled $(TARGET) from /usr/local/bin/"
endif

# Target khusus untuk Termux
install-termux: $(TARGET)
	@echo "Installing for Termux/Android (forced)..."
	@if [ -z "$$PREFIX" ]; then \
		echo "PREFIX not set. Make sure you're running this in Termux"; \
		exit 1; \
	fi
	install -m 755 $(TARGET) $(PREFIX)/bin/
	@echo "Installed $(TARGET) to $(PREFIX)/bin/"
	@echo "You can now run 'watchrun' from anywhere in Termux"

uninstall-termux:
	@echo "📱 Uninstalling from Termux/Android (forced)..."
	@if [ -z "$$PREFIX" ]; then \
		echo "PREFIX not set. Make sure you're running this in Termux"; \
		exit 1; \
	fi
	rm -f $(PREFIX)/bin/$(TARGET)
	@echo "Uninstalled $(TARGET) from $(PREFIX)/bin/"

debug: CFLAGS += -g -DDEBUG -O0
debug: clean $(TARGET)
	@echo "Debug build completed"

release: CFLAGS += -DNDEBUG -s
release: clean $(TARGET)
	@echo "Release build completed"

test: $(TARGET)
	@echo "Running basic tests..."
	./$(TARGET) --help > /dev/null
	./$(TARGET) --version > /dev/null
	@echo "Basic tests passed"

format:
	@if command -v clang-format >/dev/null 2>&1; then \
		clang-format -i $(SOURCES) $(HEADERS); \
		echo "Code formatted"; \
	else \
		echo "clang-format not found, skipping formatting"; \
	fi

analyze:
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --std=c99 $(SOURCES); \
		echo "Static analysis completed"; \
	else \
		echo "cppcheck not found, skipping analysis"; \
	fi

package: release
	@mkdir -p dist
	@tar -czf dist/watchrun-$(shell date +%Y%m%d).tar.gz $(TARGET) README.md Makefile $(SOURCES) $(HEADERS)
	@echo "Package created in dist/"

dev: CFLAGS += -g -DDEBUG -O0 -fsanitize=address -fsanitize=undefined
dev: LDFLAGS += -fsanitize=address -fsanitize=undefined
dev: clean $(TARGET)
	@echo "Development build with sanitizers completed"

windows:
	@if command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then \
		$(MAKE) CC=x86_64-w64-mingw32-gcc TARGET=watchrun.exe OS=Windows_NT; \
		echo "Windows build completed"; \
	else \
		echo "MinGW not found, cannot cross-compile for Windows"; \
	fi

# Target untuk mengecek environment
check-env:
	@echo "Environment Information:"
	@echo "   OS: $(shell uname -s)"
	@echo "   Architecture: $(shell uname -m)"
	@echo "   Termux detected: $(TERMUX_DETECTED)"
	@if [ "$(TERMUX_DETECTED)" = "yes" ]; then \
		echo "   Termux version: $${TERMUX_VERSION:-unknown}"; \
		echo "   PREFIX: $${PREFIX:-not set}"; \
		echo "   Install path: $${PREFIX}/bin/"; \
	else \
		echo "   Install path: /usr/local/bin/"; \
	fi

help:
	@echo "Available targets:"
	@echo "  all          - Build the program (default)"
	@echo "  clean        - Remove build files"
	@echo "  install      - Install to system (auto-detects Termux)"
	@echo "  uninstall    - Remove from system (auto-detects Termux)"
	@echo "  install-termux   - Force install for Termux/Android"
	@echo "  uninstall-termux - Force uninstall from Termux/Android"
	@echo "  debug        - Build with debug symbols"
	@echo "  release      - Build optimized release version"
	@echo "  dev          - Build with sanitizers for development"
	@echo "  test         - Run basic functionality tests"
	@echo "  format       - Format code with clang-format"
	@echo "  analyze      - Run static analysis with cppcheck"
	@echo "  package      - Create distribution package"
	@echo "  windows      - Cross-compile for Windows"
	@echo "  check-env    - Show environment information"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "  Termux/Android specific:"
	@echo "  The install/uninstall targets will automatically detect Termux"
	@echo "  and use the appropriate paths ($$PREFIX/bin/)"

.PHONY: all clean install uninstall install-termux uninstall-termux debug release dev test format analyze package windows check-env help

main.o: main.c watchrun.h
args.o: args.c watchrun.h
watcher.o: watcher.c watchrun.h
colors.o: colors.c watchrun.h
utils.o: utils.c watchrun.h