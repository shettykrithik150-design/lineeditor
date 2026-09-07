/* ============================================================
 * editor.c — A simple command-line line editor
 *
 * Data structure: dynamic (growable) array of C strings.
 *   - char **lines   : array of pointers to heap-allocated line strings
 *   - int count      : number of lines currently stored
 *   - int capacity   : allocated size of the lines array (doubles when full)
 *
 * Why a dynamic array instead of a linked list?
 *   The most common operations here are "display everything" and
 *   "go to line N" (for insert/delete/replace/search), both of which
 *   are O(1) to index into with an array but O(n) to reach in a linked
 *   list. Insert/delete do require shifting elements (O(n)), but for a
 *   small in-memory document that cost is negligible, and we avoid the
 *   extra pointer bookkeeping a linked list would need.
 *
 * Supported commands (see HELP.md for details):
 *   insert <line#> <text>
 *   delete <line#>
 *   display
 *   save <filename>
 *   load <filename>
 *   search <word>
 *   count
 *   help
 *   quit
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 8
#define MAX_INPUT_LEN 1024

typedef struct {
    char **lines;   /* array of heap-allocated line strings */
    int count;      /* number of lines currently in use     */
    int capacity;   /* allocated slots in the lines array   */
} Document;

/* ---------- Document lifecycle ---------- */

void doc_init(Document *doc) {
    doc->capacity = INITIAL_CAPACITY;
    doc->count = 0;
    doc->lines = malloc(sizeof(char *) * doc->capacity);
    if (!doc->lines) {
        fprintf(stderr, "Fatal: out of memory initializing document.\n");
        exit(1);
    }
}

void doc_free(Document *doc) {
    for (int i = 0; i < doc->count; i++) {
        free(doc->lines[i]);
    }
    free(doc->lines);
    doc->lines = NULL;
    doc->count = 0;
    doc->capacity = 0;
}

/* Grow the backing array when it's full. */
static void doc_grow(Document *doc) {
    int new_capacity = doc->capacity * 2;
    char **new_lines = realloc(doc->lines, sizeof(char *) * new_capacity);
    if (!new_lines) {
        fprintf(stderr, "Error: out of memory while growing document.\n");
        return; /* keep old array intact; caller's insert will just fail safely */
    }
    doc->lines = new_lines;
    doc->capacity = new_capacity;
}

/* ---------- Core features ---------- */

/* Insert `text` at 1-based position `line_num`, shifting later lines down.
 * Valid positions are 1..count+1 (count+1 means "append at end"). */
void doc_insert(Document *doc, int line_num, const char *text) {
    if (line_num < 1 || line_num > doc->count + 1) {
        printf("Error: line %d is out of range (valid: 1-%d).\n",
               line_num, doc->count + 1);
        return;
    }

    if (doc->count == doc->capacity) {
        doc_grow(doc);
        if (doc->count == doc->capacity) return; /* grow failed */
    }

    char *copy = malloc(strlen(text) + 1);
    if (!copy) {
        printf("Error: out of memory; could not insert line.\n");
        return;
    }
    strcpy(copy, text);

    /* Shift everything from line_num..count down by one slot,
       working from the back so we don't overwrite data. */
    int index = line_num - 1; /* convert to 0-based array index */
    for (int i = doc->count; i > index; i--) {
        doc->lines[i] = doc->lines[i - 1];
    }
    doc->lines[index] = copy;
    doc->count++;

    printf("Inserted at line %d.\n", line_num);
}

/* Delete the line at 1-based position `line_num`, shifting later lines up. */
void doc_delete(Document *doc, int line_num) {
    if (doc->count == 0) {
        printf("Error: document is empty, nothing to delete.\n");
        return;
    }
    if (line_num < 1 || line_num > doc->count) {
        printf("Error: line %d is out of range (valid: 1-%d).\n",
               line_num, doc->count);
        return;
    }

    int index = line_num - 1;
    free(doc->lines[index]);

    for (int i = index; i < doc->count - 1; i++) {
        doc->lines[i] = doc->lines[i + 1];
    }
    doc->count--;

    printf("Deleted line %d.\n", line_num);
}

/* Print every line with its 1-based line number. */
void doc_display(const Document *doc) {
    if (doc->count == 0) {
        printf("(document is empty)\n");
        return;
    }
    for (int i = 0; i < doc->count; i++) {
        printf("%4d: %s\n", i + 1, doc->lines[i]);
    }
}

/* Write the document to a plain text file, one line per document line. */
void doc_save(const Document *doc, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        printf("Error: could not open '%s' for writing.\n", filename);
        return;
    }
    for (int i = 0; i < doc->count; i++) {
        fprintf(fp, "%s\n", doc->lines[i]);
    }
    fclose(fp);
    printf("Saved %d line(s) to '%s'.\n", doc->count, filename);
}

