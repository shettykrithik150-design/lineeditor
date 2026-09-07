
/*
 * ============================================================
 * editor.c - Simple Command-Line Line Editor
 *
 * Data Structure:
 *     Dynamic array of strings
 *
 * Core Features:
 *     1. Insert a line
 *     2. Delete a line
 *     3. Display the document
 *
 * Bonus Features:
 *     4. Save / Load file
 *     5. Search
 *     6. Line count / Word count
 *
 * Commands:
 *     insert <line_number> <text>
 *     delete <line_number>
 *     display
 *     save <filename>
 *     load <filename>
 *     search <word or phrase>
 *     count
 *     help
 *     quit
 *
 * Compile:
 *     gcc -Wall -Wextra -std=c11 editor.c -o editor
 *
 * Run:
 *     ./editor
 *
 * Or load a file automatically at startup:
 *     ./editor document.txt
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INITIAL_CAPACITY 8
#define MAX_INPUT 1024
#define MAX_FILENAME 256

/* ------------------------------------------------------------
 * Document structure
 * ------------------------------------------------------------ */

typedef struct {
    char **lines;
    int count;
    int capacity;
} Document;


/* ------------------------------------------------------------
 * Utility function
 * ------------------------------------------------------------ */

/* Remove the newline character from a string */
void remove_newline(char *str)
{
    size_t len = strlen(str);

    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';
    }
}


/* Create a copy of a string using dynamic memory */
char *copy_string(const char *text)
{
    char *copy = malloc(strlen(text) + 1);

    if (copy == NULL) {
        return NULL;
    }

    strcpy(copy, text);
    return copy;
}


/* ------------------------------------------------------------
 * Document initialization and cleanup
 * ------------------------------------------------------------ */

void initialize_document(Document *doc)
{
    doc->count = 0;
    doc->capacity = INITIAL_CAPACITY;

    doc->lines = malloc(doc->capacity * sizeof(char *));

    if (doc->lines == NULL) {
        printf("Error: Unable to allocate memory.\n");
        exit(EXIT_FAILURE);
    }
}


void free_document(Document *doc)
{
    int i;

    for (i = 0; i < doc->count; i++) {
        free(doc->lines[i]);
    }

    free(doc->lines);

    doc->lines = NULL;
    doc->count = 0;
    doc->capacity = 0;
}


/* ------------------------------------------------------------
 * Increase document capacity
 * ------------------------------------------------------------ */

int grow_document(Document *doc)
{
    int new_capacity = doc->capacity * 2;

    char **temp = realloc(
        doc->lines,
        new_capacity * sizeof(char *)
    );

    if (temp == NULL) {
        printf("Error: Unable to increase document capacity.\n");
        return 0;
    }

    doc->lines = temp;
    doc->capacity = new_capacity;

    return 1;
}


/* ------------------------------------------------------------
 * CORE FEATURE 1: Insert a line
 * ------------------------------------------------------------ */

void insert_line(Document *doc, int line_number, const char *text)
{
    int index;
    int i;
    char *new_line;

    /*
     * Valid insertion positions are:
     * 1 through count + 1
     *
     * count + 1 means inserting at the end.
     */
    if (line_number < 1 || line_number > doc->count + 1) {
        printf(
            "Error: Invalid line number. "
            "Valid range is 1-%d.\n",
            doc->count + 1
        );
        return;
    }

    /* Grow the array if necessary */
    if (doc->count == doc->capacity) {
        if (!grow_document(doc)) {
            return;
        }
    }

    /* Allocate memory for the new line */
    new_line = copy_string(text);

    if (new_line == NULL) {
        printf("Error: Memory allocation failed.\n");
        return;
    }

    index = line_number - 1;

    /*
     * Shift existing lines one position to the right.
     * Start from the end to avoid overwriting data.
     */
    for (i = doc->count; i > index; i--) {
        doc->lines[i] = doc->lines[i - 1];
    }

    doc->lines[index] = new_line;
    doc->count++;

    printf("Line inserted successfully.\n");
}


/* ------------------------------------------------------------
 * CORE FEATURE 2: Delete a line
 * ------------------------------------------------------------ */

void delete_line(Document *doc, int line_number)
{
    int index;
    int i;

    if (doc->count == 0) {
        printf("Error: Document is empty.\n");
        return;
    }

    if (line_number < 1 || line_number > doc->count) {
        printf(
            "Error: Invalid line number. "
            "Valid range is 1-%d.\n",
            doc->count
        );
        return;
    }

    index = line_number - 1;

    /* Free memory used by the deleted line */
    free(doc->lines[index]);

    /*
     * Shift all lines after the deleted line
     * one position to the left.
     */
    for (i = index; i < doc->count - 1; i++) {
        doc->lines[i] = doc->lines[i + 1];
    }

    doc->count--;

    printf("Line %d deleted successfully.\n", line_number);
}


