/**
 * @file  test_freqlib.c
 * @brief Unit tests for freqlib.
 *
 * Tests are grouped into three suites:
 *   - char_freq  — character counting
 *   - word_freq  — word counting
 *   - top_k      — top-k selection
 *
 * Exit status: 0 if all tests pass, 1 otherwise.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freqlib.h"

/* ------------------------------------------------------------------ */
/*  Minimal test framework                                              */
/* ------------------------------------------------------------------ */

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(expr, label)                                  \
    do {                                                    \
        if (expr) {                                         \
            printf("  PASS  %s\n", (label));               \
            g_pass++;                                       \
        } else {                                            \
            printf("  FAIL  %s  (line %d)\n",              \
                   (label), __LINE__);                      \
            g_fail++;                                       \
        }                                                   \
    } while (0)

/* ------------------------------------------------------------------ */
/*  char_freq tests                                                     */
/* ------------------------------------------------------------------ */

static void suite_char_freq(void)
{
    puts("\n[char_freq]");
    char_freq_t cf;

    /* Error paths */
    CHECK(freq_char_count(NULL, &cf) == FREQ_ERR_NULL,
          "NULL text -> FREQ_ERR_NULL");
    CHECK(freq_char_count("hello", NULL) == FREQ_ERR_NULL,
          "NULL out  -> FREQ_ERR_NULL");
    CHECK(freq_char_count("", &cf) == FREQ_ERR_EMPTY,
          "empty string -> FREQ_ERR_EMPTY");

    /* Typical case */
    CHECK(freq_char_count("aabbc", &cf) == FREQ_OK,
          "\"aabbc\" returns FREQ_OK");
    CHECK(cf.counts[(unsigned char)'a'] == 2, "count('a') == 2");
    CHECK(cf.counts[(unsigned char)'b'] == 2, "count('b') == 2");
    CHECK(cf.counts[(unsigned char)'c'] == 1, "count('c') == 1");
    CHECK(cf.total == 5,                       "total == 5");
    freq_char_free(&cf);
    CHECK(cf.total == 0, "freq_char_free zeros total");

    /* Single character */
    freq_char_count("z", &cf);
    CHECK(cf.counts[(unsigned char)'z'] == 1, "single char: count('z') == 1");
    CHECK(cf.total == 1,                       "single char: total == 1");
    freq_char_free(&cf);

    /* Only spaces */
    freq_char_count("   ", &cf);
    CHECK(cf.counts[(unsigned char)' '] == 3, "spaces: count(' ') == 3");
    CHECK(cf.total == 3,                       "spaces: total == 3");
    freq_char_free(&cf);

    /* Repeated single character */
    freq_char_count("aaaaaaaaaa", &cf);
    CHECK(cf.counts[(unsigned char)'a'] == 10, "10 x 'a': count == 10");
    freq_char_free(&cf);
}

/* ------------------------------------------------------------------ */
/*  word_freq tests                                                     */
/* ------------------------------------------------------------------ */

