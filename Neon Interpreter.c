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
        // Example: PRINT Hello, World!
        if (strncmp(line, "PRINT ", 6) == 0) {
            printf("%s", line + 6);
        }
        // Add more NeonScript commands here!
    }

    fclose(fp);
    return 0;
}
