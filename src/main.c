/**
 * @file  main.c
 * @brief Demonstration application for the freqlib library.
 *
 * Usage:
 *   ./app
 *
 * Analyses a hard-coded sample passage and prints:
 *   - The top 10 most-frequent words.
 *   - The frequency of every printable ASCII character that appears.
 */

#include <stdio.h>
#include <stdlib.h>

#include "freqlib.h"

/* ------------------------------------------------------------------ */
/*  Sample text                                                         */
/* ------------------------------------------------------------------ */

static const char SAMPLE[] =
    "To be or not to be that is the question "
    "Whether tis nobler in the mind to suffer "
    "The slings and arrows of outrageous fortune "
    "Or to take arms against a sea of troubles "
    "And by opposing end them to die to sleep "
    "No more and by a sleep to say we end "
    "The heartache and the thousand natural shocks "
    "That flesh is heir to tis a consummation";

/* ------------------------------------------------------------------ */
/*  Helper: print a horizontal separator                                */
/* ------------------------------------------------------------------ */

static void separator(void)
{
    fputs("------------------------------------------------\n", stdout);
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */

int main(void)
{
    int rc;

    puts("freqlib demo");
    separator();

    /* ---- character frequencies ------------------------------------ */
    char_freq_t cf;
    rc = freq_char_count(SAMPLE, &cf);
    if (rc != FREQ_OK) {
        fprintf(stderr, "freq_char_count failed (%d)\n", rc);
        return EXIT_FAILURE;
    }

    puts("Character frequencies (printable ASCII, count > 0):");
    for (int c = 32; c < 127; c++) {
        if (cf.counts[c] > 0)
            printf("  '%c'  %4zu\n", c, cf.counts[c]);
    }
    printf("  total: %zu\n", cf.total);
    freq_char_free(&cf);

    separator();

    /* ---- word frequencies ----------------------------------------- */
    word_freq_t wf;
    rc = freq_word_count(SAMPLE, &wf);
    if (rc != FREQ_OK) {
        fprintf(stderr, "freq_word_count failed (%d)\n", rc);
        return EXIT_FAILURE;
    }

    printf("Distinct words: %zu\n", wf.size);
    separator();

    /* ---- top-k ---------------------------------------------------- */
    word_entry_t *top  = NULL;
    size_t        topn = 0;

    rc = freq_top_k(&wf, 10, &top, &topn);
    if (rc != FREQ_OK) {
        fprintf(stderr, "freq_top_k failed (%d)\n", rc);
        freq_word_free(&wf);
        return EXIT_FAILURE;
    }

    puts("Top 10 most frequent words:");
    for (size_t i = 0; i < topn; i++)
        printf("  %2zu.  %-20s  %zu\n", i + 1, top[i].word, top[i].count);

    free(top);
    freq_word_free(&wf);

    separator();
    puts("Done.");
    return EXIT_SUCCESS;
}
