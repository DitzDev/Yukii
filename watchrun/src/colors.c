#include "watchrun.h"

static int colors_enabled = 1;

void init_colors(void) {
    #ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    }
    colors_enabled = 1;
    #else
    colors_enabled = isatty(fileno(stdout));
    #endif
    
    if (getenv("NO_COLOR") != NULL) {
        colors_enabled = 0;
    }
}

void print_colored(const char *text, const char *color) {
    if (colors_enabled) {
        printf("%s %s%s", color, text, COLOR_RESET);
    } else {
        printf("%s", text);
    }
}

void print_success(const char *text) {
    if (colors_enabled) {
        printf("%s %s%s\n", COLOR_GREEN, text, COLOR_RESET);
    } else {
        printf("[SUCCESS] %s\n", text);
    }
}

void print_error(const char *text) {
    if (colors_enabled) {
        fprintf(stderr, "%s %s%s", COLOR_RED, text, COLOR_RESET);
    } else {
        fprintf(stderr, "[ERROR] %s", text);
    }
}

void print_warning(const char *text) {
    if (colors_enabled) {
        printf("%s %s%s\n", COLOR_YELLOW, text, COLOR_RESET);
    } else {
        printf("[WARNING] %s\n", text);
    }
}

void print_info(const char *text) {
    if (colors_enabled) {
        printf("%s %s%s\n", COLOR_BLUE, text, COLOR_RESET);
    } else {
        printf("[INFO] %s\n", text);
    }
}

void clear_screen(void) {
    if (colors_enabled) {
        printf("\033[2J\033[H");
    } else {
        for (int i = 0; i < 50; i++) {
            printf("\n");
        }
    }
    fflush(stdout);
}