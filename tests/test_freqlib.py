#!/usr/bin/env python3
"""
test_freqlib.py — Python integration tests for freqlib via ctypes.

Prerequisites
-------------
Build the shared library first::

    make shared

Then run this file::

    make py-test
    # or directly:
    python3 tests/test_freqlib.py
"""

import ctypes
import os
import sys

# ---------------------------------------------------------------------------
# Library loading
# ---------------------------------------------------------------------------

_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
_LIB  = os.path.join(_ROOT, "build", "libfreqlib.so")

try:
    lib = ctypes.CDLL(_LIB)
except OSError as exc:
    print(f"Cannot load {_LIB}: {exc}")
    print("Run 'make shared' first.")
    sys.exit(1)

# ---------------------------------------------------------------------------
# Constants (must match freqlib.h)
# ---------------------------------------------------------------------------

FREQ_OK        =  0
FREQ_ERR_NULL  = -1
FREQ_ERR_EMPTY = -2
FREQ_ERR_ALLOC = -3
FREQ_ERR_PARAM = -4

FREQ_ASCII_SIZE = 128
FREQ_WORD_MAXLEN = 63

# ---------------------------------------------------------------------------
# ctypes structure definitions
# ---------------------------------------------------------------------------

class CharFreq(ctypes.Structure):
    _fields_ = [
        ("counts", ctypes.c_size_t * FREQ_ASCII_SIZE),
        ("total",  ctypes.c_size_t),
    ]


class WordEntry(ctypes.Structure):
    _fields_ = [
        ("word",  ctypes.c_char * (FREQ_WORD_MAXLEN + 1)),
        ("count", ctypes.c_size_t),
    ]


class WordFreq(ctypes.Structure):
    _fields_ = [
        ("entries", ctypes.POINTER(WordEntry)),
        ("size",    ctypes.c_size_t),
        ("cap",     ctypes.c_size_t),
    ]

# ---------------------------------------------------------------------------
# Function bindings
# ---------------------------------------------------------------------------

lib.freq_char_count.argtypes  = [ctypes.c_char_p, ctypes.POINTER(CharFreq)]
lib.freq_char_count.restype   = ctypes.c_int

lib.freq_char_free.argtypes   = [ctypes.POINTER(CharFreq)]
lib.freq_char_free.restype    = None

lib.freq_word_count.argtypes  = [ctypes.c_char_p, ctypes.POINTER(WordFreq)]
lib.freq_word_count.restype   = ctypes.c_int

lib.freq_word_free.argtypes   = [ctypes.POINTER(WordFreq)]
lib.freq_word_free.restype    = None

lib.freq_top_k.argtypes = [
    ctypes.POINTER(WordFreq),
    ctypes.c_size_t,
    ctypes.POINTER(ctypes.POINTER(WordEntry)),
    ctypes.POINTER(ctypes.c_size_t),
]
lib.freq_top_k.restype = ctypes.c_int

# ---------------------------------------------------------------------------
# Minimal test runner
# ---------------------------------------------------------------------------

_passed = 0
_failed = 0


def check(cond: bool, label: str) -> None:
    global _passed, _failed
    if cond:
        print(f"  PASS  {label}")
        _passed += 1
    else:
        print(f"  FAIL  {label}")
        _failed += 1

# ---------------------------------------------------------------------------
# char_freq tests
# ---------------------------------------------------------------------------

def test_char_freq() -> None:
    print("\n[char_freq]")
    cf = CharFreq()

    check(lib.freq_char_count(None, ctypes.byref(cf)) == FREQ_ERR_NULL,
          "NULL text -> FREQ_ERR_NULL")
    check(lib.freq_char_count(b"", ctypes.byref(cf)) == FREQ_ERR_EMPTY,
          "empty -> FREQ_ERR_EMPTY")

    rc = lib.freq_char_count(b"aabbc", ctypes.byref(cf))
    check(rc == FREQ_OK,                    "\"aabbc\" -> FREQ_OK")
    check(cf.counts[ord('a')] == 2,         "count('a') == 2")
    check(cf.counts[ord('b')] == 2,         "count('b') == 2")
    check(cf.counts[ord('c')] == 1,         "count('c') == 1")
    check(cf.total == 5,                    "total == 5")
    lib.freq_char_free(ctypes.byref(cf))
    check(cf.total == 0,                    "freq_char_free zeros total")

    lib.freq_char_count(b"hello world", ctypes.byref(cf))
    check(cf.counts[ord(' ')] == 1,         "space counted")
    lib.freq_char_free(ctypes.byref(cf))

# ---------------------------------------------------------------------------
# word_freq tests
# ---------------------------------------------------------------------------

def test_word_freq() -> None:
    print("\n[word_freq]")
    wf = WordFreq()

    check(lib.freq_word_count(None, ctypes.byref(wf)) == FREQ_ERR_NULL,
          "NULL text -> FREQ_ERR_NULL")
    check(lib.freq_word_count(b"", ctypes.byref(wf)) == FREQ_ERR_EMPTY,
          "empty -> FREQ_ERR_EMPTY")

    rc = lib.freq_word_count(b"the cat and the dog", ctypes.byref(wf))
    check(rc == FREQ_OK, "\"the cat and the dog\" -> FREQ_OK")
    check(wf.size == 4,  "4 distinct words")

    the_count = next(
        (wf.entries[i].count for i in range(wf.size)
         if wf.entries[i].word == b"the"),
        0,
    )
    check(the_count == 2, "\"the\" appears 2 times")
    lib.freq_word_free(ctypes.byref(wf))
    check(not wf.entries, "freq_word_free nulls entries")

    lib.freq_word_count(b"hello, world! hello.", ctypes.byref(wf))
    h2 = next(
        (wf.entries[i].count for i in range(wf.size)
         if wf.entries[i].word == b"hello"),
        0,
    )
    check(h2 == 2, "punctuation-separated \"hello\" counted twice")
    lib.freq_word_free(ctypes.byref(wf))

# ---------------------------------------------------------------------------
# top_k tests
# ---------------------------------------------------------------------------

def test_top_k() -> None:
    print("\n[top_k]")
    wf   = WordFreq()
    rptr = ctypes.POINTER(WordEntry)()
    rn   = ctypes.c_size_t(0)

    lib.freq_word_count(b"a b a c a b", ctypes.byref(wf))

    check(lib.freq_top_k(None, 2, ctypes.byref(rptr), ctypes.byref(rn))
          == FREQ_ERR_NULL, "NULL wf -> FREQ_ERR_NULL")
    check(lib.freq_top_k(ctypes.byref(wf), 0,
                         ctypes.byref(rptr), ctypes.byref(rn))
          == FREQ_ERR_PARAM, "k==0 -> FREQ_ERR_PARAM")

    rc = lib.freq_top_k(ctypes.byref(wf), 2,
                        ctypes.byref(rptr), ctypes.byref(rn))
    check(rc == FREQ_OK,   "top-2 -> FREQ_OK")
    check(rn.value == 2,   "rn == 2")
    check(rptr[0].word == b"a" and rptr[0].count == 3,
          "res[0] == (\"a\", 3)")
    check(rptr[1].word == b"b" and rptr[1].count == 2,
          "res[1] == (\"b\", 2)")

    # free the result array via ctypes libc
    libc = ctypes.CDLL(None)
    libc.free(rptr)

    lib.freq_word_free(ctypes.byref(wf))

# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    print("=== freqlib Python test suite ===")
    test_char_freq()
    test_word_freq()
    test_top_k()
    print(f"\n{_passed} passed, {_failed} failed")
    sys.exit(0 if _failed == 0 else 1)