/* ------------------------------------------------------------
 * CORE FEATURE 3: Display document
 * ------------------------------------------------------------ */

void display_document(const Document *doc)
{
    int i;

    if (doc->count == 0) {
        printf("\nDocument is empty.\n");
        return;
    }

    printf("\n========== DOCUMENT ==========\n");

    for (i = 0; i < doc->count; i++) {
        printf("%3d | %s\n", i + 1, doc->lines[i]);
    }

    printf("==============================\n");
}


/* ------------------------------------------------------------
 * BONUS FEATURE: Save document
 * ------------------------------------------------------------ */

int save_document(const Document *doc, const char *filename)
{
    FILE *file;
    int i;

    file = fopen(filename, "w");

    if (file == NULL) {
        printf("Error: Could not open '%s' for writing.\n", filename);
        return 0;
    }

    for (i = 0; i < doc->count; i++) {
        fprintf(file, "%s\n", doc->lines[i]);
    }

    fclose(file);

    printf(
        "Document saved successfully to '%s'.\n",
        filename
    );

    return 1;
}


/* ------------------------------------------------------------
 * BONUS FEATURE: Load document
 * ------------------------------------------------------------ */

int load_document(Document *doc, const char *filename)
{
    FILE *file;
    char buffer[MAX_INPUT];
    int loaded = 0;

    file = fopen(filename, "r");

    if (file == NULL) {
        printf("Error: Could not open '%s' for reading.\n", filename);
        return 0;
    }

    /*
     * First clear the existing document.
     */
    while (doc->count > 0) {
        free(doc->lines[doc->count - 1]);
        doc->count--;
    }

    while (fgets(buffer, sizeof(buffer), file) != NULL) {

        remove_newline(buffer);

        /*
         * If the line is too long for the input buffer,
         * the remaining part will be read separately.
         * For this small editor, the maximum line size is
         * intentionally limited.
         */

        if (doc->count == doc->capacity) {
            if (!grow_document(doc)) {
                fclose(file);
                return 0;
            }
        }

        doc->lines[doc->count] = copy_string(buffer);

        if (doc->lines[doc->count] == NULL) {
            printf("Error: Memory allocation failed.\n");
            fclose(file);
            return 0;
        }

        doc->count++;
        loaded++;
    }

    fclose(file);

    printf(
        "Loaded %d line(s) from '%s'.\n",
        loaded,
        filename
    );

    return 1;
}


/* ------------------------------------------------------------
 * BONUS FEATURE: Search
 * ------------------------------------------------------------ */

void search_document(const Document *doc, const char *query)
{
    int i;
    int found = 0;

    if (doc->count == 0) {
        printf("Document is empty.\n");
        return;
    }

    for (i = 0; i < doc->count; i++) {

        if (strstr(doc->lines[i], query) != NULL) {

            if (!found) {
                printf("Found on line(s): ");
            }

            printf("%d ", i + 1);
            found = 1;
        }
    }

    if (found) {
        printf("\n");
    } else {
        printf(
            "No line contains \"%s\".\n",
            query
        );
    }
}


/* ------------------------------------------------------------
 * BONUS FEATURE: Line count and word count
 * ------------------------------------------------------------ */

void document_statistics(const Document *doc)
{
    int i;
    int words = 0;

    for (i = 0; i < doc->count; i++) {

        int inside_word = 0;
        const char *p = doc->lines[i];

        while (*p != '\0') {

            if (isspace((unsigned char)*p)) {
                inside_word = 0;
            }
            else if (!inside_word) {
                inside_word = 1;
                words++;
            }

            p++;
        }
    }

    printf("\nDocument Statistics\n");
    printf("-------------------\n");
    printf("Lines : %d\n", doc->count);
    printf("Words : %d\n", words);
}


/* ------------------------------------------------------------
 * HELP
 * ------------------------------------------------------------ */

void display_help(void)
{
    printf(
        "\n============== HELP ==============\n"
        "\n"
        "insert <line> <text>\n"
        "    Insert a new line at the given line number.\n"
        "    Example: insert 1 Hello World\n"
        "\n"
        "delete <line>\n"
        "    Delete the specified line.\n"
        "    Example: delete 2\n"
        "\n"
        "display\n"
        "    Display all lines with line numbers.\n"
        "    Example: display\n"
        "\n"
        "save <filename>\n"
        "    Save the document to a text file.\n"
        "    Example: save document.txt\n"
        "\n"
        "load <filename>\n"
        "    Load a text file into the editor.\n"
        "    Example: load document.txt\n"
        "\n"
        "search <word or phrase>\n"
        "    Search for a word or phrase in the document.\n"
        "    Example: search hello\n"
        "\n"
        "count\n"
        "    Display the number of lines and words.\n"
        "    Example: count\n"
        "\n"
        "help\n"
        "    Display this help information.\n"
        "\n"
        "quit\n"
        "    Exit the editor.\n"
        "\n"
        "==================================\n"
    );
}


