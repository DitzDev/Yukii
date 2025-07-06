#ifndef WATCHRUN_H
#define WATCHRUN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define sleep(x) Sleep((x) * 1000)
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"
#else
#include <sys/wait.h>
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"
#endif

#define MAX_PATH_LEN 4096
#define MAX_EXTENSIONS 32
#define MAX_COMMANDS 16
#define MAX_PATTERNS 64
#define MAX_CONFIG_LINE 1024
#define VERSION "1.0.0"

typedef struct {
    char path[MAX_PATH_LEN];
    time_t mtime;
} FileInfo;

typedef struct {
    char **extensions;
    int ext_count;
    char **commands;
    int cmd_count;
    char **include_patterns;
    int include_count;
    char **exclude_patterns;
    int exclude_count;
    char watch_path[MAX_PATH_LEN];
    int interval;
    int no_clear;
    int daemon_mode;
    int json_output;
    int recursive;
    int verbose;
    int quiet;
    char config_file[MAX_PATH_LEN];
    FileInfo *files;
    int file_count;
    int file_capacity;
} WatchConfig;

void print_banner(void);
void print_usage(const char *prog_name);
int parse_args(int argc, char *argv[], WatchConfig *config);
void init_config(WatchConfig *config);
void cleanup_config(WatchConfig *config);
int load_config_file(WatchConfig *config);
int save_config_file(WatchConfig *config);

int watch_directory(WatchConfig *config);
int scan_directory(const char *path, WatchConfig *config, int is_initial);
int check_file_extension(const char *filename, WatchConfig *config);
int check_patterns(const char *filename, WatchConfig *config);
void execute_commands(WatchConfig *config, const char *changed_file);

void init_colors(void);
void print_colored(const char *text, const char *color);
void print_success(const char *text);
void print_error(const char *text);
void print_warning(const char *text);
void print_info(const char *text);
void clear_screen(void);

char *get_current_time_str(void);
void create_daemon(void);
int is_directory(const char *path);
void normalize_path(char *path);
int match_pattern(const char *str, const char *pattern);

void setup_signal_handlers(void);
void signal_handler(int sig);

extern volatile sig_atomic_t running;
extern WatchConfig *global_config;

#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"
#define COLOR_BOLD "\033[1m"
#define COLOR_DIM "\033[2m"

#define STYLE_BOLD "\033[1m"
#define STYLE_UNDERLINE "\033[4m"
#define STYLE_BLINK "\033[5m"

#endif // WATCHRUN_H