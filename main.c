#include "xdg.h"
#include "data.h"
#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define NAME "welcome-home"
#define DEBUG

size_t get_path_last_mod_time(const char *path) {
    struct stat st;
    if (lstat(path, &st) == -1) {
        return 0;
    }

    return st.st_mtime;
}

int main(int argc, char **argv) {
    time_t system_time = time(NULL);
    time_t system_date = system_time - (system_time % 86400);

    const char *data_path = get_data_path(NAME);
    if (data_path == NULL) {
        return 1;
    }

    struct data *data = read_or_create_data(data_path, ".data");
    if (data == NULL) {
        return 1;
    }

    if (data->last_print_time > system_date) {
        // We've already printed today

        // If the boot time is available in the system, we also print after a restart
        struct timespec system_boot_time;
        if (clock_gettime(CLOCK_BOOTTIME, &system_boot_time) != 0 || data->last_print_time > system_boot_time.tv_sec) {
#ifdef DEBUG
            fprintf(stdout, "DEBUG: Welcome message not necessary.\n");
#endif

            free_data(data);
            return 0;
        }
    }

    const char *assets_path = get_config_path(NAME);
    if (assets_path == NULL) {
        free_data(data);
        return 1;
    }

    size_t assets_last_mod_time = get_path_last_mod_time(assets_path);
    if (data->last_cache_time == 0 || assets_last_mod_time > data->last_cache_time) {
        // Invalidate cache
        cache_data(data, assets_path, system_time);
    }

    if (!print_random_art(STDOUT_FILENO, data, assets_path)) {
        free_data(data);
        return 1;
    }
    data->last_print_time = system_time;

    if (!write_data(data, data_path, ".data")) {
        free_data(data);
        return 1;
    }

    free_data(data);
    return 0;
}
