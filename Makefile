# =============================================================================
#  freqlib — Text Frequency Analysis Library
#  Variant 4, Lab 10 — Основы программирования
# =============================================================================

CC      := gcc
CFLAGS  := -Wall -Wextra -Wpedantic -std=c11 -Iinclude
CFLAGS_R := $(CFLAGS) -O2
CFLAGS_D := $(CFLAGS) -g -O0

SRC_LIB  := src/freqlib.c
SRC_APP  := src/main.c
SRC_TEST := tests/test_freqlib.c

BUILD    := build
LIB_SO   := $(BUILD)/libfreqlib.so
APP_BIN  := $(BUILD)/app
TEST_BIN := $(BUILD)/test_freqlib
SAN_BIN  := $(BUILD)/test_san

REPORTS  := reports

# Exported so Python tests can find the library without LD_LIBRARY_PATH
export LD_LIBRARY_PATH := $(abspath $(BUILD)):$(LD_LIBRARY_PATH)

.PHONY: all shared app run test py-test syntax analyze sanitize \
        docs-html docs-pdf clean help

# ─────────────────────────────────────────────────────────────────────────────
#  Default
# ─────────────────────────────────────────────────────────────────────────────
all: shared app test
	@echo ""
	@echo "Build complete.  Run 'make run' or 'make py-test'."

# ─────────────────────────────────────────────────────────────────────────────
#  Library
# ─────────────────────────────────────────────────────────────────────────────
shared: $(LIB_SO)

$(LIB_SO): $(SRC_LIB) include/freqlib.h | $(BUILD)
	$(CC) $(CFLAGS_R) -fPIC -shared -o $@ $(SRC_LIB)
	@echo "[shared]  $@"

# ─────────────────────────────────────────────────────────────────────────────
#  Demo application
# ─────────────────────────────────────────────────────────────────────────────
app: $(APP_BIN)

$(APP_BIN): $(SRC_APP) $(LIB_SO) | $(BUILD)
	$(CC) $(CFLAGS_R) -o $@ $(SRC_APP) \
	    -L$(BUILD) -lfreqlib -Wl,-rpath,$(abspath $(BUILD))
	@echo "[app]     $@"

run: app
	@echo ""; $(APP_BIN); echo ""

# ─────────────────────────────────────────────────────────────────────────────
#  C unit tests
# ─────────────────────────────────────────────────────────────────────────────
$(TEST_BIN): $(SRC_TEST) $(LIB_SO) | $(BUILD)
	$(CC) $(CFLAGS_D) -o $@ $(SRC_TEST) \
	    -L$(BUILD) -lfreqlib -Wl,-rpath,$(abspath $(BUILD))

test: $(TEST_BIN)
	@echo ""; $(TEST_BIN); echo ""

# ─────────────────────────────────────────────────────────────────────────────
#  Python integration tests
# ─────────────────────────────────────────────────────────────────────────────
py-test: shared
	@echo ""; python3 tests/test_freqlib.py; echo ""

# ─────────────────────────────────────────────────────────────────────────────
#  Syntax-only check
# ─────────────────────────────────────────────────────────────────────────────
syntax:
	$(CC) $(CFLAGS) -fsyntax-only $(SRC_LIB)
	$(CC) $(CFLAGS) -fsyntax-only $(SRC_APP)
	$(CC) $(CFLAGS) -fsyntax-only $(SRC_TEST)
	@echo "[syntax]  OK — no errors"

# ─────────────────────────────────────────────────────────────────────────────
#  Static analysis
# ─────────────────────────────────────────────────────────────────────────────
analyze: | $(REPORTS)
	@echo "--- gcc -fanalyzer ---"
	$(CC) $(CFLAGS) -fanalyzer -c $(SRC_LIB) -o /dev/null \
	    2>&1 | tee $(REPORTS)/analyzer.txt || true
	@echo "--- cppcheck ---"
	cppcheck --enable=all --std=c11 --suppress=missingIncludeSystem \
	    -Iinclude $(SRC_LIB) $(SRC_APP) $(SRC_TEST) \
	    2>&1 | tee $(REPORTS)/cppcheck.txt || true
	@echo "[analyze] reports saved to $(REPORTS)/"

# ─────────────────────────────────────────────────────────────────────────────
#  Sanitizers (AddressSanitizer + UBSan)
# ─────────────────────────────────────────────────────────────────────────────
$(SAN_BIN): $(SRC_TEST) $(SRC_LIB) | $(BUILD)
	$(CC) $(CFLAGS_D) -fsanitize=address,undefined \
	    -o $@ $(SRC_TEST) $(SRC_LIB) -Iinclude

sanitize: $(SAN_BIN)
	@echo ""; $(SAN_BIN); echo ""

# ─────────────────────────────────────────────────────────────────────────────
#  Documentation (Doxygen)
# ─────────────────────────────────────────────────────────────────────────────
docs-html: docs/Doxyfile
	cd docs && doxygen Doxyfile
	@echo "[docs]    docs/html/index.html"

docs-pdf: docs-html
	$(MAKE) -C docs/latex > /dev/null 2>&1 && \
	    echo "[docs]    docs/latex/refman.pdf" || \
	    echo "[docs]    LaTeX not available — HTML only"

# ─────────────────────────────────────────────────────────────────────────────
#  Utility
# ─────────────────────────────────────────────────────────────────────────────
$(BUILD):
	mkdir -p $(BUILD)

$(REPORTS):
	mkdir -p $(REPORTS)

clean:
	rm -rf $(BUILD) $(REPORTS) docs/html docs/latex
	find . -name "__pycache__" -exec rm -rf {} + 2>/dev/null || true
	find . -name "*.pyc"       -delete 2>/dev/null || true
	@echo "[clean]   done"

help:
	@echo ""
	@echo "  make all        shared + app + C tests"
	@echo "  make shared     build libfreqlib.so"
	@echo "  make app        build demo application"
	@echo "  make run        run demo application"
	@echo "  make test       run C unit tests"
	@echo "  make py-test    run Python ctypes tests"
	@echo "  make syntax     compiler syntax-only check"
	@echo "  make analyze    gcc -fanalyzer + cppcheck"
	@echo "  make sanitize   AddressSanitizer + UBSan"
	@echo "  make docs-html  generate Doxygen HTML docs"
	@echo "  make docs-pdf   generate Doxygen PDF docs"
	@echo "  make clean      remove all build artifacts"
	@echo ""
