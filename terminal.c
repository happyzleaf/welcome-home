#include "terminal.h"

#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define OFFSET_Y -2 // Hardcoded, based on my machine, hopefully this works everywhere. TODO: Keep an eye.
#define BUFFER_SIZE 4096

void get_terminal_size(int fd, int *cols, int *rows) {
    struct winsize size;
    ioctl(fd, TIOCGWINSZ, &size);
    *cols = size.ws_col;
    *rows = size.ws_row + OFFSET_Y;
}

int print_random_art(int out, struct data *data, const char *assets_path, int debug) {
    if (data->cache_len == 0) {
        if (debug) {
            fprintf(stderr, "DEBUG: No art available.\n");
        }

        return 0;
    }

    int cols;
    int rows;
    get_terminal_size(out, &cols, &rows);

    if (debug) {
        fprintf(stdout, "DEBUG: terminal size: %dx%d\n", cols, rows);
    }

    int min_delta_index = -1;
    int min_delta = INT_MAX;
    for (int i = 0; i < data->cache_len; ++i) {
        int delta_x = cols - data->cache[i]->cols;
        int delta_y = rows - data->cache[i]->rows;

        if (delta_x < 0 || delta_y < 0) {
            // Out of bounds
            continue;
        }

        // TODO: weighted random based on delta, or maybe random from within range of the largest
        int delta = delta_x + delta_y;

        if (min_delta > delta) {
            min_delta_index = i;
            min_delta = delta;
        }
    }

    if (min_delta_index == -1) {
        if (debug) {
            fprintf(stderr, "DEBUG: No art available for this size.\n");
        }

        return 1;
    }

    if (debug) {
        fprintf(stdout, "DEBUG: asset = '%s'\n", data->cache[min_delta_index]->asset);
    }

    size_t path_len = strnlen(assets_path, PATH_MAX) + strnlen(data->cache[min_delta_index]->asset, PATH_MAX) + 2;
    if (path_len > PATH_MAX) {
        fprintf(stderr, "ERROR: Asset path exceeds PATH_MAX.");
        return 0;
    }

    char *path = malloc(path_len);
    snprintf(path, path_len, "%s/%s", assets_path, data->cache[min_delta_index]->asset);

    FILE *in = fopen(path, "r");
    if (in == NULL) {
        fprintf(stderr, "ERROR: Could not read asset file.\n");
        return 0;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, in)) > 0) {
        write(out, buffer, bytes_read);
    }

    fclose(in);

    return 1;
}
