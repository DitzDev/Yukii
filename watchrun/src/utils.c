#include "watchrun.h"

char *get_current_time_str(void) {
    static char time_str[32];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
    return time_str;
}

void create_daemon(void) {
    #ifndef _WIN32
    pid_t pid = fork();
    
    if (pid < 0) {
        print_error("Failed to create daemon process");
        exit(1);
    }
    
    if (pid > 0) {
        exit(0);
    }
    
    if (setsid() < 0) {
        print_error("Failed to create new session");
        exit(1);
    }
    
    pid = fork();
    if (pid < 0) {
        print_error("Failed to fork daemon process");
        exit(1);
    }
    
    if (pid > 0) {
        exit(0);
    }
    
    if (chdir("/") < 0) {
        print_error("Failed to change working directory");
        exit(1);
    }
    
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    open("/dev/null", O_RDONLY); // stdin
    open("/dev/null", O_WRONLY); // stdout
    open("/dev/null", O_WRONLY); // stderr
    
    #else
    FreeConsole();
    #endif
}

int is_directory(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        return 0;
    }
    return S_ISDIR(path_stat.st_mode);
}

void normalize_path(char *path) {
    int len = strlen(path);
    
    if (len > 1 && (path[len-1] == '/' || path[len-1] == '\\')) {
        path[len-1] = '\0';
    }
    
    #ifdef _WIN32
    for (int i = 0; i < len; i++) {
        if (path[i] == '/') {
            path[i] = '\\';
        }
    }
    #else
    for (int i = 0; i < len; i++) {
        if (path[i] == '\\') {
            path[i] = '/';
        }
    }
    #endif
}

int match_pattern(const char *str, const char *pattern) {
    const char *s = str;
    const char *p = pattern;
    const char *star = NULL;
    const char *ss = str;
    
    while (*s) {
        if (*p == '?') {
            s++;
            p++;
        } else if (*p == '*') {
            star = p++;
            ss = s;
        } else if (*p == *s) {
            s++;
            p++;
        } else if (star) {
            p = star + 1;
            s = ++ss;
        } else {
            return 0;
        }
    }
    
    while (*p == '*') {
        p++;
    }
    
    return *p == '\0';
}

#ifdef _WIN32
int strcasecmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        int c1 = tolower((unsigned char)*s1);
        int c2 = tolower((unsigned char)*s2);
        if (c1 != c2) {
            return c1 - c2;
        }
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

void usleep(unsigned int usec) {
    Sleep(usec / 1000);
}
#endif