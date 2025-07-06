#include "watchrun.h"

typedef struct {
    const char *short_opt;
    const char *long_opt;
    int has_arg;
    const char *description;
} Option;

static Option options[] __attribute((unused)) = {
    {"-w", "--watch", 1, "Directory to watch"},
    {"-x", "--exec", 1, "Command to execute"},
    {"-e", "--ext", 1, "File extensions"},
    {"-i", "--interval", 1, "Polling interval (ms)"},
    {"-c", "--config", 1, "Configuration file"},
    {"", "--include", 1, "Include pattern"},
    {"", "--exclude", 1, "Exclude pattern"},
    {"", "--no-clear", 0, "Don't clear screen"},
    {"", "--no-recursive", 0, "Don't recurse subdirectories"},
    {"", "--daemon", 0, "Run as daemon"},
    {"", "--json", 0, "JSON output"},
    {"", "--verbose", 0, "Verbose output"},
    {"", "--quiet", 0, "Quiet mode"},
    {"", "--save-config", 0, "Save configuration"},
    {"-h", "--help", 0, "Show help"},
    {"-v", "--version", 0, "Show version"},
    {NULL, NULL, 0, NULL}
};

void init_config(WatchConfig *config) {
    memset(config, 0, sizeof(WatchConfig));
    
    config->extensions = malloc(sizeof(char*) * MAX_EXTENSIONS);
    config->commands = malloc(sizeof(char*) * MAX_COMMANDS);
    config->include_patterns = malloc(sizeof(char*) * MAX_PATTERNS);
    config->exclude_patterns = malloc(sizeof(char*) * MAX_PATTERNS);
    
    config->ext_count = 0;
    config->cmd_count = 0;
    config->include_count = 0;
    config->exclude_count = 0;
    config->interval = 1000;
    config->no_clear = 0;
    config->daemon_mode = 0;
    config->json_output = 0;
    config->recursive = 1;
    config->verbose = 0;
    config->quiet = 0;
    config->file_count = 0;
    config->file_capacity = 1024;
    
    config->files = malloc(sizeof(FileInfo) * config->file_capacity);
    
    strcpy(config->config_file, "");
}

void cleanup_config(WatchConfig *config) {
    if (config->extensions) {
        for (int i = 0; i < config->ext_count; i++) {
            free(config->extensions[i]);
        }
        free(config->extensions);
    }
    
    if (config->commands) {
        for (int i = 0; i < config->cmd_count; i++) {
            free(config->commands[i]);
        }
        free(config->commands);
    }
    
    if (config->include_patterns) {
        for (int i = 0; i < config->include_count; i++) {
            free(config->include_patterns[i]);
        }
        free(config->include_patterns);
    }
    
    if (config->exclude_patterns) {
        for (int i = 0; i < config->exclude_count; i++) {
            free(config->exclude_patterns[i]);
        }
        free(config->exclude_patterns);
    }
    
    if (config->files) {
        free(config->files);
    }
}

static void add_extension(WatchConfig *config, const char *ext_list) {
    char *ext_copy = strdup(ext_list);
    char *token = strtok(ext_copy, ",");
    
    while (token && config->ext_count < MAX_EXTENSIONS) {
        // Remove leading/trailing whitespace
        while (*token == ' ') token++;
        char *end = token + strlen(token) - 1;
        while (end > token && *end == ' ') *end-- = '\0';
        
        config->extensions[config->ext_count] = strdup(token);
        config->ext_count++;
        token = strtok(NULL, ",");
    }
    
    free(ext_copy);
}

static void add_command(WatchConfig *config, const char *cmd) {
    if (config->cmd_count < MAX_COMMANDS) {
        config->commands[config->cmd_count] = strdup(cmd);
        config->cmd_count++;
    }
}

static void add_pattern(char ***patterns, int *count, int max_count, const char *pattern) {
    if (*count < max_count) {
        (*patterns)[*count] = strdup(pattern);
        (*count)++;
    }
}

static int match_option(const char *arg, const char *short_opt, const char *long_opt) {
    if (strlen(short_opt) > 0 && strcmp(arg, short_opt) == 0) {
        return 1;
    }
    if (strcmp(arg, long_opt) == 0) {
        return 1;
    }
    return 0;
}

