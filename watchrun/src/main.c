#include "watchrun.h"

volatile sig_atomic_t running = 1;
WatchConfig *global_config = NULL;

int main(int argc, char *argv[]) {
    WatchConfig config;
    
    init_config(&config);
    global_config = &config;
    
    setup_signal_handlers();
    init_colors();
    
    if (argc == 1) {
        print_banner();
        print_usage(argv[0]);
        cleanup_config(&config);
        return 1;
    }
    
    if (parse_args(argc, argv, &config) != 0) {
        cleanup_config(&config);
        return 1;
    }
    
    if (strlen(config.config_file) > 0) {
        if (load_config_file(&config) != 0 && config.verbose) {
            print_warning("Could not load config file, using defaults");
        }
    }
    
    if (strlen(config.watch_path) == 0) {
        print_error("Watch path is required (-w/--watch)");
        cleanup_config(&config);
        return 1;
    }
    
    if (config.cmd_count == 0) {
        print_error("At least one command is required (-x/--exec)");
        cleanup_config(&config);
        return 1;
    }
    
    if (!is_directory(config.watch_path)) {
        print_error("Watch path is not a valid directory");
        cleanup_config(&config);
        return 1;
    }
    
    if (config.daemon_mode) {
        create_daemon();
    }
    
    if (!config.quiet) {
        print_banner();
        
        printf("%s[watchrun]%s Starting file watcher...\n", 
               COLOR_CYAN, COLOR_RESET);
        printf("%s[watchrun]%s Watching: %s%s%s\n", 
               COLOR_CYAN, COLOR_RESET, COLOR_YELLOW, config.watch_path, COLOR_RESET);
        
        if (config.ext_count > 0) {
            printf("%s[watchrun]%s Extensions: ", COLOR_CYAN, COLOR_RESET);
            for (int i = 0; i < config.ext_count; i++) {
                printf("%s.%s%s", COLOR_GREEN, config.extensions[i], COLOR_RESET);
                if (i < config.ext_count - 1) printf(", ");
            }
            printf("\n");
        }
        
        printf("%s[watchrun]%s Commands: ", COLOR_CYAN, COLOR_RESET);
        for (int i = 0; i < config.cmd_count; i++) {
            printf("%s%s%s", COLOR_MAGENTA, config.commands[i], COLOR_RESET);
            if (i < config.cmd_count - 1) printf(", ");
        }
        printf("\n");
        
        printf("%s[watchrun]%s Poll interval: %s%dms%s\n", 
               COLOR_CYAN, COLOR_RESET, COLOR_BLUE, config.interval, COLOR_RESET);
        printf("%s[watchrun]%s Press Ctrl+C to stop\n\n", 
               COLOR_CYAN, COLOR_RESET);
    }
    
    // Start watching
    int result = watch_directory(&config);
    
    cleanup_config(&config);
    return result;
}

void print_banner(void) {
    printf("%sWatchRun made by DitzDev. Copyright ©2025 MIT License%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s\e[4mhttps://github.com/DitzDev%s\n\n", COLOR_CYAN, COLOR_RESET);
}

void print_usage(const char *prog_name) {
    printf("%sUsage:%s %s [OPTIONS]\n\n", COLOR_BOLD, COLOR_RESET, prog_name);
    
    printf("%sRequired Options:%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %s-w, --watch PATH%s      Directory to watch for changes\n", COLOR_GREEN, COLOR_RESET);
    printf("  %s-x, --exec CMD%s        Command to execute on changes (can be used multiple times)\n", COLOR_GREEN, COLOR_RESET);
    
    printf("\n%sOptional Options:%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %s-e, --ext EXT%s         File extensions to watch (comma-separated, e.g., c,h,py)\n", COLOR_BLUE, COLOR_RESET);
    printf("  %s-i, --interval MS%s     Polling interval in milliseconds (default: 1000)\n", COLOR_BLUE, COLOR_RESET);
    printf("  %s-c, --config FILE%s     Configuration file path\n", COLOR_BLUE, COLOR_RESET);
    printf("  %s--include PATTERN%s     Include files matching pattern (can be used multiple times)\n", COLOR_BLUE, COLOR_RESET);
    printf("  %s--exclude PATTERN%s     Exclude files matching pattern (can be used multiple times)\n", COLOR_BLUE, COLOR_RESET);
    printf("  %s--no-clear%s            Don't clear screen before running commands\n", COLOR_BLUE, COLOR_RESET);
    printf("  %s--no-recursive%s        Don't watch subdirectories\n", COLOR_BLUE, COLOR_RESET);
    printf("  %s--daemon%s              Run as daemon process\n", COLOR_BLUE, COLOR_RESET);
    printf("  %s--json%s                Output in JSON format\n", COLOR_BLUE, COLOR_RESET);
    printf("  %s--verbose%s             Verbose output\n", COLOR_BLUE, COLOR_RESET);
    printf("  %s--quiet%s               Suppress banner and info messages\n", COLOR_BLUE, COLOR_RESET);
    printf("  %s--save-config%s         Save current configuration to file\n", COLOR_BLUE, COLOR_RESET);
    printf("  %s-h, --help%s            Show this help message\n", COLOR_BLUE, COLOR_RESET);
    printf("  %s-v, --version%s         Show version information\n", COLOR_BLUE, COLOR_RESET);
    
    printf("\n%sExamples:%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %s# Watch C files and run make%s\n", COLOR_DIM, COLOR_RESET);
    printf("  %s -w src -e c,h -x \"make\"\n", prog_name);
    
    printf("\n  %s# Watch Python files with custom interval%s\n", COLOR_DIM, COLOR_RESET);
    printf("  %s -w . -e py -x \"python test.py\" -i 500\n", prog_name);
    
    printf("\n  %s# Use patterns and multiple commands%s\n", COLOR_DIM, COLOR_RESET);
    printf("  %s -w src --include \"*.c\" --exclude \"*test*\" -x \"make\" -x \"./run_tests\"\n", prog_name);
    
    printf("\n  %s# Run as daemon with config file%s\n", COLOR_DIM, COLOR_RESET);
    printf("  %s -c ~/.watchrunrc --daemon\n", prog_name);
    
    printf("\n%sConfiguration File Example (.watchrunrc):%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("  watch_path=src\n");
    printf("  extensions=c,h,cpp,hpp\n");
    printf("  commands=make,./run_tests\n");
    printf("  interval=1000\n");
    printf("  include_patterns=*.c,*.h\n");
    printf("  exclude_patterns=*test*,*tmp*\n");
    printf("  recursive=true\n");
    printf("  verbose=false\n");
}

void setup_signal_handlers(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
#ifndef _WIN32
    signal(SIGHUP, signal_handler);
#endif
}

void signal_handler(int sig) {
    running = 0;
    if (global_config && !global_config->quiet) {
        printf("\n%s[watchrun]%s Received signal %d, shutting down...\n", 
               COLOR_YELLOW, COLOR_RESET, sig);
    }
}