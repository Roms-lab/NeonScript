#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#endif
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

// Helper: Set terminal text color using improved ANSI escape codes
void set_text_color(const char* color) {
    if (strcmp(color, "red") == 0) {
        printf("\033[1;31m");  // Bright red
        fflush(stdout);
    } else if (strcmp(color, "green") == 0) {
        printf("\033[1;32m");  // Bright green
        fflush(stdout);
    } else if (strcmp(color, "yellow") == 0) {
        printf("\033[1;33m");  // Bright yellow
        fflush(stdout);
    } else if (strcmp(color, "blue") == 0) {
        printf("\033[1;34m");  // Bright blue
        fflush(stdout);
    } else if (strcmp(color, "magenta") == 0) {
        printf("\033[1;35m");  // Bright magenta
        fflush(stdout);
    } else if (strcmp(color, "cyan") == 0) {
        printf("\033[1;36m");  // Bright cyan
        fflush(stdout);
    } else if (strcmp(color, "white") == 0) {
        printf("\033[1;37m");  // Bright white
        fflush(stdout);
    } else if (strcmp(color, "reset") == 0) {
        printf("\033[0m");     // Reset to default
        fflush(stdout);
    } else {
        printf("Unknown color: %s\n", color);
    }
}

// Helper: Trim leading/trailing whitespace (in-place)
void trim_whitespace(char* str) {
    if (!str) return;
    // Trim leading
    char* start = str;
    while (*start == ' ' || *start == '\t') start++;
    if (start != str) memmove(str, start, strlen(start) + 1);
    // Trim trailing
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t' || str[len - 1] == '\r' || str[len - 1] == '\n')) {
        str[len - 1] = '\0';
        len--;
    }
}

// Helper: Evaluate a simple boolean condition
// Supported forms:
// - true / false / 1 / 0
// - varName
// - varName == value
// - varName != value
// value may be an unquoted word or a quoted string "like this"
int eval_condition(const char* expr_in) {
    if (!expr_in) return 0;
    char expr[256];
    strncpy(expr, expr_in, sizeof(expr) - 1);
    expr[sizeof(expr) - 1] = '\0';
    trim_whitespace(expr);

    if (expr[0] == '\0') return 0;

    // Simple literals
    if (strcmp(expr, "true") == 0 || strcmp(expr, "1") == 0) return 1;
    if (strcmp(expr, "false") == 0 || strcmp(expr, "0") == 0) return 0;

    // Search for == or !=
    char* op = strstr(expr, "==");
    int is_not_equal = 0;
    if (!op) {
        op = strstr(expr, "!=");
        if (op) is_not_equal = 1;
    }

    if (op) {
        // Split into left and right
        char left[128];
        char right[256];
        size_t left_len = (size_t)(op - expr);
        if (left_len >= sizeof(left)) left_len = sizeof(left) - 1;
        strncpy(left, expr, left_len);
        left[left_len] = '\0';
        strncpy(right, op + 2, sizeof(right) - 1);
        right[sizeof(right) - 1] = '\0';
        trim_whitespace(left);
        trim_whitespace(right);

        // Resolve left variable value (allow optional leading $)
        const char* left_name = left;
        if (left_name[0] == '$') left_name++;
        char* left_val = get_var(left_name);
        const char* lval = left_val ? left_val : "";

        // Normalize right side: strip quotes if present
        if (right[0] == '"') {
            size_t rlen = strlen(right);
            if (rlen >= 2 && right[rlen - 1] == '"') {
                right[rlen - 1] = '\0';
                memmove(right, right + 1, rlen - 1);
            }
        }

        int cmp = strcmp(lval, right);
        return is_not_equal ? (cmp != 0) : (cmp == 0);
    }

    // Treat as variable truthiness (allow optional leading $)
    const char* name = expr;
    if (name[0] == '$') name++;
    char* val = get_var(name);
    if (!val) return 0;
    // Non-empty and not "0" is true
    return (val[0] != '\0' && !(val[0] == '0' && val[1] == '\0'));
}