/* ------------------------------------------------------------
 * MAIN COMMAND LOOP
 * ------------------------------------------------------------ */

int main(int argc, char *argv[])
{
    Document doc;
    char input[MAX_INPUT];

    initialize_document(&doc);

    printf("\n=====================================\n");
    printf("       SIMPLE LINE EDITOR\n");
    printf("=====================================\n");

    /*
     * Optional startup file.
     *
     * Usage:
     *     ./editor document.txt
     */
    if (argc >= 2) {
        load_document(&doc, argv[1]);
    }

    printf("Type 'help' to see available commands.\n");

    while (1) {

        char command[32];

        printf("\n> ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        remove_newline(input);

        /* Ignore empty input */
        if (strlen(input) == 0) {
            continue;
        }

        /*
         * Read the first word as the command.
         */
        if (sscanf(input, "%31s", command) != 1) {
            continue;
        }


        /* ----------------------------------------------------
         * INSERT
         * ---------------------------------------------------- */

        if (strcmp(command, "insert") == 0) {

            int line_number;
            char *text;

            /*
             * Find the line number after "insert".
             */
            text = input + strlen(command);

            while (*text == ' ') {
                text++;
            }

            if (sscanf(text, "%d", &line_number) != 1) {
                printf(
                    "Usage: insert <line> <text>\n"
                );
                continue;
            }

            /*
             * Move pointer past the line number.
             */
            while (*text != '\0' && !isspace((unsigned char)*text)) {
                text++;
            }

            while (*text == ' ') {
                text++;
            }

            if (*text == '\0') {
                printf(
                    "Usage: insert <line> <text>\n"
                );
                continue;
            }

            insert_line(&doc, line_number, text);
        }


        /* ----------------------------------------------------
         * DELETE
         * ---------------------------------------------------- */

        else if (strcmp(command, "delete") == 0) {

            int line_number;

            if (sscanf(
                    input + strlen(command),
                    "%d",
                    &line_number
                ) != 1) {

                printf(
                    "Usage: delete <line>\n"
                );

                continue;
            }

            delete_line(&doc, line_number);
        }


        /* ----------------------------------------------------
         * DISPLAY
         * ---------------------------------------------------- */

        else if (strcmp(command, "display") == 0) {

            display_document(&doc);
        }


        /* ----------------------------------------------------
         * SAVE
         * ---------------------------------------------------- */

        else if (strcmp(command, "save") == 0) {

            char filename[MAX_FILENAME];

            if (sscanf(
                    input + strlen(command),
                    "%255s",
                    filename
                ) != 1) {

                printf(
                    "Usage: save <filename>\n"
                );

                continue;
            }

            save_document(&doc, filename);
        }


        /* ----------------------------------------------------
         * LOAD
         * ---------------------------------------------------- */

        else if (strcmp(command, "load") == 0) {

            char filename[MAX_FILENAME];

            if (sscanf(
                    input + strlen(command),
                    "%255s",
                    filename
                ) != 1) {

                printf(
                    "Usage: load <filename>\n"
                );

                continue;
            }

            load_document(&doc, filename);
        }


        /* ----------------------------------------------------
         * SEARCH
         * ---------------------------------------------------- */

        else if (strcmp(command, "search") == 0) {

            char *query = input + strlen(command);

            while (*query == ' ') {
                query++;
            }

            if (*query == '\0') {
                printf(
                    "Usage: search <word or phrase>\n"
                );

                continue;
            }

            search_document(&doc, query);
        }


        /* ----------------------------------------------------
         * COUNT
         * ---------------------------------------------------- */

        else if (strcmp(command, "count") == 0) {

            document_statistics(&doc);
        }


        /* ----------------------------------------------------
         * HELP
         * ---------------------------------------------------- */

        else if (strcmp(command, "help") == 0) {

            display_help();
        }


        /* ----------------------------------------------------
         * QUIT
         * ---------------------------------------------------- */

        else if (
            strcmp(command, "quit") == 0 ||
            strcmp(command, "exit") == 0
        ) {

            break;
        }


        /* ----------------------------------------------------
         * UNKNOWN COMMAND
         * ---------------------------------------------------- */

        else {

            printf(
                "Unknown command: %s\n",
                command
            );

            printf(
                "Type 'help' for available commands.\n"
            );
        }
    }

    free_document(&doc);

    printf("\nGoodbye!\n");

    return 0;
}
