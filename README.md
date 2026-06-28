# freqlib

A lightweight C library for frequency analysis of ASCII text.

Counts per-character and per-word occurrences in any NUL-terminated string
and exposes a top-*k* query for the most frequent words.
The shared library can be called from C or Python (`ctypes`).

---

> **Note:** This project is a research work and assignment submitted to **Belgorod State Technological University named after V.G. Shukhov**.
---

## Features

| Feature | API |
|---|---|
| Character frequency counting | `freq_char_count` / `freq_char_free` |
| Word frequency counting | `freq_word_count` / `freq_word_free` |
| Top-*k* most frequent words | `freq_top_k` |
| Uniform error-code scheme | `FREQ_OK`, `FREQ_ERR_*` |
| Python integration | `ctypes` — see `tests/test_freqlib.py` |

---

## Project layout

```
freqlib/
├── include/
│   └── freqlib.h          Public API and Doxygen comments
├── src/
│   ├── freqlib.c          Library implementation
│   └── main.c             Demo application
├── tests/
│   ├── test_freqlib.c     C unit tests
│   └── test_freqlib.py    Python integration tests (ctypes)
├── latex_report   
│   ├── sections
│   │   ├── cover.tex
│   │   └── body.tex
│   ├── media
│   │   └── logo.png
│   ├── main.tex
│   └── main.pdf
├── docs/
│   └── Doxyfile           Doxygen configuration
├── reports/               Static-analysis output (git-ignored)
├── build/                 Compiled artifacts  (git-ignored)
├── Makefile
└── README.md
```

---

## Quick start

```bash
# Build library, demo app, and run C tests in one command
make all

# Run the demo application
make run

# Run Python integration tests
make py-test
```

---

## All Makefile targets

| Target | Description |
|---|---|
| `make all` | Build shared library + app + C tests |
| `make shared` | Build `build/libfreqlib.so` |
| `make app` | Build `build/app` demo application |
| `make run` | Run the demo application |
| `make test` | Run C unit tests |
| `make py-test` | Run Python ctypes tests |
| `make syntax` | Compiler syntax-only check (`-fsyntax-only`) |
| `make analyze` | `gcc -fanalyzer` + `cppcheck` |
| `make sanitize` | AddressSanitizer + UndefinedBehaviorSanitizer |
| `make docs-html` | Generate Doxygen HTML documentation |
| `make docs-pdf` | Generate Doxygen PDF documentation |
| `make clean` | Remove all build artifacts |

---

## Requirements

- GCC ≥ 7 (C11 support required)
- Python ≥ 3.6 (for `make py-test`)
- Doxygen (for `make docs-html`)
- cppcheck (for `make analyze`, optional)

---

## API overview

### Return codes

```c
FREQ_OK        =  0   // success
FREQ_ERR_NULL  = -1   // NULL pointer argument
FREQ_ERR_EMPTY = -2   // empty input string
FREQ_ERR_ALLOC = -3   // memory allocation failure
FREQ_ERR_PARAM = -4   // invalid parameter value
```

### Character frequencies

```c
char_freq_t cf;
if (freq_char_count("hello world", &cf) == FREQ_OK) {
    printf("'l' appears %zu times\n", cf.counts['l']);  // 3
    freq_char_free(&cf);
}
```

### Word frequencies

```c
word_freq_t wf;
if (freq_word_count("to be or not to be", &wf) == FREQ_OK) {
    printf("%zu distinct words\n", wf.size);
    freq_word_free(&wf);
}
```

### Top-*k* words

```c
word_entry_t *top = NULL;
size_t        topn = 0;
if (freq_top_k(&wf, 5, &top, &topn) == FREQ_OK) {
    for (size_t i = 0; i < topn; i++)
        printf("%s: %zu\n", top[i].word, top[i].count);
    free(top);
}
```

### Python (ctypes)

```python
import ctypes, os
lib = ctypes.CDLL("build/libfreqlib.so")
# See tests/test_freqlib.py for full binding definitions.
```

---

## Memory rules

- `freq_char_count` fills a caller-supplied struct; no heap allocation.
  Call `freq_char_free` when finished.
- `freq_word_count` allocates `wf.entries` internally.
  Release with `freq_word_free`.
- `freq_top_k` allocates the result array.
  The caller must release it with `free()`.

---

## License

This project is submitted as coursework and is not distributed under
an open-source license.

---

## Changelog

### 1.0.0
- Initial release: `freq_char_count`, `freq_word_count`, `freq_top_k`
- Fixed: `freq_word_count` now returns `FREQ_ERR_EMPTY` for strings
  that contain no alphanumeric words (e.g. `"!!! ???"`).
