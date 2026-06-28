/**
 * @file  freqlib.h
 * @brief Text frequency analysis library — public interface.
 *
 * Provides three categories of analysis over NUL-terminated ASCII text:
 *   - Per-character occurrence counting (`char_freq_*`).
 *   - Per-word occurrence counting       (`word_freq_*`).
 *   - Top-k most-frequent word retrieval (`freq_top_k`).
 *
 * ## Error handling
 * Every function returns one of the `FREQ_E*` constants defined below.
 * `FREQ_OK` (0) indicates success; negative values indicate errors.
 *
 * ## Memory ownership
 * - `freq_char_count` fills a caller-supplied `char_freq_t` in place;
 *   no heap allocation is performed.  Call `freq_char_free` when done.
 * - `freq_word_count` allocates `wf->entries` internally; the caller
 *   must eventually call `freq_word_free` to release it.
 * - `freq_top_k` allocates a new array and stores its address in
 *   `*result`; the caller must release it with `free()`.
 */

#ifndef FREQLIB_H
#define FREQLIB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  Return codes                                                        */
/* ------------------------------------------------------------------ */

/** @defgroup retcodes Return codes */
/** @{ */
#define FREQ_OK          0  /**< Operation completed successfully.        */
#define FREQ_ERR_NULL   -1  /**< A required pointer argument was NULL.    */
#define FREQ_ERR_EMPTY  -2  /**< Input string was empty (length 0).       */
#define FREQ_ERR_ALLOC  -3  /**< A heap allocation failed.                */
#define FREQ_ERR_PARAM  -4  /**< A numeric parameter had an invalid value.*/
/** @} */

/* ------------------------------------------------------------------ */
/*  Constants                                                           */
/* ------------------------------------------------------------------ */

/** Number of code points in the 7-bit ASCII range (0–127). */
#define FREQ_ASCII_SIZE 128

/** Maximum number of bytes accepted per word (excluding NUL terminator). */
#define FREQ_WORD_MAXLEN 63

/* ------------------------------------------------------------------ */
/*  Character-frequency API                                             */
/* ------------------------------------------------------------------ */

/**
 * @brief Aggregate per-character occurrence counts for ASCII text.
 *
 * Non-ASCII bytes (values >= 128) are silently ignored.
 */
typedef struct {
    size_t counts[FREQ_ASCII_SIZE]; /**< counts[c] == occurrences of char c. */
    size_t total;                   /**< Total ASCII characters seen.         */
} char_freq_t;

/**
 * @brief Count occurrences of every ASCII character in @p text.
 *
 * @param[in]  text  NUL-terminated input string.  Must not be NULL.
 * @param[out] out   Destination structure.  Must not be NULL.
 *                   Initialised by this function; prior contents ignored.
 * @return ::FREQ_OK, ::FREQ_ERR_NULL, or ::FREQ_ERR_EMPTY.
 */
int freq_char_count(const char *text, char_freq_t *out);

/**
 * @brief Reset all fields of @p cf to zero.
 * @param[in,out] cf  Must not be NULL.
 */
void freq_char_free(char_freq_t *cf);

/* ------------------------------------------------------------------ */
/*  Word-frequency API                                                  */
/* ------------------------------------------------------------------ */

/**
 * @brief A single word and its occurrence count.
 */
typedef struct {
    char   word[FREQ_WORD_MAXLEN + 1]; /**< NUL-terminated word.  */
    size_t count;                       /**< Number of occurrences. */
} word_entry_t;

/**
 * @brief Dynamic table that maps distinct words to their frequencies.
 *
 * Ownership of `entries` belongs to the library; release with
 * `freq_word_free()`.
 */
typedef struct {
    word_entry_t *entries; /**< Heap-allocated array of word entries.      */
    size_t        size;    /**< Number of distinct words currently stored. */
    size_t        cap;     /**< Allocated capacity (number of slots).      */
} word_freq_t;

/**
 * @brief Count occurrences of every word found in @p text.
 *
 * A word is a maximal run of alphanumeric characters (`isalnum`).
 * Words longer than ::FREQ_WORD_MAXLEN characters are silently truncated.
 * Comparison is byte-exact (case-sensitive).
 *
 * @param[in]  text  NUL-terminated input string.  Must not be NULL.
 * @param[out] out   Destination table.  Must not be NULL.
 *                   Initialised by this function; prior contents ignored.
 * @return ::FREQ_OK, ::FREQ_ERR_NULL, ::FREQ_ERR_EMPTY,
 *         or ::FREQ_ERR_ALLOC.
 */
int freq_word_count(const char *text, word_freq_t *out);

/**
 * @brief Release all resources owned by @p wf and zero its fields.
 * @param[in,out] wf  Must not be NULL.
 */
void freq_word_free(word_freq_t *wf);

/* ------------------------------------------------------------------ */
/*  Top-k API                                                           */
/* ------------------------------------------------------------------ */

/**
 * @brief Return the @p k most-frequent words from a populated table.
 *
 * The returned array is sorted in descending order of frequency.
 * If fewer than @p k distinct words exist the array contains all of them.
 *
 * @param[in]  wf        Populated word-frequency table.  Must not be NULL.
 * @param[in]  k         Number of top words requested.  Must be > 0.
 * @param[out] result    Set to a newly `malloc`-ed array of
 *                       `min(k, wf->size)` ::word_entry_t values.
 *                       The caller must release this with `free()`.
 *                       Set to NULL on error.
 * @param[out] result_n  Set to the actual number of entries returned.
 *                       Set to 0 on error.
 * @return ::FREQ_OK, ::FREQ_ERR_NULL, ::FREQ_ERR_PARAM,
 *         or ::FREQ_ERR_ALLOC.
 */
int freq_top_k(const word_freq_t *wf, size_t k,
               word_entry_t **result, size_t *result_n);

#ifdef __cplusplus
}
#endif

#endif /* FREQLIB_H */