// Helper: Execute a single command line (does not support control structures)
void execute_single_command(const char* original_cmd) {
    if (!original_cmd) return;
    // Skip leading whitespace
    const char* cmd = original_cmd;
    while (*cmd == ' ' || *cmd == '\t') cmd++;

    if (strncmp(cmd, "print ", 6) == 0) {
        const char* msg = cmd + 6;
        char output[512] = "";
        for (int i = 0; msg[i];) {
            if (msg[i] == '$') {
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
                int l = (int)strlen(output);
                output[l] = msg[i];
                output[l+1] = '\0';
                i++;
            }
        }
        printf("%s\n", output);
    }
    else if (strncmp(cmd, "neon.textcolor(", 16) == 0) {
        char color[64];
        if (sscanf(cmd, "neon.textcolor(%63[^)])", color) == 1) {
            char *start = color;
            while (*start == ' ') start++;
            char *end = start + strlen(start) - 1;
            while (end > start && (*end == ' ' || *end == '\n' || *end == '\r')) {
                *end = '\0';
                end--;
            }
            set_text_color(start);
        }
    }
    else if (strncmp(cmd, "set ", 4) == 0) {
        char varname[64], varval[256];
        if (sscanf(cmd + 4, "%63s %255[^\n]", varname, varval) == 2) {
            set_var(varname, varval);
        }
    }
    else if (strncmp(cmd, "input ", 6) == 0) {
        const char* input_line = cmd + 6;
        char prompt[256] = "";
        char varname[64] = "";

        if (input_line[0] == '"') {
            int i = 1;
            while (input_line[i] && input_line[i] != '"') {
                prompt[i-1] = input_line[i];
                i++;
            }
            prompt[i-1] = '\0';
            i += 2; // skip closing quote and space
            sscanf(input_line + i, "%63s", varname);
            printf("%s", prompt);
        } else {
            sscanf(input_line, "%63s", varname);
        }

        char user_input[256];
        if (fgets(user_input, sizeof(user_input), stdin)) {
            size_t in_len = strlen(user_input);
            if (in_len > 0 && (user_input[in_len - 1] == '\n' || user_input[in_len - 1] == '\r')) {
                while (in_len > 0 && (user_input[in_len - 1] == '\n' || user_input[in_len - 1] == '\r')) {
                    user_input[in_len - 1] = '\0';
                    in_len--;
                }
            }
            set_var(varname, user_input);
        }
    }
}

int main(int argc, char *argv[]) {
#ifdef _WIN32
    // Enable ANSI escape sequences in Windows Console (improved)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            if (!SetConsoleMode(hOut, dwMode)) {
                // Fallback: Try enabling processed output as well
                dwMode |= ENABLE_PROCESSED_OUTPUT;
                SetConsoleMode(hOut, dwMode);
            }
        }
    }
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
        // IF / ELIF / ELSE single-line blocks
        else if (strncmp(line, "if (", 4) == 0) {
            // Extract condition inside parentheses
            char cond_raw[256] = "";
            if (sscanf(line, "if (%255[^)])", cond_raw) == 1) {
                trim_whitespace(cond_raw);
                int executed = 0;

                // Read body for initial if
                char body_line[256];
                if (fgets(body_line, sizeof(body_line), fp)) {
                    size_t b_len = strlen(body_line);
                    if (b_len > 0 && body_line[b_len - 1] == '\n') body_line[b_len - 1] = '\0';
                    if (eval_condition(cond_raw) && !executed) {
                        execute_single_command(body_line);
                        executed = 1;
                    }
                }

                // Process subsequent elif/else headers if present
                while (1) {
                    long pos_before_header = ftell(fp);
                    char header[256];
                    if (!fgets(header, sizeof(header), fp)) {
                        break; // EOF
                    }
                    size_t h_len = strlen(header);
                    if (h_len > 0 && header[h_len - 1] == '\n') header[h_len - 1] = '\0';

                    // Trim leading whitespace for header check
                    char* hptr = header;
                    while (*hptr == ' ' || *hptr == '\t') hptr++;

                    if (strncmp(hptr, "elif (", 6) == 0) {
                        char elif_cond_raw[256] = "";
                        if (sscanf(hptr, "elif (%255[^)])", elif_cond_raw) == 1) {
                            trim_whitespace(elif_cond_raw);
                            char elif_body[256];
                            if (fgets(elif_body, sizeof(elif_body), fp)) {
                                size_t eb_len = strlen(elif_body);
                                if (eb_len > 0 && elif_body[eb_len - 1] == '\n') elif_body[eb_len - 1] = '\0';
                                if (!executed && eval_condition(elif_cond_raw)) {
                                    execute_single_command(elif_body);
                                    executed = 1;
                                }
                            } else {
                                break; // Malformed chain at EOF
                            }
                            continue; // Continue checking next header
                        } else {
                            // Malformed elif, stop processing chain
                            break;
                        }
                    } else if (strncmp(hptr, "else", 4) == 0) {
                        char else_body[256];
                        if (fgets(else_body, sizeof(else_body), fp)) {
                            size_t es_len = strlen(else_body);
                            if (es_len > 0 && else_body[es_len - 1] == '\n') else_body[es_len - 1] = '\0';
                            if (!executed) {
                                execute_single_command(else_body);
                                executed = 1;
                            }
                        }
                        // else is terminal for the chain
                        break;
                    } else {
                        // Not part of the chain; rewind so main loop processes it
                        fseek(fp, pos_before_header, SEEK_SET);
                        break;
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