static void suite_word_freq(void)
{
    puts("\n[word_freq]");
    word_freq_t wf;

    /* Error paths */
    CHECK(freq_word_count(NULL, &wf) == FREQ_ERR_NULL,
          "NULL text -> FREQ_ERR_NULL");
    CHECK(freq_word_count("hi", NULL) == FREQ_ERR_NULL,
          "NULL out  -> FREQ_ERR_NULL");
    CHECK(freq_word_count("", &wf) == FREQ_ERR_EMPTY,
          "empty string -> FREQ_ERR_EMPTY");
    CHECK(freq_word_count("!!! ???", &wf) == FREQ_ERR_EMPTY,
          "no words -> FREQ_ERR_EMPTY");

    /* Typical case */
    CHECK(freq_word_count("the cat and the dog", &wf) == FREQ_OK,
          "\"the cat and the dog\" -> FREQ_OK");
    CHECK(wf.size == 4, "4 distinct words");

    int the_ok = 0;
    for (size_t i = 0; i < wf.size; i++) {
        if (strcmp(wf.entries[i].word, "the") == 0 &&
            wf.entries[i].count == 2)
            the_ok = 1;
    }
    CHECK(the_ok, "\"the\" appears 2 times");
    freq_word_free(&wf);
    CHECK(wf.entries == NULL, "freq_word_free nulls entries");

    /* Single word */
    freq_word_count("hello", &wf);
    CHECK(wf.size == 1,                                  "1 distinct word");
    CHECK(strcmp(wf.entries[0].word, "hello") == 0,     "word is \"hello\"");
    CHECK(wf.entries[0].count == 1,                      "count == 1");
    freq_word_free(&wf);

    /* Punctuation and digits treated as separators / word chars */
    freq_word_count("hello, world! hello.", &wf);
    int h2 = 0;
    for (size_t i = 0; i < wf.size; i++)
        if (strcmp(wf.entries[i].word, "hello") == 0 &&
            wf.entries[i].count == 2) h2 = 1;
    CHECK(h2, "punctuation-separated \"hello\" counted twice");
    freq_word_free(&wf);

    /* Long word truncation — must not crash */
    char big[200];
    memset(big, 'x', 100); big[100] = '\0';
    CHECK(freq_word_count(big, &wf) == FREQ_OK,
          "100-char word does not crash");
    CHECK(strlen(wf.entries[0].word) == FREQ_WORD_MAXLEN,
          "long word truncated to FREQ_WORD_MAXLEN");
    freq_word_free(&wf);

    /* Case sensitivity */
    freq_word_count("Hello hello HELLO", &wf);
    CHECK(wf.size == 3, "case-sensitive: 3 distinct entries");
    freq_word_free(&wf);
}

/* ------------------------------------------------------------------ */
/*  top_k tests                                                         */
/* ------------------------------------------------------------------ */

static void suite_top_k(void)
{
    puts("\n[top_k]");
    word_freq_t   wf;
    word_entry_t *res  = NULL;
    size_t        resn = 0;

    /* Build a test table */
    freq_word_count("a b a c a b", &wf);

    /* Error paths */
    CHECK(freq_top_k(NULL, 2, &res, &resn) == FREQ_ERR_NULL,
          "NULL wf -> FREQ_ERR_NULL");
    CHECK(freq_top_k(&wf, 2, NULL, &resn) == FREQ_ERR_NULL,
          "NULL result -> FREQ_ERR_NULL");
    CHECK(freq_top_k(&wf, 2, &res, NULL) == FREQ_ERR_NULL,
          "NULL result_n -> FREQ_ERR_NULL");
    CHECK(freq_top_k(&wf, 0, &res, &resn) == FREQ_ERR_PARAM,
          "k == 0 -> FREQ_ERR_PARAM");

    /* Typical case: top-2 */
    CHECK(freq_top_k(&wf, 2, &res, &resn) == FREQ_OK,
          "top-2 returns FREQ_OK");
    CHECK(resn == 2, "resn == 2");
    CHECK(strcmp(res[0].word, "a") == 0 && res[0].count == 3,
          "res[0] == (\"a\", 3)");
    CHECK(strcmp(res[1].word, "b") == 0 && res[1].count == 2,
          "res[1] == (\"b\", 2)");
    free(res); res = NULL;

    /* k > table size */
    CHECK(freq_top_k(&wf, 100, &res, &resn) == FREQ_OK,
          "k > size: FREQ_OK");
    CHECK(resn == wf.size, "k > size: resn == wf.size");
    free(res); res = NULL;

    /* k == 1 */
    CHECK(freq_top_k(&wf, 1, &res, &resn) == FREQ_OK,
          "k == 1: FREQ_OK");
    CHECK(resn == 1 && strcmp(res[0].word, "a") == 0,
          "k == 1: top word is \"a\"");
    free(res); res = NULL;

    freq_word_free(&wf);

    /* Empty table */
    memset(&wf, 0, sizeof(wf));
    CHECK(freq_top_k(&wf, 5, &res, &resn) == FREQ_OK,
          "empty table: FREQ_OK");
    CHECK(resn == 0 && res == NULL, "empty table: resn==0, res==NULL");
}

/* ------------------------------------------------------------------ */
/*  Entry point                                                         */
/* ------------------------------------------------------------------ */

int main(void)
{
    puts("=== freqlib test suite ===");

    suite_char_freq();
    suite_word_freq();
    suite_top_k();

    printf("\n%d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