static char* get_arg_value(int argc, char *argv[], int *i) {
    if (*i + 1 >= argc) {
        print_error("Missing argument value");
        return NULL;
    }
    (*i)++;
    return argv[*i];
}

int parse_args(int argc, char *argv[], WatchConfig *config) {
    int save_config_requested = 0;
    
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        
        if (match_option(arg, "-w", "--watch")) {
            char *value = get_arg_value(argc, argv, &i);
            if (!value) return -1;
            strncpy(config->watch_path, value, MAX_PATH_LEN - 1);
            normalize_path(config->watch_path);
        }
        else if (match_option(arg, "-x", "--exec")) {
            char *value = get_arg_value(argc, argv, &i);
            if (!value) return -1;
            add_command(config, value);
        }
        else if (match_option(arg, "-e", "--ext")) {
            char *value = get_arg_value(argc, argv, &i);
            if (!value) return -1;
            add_extension(config, value);
        }
        else if (match_option(arg, "-i", "--interval")) {
            char *value = get_arg_value(argc, argv, &i);
            if (!value) return -1;
            config->interval = atoi(value);
            if (config->interval < 100) {
                print_warning("Interval too small, setting to 100ms");
                config->interval = 100;
            }
        }
        else if (match_option(arg, "-c", "--config")) {
            char *value = get_arg_value(argc, argv, &i);
            if (!value) return -1;
            strncpy(config->config_file, value, MAX_PATH_LEN - 1);
        }
        else if (match_option(arg, "", "--include")) {
            char *value = get_arg_value(argc, argv, &i);
            if (!value) return -1;
            add_pattern(&config->include_patterns, &config->include_count, 
                       MAX_PATTERNS, value);
        }
        else if (match_option(arg, "", "--exclude")) {
            char *value = get_arg_value(argc, argv, &i);
            if (!value) return -1;
            add_pattern(&config->exclude_patterns, &config->exclude_count, 
                       MAX_PATTERNS, value);
        }
        else if (match_option(arg, "", "--no-clear")) {
            config->no_clear = 1;
        }
        else if (match_option(arg, "", "--no-recursive")) {
            config->recursive = 0;
        }
        else if (match_option(arg, "", "--daemon")) {
            config->daemon_mode = 1;
        }
        else if (match_option(arg, "", "--json")) {
            config->json_output = 1;
        }
        else if (match_option(arg, "", "--verbose")) {
            config->verbose = 1;
        }
        else if (match_option(arg, "", "--quiet")) {
            config->quiet = 1;
        }
        else if (match_option(arg, "", "--save-config")) {
            save_config_requested = 1;
        }
        else if (match_option(arg, "-h", "--help")) {
            print_usage(argv[0]);
            return -1;
        }
        else if (match_option(arg, "-v", "--version")) {
            printf("watchrun version %s\n", VERSION);
            printf("Compiled on %s %s\n", __DATE__, __TIME__);
            return -1;
        }
        else {
            print_error("Unknown option: ");
            printf("%s\n", arg);
            print_usage(argv[0]);
            return -1;
        }
    }
    
    if (save_config_requested) {
        if (strlen(config->config_file) == 0) {
            strcpy(config->config_file, ".watchrunrc");
        }
        if (save_config_file(config) == 0) {
            print_success("Configuration saved");
        } else {
            print_error("Failed to save configuration");
        }
        return -1;
    }
    
    return 0;
}