/* Load a document from a text file, replacing whatever is currently in memory. */
void doc_load(Document *doc, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Error: could not open '%s' for reading.\n", filename);
        return;
    }

    /* Clear out the existing document first. */
    for (int i = 0; i < doc->count; i++) free(doc->lines[i]);
    doc->count = 0;

    char buffer[MAX_INPUT_LEN];
    int loaded = 0;
    while (fgets(buffer, sizeof(buffer), fp)) {
        /* Strip trailing newline, if present. */
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';

        if (doc->count == doc->capacity) {
            doc_grow(doc);
            if (doc->count == doc->capacity) break; /* grow failed; stop loading */
        }
        char *copy = malloc(strlen(buffer) + 1);
        if (!copy) break;
        strcpy(copy, buffer);
        doc->lines[doc->count++] = copy;
        loaded++;
    }

    fclose(fp);
    printf("Loaded %d line(s) from '%s'.\n", loaded, filename);
}

/* ---------- Bonus features ---------- */

/* Report every line number containing `word` as a substring. */
void doc_search(const Document *doc, const char *word) {
    int found = 0;
    for (int i = 0; i < doc->count; i++) {
        if (strstr(doc->lines[i], word) != NULL) {
            if (!found) printf("Found on line(s): ");
            printf("%d ", i + 1);
            found = 1;
        }
    }
    if (found) printf("\n");
    else printf("'%s' not found in document.\n", word);
}

/* Report line count and total word count. */
void doc_count(const Document *doc) {
    int word_total = 0;
    for (int i = 0; i < doc->count; i++) {
        /* Count words by counting whitespace-separated tokens. */
        int in_word = 0;
        for (const char *p = doc->lines[i]; *p; p++) {
            if (*p == ' ' || *p == '\t') {
                in_word = 0;
            } else if (!in_word) {
                in_word = 1;
                word_total++;
            }
        }
    }
    printf("Lines: %d, Words: %d\n", doc->count, word_total);
}

/* ---------- Help text ---------- */

void print_help(void) {
    printf(
        "Commands:\n"
        "  insert <line#> <text>   Insert text at the given line number\n"
        "  delete <line#>          Delete the given line number\n"
        "  display                 Show the whole document\n"
        "  save <filename>         Save the document to a text file\n"
        "  load <filename>         Load a document from a text file\n"
        "  search <word>           Find which line(s) contain a word\n"
        "  count                   Show line and word counts\n"
        "  help                    Show this message\n"
        "  quit                    Exit the editor\n"
    );
}

/* ---------- Command loop ---------- */

int main(void) {
    Document doc;
    doc_init(&doc);

    char input[MAX_INPUT_LEN];

    printf("Simple Line Editor. Type 'help' for a list of commands.\n");

    while (1) {
        printf("> ");
        if (!fgets(input, sizeof(input), stdin)) break; /* EOF (e.g. Ctrl+D) */

        /* Strip trailing newline. */
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') input[len - 1] = '\0';

        /* Skip blank lines. */
        if (strlen(input) == 0) continue;

        char command[32] = {0};
        int consumed = 0;
        /* %n captures how many characters were consumed by the match so far,
           which lets us grab "the rest of the line" as one argument. */
        if (sscanf(input, "%31s%n", command, &consumed) != 1) continue;

        if (strcmp(command, "insert") == 0) {
            int line_num;
            int arg_consumed = 0;
            if (sscanf(input + consumed, " %d%n", &line_num, &arg_consumed) != 1) {
                printf("Usage: insert <line#> <text>\n");
                continue;
            }
            const char *text = input + consumed + arg_consumed;
            while (*text == ' ') text++; /* skip leading spaces before the text */
            if (*text == '\0') {
                printf("Usage: insert <line#> <text>\n");
                continue;
            }
            doc_insert(&doc, line_num, text);

        } else if (strcmp(command, "delete") == 0) {
            int line_num;
            if (sscanf(input + consumed, " %d", &line_num) != 1) {
                printf("Usage: delete <line#>\n");
                continue;
            }
            doc_delete(&doc, line_num);

        } else if (strcmp(command, "display") == 0) {
            doc_display(&doc);

        } else if (strcmp(command, "save") == 0) {
            char filename[256];
            if (sscanf(input + consumed, " %255s", filename) != 1) {
                printf("Usage: save <filename>\n");
                continue;
            }
            doc_save(&doc, filename);

        } else if (strcmp(command, "load") == 0) {
            char filename[256];
            if (sscanf(input + consumed, " %255s", filename) != 1) {
                printf("Usage: load <filename>\n");
                continue;
            }
            doc_load(&doc, filename);

        } else if (strcmp(command, "search") == 0) {
            const char *word = input + consumed;
            while (*word == ' ') word++;
            if (*word == '\0') {
                printf("Usage: search <word>\n");
                continue;
            }
            doc_search(&doc, word);

        } else if (strcmp(command, "count") == 0) {
            doc_count(&doc);

        } else if (strcmp(command, "help") == 0) {
            print_help();

        } else if (strcmp(command, "quit") == 0 || strcmp(command, "exit") == 0) {
            break;

        } else {
            printf("Unknown command: '%s'. Type 'help' for a list.\n", command);
        }
    }

    doc_free(&doc);
    printf("Goodbye.\n");
    return 0;
}