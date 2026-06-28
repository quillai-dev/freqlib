/**
 * @file  freqlib.c
 * @brief Implementation of the freqlib text-frequency analysis library.
 */

#include "freqlib.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Internal helpers                                                    */
/* ------------------------------------------------------------------ */

/** Initial capacity for the word-entry array. */
#define WF_INIT_CAP 16

/**
 * @brief Linear search for @p word in @p wf.
 * @return Index of the matching entry, or -1 if not found.
 */
static int wf_find(const word_freq_t *wf, const char *word)
{
    for (size_t i = 0; i < wf->size; i++) {
        if (strcmp(wf->entries[i].word, word) == 0)
            return (int)i;
    }
    return -1;
}

/**
 * @brief Double the capacity of @p wf->entries (or initialise it).
 * @return ::FREQ_OK or ::FREQ_ERR_ALLOC.
 */
static int wf_grow(word_freq_t *wf)
{
    size_t       new_cap = (wf->cap == 0) ? WF_INIT_CAP : wf->cap * 2;
    word_entry_t *tmp    = realloc(wf->entries,
                                   new_cap * sizeof(word_entry_t));
    if (!tmp)
        return FREQ_ERR_ALLOC;

    wf->entries = tmp;
    wf->cap     = new_cap;
    return FREQ_OK;
}

/* ------------------------------------------------------------------ */
/*  char_freq implementation                                            */
/* ------------------------------------------------------------------ */

int freq_char_count(const char *text, char_freq_t *out)
{
    if (!text || !out)
        return FREQ_ERR_NULL;
    if (text[0] == '\0')
        return FREQ_ERR_EMPTY;

    memset(out, 0, sizeof(*out));

    for (const char *p = text; *p != '\0'; p++) {
        unsigned char c = (unsigned char)*p;
        if (c < FREQ_ASCII_SIZE) {
            out->counts[c]++;
            out->total++;
        }
    }
    return FREQ_OK;
}

void freq_char_free(char_freq_t *cf)
{
    if (!cf)
        return;
    memset(cf, 0, sizeof(*cf));
}

/* ------------------------------------------------------------------ */
/*  word_freq implementation                                            */
/* ------------------------------------------------------------------ */

int freq_word_count(const char *text, word_freq_t *out)
{
    if (!text || !out)
        return FREQ_ERR_NULL;
    if (text[0] == '\0')
        return FREQ_ERR_EMPTY;

    memset(out, 0, sizeof(*out));

    const char *p = text;
    while (*p != '\0') {
        /* Skip non-alphanumeric characters. */
        while (*p != '\0' && !isalnum((unsigned char)*p))
            p++;
        if (*p == '\0')
            break;

        /* Collect the next word (up to FREQ_WORD_MAXLEN bytes). */
        char   buf[FREQ_WORD_MAXLEN + 1];
        size_t len = 0;
        while (*p != '\0' && isalnum((unsigned char)*p)) {
            if (len < FREQ_WORD_MAXLEN)
                buf[len++] = *p;
            p++;
        }
        buf[len] = '\0';

        /* Update existing entry or create a new one. */
        int idx = wf_find(out, buf);
        if (idx >= 0) {
            out->entries[idx].count++;
        } else {
            if (out->size == out->cap) {
                int rc = wf_grow(out);
                if (rc != FREQ_OK) {
                    freq_word_free(out);
                    return rc;
                }
            }
            memcpy(out->entries[out->size].word, buf, len + 1);
            out->entries[out->size].count = 1;
            out->size++;
        }
    }
    if (out->size == 0) {
        freq_word_free(out);
        return FREQ_ERR_EMPTY;
    }
    return FREQ_OK;
}

void freq_word_free(word_freq_t *wf)
{
    if (!wf)
        return;
    free(wf->entries);
    memset(wf, 0, sizeof(*wf));
}

/* ------------------------------------------------------------------ */
/*  Top-k implementation                                                */
/* ------------------------------------------------------------------ */

/** qsort comparator: descending by count, ascending by word for ties. */
static int cmp_entry_desc(const void *a, const void *b)
{
    const word_entry_t *ea = (const word_entry_t *)a;
    const word_entry_t *eb = (const word_entry_t *)b;
    if (eb->count != ea->count)
        return (eb->count > ea->count) ? 1 : -1;
    return strcmp(ea->word, eb->word);
}

int freq_top_k(const word_freq_t *wf, size_t k,
               word_entry_t **result, size_t *result_n)
{
    if (!wf || !result || !result_n)
        return FREQ_ERR_NULL;

    *result   = NULL;
    *result_n = 0;

    if (k == 0)
        return FREQ_ERR_PARAM;

    if (wf->size == 0)
        return FREQ_OK;

    /* Sort a full copy so the original table is not reordered. */
    size_t        copy_n  = wf->size;
    word_entry_t *sorted  = malloc(copy_n * sizeof(word_entry_t));
    if (!sorted)
        return FREQ_ERR_ALLOC;

    memcpy(sorted, wf->entries, copy_n * sizeof(word_entry_t));
    qsort(sorted, copy_n, sizeof(word_entry_t), cmp_entry_desc);

    size_t        out_n  = (k < copy_n) ? k : copy_n;
    word_entry_t *out    = malloc(out_n * sizeof(word_entry_t));
    if (!out) {
        free(sorted);
        return FREQ_ERR_ALLOC;
    }

    memcpy(out, sorted, out_n * sizeof(word_entry_t));
    free(sorted);

    *result   = out;
    *result_n = out_n;
    return FREQ_OK;
}
