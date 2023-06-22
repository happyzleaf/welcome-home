#ifndef WELCOME_HOME_DATA_H
#define WELCOME_HOME_DATA_H

#include <stdio.h>
#include <sys/types.h>

struct cache {
    char *asset;
    size_t cols;
    size_t rows;
};

struct data {
    time_t last_print_time;
    time_t last_cache_time;

    size_t cache_len;
    struct cache **cache;
};

struct data *read_or_create_data(const char *dir, const char *name);

int write_data(struct data *data, const char *dir, const char *name);

int cache_data(struct data *data, const char *assets_path, time_t system_time, int debug);

void print_data(FILE *out, struct data *data);

void free_data(struct data *data);

#endif //WELCOME_HOME_DATA_H
