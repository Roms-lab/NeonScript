#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef _WIN32
#include <windows.h>
#endif

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

// Helper: Set terminal text color using ANSI escape codes
void set_text_color(const char* color) {
    if (strcmp(color, "red") == 0) {
        printf("\033[31m");
        fflush(stdout);
    } else if (strcmp(color, "green") == 0) {
        printf("\033[32m");
        fflush(stdout);
    } else if (strcmp(color, "yellow") == 0) {
        printf("\033[33m");
        fflush(stdout);
    } else if (strcmp(color, "blue") == 0) {
        printf("\033[34m");
        fflush(stdout);
    } else if (strcmp(color, "magenta") == 0) {
        printf("\033[35m");
        fflush(stdout);
    } else if (strcmp(color, "cyan") == 0) {
        printf("\033[36m");
        fflush(stdout);
    } else if (strcmp(color, "white") == 0) {
        printf("\033[37m");
        fflush(stdout);
    } else if (strcmp(color, "reset") == 0) {
        printf("\033[0m");
        fflush(stdout);
    } else {
        printf("Unknown color: %s\n", color);
    }
}

int main(int argc, char *argv[]) {
#ifdef _WIN32
    // Enable ANSI escape sequences in Windows Console
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif
    
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
        // neon.textcolor(color)
        else if (strncmp(line, "neon.textcolor(", 16) == 0) {
            char color[64];
            if (sscanf(line, "neon.textcolor(%63[^)])", color) == 1) {
                // Remove any leading/trailing whitespace
                char *start = color;
                while (*start == ' ') start++;
                char *end = start + strlen(start) - 1;
                while (end > start && (*end == ' ' || *end == '\n')) {
                    *end = '\0';
                    end--;
                }
                set_text_color(start);
            }
        }
        // REPEAT command
        else if (strncmp(line, "repeat (", 8) == 0) {
            int repeat_count = 0;
            if (sscanf(line, "repeat (%d)", &repeat_count) == 1 && repeat_count > 0) {
                char repeat_line[256];
                // Read the next line (the command to repeat)
                if (fgets(repeat_line, sizeof(repeat_line), fp)) {
                    size_t repeat_len = strlen(repeat_line);
                    if (repeat_len > 0 && repeat_line[repeat_len - 1] == '\n') 
                        repeat_line[repeat_len - 1] = '\0';
                    
                    // Remove leading whitespace/indentation
                    char* cmd = repeat_line;
                    while (*cmd == ' ' || *cmd == '\t') cmd++;
                    
                    // Execute the command repeat_count times
                    for (int r = 0; r < repeat_count; r++) {
                        // Handle PRINT command with variable substitution
                        if (strncmp(cmd, "print ", 6) == 0) {
                            char* msg = cmd + 6;
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
                        // Handle other commands that might be repeated
                        else if (strncmp(cmd, "neon.textcolor(", 16) == 0) {
                            char color[64];
                            if (sscanf(cmd, "neon.textcolor(%63[^)])", color) == 1) {
                                char *start = color;
                                while (*start == ' ') start++;
                                char *end = start + strlen(start) - 1;
                                while (end > start && (*end == ' ' || *end == '\n')) {
                                    *end = '\0';
                                    end--;
                                }
                                set_text_color(start);
                            }
                        }
                    }
                }
            }
        }
        // FOREVER command
        else if (strncmp(line, "forever", 7) == 0) {
            char forever_line[256];
            // Read the next line (the command to repeat forever)
            if (fgets(forever_line, sizeof(forever_line), fp)) {
                size_t forever_len = strlen(forever_line);
                if (forever_len > 0 && forever_line[forever_len - 1] == '\n') 
                    forever_line[forever_len - 1] = '\0';
                
                // Remove leading whitespace/indentation
                char* cmd = forever_line;
                while (*cmd == ' ' || *cmd == '\t') cmd++;
                
                // Execute the command forever (infinite loop)
                while (1) {
                    // Handle PRINT command with variable substitution
                    if (strncmp(cmd, "print ", 6) == 0) {
                        char* msg = cmd + 6;
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
                    // Handle other commands that might be repeated
                    else if (strncmp(cmd, "neon.textcolor(", 16) == 0) {
                        char color[64];
                        if (sscanf(cmd, "neon.textcolor(%63[^)])", color) == 1) {
                            char *start = color;
                            while (*start == ' ') start++;
                            char *end = start + strlen(start) - 1;
                            while (end > start && (*end == ' ' || *end == '\n')) {
                                *end = '\0';
                                end--;
                            }
                            set_text_color(start);
                        }
                    }
                    // Handle SET command
                    else if (strncmp(cmd, "set ", 4) == 0) {
                        char varname[64], varval[256];
                        if (sscanf(cmd + 4, "%63s %255[^\n]", varname, varval) == 2) {
                            set_var(varname, varval);
                        }
                    }
                }
            }
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
        // INPUT command with variable storage
        else if (strncmp(line, "input ", 6) == 0) {
            char* input_line = line + 6;
            char prompt[256] = "";
            char varname[64] = "";
            
            // Check if it has quotes (prompted input)
            if (input_line[0] == '"') {
                // Find closing quote
                int i = 1;
                while (input_line[i] && input_line[i] != '"') {
                    prompt[i-1] = input_line[i];
                    i++;
                }
                prompt[i-1] = '\0';
                
                // Skip quote and space, get variable name
                i += 2; // skip " and space
                sscanf(input_line + i, "%63s", varname);
                
                printf("%s", prompt);
            } else {
                // No prompt, just variable name
                sscanf(input_line, "%63s", varname);
            }
            
            // Get user input
            char user_input[256];
            if (fgets(user_input, sizeof(user_input), stdin)) {
                // Remove newline
                size_t in_len = strlen(user_input);
                if (in_len > 0 && user_input[in_len - 1] == '\n') 
                    user_input[in_len - 1] = '\0';
                
                // Store in variable
                set_var(varname, user_input);
            }
        }
    }

    fclose(fp);
    return 0;
}
