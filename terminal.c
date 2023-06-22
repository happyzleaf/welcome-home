#include "terminal.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#define BUFFER_SIZE 4096

void get_terminal_size(int fd, int *cols, int *rows) {
    struct winsize size;
    ioctl(fd, TIOCGWINSZ, &size);
    *cols = size.ws_col;
    *rows = size.ws_row;
}

int print_random_art(int fd, struct data *data, const char *assets_path) {
    if (data->cache_len == 0) {
        fprintf(stderr, "ERROR: Could not find any art?\n");
        return 0;
    }

    int cols;
    int rows;
    get_terminal_size(fd, &cols, &rows);

    int min_delta_index = -1;
    int min_delta = INT_MAX;
    for (int i = 0; i < data->cache_len; ++i) {
        int deltaX = cols - data->cache[i]->cols;
        int deltaY = rows - data->cache[i]->rows;
        int delta = deltaX * deltaX + deltaY * deltaY;

        if (min_delta > delta) {
            min_delta_index = i;
            min_delta = delta;
        }
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
        fprintf(stderr, "ERROR: Could not read asset.\n");
        return 0;
    }

    FILE *out = fdopen(fd, "w+");
    if (out == NULL) {
        fprintf(stderr, "ERROR: Could not write to output.\n");
        return 0;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, in)) > 0) {
        fwrite(buffer, 1, bytes_read, out);
    }

    fclose(in);
    fclose(out);

    return 1;
}
