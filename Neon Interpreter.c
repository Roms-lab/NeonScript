#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <source-file.neo>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        printf("Cannot open file %s\n", argv[1]);
        return 1;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        // Remove trailing newline for easier processing
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';

        // Ignore comment lines
        if (strncmp(line, "//", 2) == 0) {
            continue;
        }
        // PRINT command
        else if (strncmp(line, "PRINT ", 6) == 0) {
            printf("%s\n", line + 6);
        }
        // INPUT command
        else if (strncmp(line, "INPUT ", 6) == 0) {
            printf("%s", line + 6); // Prompt
            printf(" "); // Add a space after prompt
            char user_input[256];
            if (fgets(user_input, sizeof(user_input), stdin)) {
                // Remove newline from input
                size_t in_len = strlen(user_input);
                if (in_len > 0 && user_input[in_len - 1] == '\n') user_input[in_len - 1] = '\0';
                printf("%s\n", user_input);
            }
        }
        // You can add more commands here
    }

    fclose(fp);
    return 0;
}
