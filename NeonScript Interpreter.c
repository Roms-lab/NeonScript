#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_VARS 100

typedef struct {
    char name[64];
    char value[256];
} Variable;

Variable vars[MAX_VARS];
int var_count = 0;

// Helper: Find variable by name
char* get_var(const char* name) {
    for (int i = 0; i < var_count; ++i) {
        if (strcmp(vars[i].name, name) == 0) {
            return vars[i].value;
        }
    }
    return NULL;
}

// Helper: Set variable
void set_var(const char* name, const char* value) {
    for (int i = 0; i < var_count; ++i) {
        if (strcmp(vars[i].name, name) == 0) {
            strncpy(vars[i].value, value, 255);
            return;
        }
    }
    // New variable
    if (var_count < MAX_VARS) {
        strncpy(vars[var_count].name, name, 63);
        strncpy(vars[var_count].value, value, 255);
        var_count++;
    }
}

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
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';

        if (strncmp(line, "//", 2) == 0) {
            continue;
        }
        // SET command
        else if (strncmp(line, "set ", 4) == 0) {
            char varname[64], varval[256];
            if (sscanf(line + 4, "%63s %255[^\n]", varname, varval) == 2) {
                set_var(varname, varval);
            }
        }
        // PRINT command with variable substitution
        else if (strncmp(line, "print ", 6) == 0) {
            char* msg = line + 6;
            char output[512] = "";
            for (int i = 0; msg[i];) {
                if (msg[i] == '$') {
                    // Find variable name
                    int j = i + 1;
                    char varname[64] = "";
                    int v = 0;
                    while (msg[j] && ((msg[j] >= 'a' && msg[j] <= 'z') || (msg[j] >= 'A' && msg[j] <= 'Z') || (msg[j] >= '0' && msg[j] <= '9') || msg[j] == '_')) {
                        varname[v++] = msg[j++];
                    }
                    varname[v] = '\0';
                    char* value = get_var(varname);
                    if (value)
                        strcat(output, value);
                    i = j;
                } else {
                    int l = strlen(output);
                    output[l] = msg[i];
                    output[l+1] = '\0';
                    i++;
                }
            }
            printf("%s\n", output);
        }
        // INPUT & DELAY as before...
        // ...
    }

    fclose(fp);
    return 0;
}
