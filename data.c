#include "data.h"

#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

void serialize_data(FILE *out, struct data *data) {
    fwrite(&data->last_print_time, sizeof(time_t), 1, out);
    fwrite(&data->last_cache_time, sizeof(time_t), 1, out);
    fwrite(&data->cache_len, sizeof(size_t), 1, out);
    for (int i = 0; i < data->cache_len; ++i) {
        size_t asset_len = strnlen(data->cache[i]->asset, PATH_MAX) + 1;
        fwrite(&asset_len, sizeof(size_t), 1, out);
        fwrite(data->cache[i]->asset, 1, asset_len, out);
        fwrite(&data->cache[i]->cols, sizeof(size_t), 1, out);
        fwrite(&data->cache[i]->rows, sizeof(size_t), 1, out);
    }
}

void deserialize_data(FILE *in, struct data *data) {
    fread(&data->last_print_time, sizeof(time_t), 1, in);
    fread(&data->last_cache_time, sizeof(time_t), 1, in);
    fread(&data->cache_len, sizeof(size_t), 1, in);

    data->cache = malloc(data->cache_len * sizeof(struct cache *));
    for (int i = 0; i < data->cache_len; ++i) {
        data->cache[i] = malloc(sizeof(struct cache));

        size_t asset_len;
        fread(&asset_len, sizeof(size_t), 1, in);
        data->cache[i]->asset = malloc(asset_len);
        fread(data->cache[i]->asset, 1, asset_len, in);

        fread(&data->cache[i]->cols, sizeof(size_t), 1, in);
        fread(&data->cache[i]->rows, sizeof(size_t), 1, in);
    }
}

struct data *read_or_create_data(const char *dir, const char *name) {
    size_t path_len = strnlen(dir, PATH_MAX) + strnlen(name, PATH_MAX) + 2;
    if (path_len > PATH_MAX) {
        fprintf(stderr, "ERROR: Data path exceeded PATH_MAX.\n");
        return NULL;
    }

    char *path = malloc(path_len);
    if (path == NULL) {
        fprintf(stderr, "ERROR: Out of memory.\n");
        return NULL;
    }

    snprintf(path, path_len, "%s/%s", dir, name);

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        struct data *empty = malloc(sizeof(struct data));
        empty->last_print_time = 0;
        empty->last_cache_time = 0;
        empty->cache_len = 0;
        empty->cache = NULL;
        return empty;
    }

    struct data *data = malloc(sizeof(struct data));
    deserialize_data(file, data);
    fclose(file);

    return data;
}

int write_data(struct data *data, const char *dir, const char *name) {
    size_t path_len = strnlen(dir, PATH_MAX) + strnlen(name, PATH_MAX) + 2;
    if (path_len > PATH_MAX) {
        fprintf(stderr, "ERROR: Data path exceeded PATH_MAX.\n");
        return 0;
    }

    char *path = malloc(path_len);
    if (path == NULL) {
        fprintf(stderr, "ERROR: Out of memory.\n");
        return 0;
    }

    snprintf(path, path_len, "%s/%s", dir, name);

    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        fprintf(stderr, "ERROR: Could not open data file.\n");
        return 0;
    }

    serialize_data(file, data);
    fclose(file);

    return 1;
}

int is_file_or_sym(const struct dirent *entry, char *path_buffer, const char *assets_path, size_t assets_path_len, int debug) {
    // Preliminary check since it doesn't cost much and could even save time
    if (entry->d_type != DT_REG && entry->d_type != DT_LNK) {
        return 0;
    }

    size_t path_len = assets_path_len + strnlen(entry->d_name, PATH_MAX) + 2;
    if (path_len > PATH_MAX) {
        if (debug) {
#ifdef DEBUG
            fprintf(stderr, "DEBUG: Ignoring asset '%s' as it exceeds PATH_MAX.\n", entry->d_name);
#endif
        }
        return 0;
    }

    snprintf(path_buffer, path_len, "%s/%s", assets_path, entry->d_name);

    struct stat st;
    if (lstat(path_buffer, &st) == -1) {
        if (debug) {
#ifdef DEBUG
            fprintf(stderr, "DEBUG: Could not read asset '%s'.\n", entry->d_name);
#endif
        }
        return 0;
    }

    return S_ISREG(st.st_mode) || S_ISLNK(st.st_mode);
}

int cache_data(struct data *data, const char *assets_path, time_t system_time) {
    DIR *directory = opendir(assets_path);
    if (directory == NULL) {
        fprintf(stderr, "ERROR: Could not read assets directory.\n");
        return 0;
    }

    size_t assets_count = 0;
    char *path_buffer = malloc(PATH_MAX);
    size_t assets_path_len = strnlen(assets_path, PATH_MAX);
    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL) {
        if (is_file_or_sym(entry, path_buffer, assets_path, assets_path_len, 0)) {
            ++assets_count;
        }
    }

    // TODO: move this up????
    for (int i = 0; i < data->cache_len; ++i) {
        free(data->cache[i]->asset);
        free(data->cache[i]);
    }
    free(data->cache);

    data->cache_len = assets_count;
    data->cache = malloc(assets_count * sizeof(struct cache *));

    rewinddir(directory);

    char *line_buffer = NULL;
    size_t line_buffer_size = 0; // I hate this variable so much
    int index = -1;
    while ((entry = readdir(directory)) != NULL) {
        if (!is_file_or_sym(entry, path_buffer, assets_path, assets_path_len, 1)) {
            continue;
        }

        data->cache[++index] = NULL;

        FILE *file = fopen(path_buffer, "r");
        if (file == NULL) {
#ifdef DEBUG
            fprintf(stderr, "DEBUG: Could not read asset '%s'.\n", entry->d_name);
#endif
            continue;
        }

        size_t cols = 0;
        size_t rows = 0;

        ssize_t line_len;
        while ((line_len = getline(&line_buffer, &line_buffer_size, file)) != -1) {
            if (line_len > cols) {
                cols = line_len;
            }
            ++rows;
        }
        fclose(file);

        data->cache[index] = malloc(sizeof(struct cache));
        data->cache[index]->asset = malloc(strnlen(entry->d_name, PATH_MAX) + 1);
        *data->cache[index]->asset = '\0';
        strncat(data->cache[index]->asset, entry->d_name, PATH_MAX);
        data->cache[index]->cols = cols;
        data->cache[index]->rows = rows;
    }

    closedir(directory);

    free(line_buffer);
    free(path_buffer);

    data->last_cache_time = system_time;

    return 1;
}

void print_data(FILE *out, struct data *data) {
    fprintf(out, "data {\n");
    fprintf(out, "\tlast_print_time: %zu\n", data->last_print_time);
    fprintf(out, "\tlast_cache_time: %zu\n", data->last_cache_time);
    fprintf(out, "\tcache[%zu]: [", data->cache_len);
    for (int i = 0; i < data->cache_len; ++i) {
        if (i != 0) {
            fprintf(out, ",");
        }
        if (data->cache[i] == NULL) {
            printf("\n\t\t%d: NULL", i);
        } else {
            printf("\n\t\t%d: {'%s', %zu, %zu}", i, data->cache[i]->asset, data->cache[i]->cols, data->cache[i]->rows);
        }
    }
    fprintf(out, "\n\t]\n");
    fprintf(out, "}\n");
}

void free_data(struct data *data) {
    for (int i = 0; i < data->cache_len; ++i) {
        free(data->cache[i]->asset);
        free(data->cache[i]);
    }
    free(data->cache);
    free(data);
}
