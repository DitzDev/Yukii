#include "watchrun.h"

int watch_directory(WatchConfig *config) {
    if (config->verbose) {
        print_info("Starting initial directory scan...");
    }
    
    if (scan_directory(config->watch_path, config, 1) < 0) {
        print_error("Failed to scan directory");
        return -1;
    }
    
    if (config->verbose) {
        printf("%s[watchrun]%s Found %d files to monitor\n", 
               COLOR_CYAN, COLOR_RESET, config->file_count);
    }
    
    while (running) {
        int changes_detected = scan_directory(config->watch_path, config, 0);
        
        if (changes_detected > 0) {
            if (config->json_output) {
                printf("{\"timestamp\":\"%s\",\"changes\":%d,\"action\":\"triggered\"}\n",
                       get_current_time_str(), changes_detected);
                fflush(stdout);
            } else if (!config->quiet) {
                printf("%s[%s]%s %sChanges detected in %d file(s)%s\n", 
                       COLOR_CYAN, get_current_time_str(), COLOR_RESET,
                       COLOR_YELLOW, changes_detected, COLOR_RESET);
            }
            
            execute_commands(config, NULL);
        }
        
        usleep(config->interval * 1000);
    }
    
    if (!config->quiet) {
        print_info("File watching stopped");
    }
    
    return 0;
}

int scan_directory(const char *path, WatchConfig *config, int is_initial) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char full_path[MAX_PATH_LEN];
    int changes_detected = 0;
    
    dir = opendir(path);
    if (!dir) {
        if (config->verbose) {
            print_error("Cannot open directory: ");
            printf("%s\n", path);
        }
        return -1;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        snprintf(full_path, sizeof(full_path), "%s%s%s", 
                 path, PATH_SEPARATOR_STR, entry->d_name);
        
        if (stat(full_path, &file_stat) != 0) {
            continue;
        }
        
        if (S_ISDIR(file_stat.st_mode)) {
            if (config->recursive) {
                int subdir_changes = scan_directory(full_path, config, is_initial);
                if (subdir_changes > 0) {
                    changes_detected += subdir_changes;
                }
            }
        } else if (S_ISREG(file_stat.st_mode)) {
            if (!check_file_extension(entry->d_name, config) ||
                !check_patterns(entry->d_name, config)) {
                continue;
            }
            
            int file_index = -1;
            for (int i = 0; i < config->file_count; i++) {
                if (strcmp(config->files[i].path, full_path) == 0) {
                    file_index = i;
                    break;
                }
            }
            
            if (file_index == -1) {
                if (config->file_count >= config->file_capacity) {
                    config->file_capacity *= 2;
                    config->files = realloc(config->files, 
                                          sizeof(FileInfo) * config->file_capacity);
                }
                
                file_index = config->file_count;
                config->file_count++;
                strncpy(config->files[file_index].path, full_path, MAX_PATH_LEN - 1);
                config->files[file_index].mtime = file_stat.st_mtime;
                
                if (!is_initial) {
                    changes_detected++;
                    if (config->verbose && !config->json_output) {
                        printf("%s[%s]%s %sNew file:%s %s\n",
                               COLOR_CYAN, get_current_time_str(), COLOR_RESET,
                               COLOR_GREEN, COLOR_RESET, full_path);
                    }
                }
            } else {
                // Check if file was modified
                if (config->files[file_index].mtime != file_stat.st_mtime) {
                    config->files[file_index].mtime = file_stat.st_mtime;
                    if (!is_initial) {
                        changes_detected++;
                        if (config->verbose && !config->json_output) {
                            printf("%s[%s]%s %sModified:%s %s\n",
                                   COLOR_CYAN, get_current_time_str(), COLOR_RESET,
                                   COLOR_YELLOW, COLOR_RESET, full_path);
                        }
                    }
                }
            }
        }
    }
    
    closedir(dir);
    return changes_detected;
}

int check_file_extension(const char *filename, WatchConfig *config) {
    if (config->ext_count == 0) {
        return 1;
    }
    
    const char *ext = strrchr(filename, '.');
    if (!ext) {
        return 0;
    }
    ext++;
    
    for (int i = 0; i < config->ext_count; i++) {
        if (strcasecmp(ext, config->extensions[i]) == 0) {
            return 1;
        }
    }
    
    return 0;
}

int check_patterns(const char *filename, WatchConfig *config) {
    for (int i = 0; i < config->exclude_count; i++) {
        if (match_pattern(filename, config->exclude_patterns[i])) {
            return 0;
        }
    }
    
    if (config->include_count == 0) {
        return 1;
    }
    
    for (int i = 0; i < config->include_count; i++) {
        if (match_pattern(filename, config->include_patterns[i])) {
            return 1;
        }
    }
    
    return 0;
}

void execute_commands(WatchConfig *config, const char *changed_file __attribute((unused))) {
    if (!config->no_clear && !config->json_output) {
        clear_screen();
    }
    
    for (int i = 0; i < config->cmd_count; i++) {
        char *command = config->commands[i];
        
        if (config->json_output) {
            printf("{\"timestamp\":\"%s\",\"command\":\"%s\",\"status\":\"executing\"}\n",
                   get_current_time_str(), command);
            fflush(stdout);
        } else if (!config->quiet) {
            printf("%s[%s]%s %sExecuting:%s %s\n",
                   COLOR_CYAN, get_current_time_str(), COLOR_RESET,
                   COLOR_MAGENTA, COLOR_RESET, command);
        }
        
        int result = system(command);
        
        if (config->json_output) {
            printf("{\"timestamp\":\"%s\",\"command\":\"%s\",\"exit_code\":%d,\"status\":\"%s\"}\n",
                   get_current_time_str(), command, result,
                   (result == 0) ? "success" : "failed");
            fflush(stdout);
        } else if (!config->quiet) {
            if (result == 0) {
                printf("%s[%s]%s %sCommand completed successfully%s\n",
                       COLOR_CYAN, get_current_time_str(), COLOR_RESET,
                       COLOR_GREEN, COLOR_RESET);
            } else {
                printf("%s[%s]%s %sCommand failed with exit code %d%s\n",
                       COLOR_CYAN, get_current_time_str(), COLOR_RESET,
                       COLOR_RED, result, COLOR_RESET);
            }
        }
        
        if (i < config->cmd_count - 1) {
            usleep(100000); // 100ms
        }
    }
    
    if (!config->quiet && !config->json_output) {
        printf("%s[%s]%s %sWaiting for changes...%s\n\n",
               COLOR_CYAN, get_current_time_str(), COLOR_RESET,
               COLOR_BLUE, COLOR_RESET);
    }
}