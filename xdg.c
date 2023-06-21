#include "xdg.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// Thanks to Carl @ StackOverflow (https://stackoverflow.com/a/2336245)
int mkdirs(const char *dir, mode_t mode) {
    char tmp[256];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(tmp, mode) == -1 && errno != EEXIST) {
                *p = '/';
                return 0;
            }
            *p = '/';
        }
    }

    return mkdir(tmp, mode) != -1 || errno == EEXIST;
}

int dir_exists(const char *path) {
    if (path == NULL || path[0] == '\0') {
        return 0;
    }

    struct stat st;
    return lstat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

char *get_xdg_path(const char *xdg_home_env, const char *xdg_dirs_env, const char *home_sub_dir, const char *dir, int search) {
    // ${xdg_home_env}/{dir}
    char *xdg_home = getenv(xdg_home_env);
    if (dir_exists(xdg_home)) {
        size_t path_len = strnlen(xdg_home, PATH_MAX) + strnlen(dir, PATH_MAX) + 2;
        if (path_len > PATH_MAX) {
            #ifdef DEBUG
            fprintf(stderr, "DEBUG: Ignoring XDG_HOME as it exceeds PATH_MAX.\n"); // TODO print name?
            #endif
        } else {
            char *path = malloc(path_len);
            if (path == NULL) {
                fprintf(stderr, "ERROR: Out of memory.\n");
                return NULL;
            }

            snprintf(path, path_len, "%s/%s", xdg_home, dir);

            if (search) {
                if (dir_exists(path)) {
                    return path;
                }
            } else {
                if (!mkdirs(path, S_IRWXU)) {
                    // TODO: Should this not return?
                    fprintf(stderr, "ERROR: Could not create directory '%s'.\n", path);
                    free(path);
                    return NULL;
                }

                return path;
            }

            free(path);
        }
    }

    // ${xdg_dirs_env}/{dir}
    char *xdg_dirs = getenv(xdg_dirs_env);
    if (search && xdg_dirs != NULL && xdg_dirs[0] != '\0') {
        char *xdg_dir = strtok(xdg_dirs, ":");
        while (xdg_dir != NULL) {
            size_t path_len = strnlen(xdg_dir, PATH_MAX) + strnlen(dir, PATH_MAX) + 2;
            if (path_len > PATH_MAX) {
                #ifdef DEBUG
                fprintf(stderr, "DEBUG: Ignoring XDG_DIRS as it exceeds PATH_MAX.\n"); // TODO print name?
                #endif
            } else {
                char *path = malloc(path_len);
                if (path == NULL) {
                    fprintf(stderr, "ERROR: Out of memory.\n");
                    return NULL;
                }

                snprintf(path, path_len, "%s/%s", xdg_dir, dir);

                if (dir_exists(path)) {
                    return path;
                }

                free(path);
            }

            xdg_dir = strtok(NULL, ":");
        }
    }

    // $HOME/{home_sub_dir}/{dir}
    char *home = getenv("HOME");
    if (dir_exists(home)) {
        size_t path_len = strnlen(home, PATH_MAX) + strnlen(home_sub_dir, PATH_MAX) + strnlen(dir, PATH_MAX) + 3;
        if (path_len > PATH_MAX) {
            #ifdef DEBUG
            fprintf(stderr, "DEBUG: Ignoring HOME as it exceeds PATH_MAX.\n");
            #endif
        }
        char *path = malloc(path_len);
        if (path == NULL) {
            fprintf(stderr, "ERROR: Out of memory.\n");
            return NULL;
        }

        snprintf(path, path_len, "%s/%s/%s", home, home_sub_dir, dir);

        if (search) {
            if (dir_exists(path)) {
                return path;
            }
        } else {
            if (!mkdirs(path, S_IRWXU)) {
                // TODO: Should this not return?
                fprintf(stderr, "ERROR: Could not create directory '%s'.\n", path);
                free(path);
                return NULL;
            }

            return path;
        }

        free(path);
    }

    return search ? get_xdg_path(xdg_home_env, xdg_dirs_env, home_sub_dir, dir, 0) : NULL;
}

char *get_config_path(const char *config_dir) {
    return get_xdg_path("XDG_CONFIG_HOME", "XDG_CONFIG_DIRS", ".config", config_dir, 1);
}

char *get_data_path(const char *data_dir) {
    return get_xdg_path("XDG_DATA_HOME", "XDG_DATA_DIRS", ".local/share", data_dir, 1);
}