int load_config_file(WatchConfig *config) {
    FILE *file = fopen(config->config_file, "r");
    if (!file) {
        return -1;
    }
    
    char line[MAX_CONFIG_LINE];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        
        if (line[0] == '#' || line[0] == '\0') {
            continue;
        }
        
        char *equals = strchr(line, '=');
        if (!equals) continue;
        
        *equals = '\0';
        char *key = line;
        char *value = equals + 1;
        
        while (*key == ' ') key++;
        while (*value == ' ') value++;
        
        if (strcmp(key, "watch_path") == 0) {
            strncpy(config->watch_path, value, MAX_PATH_LEN - 1);
        }
        else if (strcmp(key, "extensions") == 0) {
            add_extension(config, value);
        }
        else if (strcmp(key, "commands") == 0) {
            char *cmd_copy = strdup(value);
            char *token = strtok(cmd_copy, ",");
            while (token && config->cmd_count < MAX_COMMANDS) {
                while (*token == ' ') token++;
                add_command(config, token);
                token = strtok(NULL, ",");
            }
            free(cmd_copy);
        }
        else if (strcmp(key, "interval") == 0) {
            config->interval = atoi(value);
        }
        else if (strcmp(key, "include_patterns") == 0) {
            char *pattern_copy = strdup(value);
            char *token = strtok(pattern_copy, ",");
            while (token && config->include_count < MAX_PATTERNS) {
                while (*token == ' ') token++;
                add_pattern(&config->include_patterns, &config->include_count,
                           MAX_PATTERNS, token);
                token = strtok(NULL, ",");
            }
            free(pattern_copy);
        }
        else if (strcmp(key, "exclude_patterns") == 0) {
            char *pattern_copy = strdup(value);
            char *token = strtok(pattern_copy, ",");
            while (token && config->exclude_count < MAX_PATTERNS) {
                while (*token == ' ') token++;
                add_pattern(&config->exclude_patterns, &config->exclude_count,
                           MAX_PATTERNS, token);
                token = strtok(NULL, ",");
            }
            free(pattern_copy);
        }
        else if (strcmp(key, "recursive") == 0) {
            config->recursive = (strcmp(value, "true") == 0) ? 1 : 0;
        }
        else if (strcmp(key, "verbose") == 0) {
            config->verbose = (strcmp(value, "true") == 0) ? 1 : 0;
        }
        else if (strcmp(key, "quiet") == 0) {
            config->quiet = (strcmp(value, "true") == 0) ? 1 : 0;
        }
        else if (strcmp(key, "daemon") == 0) {
            config->daemon_mode = (strcmp(value, "true") == 0) ? 1 : 0;
        }
        else if (strcmp(key, "json_output") == 0) {
            config->json_output = (strcmp(value, "true") == 0) ? 1 : 0;
        }
        else if (strcmp(key, "no_clear") == 0) {
            config->no_clear = (strcmp(value, "true") == 0) ? 1 : 0;
        }
    }
    
    fclose(file);
    return 0;
}

int save_config_file(WatchConfig *config) {
    FILE *file = fopen(config->config_file, "w");
    if (!file) {
        return -1;
    }
    
    fprintf(file, "# watchrun configuration file\n");
    fprintf(file, "# Generated on %s\n", get_current_time_str());
    fprintf(file, "\n");
    
    if (strlen(config->watch_path) > 0) {
        fprintf(file, "watch_path=%s\n", config->watch_path);
    }
    
    if (config->ext_count > 0) {
        fprintf(file, "extensions=");
        for (int i = 0; i < config->ext_count; i++) {
            fprintf(file, "%s", config->extensions[i]);
            if (i < config->ext_count - 1) fprintf(file, ",");
        }
        fprintf(file, "\n");
    }
    
    if (config->cmd_count > 0) {
        fprintf(file, "commands=");
        for (int i = 0; i < config->cmd_count; i++) {
            fprintf(file, "%s", config->commands[i]);
            if (i < config->cmd_count - 1) fprintf(file, ",");
        }
        fprintf(file, "\n");
    }
    
    fprintf(file, "interval=%d\n", config->interval);
    fprintf(file, "recursive=%s\n", config->recursive ? "true" : "false");
    fprintf(file, "verbose=%s\n", config->verbose ? "true" : "false");
    fprintf(file, "quiet=%s\n", config->quiet ? "true" : "false");
    fprintf(file, "daemon=%s\n", config->daemon_mode ? "true" : "false");
    fprintf(file, "json_output=%s\n", config->json_output ? "true" : "false");
    fprintf(file, "no_clear=%s\n", config->no_clear ? "true" : "false");
    
    if (config->include_count > 0) {
        fprintf(file, "include_patterns=");
        for (int i = 0; i < config->include_count; i++) {
            fprintf(file, "%s", config->include_patterns[i]);
            if (i < config->include_count - 1) fprintf(file, ",");
        }
        fprintf(file, "\n");
    }
    
    if (config->exclude_count > 0) {
        fprintf(file, "exclude_patterns=");
        for (int i = 0; i < config->exclude_count; i++) {
            fprintf(file, "%s", config->exclude_patterns[i]);
            if (i < config->exclude_count - 1) fprintf(file, ",");
        }
        fprintf(file, "\n");
    }
    
    fclose(file);
    return 0;
}